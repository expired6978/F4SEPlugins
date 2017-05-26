package Shared.AS3 
{
    import Mobile.ScrollList.*;
    import Shared.*;
    import Shared.AS3.COMPANIONAPP.*;
    import __AS3__.vec.*;
    import flash.display.*;
    import flash.events.*;
    import flash.geom.*;
    import flash.ui.*;
    import flash.utils.*;
    
    public class BSScrollingList extends flash.display.MovieClip
    {
		public static const MOBILE_ITEM_PRESS:String="BSScrollingList::mobileItemPress";
        public static const TEXT_OPTION_NONE:String="None";
        public static const TEXT_OPTION_SHRINK_TO_FIT:String="Shrink To Fit";
        public static const TEXT_OPTION_MULTILINE:String="Multi-Line";
        public static const SELECTION_CHANGE:String="BSScrollingList::selectionChange";
        public static const ITEM_PRESS:String="BSScrollingList::itemPress";
        public static const LIST_PRESS:String="BSScrollingList::listPress";
        public static const LIST_ITEMS_CREATED:String="BSScrollingList::listItemsCreated";
        public static const PLAY_FOCUS_SOUND:String="BSScrollingList::playFocusSound";
        protected var iSelectedClipIndex:int;
        protected var iListItemsShown:uint;
        protected var uiNumListItems:uint;
        protected var ListEntryClass:Class;
        protected var fListHeight:Number;
        protected var fVerticalSpacing:Number;
        protected var iScrollPosition:uint;
        protected var iMaxScrollPosition:uint;
        protected var bMouseDrivenNav:Boolean;
        public var scrollList:Mobile.ScrollList.MobileScrollList;
        protected var iPlatform:Number;
        protected var bInitialized:Boolean;
        protected var strTextOption:String;
        protected var bDisableSelection:Boolean;
        protected var bDisableInput:Boolean;
        protected var bReverseList:Boolean;
        protected var bUpdated:Boolean;
        protected var bRestoreListIndex:Boolean;
        internal var _itemRendererClassName:String;
        public var border:flash.display.MovieClip;
        public var ScrollUp:flash.display.MovieClip;
        public var ScrollDown:flash.display.MovieClip;
        protected var EntriesA:Array;
        protected var EntryHolder_mc:flash.display.MovieClip;
        protected var _filterer:Shared.AS3.ListFilterer;
        protected var iSelectedIndex:int;
        protected var fShownItemsHeight:Number;
		
        public function BSScrollingList()
        {
            super();
            EntriesA = new Array();
            _filterer = new Shared.AS3.ListFilterer();
            addEventListener(Shared.AS3.ListFilterer.FILTER_CHANGE, onFilterChange);
            strTextOption = TEXT_OPTION_NONE;
            fVerticalSpacing = 0;
            uiNumListItems = 0;
            bRestoreListIndex = true;
            bDisableSelection = false;
            bDisableInput = false;
            bMouseDrivenNav = false;
            bReverseList = false;
            bUpdated = false;
            bInitialized = false;
            if (loaderInfo != null) 
            {
                loaderInfo.addEventListener(flash.events.Event.INIT, onComponentInit);
            }
            addEventListener(flash.events.Event.ADDED_TO_STAGE, onStageInit);
            addEventListener(flash.events.Event.REMOVED_FROM_STAGE, onStageDestruct);
            addEventListener(flash.events.KeyboardEvent.KEY_DOWN, onKeyDown);
            addEventListener(flash.events.KeyboardEvent.KEY_UP, onKeyUp);
            if (!needMobileScrollList) 
            {
                addEventListener(flash.events.MouseEvent.MOUSE_WHEEL, onMouseWheel);
            }
            if (border == null) 
            {
                throw new Error("No \'border\' clip found.  BSScrollingList requires a border rect to define its extents.");
            }
            EntryHolder_mc = new flash.display.MovieClip();
            addChildAt(EntryHolder_mc, getChildIndex(border) + 1);
            iSelectedIndex = -1;
            iSelectedClipIndex = -1;
            iScrollPosition = 0;
            iMaxScrollPosition = 0;
            iListItemsShown = 0;
            fListHeight = 0;
            iPlatform = 1;
        }

        public function get numListItems():uint
        {
            return uiNumListItems;
        }

        public function set numListItems(arg1:uint):*
        {
            uiNumListItems = arg1;
        }

        internal function get needMobileScrollList():Boolean
        {
            return Shared.AS3.COMPANIONAPP.CompanionAppMode.isOn;
        }

        public function set listEntryClass(rendererClass:String):*
        {
            ListEntryClass = flash.utils.getDefinitionByName(rendererClass) as Class;
            _itemRendererClassName = rendererClass;
        }

        public function get restoreListIndex():Boolean
        {
            return bRestoreListIndex;
        }

        public function set restoreListIndex(index:Boolean):*
        {
            bRestoreListIndex = index;
        }

        public function get disableSelection():Boolean
        {
            return bDisableSelection;
        }

        public function set disableSelection(disable:Boolean):*
        {
            bDisableSelection = disable;
        }

        protected function SetNumListItems(numItems:uint):*
        {
            if (ListEntryClass != null && numItems > 0) 
            {
				for(var i = 0; i < numItems; i++)
                {
                    var newEntry = GetNewListEntry(i);
                    if (newEntry == null) 
                    {
                        trace("BSScrollingList::SetNumListItems -- List Entry Class is invalid or does not derive from BSScrollingListEntry.");
                    }
                    else 
                    {
                        newEntry.clipIndex = i;
                        newEntry.addEventListener(flash.events.MouseEvent.MOUSE_OVER, onEntryRollover);
                        newEntry.addEventListener(flash.events.MouseEvent.CLICK, onEntryPress);
                        EntryHolder_mc.addChild(newEntry);
                    }
                }
                bInitialized = true;
                dispatchEvent(new flash.events.Event(LIST_ITEMS_CREATED, true, true));
            }
        }

        protected function GetNewListEntry(index:uint):Shared.AS3.BSScrollingListEntry
        {
            return new ListEntryClass() as Shared.AS3.BSScrollingListEntry;
        }

        public function UpdateList():*
        {
            var loc7:*=null;
            var loc8:*=null;
            if (!bInitialized || numListItems == 0) 
            {
                trace("BSScrollingList::UpdateList -- Can\'t update list before list has been created.");
            }
            var loc1:*=0;
            var loc2:*=this._filterer.ClampIndex(0);
            var loc3:*=loc2;
            var loc4:*=0;
            while (loc4 < this.EntriesA.length) 
            {
                this.EntriesA[loc4].clipIndex = int.MAX_VALUE;
                if (loc4 < this.iScrollPosition) 
                {
                    loc2 = this._filterer.GetNextFilterMatch(loc2);
                }
                ++loc4;
            }
            var loc5:*=0;
            while (loc5 < this.uiNumListItems) 
            {
				loc7 = this.GetClipByIndex(loc5);
                if (loc7) 
                {
                    loc7.visible = false;
                    loc7.itemIndex = int.MAX_VALUE;
                }
                ++loc5;
            }
            var loc6:*=new Vector.<Object>();
            this.iListItemsShown = 0;
            if (this.needMobileScrollList) 
            {
                while (!(loc3 == int.MAX_VALUE) && !(loc3 == -1) && loc3 < this.EntriesA.length && loc1 <= this.fListHeight) 
                {
                    loc6.push(this.EntriesA[loc3]);
                    loc3 = this._filterer.GetNextFilterMatch(loc3);
                }
            }
            while (!(loc2 == int.MAX_VALUE) && !(loc2 == -1) && loc2 < this.EntriesA.length && this.iListItemsShown < this.uiNumListItems && loc1 <= this.fListHeight) 
            {
				loc8 = this.GetClipByIndex(this.iListItemsShown)
                if (loc8) 
                {
                    this.SetEntry(loc8, this.EntriesA[loc2]);
                    this.EntriesA[loc2].clipIndex = this.iListItemsShown;
                    loc8.itemIndex = loc2;
                    loc8.visible = !this.needMobileScrollList;
                    loc1 = loc1 + loc8.height;
                    if (loc1 <= this.fListHeight && this.iListItemsShown < this.uiNumListItems) 
                    {
                        loc1 = loc1 + this.fVerticalSpacing;
                        var loc9:*;
                        var loc10:*=((loc9 = this).iListItemsShown + 1);
                        loc9.iListItemsShown = loc10;
                    }
                    else if (this.textOption == TEXT_OPTION_MULTILINE) 
                    {
                        loc10 = ((loc9 = this).iListItemsShown + 1);
                        loc9.iListItemsShown = loc10;
                    }
                    else 
                    {
                        this.EntriesA[loc2].clipIndex = int.MAX_VALUE;
                        loc8.visible = false;
                    }
                }
                loc2 = this._filterer.GetNextFilterMatch(loc2);
            }
            if (this.needMobileScrollList) 
            {
                this.setMobileScrollingListData(loc6);
            }
            this.PositionEntries();
            if (this.ScrollUp != null) 
            {
                this.ScrollUp.visible = this.scrollPosition > 0;
            }
            if (this.ScrollDown != null) 
            {
                this.ScrollDown.visible = this.scrollPosition < this.iMaxScrollPosition;
            }
            this.bUpdated = true;
        }

        public function get shownItemsHeight():Number
        {
            return fShownItemsHeight;
        }

        protected function PositionEntries():*
        {
            var totalHeight = 0;
            var borderHeight = border.y;
			for(var i = 0; i < iListItemsShown; i++)
            {
                GetClipByIndex(i).y = borderHeight + totalHeight;
                totalHeight += GetClipByIndex(i).height + fVerticalSpacing;
            }
            fShownItemsHeight = totalHeight;
        }

        public function InvalidateData():*
        {
            var selectionChanged = false;
            _filterer.filterArray = EntriesA;
            fListHeight = border.height;
            CalculateMaxScrollPosition();
            if (!restoreListIndex) 
            {
                if (iSelectedIndex >= EntriesA.length) 
                {
                    iSelectedIndex = EntriesA.length - 1;
                    selectionChanged = true;
                }
            }
            if (iScrollPosition > iMaxScrollPosition) 
            {
                iScrollPosition = iMaxScrollPosition;
            }
            UpdateList();
            if (restoreListIndex && !needMobileScrollList) 
            {
                selectedClipIndex = iSelectedClipIndex;
            }
            else if (selectionChanged) 
            {
                dispatchEvent(new flash.events.Event(SELECTION_CHANGE, true, true));
            }
        }

        public function UpdateSelectedEntry():*
        {
            if (iSelectedIndex != -1) 
            {
                SetEntry(GetClipByIndex(EntriesA[iSelectedIndex].clipIndex), EntriesA[iSelectedIndex]);
            }
        }

        public function UpdateEntry(entryObject:Object):*
        {
            SetEntry(GetClipByIndex(entryObject.clipIndex), entryObject);
        }

        public function onFilterChange():*
        {
            this.iSelectedIndex = this._filterer.ClampIndex(this.iSelectedIndex);
            this.CalculateMaxScrollPosition();
        }

        protected function CalculateMaxScrollPosition():*
        {
            var loc2:*=NaN;
            var loc3:*=0;
            var loc4:*=0;
            var loc5:*=0;
            var loc6:*=0;
            var loc7:*=0;
            var loc1:*=this._filterer.EntryMatchesFilter(this.EntriesA[(this.EntriesA.length - 1)]) ? (this.EntriesA.length - 1) : this._filterer.GetPrevFilterMatch((this.EntriesA.length - 1));
            if (loc1 != int.MAX_VALUE) 
            {
                loc2 = this.GetEntryHeight(loc1);
                loc3 = loc1;
                loc4 = 1;
                while (!(loc3 == int.MAX_VALUE) && loc2 < this.fListHeight && loc4 < this.uiNumListItems) 
                {
                    loc5 = loc3;
                    loc3 = this._filterer.GetPrevFilterMatch(loc3);
                    if (loc3 == int.MAX_VALUE) 
                    {
                        continue;
                    }
                    loc2 = loc2 + (this.GetEntryHeight(loc3) + this.fVerticalSpacing);
                    if (loc2 < this.fListHeight) 
                    {
                        ++loc4;
                        continue;
                    }
                    loc3 = loc5;
                }
                if (loc3 != int.MAX_VALUE) 
                {
                    loc6 = 0;
                    loc7 = this._filterer.GetPrevFilterMatch(loc3);
                    while (loc7 != int.MAX_VALUE) 
                    {
                        ++loc6;
                        loc7 = this._filterer.GetPrevFilterMatch(loc7);
                    }
                    this.iMaxScrollPosition = loc6;
                }
                else 
                {
                    this.iMaxScrollPosition = 0;
                }
            }
            else 
            {
                this.iMaxScrollPosition = 0;
            }
        }

        protected function GetEntryHeight(i:Number):Number
        {
            var clip = GetClipByIndex(0);
            SetEntry(clip, EntriesA[i]);
            return clip == null ? 0 : clip.height;
        }

        public function moveSelectionUp():*
        {
            if (bDisableSelection) 
            {
                scrollPosition--;
            }
            else if (selectedIndex > 0) 
            {
                var filteredIndex = _filterer.GetPrevFilterMatch(selectedIndex);
                if (filteredIndex != int.MAX_VALUE) 
                {
                    selectedIndex = filteredIndex;
                    bMouseDrivenNav = false;
                    dispatchEvent(new Event(PLAY_FOCUS_SOUND, true, true));
                }
            }
        }

        public function moveSelectionDown():*
        {
            if (bDisableSelection) 
            {
                scrollPosition++;
            }
            else if (selectedIndex < (EntriesA.length - 1)) 
            {
                var filteredIndex = _filterer.GetNextFilterMatch(selectedIndex);
                if (filteredIndex != int.MAX_VALUE) 
                {
                    selectedIndex = filteredIndex;
                    bMouseDrivenNav = false;
                    dispatchEvent(new Event(PLAY_FOCUS_SOUND, true, true));
                }
            }
        }

        protected function onItemPress():*
        {
            if (!this.bDisableInput && !this.bDisableSelection && !(this.iSelectedIndex == -1)) 
            {
                dispatchEvent(new flash.events.Event(ITEM_PRESS, true, true));
            }
            else 
            {
                dispatchEvent(new flash.events.Event(LIST_PRESS, true, true));
            }
        }

        protected function SetEntry(listEntry:Shared.AS3.BSScrollingListEntry, entryObject:Object):*
        {
            if (listEntry != null) 
            {
                listEntry.selected = entryObject == selectedEntry;
                listEntry.SetEntryText(entryObject, strTextOption);
            }
        }

        protected function onSetPlatform(arg1:flash.events.Event):*
        {
            var loc1:*=arg1 as Shared.PlatformChangeEvent;
            this.SetPlatform(loc1.uiPlatform, loc1.bPS3Switch);
        }

        public function SetPlatform(arg1:Number, arg2:Boolean):*
        {
            iPlatform = arg1;
            bMouseDrivenNav = iPlatform != 0 ? false : true;
        }

        protected function destroyMobileScrollingList():void
        {
            if (scrollList != null) 
            {
                scrollList.removeEventListener(Mobile.ScrollList.MobileScrollList.ITEM_SELECT, onMobileScrollListItemSelected);
                removeChild(scrollList);
                scrollList.destroy();
            }
        }

        protected function onMobileScrollListItemSelected(arg1:Mobile.ScrollList.EventWithParams):void
        {
            var loc1:*=arg1.params.renderer as Mobile.ScrollList.MobileListItemRenderer;
            if (loc1.data == null) 
            {
                return;
            }
            var loc2:*=loc1.data.id;
            var loc3:*=this.iSelectedIndex;
            this.iSelectedIndex = this.GetEntryFromClipIndex(loc2);
            var loc4:*=0;
            while (loc4 < this.EntriesA.length) 
            {
                if (this.EntriesA[loc4] == loc1.data) 
                {
                    this.iSelectedIndex = loc4;
                    break;
                }
                ++loc4;
            }
            if (!this.EntriesA[this.iSelectedIndex].isDivider) 
            {
                if (loc3 == this.iSelectedIndex) 
                {
                    if (this.scrollList.itemRendererLinkageId == Shared.AS3.COMPANIONAPP.BSScrollingListInterface.RADIO_RENDERER_LINKAGE_ID || this.scrollList.itemRendererLinkageId == Shared.AS3.COMPANIONAPP.BSScrollingListInterface.QUEST_RENDERER_LINKAGE_ID || this.scrollList.itemRendererLinkageId == Shared.AS3.COMPANIONAPP.BSScrollingListInterface.QUEST_OBJECTIVES_RENDERER_LINKAGE_ID || this.scrollList.itemRendererLinkageId == Shared.AS3.COMPANIONAPP.BSScrollingListInterface.INVENTORY_RENDERER_LINKAGE_ID || this.scrollList.itemRendererLinkageId == Shared.AS3.COMPANIONAPP.BSScrollingListInterface.PIPBOY_MESSAGE_RENDERER_LINKAGE_ID) 
                    {
                        this.onItemPress();
                    }
                }
                else 
                {
                    this.iSelectedClipIndex = this.iSelectedIndex == -1 ? -1 : this.EntriesA[this.iSelectedIndex].clipIndex;
                    dispatchEvent(new flash.events.Event(SELECTION_CHANGE, true, true));
                    if (this.scrollList.itemRendererLinkageId == Shared.AS3.COMPANIONAPP.BSScrollingListInterface.PIPBOY_MESSAGE_RENDERER_LINKAGE_ID) 
                    {
                        this.onItemPress();
                    }
                    dispatchEvent(new flash.events.Event(MOBILE_ITEM_PRESS, true, true));
                }
            }
        }

        protected function setMobileScrollingListData(arg1:__AS3__.vec.Vector.<Object>):void
        {
            if (arg1 == null) 
            {
                trace("setMobileScrollingListData::Error: No data received to display List Items!");
            }
            else if (arg1.length > 0) 
            {
                this.scrollList.setData(arg1);
            }
            else 
            {
                this.scrollList.invalidateData();
            }
        }

        public function onComponentInit(event:Event)
        {
            if (needMobileScrollList) 
            {
                createMobileScrollingList();
                if (border != null) 
                {
                    border.alpha = 0;
                }
            }
            if (loaderInfo != null) 
            {
                loaderInfo.removeEventListener(Event.INIT, onComponentInit);
            }
            if (!bInitialized) 
            {
                SetNumListItems(uiNumListItems);
            }
        }

        protected function onStageInit(event:Event)
        {
            stage.addEventListener(Shared.PlatformChangeEvent.PLATFORM_CHANGE, this.onSetPlatform);
            if (!bInitialized) 
            {
                SetNumListItems(uiNumListItems);
            }
            if (ScrollUp != null && !Shared.AS3.COMPANIONAPP.CompanionAppMode.isOn) 
            {
                ScrollUp.addEventListener(MouseEvent.CLICK, onScrollArrowClick);
            }
            if (ScrollDown != null && !Shared.AS3.COMPANIONAPP.CompanionAppMode.isOn) 
            {
                ScrollDown.addEventListener(MouseEvent.CLICK, onScrollArrowClick);
            }
        }

        protected function onStageDestruct(event:Event):*
        {
            stage.removeEventListener(Shared.PlatformChangeEvent.PLATFORM_CHANGE, onSetPlatform);
            if (needMobileScrollList) 
            {
                destroyMobileScrollingList();
            }
        }

        public function onScrollArrowClick(event:Event):*
        {
            if (!bDisableInput && !bDisableSelection) 
            {
                doSetSelectedIndex(-1);
                if (event.target == ScrollUp || event.target.parent == ScrollUp) 
                {
                    scrollPosition--;
                }
                else if (event.target == ScrollDown || event.target.parent == ScrollDown) 
                {
                    scrollPosition++;
                }
                event.stopPropagation();
            }
        }

        public function onEntryRollover(event:Event):*
        {
            bMouseDrivenNav = true;
            if (!bDisableInput && !bDisableSelection) 
            {
                var previousIndex = iSelectedIndex;
                doSetSelectedIndex((event.currentTarget as Shared.AS3.BSScrollingListEntry).itemIndex);
                if (previousIndex != iSelectedIndex) 
                {
                    dispatchEvent(new Event(PLAY_FOCUS_SOUND, true, true));
                }
            }
        }

        public function onEntryPress(event:MouseEvent):*
        {
            event.stopPropagation();
            bMouseDrivenNav = true;
            onItemPress();
        }

        public function ClearList():*
        {
            EntriesA.splice(0, EntriesA.length);
        }

        public function GetClipByIndex(index:uint):Shared.AS3.BSScrollingListEntry
        {
            return index < EntryHolder_mc.numChildren ? EntryHolder_mc.getChildAt(index) as Shared.AS3.BSScrollingListEntry : null;
        }

        public function GetEntryFromClipIndex(index:int):int
        {
            var foundIndex = -1;
			for(var i = 0; i < this.EntriesA.length; i++)
			{
                if (EntriesA[i].clipIndex <= index) 
                {
                    foundIndex = i;
                }
            }
            return foundIndex;
        }

        public function onKeyDown(event:KeyboardEvent):*
        {
            if (!bDisableInput) {
				if (event.keyCode == flash.ui.Keyboard.DOWN) {
                    moveSelectionDown();
                    event.stopPropagation();
                } else if (event.keyCode == flash.ui.Keyboard.UP) {
                    moveSelectionUp();
                    event.stopPropagation();
                }
            }
        }

        public function onKeyUp(event:KeyboardEvent):*
        {
            if (!bDisableInput && !bDisableSelection && event.keyCode == flash.ui.Keyboard.ENTER) 
            {
                onItemPress();
                event.stopPropagation();
            }
        }

        public function onMouseWheel(event:MouseEvent):*
        {
            if (!bDisableInput && !bDisableSelection && iMaxScrollPosition > 0) 
            {
                var previousIndex = scrollPosition;
                if (event.delta < 0) 
                {
                    scrollPosition++;
                }
                else if (event.delta > 0) 
                {
                    scrollPosition--;
                }
                SetFocusUnderMouse();
                event.stopPropagation();
                if (previousIndex != scrollPosition) 
                {
                    dispatchEvent(new Event(PLAY_FOCUS_SOUND, true, true));
                }
            }
        }

        internal function SetFocusUnderMouse():*
        {
            var clip:*=null;
            var border:*=null;
            var point:*=null;
			for(var i = 0; i < iListItemsShown; i++)
            {
                clip = this.GetClipByIndex(i);
                border = clip.border;
                point = localToGlobal(new flash.geom.Point(mouseX, mouseY));
                if (border.hitTestPoint(point.x, point.y, false)) 
                {
                    selectedIndex = clip.itemIndex;
                }
            }
        }

        public function get filterer():Shared.AS3.ListFilterer
        {
            return _filterer;
        }

        public function get itemsShown():uint
        {
            return iListItemsShown;
        }

        public function get initialized():Boolean
        {
            return bInitialized;
        }

        public function get selectedIndex():int
        {
            return iSelectedIndex;
        }

        public function set selectedIndex(index:int):*
        {
            doSetSelectedIndex(index);
        }

        public function get selectedClipIndex():int
        {
            return iSelectedClipIndex;
        }

        public function set selectedClipIndex(arg1:int):*
        {
            doSetSelectedIndex(this.GetEntryFromClipIndex(arg1));
        }

        public function set filterer(arg1:Shared.AS3.ListFilterer):*
        {
            _filterer = arg1;
        }

        protected function createMobileScrollingList():void
        {
            var loc1:*=NaN;
            var loc2:*=NaN;
            var loc3:*=NaN;
            var loc4:*=null;
            var loc5:*=false;
            var loc6:*=false;
            if (this._itemRendererClassName != null) 
            {
                loc1 = Shared.AS3.COMPANIONAPP.BSScrollingListInterface.GetMobileScrollListProperties(this._itemRendererClassName).maskDimension;
                loc2 = Shared.AS3.COMPANIONAPP.BSScrollingListInterface.GetMobileScrollListProperties(this._itemRendererClassName).spaceBetweenButtons;
                loc3 = Shared.AS3.COMPANIONAPP.BSScrollingListInterface.GetMobileScrollListProperties(this._itemRendererClassName).scrollDirection;
                loc4 = Shared.AS3.COMPANIONAPP.BSScrollingListInterface.GetMobileScrollListProperties(this._itemRendererClassName).linkageId;
                loc5 = Shared.AS3.COMPANIONAPP.BSScrollingListInterface.GetMobileScrollListProperties(this._itemRendererClassName).clickable;
                loc6 = Shared.AS3.COMPANIONAPP.BSScrollingListInterface.GetMobileScrollListProperties(this._itemRendererClassName).reversed;
                this.scrollList = new Mobile.ScrollList.MobileScrollList(loc1, loc2, loc3);
                this.scrollList.itemRendererLinkageId = loc4;
                this.scrollList.noScrollShortList = true;
                this.scrollList.clickable = loc5;
                this.scrollList.endListAlign = loc6;
                this.scrollList.textOption = this.strTextOption;
                this.scrollList.setScrollIndicators(this.ScrollUp, this.ScrollDown);
                this.scrollList.x = 0;
                this.scrollList.y = 0;
                addChild(this.scrollList);
                this.scrollList.addEventListener(Mobile.ScrollList.MobileScrollList.ITEM_SELECT, this.onMobileScrollListItemSelected, false, 0, true);
            }
            return;
        }

        protected function doSetSelectedIndex(index:int):*
        {
            var loc1:*=0;
            var loc2:*=false;
            var loc3:*=0;
            var loc4:*=null;
            var loc5:*=0;
            var loc6:*=0;
            var loc7:*=0;
            var loc8:*=0;
            var loc9:*=0;
            var loc10:*=0;
            if (!this.bInitialized || this.numListItems == 0) 
            {
                trace("BSScrollingList::doSetSelectedIndex -- Can\'t set selection before list has been created.");
            }
            if (!this.bDisableSelection && !(index == this.iSelectedIndex)) 
            {
                loc1 = this.iSelectedIndex;
                this.iSelectedIndex = index;
                if (this.EntriesA.length == 0) 
                {
                    this.iSelectedIndex = -1;
                }
                if (!(loc1 == -1) && loc1 < this.EntriesA.length && !(this.EntriesA[loc1].clipIndex == int.MAX_VALUE)) 
                {
                    this.SetEntry(this.GetClipByIndex(this.EntriesA[loc1].clipIndex), this.EntriesA[loc1]);
                }
                if (this.iSelectedIndex != -1) 
                {
                    this.iSelectedIndex = this._filterer.ClampIndex(this.iSelectedIndex);
                    if (this.iSelectedIndex == int.MAX_VALUE) 
                    {
                        this.iSelectedIndex = -1;
                    }
                    if (!(this.iSelectedIndex == -1) && !(loc1 == this.iSelectedIndex)) 
                    {
                        loc2 = false;
                        if (this.textOption == TEXT_OPTION_MULTILINE) 
                        {
                            if (!((loc3 = this.GetEntryFromClipIndex((this.uiNumListItems - 1))) == -1) && loc3 == this.iSelectedIndex && !(this.EntriesA[loc3].clipIndex == int.MAX_VALUE)) 
                            {
                                if (!((loc4 = this.GetClipByIndex(this.EntriesA[loc3].clipIndex)) == null) && loc4.y + loc4.height > this.fListHeight) 
                                {
                                    loc2 = true;
                                }
                            }
                        }
                        if (!(this.EntriesA[this.iSelectedIndex].clipIndex == int.MAX_VALUE) && !loc2) 
                        {
                            this.SetEntry(this.GetClipByIndex(this.EntriesA[this.iSelectedIndex].clipIndex), this.EntriesA[this.iSelectedIndex]);
                        }
                        else 
                        {
                            loc5 = this.GetEntryFromClipIndex(0);
                            loc6 = this.GetEntryFromClipIndex((this.uiNumListItems - 1));
                            loc8 = 0;
                            if (this.iSelectedIndex < loc5) 
                            {
                                loc7 = loc5;
                                do 
                                {
                                    loc7 = this._filterer.GetPrevFilterMatch(loc7);
                                    --loc8;
                                }
                                while (loc7 != this.iSelectedIndex);
                            }
                            else if (this.iSelectedIndex > loc6) 
                            {
                                loc7 = loc6;
                                do 
                                {
                                    loc7 = this._filterer.GetNextFilterMatch(loc7);
                                    ++loc8;
                                }
                                while (loc7 != this.iSelectedIndex);
                            }
                            else if (loc2) 
                            {
                                ++loc8;
                            }
                            this.scrollPosition = this.scrollPosition + loc8;
                        }
                    }
                    if (this.needMobileScrollList) 
                    {
                        if (this.scrollList != null) 
                        {
                            if (this.iSelectedIndex == -1) 
                            {
                                this.scrollList.selectedIndex = -1;
                            }
                            else 
                            {
                                loc9 = this.EntriesA[this.iSelectedIndex].clipIndex;
                                loc10 = 0;
                                while (loc10 < this.scrollList.data.length) 
                                {
                                    if (this.EntriesA[this.iSelectedIndex] == this.scrollList.data[loc10]) 
                                    {
                                        loc9 = loc10;
                                        break;
                                    }
                                    ++loc10;
                                }
                                this.scrollList.selectedIndex = loc9;
                            }
                        }
                    }
                }
                if (loc1 != this.iSelectedIndex) 
                {
                    this.iSelectedClipIndex = this.iSelectedIndex == -1 ? -1 : this.EntriesA[this.iSelectedIndex].clipIndex;
                    dispatchEvent(new flash.events.Event(SELECTION_CHANGE, true, true));
                }
            }
            return;
        }

        public function get scrollPosition():uint
        {
            return iScrollPosition;
        }

        public function get maxScrollPosition():uint
        {
            return iMaxScrollPosition;
        }

        public function set scrollPosition(index:uint)
        {
            if (index != iScrollPosition && index >= 0 && index <= iMaxScrollPosition) 
            {
                updateScrollPosition(index);
            }
        }

        protected function updateScrollPosition(index:uint):*
        {
            iScrollPosition = index;
            UpdateList();
        }

        public function get selectedEntry():Object
        {
            return EntriesA[iSelectedIndex];
        }

        public function get entryList():Array
        {
            return EntriesA;
        }

        public function set entryList(list:Array):*
        {
            EntriesA = list;
            if (EntriesA == null) 
            {
                EntriesA = new Array();
            }
        }

        public function get disableInput():Boolean
        {
            return bDisableInput;
        }

        public function set disableInput(disable:Boolean):*
        {
            bDisableInput = disable;
        }

        public function get textOption():String
        {
            return strTextOption;
        }

        public function set textOption(option:String):*
        {
            strTextOption = option;
        }

        public function get verticalSpacing():*
        {
            return fVerticalSpacing;
        }

        public function set verticalSpacing(spacing:Number):*
        {
            fVerticalSpacing = spacing;
        }
    }
}
