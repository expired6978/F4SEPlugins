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
            addEventListener(Shared.AS3.ListFilterer.FILTER_CHANGE, this.onFilterChange);
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
                loaderInfo.addEventListener(flash.events.Event.INIT, this.onComponentInit);
            }
            addEventListener(flash.events.Event.ADDED_TO_STAGE, onStageInit);
            addEventListener(flash.events.Event.REMOVED_FROM_STAGE, onStageDestruct);
            addEventListener(flash.events.KeyboardEvent.KEY_DOWN, onKeyDown);
            addEventListener(flash.events.KeyboardEvent.KEY_UP, onKeyUp);
            if (!needMobileScrollList) 
            {
                addEventListener(flash.events.MouseEvent.MOUSE_WHEEL, onMouseWheel);
            }
            if (this.border == null) 
            {
                throw new Error("No \'border\' clip found.  BSScrollingList requires a border rect to define its extents.");
            }
            EntryHolder_mc = new flash.display.MovieClip();
            addChildAt(EntryHolder_mc, getChildIndex(this.border) + 1);
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

        public function set listEntryClass(arg1:String):*
        {
            ListEntryClass = flash.utils.getDefinitionByName(arg1) as Class;
            _itemRendererClassName = arg1;
        }

        public function get restoreListIndex():Boolean
        {
            return bRestoreListIndex;
        }

        public function set restoreListIndex(arg1:Boolean):*
        {
            bRestoreListIndex = arg1;
        }

        public function get disableSelection():Boolean
        {
            return bDisableSelection;
        }

        public function set disableSelection(arg1:Boolean):*
        {
            bDisableSelection = arg1;
        }

        protected function SetNumListItems(arg1:uint):*
        {
            var loc1:*=0;
            var loc2:*=null;
            if (!(this.ListEntryClass == null) && arg1 > 0) 
            {
                loc1 = 0;
                while (loc1 < arg1) 
                {
                    loc2 = this.GetNewListEntry(loc1);
                    if (loc2 == null) 
                    {
                        trace("BSScrollingList::SetNumListItems -- List Entry Class is invalid or does not derive from BSScrollingListEntry.");
                    }
                    else 
                    {
                        loc2.clipIndex = loc1;
                        loc2.addEventListener(flash.events.MouseEvent.MOUSE_OVER, this.onEntryRollover);
                        loc2.addEventListener(flash.events.MouseEvent.CLICK, this.onEntryPress);
                        this.EntryHolder_mc.addChild(loc2);
                    }
                    ++loc1;
                }
                this.bInitialized = true;
                dispatchEvent(new flash.events.Event(LIST_ITEMS_CREATED, true, true));
            }
            return;
        }

        protected function GetNewListEntry(arg1:uint):Shared.AS3.BSScrollingListEntry
        {
            return new this.ListEntryClass() as Shared.AS3.BSScrollingListEntry;
        }

        public function UpdateList():*
        {
            var loc7:*=null;
            if (!bInitialized || numListItems == 0) 
            {
                trace("BSScrollingList::UpdateList -- Can\'t update list before list has been created.");
            }
            var loc1:*=0;
            var loc2:*=_filterer.ClampIndex(0);
            var loc3:*=loc2;
            var loc4:*=0;
            while (loc4 < EntriesA.length) 
            {
                EntriesA[loc4].clipIndex = int.MAX_VALUE;
                if (loc4 < iScrollPosition) 
                {
                    loc2 = _filterer.GetNextFilterMatch(loc2);
                }
                ++loc4;
            }
            var loc5:*=0;
            while (loc5 < uiNumListItems) 
            {
				loc7 = GetClipByIndex(loc5);
                if (loc7) 
                {
                    loc7.visible = false;
                    loc7.itemIndex = int.MAX_VALUE;
                }
                ++loc5;
            }
            var loc6:*=new Vector.<Object>();
            iListItemsShown = 0;
            if (needMobileScrollList) 
            {
                while (!(loc3 == int.MAX_VALUE) && !(loc3 == -1) && loc3 < EntriesA.length && loc1 <= fListHeight) 
                {
                    loc6.push(EntriesA[loc3]);
                    loc3 = _filterer.GetNextFilterMatch(loc3);
                }
            }
            while (!(loc2 == int.MAX_VALUE) && !(loc2 == -1) && loc2 < EntriesA.length && iListItemsShown < uiNumListItems && loc1 <= fListHeight) 
            {
				var clip = GetClipByIndex(iListItemsShown)
                if (clip) 
                {
                    SetEntry(clip, EntriesA[loc2]);
                    EntriesA[loc2].clipIndex = iListItemsShown;
                    loc8.itemIndex = loc2;
                    loc8.visible = !needMobileScrollList;
                    loc1 = loc1 + clip.height;
                    if (loc1 <= fListHeight && iListItemsShown < uiNumListItems) 
                    {
                        loc1 = loc1 + fVerticalSpacing;
                        iListItemsShown = iListItemsShown + 1;
                    }
                    else if (textOption == TEXT_OPTION_MULTILINE) 
                    {
                        iListItemsShown = iListItemsShown + 1;
                    }
                    else 
                    {
                        EntriesA[loc2].clipIndex = int.MAX_VALUE;
                        clip.visible = false;
                    }
                }
                loc2 = _filterer.GetNextFilterMatch(loc2);
            }
            if (this.needMobileScrollList) 
            {
                this.setMobileScrollingListData(loc6);
            }
            PositionEntries();
            if (this.ScrollUp != null) 
            {
                this.ScrollUp.visible = scrollPosition > 0;
            }
            if (this.ScrollDown != null) 
            {
                this.ScrollDown.visible = scrollPosition < iMaxScrollPosition;
            }
            bUpdated = true;
        }

        public function get shownItemsHeight():Number
        {
            return fShownItemsHeight;
        }

        protected function PositionEntries():*
        {
            var loc1:*=0;
            var loc2:*=this.border.y;
            var loc3:*=0;
            while (loc3 < this.iListItemsShown) 
            {
                GetClipByIndex(loc3).y = loc2 + loc1;
                loc1 = loc1 + (GetClipByIndex(loc3).height + fVerticalSpacing);
                ++loc3;
            }
            fShownItemsHeight = loc1;
        }

        public function InvalidateData():*
        {
            var loc1:*=false;
            _filterer.filterArray = EntriesA;
            fListHeight = this.border.height;
            CalculateMaxScrollPosition();
            if (!restoreListIndex) 
            {
                if (iSelectedIndex >= EntriesA.length) 
                {
                    iSelectedIndex = (EntriesA.length - 1);
                    loc1 = true;
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
            else if (loc1) 
            {
                dispatchEvent(new flash.events.Event(SELECTION_CHANGE, true, true));
            }
            return;
        }

        public function UpdateSelectedEntry():*
        {
            if (iSelectedIndex != -1) 
            {
                SetEntry(GetClipByIndex(EntriesA[iSelectedIndex].clipIndex), EntriesA[iSelectedIndex]);
            }
        }

        public function UpdateEntry(arg1:Object):*
        {
            SetEntry(GetClipByIndex(arg1.clipIndex), arg1);
        }

        public function onFilterChange():*
        {
            iSelectedIndex = _filterer.ClampIndex(iSelectedIndex);
            CalculateMaxScrollPosition();
        }

        protected function CalculateMaxScrollPosition():*
        {
            var loc2:*=NaN;
            var loc3:*=0;
            var loc4:*=0;
            var loc5:*=0;
            var loc6:*=0;
            var loc7:*=0;
            var loc1:*=_filterer.EntryMatchesFilter(EntriesA[(EntriesA.length - 1)]) ? (EntriesA.length - 1) : _filterer.GetPrevFilterMatch((EntriesA.length - 1));
            if (loc1 != int.MAX_VALUE) 
            {
                loc2 = GetEntryHeight(loc1);
                loc3 = loc1;
                loc4 = 1;
                while (!(loc3 == int.MAX_VALUE) && loc2 < fListHeight && loc4 < uiNumListItems) 
                {
                    loc5 = loc3;
                    loc3 = _filterer.GetPrevFilterMatch(loc3);
                    if (loc3 == int.MAX_VALUE) 
                    {
                        continue;
                    }
                    loc2 = loc2 + (GetEntryHeight(loc3) + fVerticalSpacing);
                    if (loc2 < fListHeight) 
                    {
                        ++loc4;
                        continue;
                    }
                    loc3 = loc5;
                }
                if (loc3 != int.MAX_VALUE) 
                {
                    loc6 = 0;
                    loc7 = _filterer.GetPrevFilterMatch(loc3);
                    while (loc7 != int.MAX_VALUE) 
                    {
                        ++loc6;
                        loc7 = _filterer.GetPrevFilterMatch(loc7);
                    }
                    iMaxScrollPosition = loc6;
                }
                else 
                {
                    iMaxScrollPosition = 0;
                }
            }
            else 
            {
                iMaxScrollPosition = 0;
            }
        }

        protected function GetEntryHeight(arg1:Number):Number
        {
            var loc1:*=GetClipByIndex(0);
            SetEntry(loc1, EntriesA[arg1]);
            return loc1 == null ? 0 : loc1.height;
        }

        public function moveSelectionUp():*
        {
            var loc1:*=NaN;
            if (bDisableSelection) 
            {
                --scrollPosition;
            }
            else if (selectedIndex > 0) 
            {
                loc1 = _filterer.GetPrevFilterMatch(selectedIndex);
                if (loc1 != int.MAX_VALUE) 
                {
                    selectedIndex = loc1;
                    bMouseDrivenNav = false;
                    dispatchEvent(new flash.events.Event(PLAY_FOCUS_SOUND, true, true));
                }
            }
        }

        public function moveSelectionDown():*
        {
            var loc1:*=NaN;
            if (bDisableSelection) 
            {
                scrollPosition = scrollPosition + 1;
            }
            else if (selectedIndex < (EntriesA.length - 1)) 
            {
                loc1 = _filterer.GetNextFilterMatch(selectedIndex);
                if (_filterer.GetNextFilterMatch(selectedIndex) != int.MAX_VALUE) 
                {
                    selectedIndex = loc1;
                    bMouseDrivenNav = false;
                    dispatchEvent(new flash.events.Event(PLAY_FOCUS_SOUND, true, true));
                }
            }
        }

        protected function onItemPress():*
        {
            if (!bDisableInput && !bDisableSelection && !(iSelectedIndex == -1)) 
            {
                dispatchEvent(new flash.events.Event(ITEM_PRESS, true, true));
            }
            else 
            {
                dispatchEvent(new flash.events.Event(LIST_PRESS, true, true));
            }
        }

        protected function SetEntry(arg1:Shared.AS3.BSScrollingListEntry, arg2:Object):*
        {
            if (arg1 != null) 
            {
                arg1.selected = arg2 == selectedEntry;
                arg1.SetEntryText(arg2, strTextOption);
            }
        }

        protected function onSetPlatform(arg1:flash.events.Event):*
        {
            var loc1:*=arg1 as Shared.PlatformChangeEvent;
            SetPlatform(loc1.uiPlatform, loc1.bPS3Switch);
        }

        public function SetPlatform(arg1:Number, arg2:Boolean):*
        {
            iPlatform = arg1;
            bMouseDrivenNav = this.iPlatform != 0 ? false : true;
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
            return;
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
            return;
        }

        public function onComponentInit(arg1:flash.events.Event):*
        {
            if (this.needMobileScrollList) 
            {
                this.createMobileScrollingList();
                if (this.border != null) 
                {
                    this.border.alpha = 0;
                }
            }
            if (loaderInfo != null) 
            {
                loaderInfo.removeEventListener(flash.events.Event.INIT, this.onComponentInit);
            }
            if (!this.bInitialized) 
            {
                this.SetNumListItems(this.uiNumListItems);
            }
        }

        protected function onStageInit(arg1:flash.events.Event):*
        {
            stage.addEventListener(Shared.PlatformChangeEvent.PLATFORM_CHANGE, this.onSetPlatform);
            if (!this.bInitialized) 
            {
                this.SetNumListItems(this.uiNumListItems);
            }
            if (!(this.ScrollUp == null) && !Shared.AS3.COMPANIONAPP.CompanionAppMode.isOn) 
            {
                this.ScrollUp.addEventListener(flash.events.MouseEvent.CLICK, onScrollArrowClick);
            }
            if (!(this.ScrollDown == null) && !Shared.AS3.COMPANIONAPP.CompanionAppMode.isOn) 
            {
                this.ScrollDown.addEventListener(flash.events.MouseEvent.CLICK, onScrollArrowClick);
            }
        }

        protected function onStageDestruct(arg1:flash.events.Event):*
        {
            stage.removeEventListener(Shared.PlatformChangeEvent.PLATFORM_CHANGE, onSetPlatform);
            if (needMobileScrollList) 
            {
                destroyMobileScrollingList();
            }
        }

        public function onScrollArrowClick(event:flash.events.Event):*
        {
            if (!bDisableInput && !bDisableSelection) 
            {
                doSetSelectedIndex(-1);
                if (event.target == this.ScrollUp || event.target.parent == this.ScrollUp) 
                {
                    --scrollPosition;
                }
                else if (event.target == this.ScrollDown || event.target.parent == this.ScrollDown) 
                {
                    scrollPosition = scrollPosition + 1;
                }
                event.stopPropagation();
            }
        }

        public function onEntryRollover(event:flash.events.Event):*
        {
            var index:*=undefined;
            bMouseDrivenNav = true;
            if (!bDisableInput && !this.bDisableSelection) 
            {
                index = iSelectedIndex;
                doSetSelectedIndex((event.currentTarget as Shared.AS3.BSScrollingListEntry).itemIndex);
                if (index != iSelectedIndex) 
                {
                    dispatchEvent(new flash.events.Event(PLAY_FOCUS_SOUND, true, true));
                }
            }
        }

        public function onEntryPress(event:flash.events.MouseEvent):*
        {
            event.stopPropagation();
            bMouseDrivenNav = true;
            onItemPress();
        }

        public function ClearList():*
        {
            EntriesA.splice(0, EntriesA.length);
        }

        public function GetClipByIndex(arg1:uint):Shared.AS3.BSScrollingListEntry
        {
            return arg1 < EntryHolder_mc.numChildren ? EntryHolder_mc.getChildAt(arg1) as Shared.AS3.BSScrollingListEntry : null;
        }

        public function GetEntryFromClipIndex(arg1:int):int
        {
            var loc1:*=-1;
            var loc2:*=0;
            while (loc2 < EntriesA.length) 
            {
                if (EntriesA[loc2].clipIndex <= arg1) 
                {
                    loc1 = loc2;
                }
                ++loc2;
            }
            return loc1;
        }

        public function onKeyDown(event:flash.events.KeyboardEvent)
        {
            if (!bDisableInput) 
            {
                if (event.keyCode == flash.ui.Keyboard.UP) 
                {
					moveSelectionUp();
                    event.stopPropagation();
				}
				else if (event.keyCode == flash.ui.Keyboard.DOWN) 
                {
                    moveSelectionDown();
                    event.stopPropagation();
                }
            }
        }

        public function onKeyUp(event:flash.events.KeyboardEvent)
        {
            if (!bDisableInput && !bDisableSelection && event.keyCode == flash.ui.Keyboard.ENTER) 
            {
                onItemPress();
                event.stopPropagation();
            }
        }

        public function onMouseWheel(event:flash.events.MouseEvent)
        {
            if (!bDisableInput && !bDisableSelection && iMaxScrollPosition > 0) 
            {
                var scrollPos = scrollPosition;
                if (event.delta < 0) 
                {
                    scrollPosition = scrollPosition + 1;
                }
                else if (event.delta > 0) 
                {
                    --scrollPosition;
                }
                SetFocusUnderMouse();
                event.stopPropagation();
                if (scrollPos != scrollPosition) 
                {
                    dispatchEvent(new flash.events.Event(PLAY_FOCUS_SOUND, true, true));
                }
            }
        }

        internal function SetFocusUnderMouse()
        {
            var clip:*=null;
            var border:*=null;
            var point:*=null;
            var i:*=0;
            while (i < this.iListItemsShown) 
            {
                clip = this.GetClipByIndex(i);
                border = clip.border;
                point = localToGlobal(new flash.geom.Point(mouseX, mouseY));
                if (clip.hitTestPoint(point.x, point.y, false)) 
                {
                    this.selectedIndex = clip.itemIndex;
                }
                ++i;
            }
            return;
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

        public function set selectedClipIndex(index:int):*
        {
            doSetSelectedIndex(GetEntryFromClipIndex(index));
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
            var currentIndex:*=0;
            var loc2:*=false;
            var loc3:*=0;
            var loc4:*=null;
            var loc5:*=0;
            var loc6:*=0;
            var loc7:*=0;
            var loc8:*=0;
            var loc9:*=0;
            var loc10:*=0;
            if (!bInitialized || numListItems == 0) 
            {
                trace("BSScrollingList::doSetSelectedIndex -- Can\'t set selection before list has been created.");
            }
            if (!bDisableSelection && !(index == iSelectedIndex)) 
            {
                currentIndex = iSelectedIndex;
                iSelectedIndex = index;
                if (EntriesA.length == 0) 
                {
                    iSelectedIndex = -1;
                }
                if (!(currentIndex == -1) && currentIndex < EntriesA.length && !(EntriesA[currentIndex].clipIndex == int.MAX_VALUE)) 
                {
                    SetEntry(GetClipByIndex(EntriesA[currentIndex].clipIndex), EntriesA[currentIndex]);
                }
                if (iSelectedIndex != -1) 
                {
                    iSelectedIndex = _filterer.ClampIndex(iSelectedIndex);
                    if (iSelectedIndex == int.MAX_VALUE) 
                    {
                        iSelectedIndex = -1;
                    }
                    if (!(iSelectedIndex == -1) && !(currentIndex == iSelectedIndex)) 
                    {
                        loc2 = false;
                        if (textOption == TEXT_OPTION_MULTILINE) 
                        {
                            if (!((loc3 = GetEntryFromClipIndex((uiNumListItems - 1))) == -1) && loc3 == iSelectedIndex && !(EntriesA[loc3].clipIndex == int.MAX_VALUE)) 
                            {
                                if (!((loc4 = GetClipByIndex(EntriesA[loc3].clipIndex)) == null) && loc4.y + loc4.height > fListHeight) 
                                {
                                    loc2 = true;
                                }
                            }
                        }
                        if (!(EntriesA[iSelectedIndex].clipIndex == int.MAX_VALUE) && !loc2) 
                        {
                            SetEntry(GetClipByIndex(EntriesA[iSelectedIndex].clipIndex), EntriesA[iSelectedIndex]);
                        }
                        else 
                        {
                            loc5 = GetEntryFromClipIndex(0);
                            loc6 = GetEntryFromClipIndex((uiNumListItems - 1));
                            loc8 = 0;
                            if (iSelectedIndex < loc5) 
                            {
                                loc7 = loc5;
                                do 
                                {
                                    loc7 = _filterer.GetPrevFilterMatch(loc7);
                                    --loc8;
                                }
                                while (loc7 != iSelectedIndex);
                            }
                            else if (iSelectedIndex > loc6) 
                            {
                                loc7 = loc6;
                                do 
                                {
                                    loc7 = _filterer.GetNextFilterMatch(loc7);
                                    ++loc8;
                                }
                                while (loc7 != iSelectedIndex);
                            }
                            else if (loc2) 
                            {
                                ++loc8;
                            }
                            scrollPosition = scrollPosition + loc8;
                        }
                    }
                    if (needMobileScrollList) 
                    {
                        if (scrollList != null) 
                        {
                            if (iSelectedIndex == -1) 
                            {
                                scrollList.selectedIndex = -1;
                            }
                            else 
                            {
                                loc9 = EntriesA[iSelectedIndex].clipIndex;
                                loc10 = 0;
                                while (loc10 < scrollList.data.length) 
                                {
                                    if (EntriesA[iSelectedIndex] == scrollList.data[loc10]) 
                                    {
                                        loc9 = loc10;
                                        break;
                                    }
                                    ++loc10;
                                }
                                scrollList.selectedIndex = loc9;
                            }
                        }
                    }
                }
                if (currentIndex != iSelectedIndex) 
                {
                    iSelectedClipIndex = iSelectedIndex == -1 ? -1 : EntriesA[iSelectedIndex].clipIndex;
                    dispatchEvent(new flash.events.Event(SELECTION_CHANGE, true, true));
                }
            }
        }

        public function get scrollPosition():uint
        {
            return iScrollPosition;
        }

        public function get maxScrollPosition():uint
        {
            return iMaxScrollPosition;
        }

        public function set scrollPosition(scrollPos:uint):*
        {
            if (!(scrollPos == iScrollPosition) && scrollPos >= 0 && scrollPos <= iMaxScrollPosition) 
            {
                updateScrollPosition(scrollPos);
            }
        }

        protected function updateScrollPosition(scrollPos:uint):*
        {
            iScrollPosition = scrollPos;
            UpdateList();
        }

        public function get selectedEntry():Object
        {
            return EntriesA[this.iSelectedIndex];
        }

        public function get entryList():Array
        {
            return EntriesA;
        }

        public function set entryList(entries:Array):*
        {
            EntriesA = entries;
            if (EntriesA == null) 
            {
                EntriesA = new Array();
            }
        }

        public function get disableInput():Boolean
        {
            return bDisableInput;
        }

        public function set disableInput(disable:Boolean)
        {
            bDisableInput = disable;
        }

        public function get textOption():String
        {
            return strTextOption;
        }

        public function set textOption(option:String)
        {
            strTextOption = option;
        }

        public function get verticalSpacing():Number
        {
            return fVerticalSpacing;
        }

        public function set verticalSpacing(spacing:Number)
        {
            fVerticalSpacing = spacing;
        }
    }
}
