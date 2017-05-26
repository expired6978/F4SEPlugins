package Mobile.ScrollList 
{
    import Shared.*;
    import Shared.AS3.*;
    import __AS3__.vec.*;
    import flash.display.*;
    import flash.events.*;
    import flash.geom.*;
    
    public class MobileScrollList extends flash.display.MovieClip
    {
        public function MobileScrollList(arg1:Number, arg2:Number=0, arg3:uint=1)
        {
            this._mouseDownPoint = new flash.geom.Point();
            this._prevMouseDownPoint = new flash.geom.Point();
            super();
            this._scrollDim = arg1;
            this._deltaBetweenButtons = arg2;
            this._direction = arg3;
            this._selectedIndex = -1;
            this._tempSelectedIndex = -1;
            this._position = NaN;
            this.hasBackground = false;
            this.noScrollShortList = false;
            this._clickable = true;
            this.endListAlign = false;
            this._availableRenderers = new Vector.<Mobile.ScrollList.MobileListItemRenderer>();
            return;
        }

        public function get clickable():Boolean
        {
            return this._clickable;
        }

        public function set clickable(arg1:Boolean):void
        {
            this._clickable = arg1;
            return;
        }

        public function get data():__AS3__.vec.Vector.<Object>
        {
            return this._data;
        }

        public function get endListAlign():Boolean
        {
            return this._endListAlign;
        }

        public function set endListAlign(arg1:Boolean):void
        {
            this._endListAlign = arg1;
            return;
        }

        public function get textOption():String
        {
            return this._textOption;
        }

        public function set textOption(arg1:String):void
        {
            this._textOption = arg1;
            return;
        }

        public function get elasticity():Boolean
        {
            return this._elasticity;
        }

        public function get renderers():__AS3__.vec.Vector.<Mobile.ScrollList.MobileListItemRenderer>
        {
            return this._renderers;
        }

        public function set elasticity(arg1:Boolean):void
        {
            this._elasticity = arg1;
            return;
        }

        public function get itemRendererLinkageId():String
        {
            return this._itemRendererLinkageId;
        }

        public function get selectedIndex():int
        {
            return this._selectedIndex;
        }

        public function set selectedIndex(arg1:int):void
        {
            var loc1:*=this.getRendererAt(this._selectedIndex);
            if (loc1 != null) 
            {
                loc1.unselectItem();
            }
            this._selectedIndex = arg1;
            var loc2:*=this.getRendererAt(this._selectedIndex);
            if (loc2 != null) 
            {
                loc2.selectItem();
            }
            this.setPosition();
            return;
        }

        public function get selectedRenderer():Mobile.ScrollList.MobileListItemRenderer
        {
            return this.getRendererAt(this.selectedIndex);
        }

        public function invalidateData():void
        {
            var loc1:*=0;
            if (this._data != null) 
            {
                loc1 = 0;
                while (loc1 < this._data.length) 
                {
                    this.removeRenderer(loc1);
                    ++loc1;
                }
            }
            if (this._scrollMask != null) 
            {
                removeChild(this._scrollMask);
                this._scrollMask = null;
            }
            if (this._background != null) 
            {
                removeChild(this._background);
                this._background = null;
            }
            if (this._touchZone != null) 
            {
                this._scrollList.removeChild(this._touchZone);
                this._touchZone = null;
            }
            if (this._scrollList != null) 
            {
                if (this._scrollList.stage != null) 
                {
                    this._scrollList.stage.removeEventListener(flash.events.MouseEvent.MOUSE_UP, this.mouseUpHandler);
                    this._scrollList.stage.removeEventListener(flash.events.MouseEvent.MOUSE_MOVE, this.mouseMoveHandler);
                }
                this._scrollList.mask = null;
            }
            this._tempSelectedIndex = -1;
            this._bounds = null;
            this._data = null;
            this._renderers = null;
            this._mouseDown = false;
            return;
        }

        public function get position():Number
        {
            return this._position;
        }

        public function set position(arg1:Number):void
        {
            this._position = arg1;
            return;
        }

        public function set needFullRefresh(arg1:Boolean):void
        {
            if (arg1) 
            {
                this._selectedIndex = -1;
                this._position = NaN;
                this.setPosition();
            }
            return;
        }

        internal function get canScroll():Boolean
        {
            var loc1:*=this._direction != HORIZONTAL ? this._scrollList.height < this._bounds.height : this._scrollList.width < this._bounds.width;
            if (!(this.noScrollShortList && loc1)) 
            {
                return true;
            }
            return false;
        }

        public function setData(arg1:__AS3__.vec.Vector.<Object>):void
        {
            var loc1:*=0;
            this.invalidateData();
            this._data = arg1;
            if (this.endListAlign) 
            {
                this._data.reverse();
            }
            this._renderers = new Vector.<Mobile.ScrollList.MobileListItemRenderer>();
            if (this._scrollList == null) 
            {
                this._scrollList = new flash.display.Sprite();
                addChild(this._scrollList);
                this._scrollList.addEventListener(flash.events.MouseEvent.MOUSE_DOWN, this.mouseDownHandler, false, 0, true);
                this._scrollList.addEventListener(flash.events.Event.ENTER_FRAME, this.enterFrameHandler, false, 0, true);
            }
            loc1 = 0;
            while (loc1 < this._data.length) 
            {
                this._renderers.push(this.addRenderer(loc1, this._data[loc1]));
                ++loc1;
            }
            if (this._deltaBetweenButtons > 0) 
            {
                this._touchZone = this.createSprite(16776960, new flash.geom.Rectangle(0, 0, this._scrollList.width, this._scrollList.height), 0);
                this._scrollList.addChildAt(this._touchZone, 0);
            }
            this._bounds = this._direction != HORIZONTAL ? new flash.geom.Rectangle(0, 0, this._rendererRect.width, this._scrollDim) : new flash.geom.Rectangle(0, 0, this._scrollDim, this._rendererRect.height);
            this.createMask();
            if (this.hasBackground) 
            {
                this.createBackground();
            }
            this.selectedIndex = this._selectedIndex;
            if (!this.canScroll) 
            {
                if (this._prevIndicator) 
                {
                    this._prevIndicator.visible = false;
                }
                if (this._nextIndicator) 
                {
                    this._nextIndicator.visible = false;
                }
            }
            this.setDataOnVisibleRenderers();
            return;
        }

        public function setScrollIndicators(arg1:flash.display.MovieClip, arg2:flash.display.MovieClip):void
        {
            this._prevIndicator = arg1;
            this._nextIndicator = arg2;
            if (this._prevIndicator) 
            {
                this._prevIndicator.visible = false;
            }
            if (this._nextIndicator) 
            {
                this._nextIndicator.visible = false;
            }
            return;
        }

        protected function setPosition():void
        {
            var loc4:*=NaN;
            if (this._data == null) 
            {
                return;
            }
            var loc1:*=this._direction != HORIZONTAL ? this._scrollList.height : this._scrollList.width;
            var loc2:*=this._direction != HORIZONTAL ? this._bounds.height : this._bounds.width;
            var loc3:*=this._direction != HORIZONTAL ? this._scrollList.y : this._scrollList.x;
            if (isNaN(this.position)) 
            {
                if (this.selectedIndex == -1) 
                {
                    if (this._direction != HORIZONTAL) 
                    {
                        this._scrollList.y = this.endListAlign ? loc2 - loc1 : 0;
                    }
                    else 
                    {
                        this._scrollList.x = this.endListAlign ? loc2 - loc1 : 0;
                    }
                    this.setDataOnVisibleRenderers();
                    return;
                }
                else 
                {
                    loc4 = this._direction != HORIZONTAL ? this.selectedRenderer.y : this.selectedRenderer.x;
                    if (this.canScroll) 
                    {
                        if (loc1 - loc4 < loc2) 
                        {
                            this._position = loc2 - loc1;
                        }
                        else 
                        {
                            this._position = -loc4;
                        }
                    }
                    else 
                    {
                        this._position = this.endListAlign ? loc2 - loc1 : 0;
                    }
                }
            }
            else if (this.canScroll) 
            {
                if (this._position + loc1 < loc2) 
                {
                    this._position = loc2 - loc1;
                }
                else if (this._position > 0) 
                {
                    this._position = 0;
                }
            }
            else 
            {
                this._position = this.endListAlign ? loc2 - loc1 : 0;
            }
            if (this._direction != HORIZONTAL) 
            {
                this._scrollList.y = this._position;
            }
            else 
            {
                this._scrollList.x = this._position;
            }
            this.setDataOnVisibleRenderers();
            return;
        }

        public function set itemRendererLinkageId(arg1:String):void
        {
            this._itemRendererLinkageId = arg1;
            return;
        }

        protected function addRenderer(arg1:int, arg2:Object):Mobile.ScrollList.MobileListItemRenderer
        {
            var loc3:*=null;
            var loc1:*=this.acquireRenderer();
            loc1.reset();
            var loc2:*=0;
            if (arg1 > 0) 
            {
                loc2 = (loc3 = this.getRendererAt((arg1 - 1))).y + loc3.height + this._deltaBetweenButtons;
            }
            loc1.y = loc2;
            if (this._textOption === Shared.AS3.BSScrollingList.TEXT_OPTION_MULTILINE) 
            {
                this.setRendererData(loc1, arg2, arg1);
            }
            loc1.visible = true;
            return loc1;
        }

        protected function addRendererListeners(arg1:Mobile.ScrollList.MobileListItemRenderer):void
        {
            arg1.addEventListener(ITEM_SELECT, this.itemSelectHandler, false, 0, true);
            arg1.addEventListener(ITEM_RELEASE, this.itemReleaseHandler, false, 0, true);
            arg1.addEventListener(ITEM_RELEASE_OUTSIDE, this.itemReleaseOutsideHandler, false, 0, true);
            return;
        }

        protected function removeRenderer(arg1:int):void
        {
            var loc1:*=this._renderers[arg1];
            if (loc1 != null) 
            {
                loc1.visible = false;
                loc1.y = 0;
                this.releaseRenderer(loc1);
            }
            return;
        }

        protected function removeRendererListeners(arg1:Mobile.ScrollList.MobileListItemRenderer):void
        {
            arg1.removeEventListener(ITEM_SELECT, this.itemSelectHandler);
            arg1.removeEventListener(ITEM_RELEASE, this.itemReleaseHandler);
            arg1.removeEventListener(ITEM_RELEASE_OUTSIDE, this.itemReleaseOutsideHandler);
            return;
        }

        protected function getRendererAt(arg1:int):Mobile.ScrollList.MobileListItemRenderer
        {
            if (this._data == null || this._renderers == null || arg1 > (this._data.length - 1) || arg1 < 0) 
            {
                return null;
            }
            if (this.endListAlign) 
            {
                return this._renderers[(this._data.length - 1) - arg1];
            }
            return this._renderers[arg1];
        }

        internal function acquireRenderer():Mobile.ScrollList.MobileListItemRenderer
        {
            var loc1:*=null;
            if (this._availableRenderers.length > 0) 
            {
                return this._availableRenderers.pop();
            }
            loc1 = Mobile.ScrollList.FlashUtil.getLibraryItem(this, this._itemRendererLinkageId) as Mobile.ScrollList.MobileListItemRenderer;
            this._scrollList.addChild(loc1);
            if (this._rendererRect === null) 
            {
                this._rendererRect = new flash.geom.Rectangle(loc1.x, loc1.y, loc1.width, loc1.height);
            }
            this.addRendererListeners(loc1);
            return loc1;
        }

        internal function releaseRenderer(arg1:Mobile.ScrollList.MobileListItemRenderer):void
        {
            this._availableRenderers.push(arg1);
            return;
        }

        protected function resetPressState(arg1:Mobile.ScrollList.MobileListItemRenderer):void
        {
            if (!(arg1 == null) && !(arg1.data == null)) 
            {
                if (this.selectedIndex != arg1.data.id) 
                {
                    arg1.unselectItem();
                }
                else 
                {
                    arg1.selectItem();
                }
            }
            return;
        }

        protected function itemSelectHandler(arg1:Mobile.ScrollList.EventWithParams):void
        {
            var loc1:*=null;
            var loc2:*=0;
            if (this.clickable) 
            {
                loc1 = arg1.params.renderer as Mobile.ScrollList.MobileListItemRenderer;
                loc2 = loc1.data.id;
                this._mousePressPos = this._direction != Mobile.ScrollList.MobileScrollList.HORIZONTAL ? stage.mouseY : stage.mouseX;
                this._tempSelectedIndex = loc2;
                loc1.pressItem();
            }
            return;
        }

        protected function itemReleaseHandler(arg1:Mobile.ScrollList.EventWithParams):void
        {
            var loc1:*=null;
            var loc2:*=0;
            if (this.clickable) 
            {
                loc1 = arg1.params.renderer as Mobile.ScrollList.MobileListItemRenderer;
                loc2 = loc1.data.id;
                if (this._tempSelectedIndex == loc2) 
                {
                    this.selectedIndex = loc2;
                    this.dispatchEvent(new Mobile.ScrollList.EventWithParams(Mobile.ScrollList.MobileScrollList.ITEM_SELECT, {"renderer":loc1}));
                }
            }
            return;
        }

        protected function itemReleaseOutsideHandler(arg1:Mobile.ScrollList.EventWithParams):void
        {
            var loc1:*=null;
            if (this.clickable) 
            {
                loc1 = arg1.params.renderer as Mobile.ScrollList.MobileListItemRenderer;
                this.resetPressState(loc1);
                this._tempSelectedIndex = -1;
            }
            return;
        }

        protected function createMask():void
        {
            this._scrollMask = this.createSprite(16711935, new flash.geom.Rectangle(this._bounds.x, this._bounds.y, this._bounds.width, this._bounds.height));
            addChild(this._scrollMask);
            this._scrollMask.mouseEnabled = false;
            this._scrollList.mask = this._scrollMask;
            return;
        }

        public function get backgroundColor():int
        {
            return this._backgroundColor;
        }

        protected function createBackground():void
        {
            this._background = this.createSprite(this.backgroundColor, new flash.geom.Rectangle(this._bounds.x, this._bounds.y, this._bounds.width, this._bounds.height));
            this._background.x = this._bounds.x;
            this._background.y = this._bounds.y;
            addChildAt(this._background, 0);
            return;
        }

        protected function createSprite(arg1:int, arg2:flash.geom.Rectangle, arg3:Number=1):flash.display.Sprite
        {
            var loc1:*;
            (loc1 = new flash.display.Sprite()).graphics.beginFill(arg1, arg3);
            loc1.graphics.drawRect(arg2.x, arg2.y, arg2.width, arg2.height);
            loc1.graphics.endFill();
            return loc1;
        }

        protected function mouseDownHandler(arg1:flash.events.MouseEvent):void
        {
            if (!this._mouseDown && this.canScroll) 
            {
                this._mouseDownPoint = new flash.geom.Point(arg1.stageX, arg1.stageY);
                this._prevMouseDownPoint = new flash.geom.Point(arg1.stageX, arg1.stageY);
                this._mouseDown = true;
                this._mouseDownPos = this._direction != HORIZONTAL ? this._scrollList.y : this._scrollList.x;
                this._scrollList.stage.addEventListener(flash.events.MouseEvent.MOUSE_UP, this.mouseUpHandler, false, 0, true);
                this._scrollList.stage.addEventListener(flash.events.MouseEvent.MOUSE_MOVE, this.mouseMoveHandler, false, 0, true);
                Shared.BGSExternalInterface.call(null, "OnScrollingStarted");
            }
            return;
        }

        protected function mouseMoveHandler(arg1:flash.events.MouseEvent):void
        {
            var loc1:*=null;
            var loc2:*=NaN;
            if (this._mouseDown && this.canScroll) 
            {
                if (!isNaN(arg1.stageX) && !isNaN(arg1.stageY)) 
                {
                    loc1 = new flash.geom.Point(arg1.stageX, arg1.stageY);
                    if (this._direction != HORIZONTAL) 
                    {
                        loc2 = loc1.y - this._prevMouseDownPoint.y;
                        if (this._scrollList.y >= this._bounds.y || this._scrollList.y <= this._bounds.y - (this._scrollList.height - this._bounds.height)) 
                        {
                            if (this.elasticity) 
                            {
                                this._scrollList.y = this._scrollList.y + loc2 * this.RESISTANCE_OUT_BOUNDS;
                            }
                            else if (!(this._scrollList.y >= this._bounds.y && loc2 > 0 || this._scrollList.y <= this._bounds.y - (this._scrollList.height - this._bounds.height) && loc2 < 0)) 
                            {
                                this._scrollList.y = this._scrollList.y + loc2;
                            }
                        }
                        else 
                        {
                            this._scrollList.y = this._scrollList.y + loc2;
                        }
                        this._position = this._scrollList.y;
                        if (Math.abs(loc1.y - this._mousePressPos) > this.DELTA_MOUSE_POS) 
                        {
                            this.resetPressState(this.getRendererAt(this._tempSelectedIndex));
                            this._tempSelectedIndex = -1;
                        }
                        this._velocity = this._velocity + (loc1.y - this._prevMouseDownPoint.y) * this.VELOCITY_MOVE_FACTOR;
                    }
                    else 
                    {
                        loc2 = loc1.x - this._prevMouseDownPoint.x;
                        if (this._scrollList.x >= this._bounds.x || this._scrollList.x <= this._bounds.x - (this._scrollList.width - this._bounds.width)) 
                        {
                            if (this.elasticity) 
                            {
                                this._scrollList.x = this._scrollList.x + loc2 * this.RESISTANCE_OUT_BOUNDS;
                            }
                            else if (!(this._scrollList.x >= this._bounds.x && loc2 > 0 || this._scrollList.x <= this._bounds.x - (this._scrollList.width - this._bounds.width) && loc2 < 0)) 
                            {
                                this._scrollList.x = this._scrollList.x + loc2;
                            }
                        }
                        else 
                        {
                            this._scrollList.x = this._scrollList.x + loc2;
                        }
                        this._position = this._scrollList.x;
                        if (Math.abs(loc1.x - this._mousePressPos) > this.DELTA_MOUSE_POS) 
                        {
                            this.resetPressState(this.getRendererAt(this._tempSelectedIndex));
                            this._tempSelectedIndex = -1;
                        }
                        this._velocity = this._velocity + (loc1.x - this._prevMouseDownPoint.x) * this.VELOCITY_MOVE_FACTOR;
                    }
                    this._prevMouseDownPoint = loc1;
                }
                if (isNaN(this.mouseX) || isNaN(this.mouseY) || this.mouseY < this._bounds.y || this.mouseY > this._bounds.height + this._bounds.y || this.mouseX < this._bounds.x || this.mouseX > this._bounds.width + this._bounds.x) 
                {
                    this.mouseUpHandler(null);
                }
                this.setDataOnVisibleRenderers();
            }
            return;
        }

        protected function mouseUpHandler(arg1:flash.events.MouseEvent):void
        {
            if (this._mouseDown && this.canScroll) 
            {
                this._mouseDown = false;
                this._scrollList.stage.removeEventListener(flash.events.MouseEvent.MOUSE_UP, this.mouseUpHandler);
                this._scrollList.stage.removeEventListener(flash.events.MouseEvent.MOUSE_MOVE, this.mouseMoveHandler);
                Shared.BGSExternalInterface.call(null, "OnScrollingStopped");
            }
            return;
        }

        internal function setDataOnVisibleRenderers():void
        {
            var loc2:*=NaN;
            var loc1:*=0;
            while (loc1 < this._renderers.length) 
            {
                if (this._renderers[loc1].data === null) 
                {
                    loc2 = this._scrollList.y + this._renderers[loc1].y;
                    if (loc2 < this._bounds.y + this._bounds.height && loc2 + this._renderers[loc1].height > this._bounds.y) 
                    {
                        this.setRendererData(this._renderers[loc1], this.data[loc1], loc1);
                    }
                }
                ++loc1;
            }
            return;
        }

        internal function setRendererData(arg1:Mobile.ScrollList.MobileListItemRenderer, arg2:Object, arg3:int):void
        {
            arg2.id = arg3;
            arg2.textOption = this._textOption;
            arg1.setData(arg2);
            return;
        }

        public function get hasBackground():Boolean
        {
            return this._hasBackground;
        }

        public function set hasBackground(arg1:Boolean):void
        {
            this._hasBackground = arg1;
            return;
        }

        public function destroy():void
        {
            this.invalidateData();
            return;
        }

        protected function enterFrameHandler(arg1:flash.events.Event):void
        {
            var loc1:*=undefined;
            var loc2:*=NaN;
            var loc3:*=NaN;
            var loc4:*=NaN;
            var loc5:*=NaN;
            if (!(this._bounds == null) && this.canScroll) 
            {
                loc1 = this._mouseDown ? this.VELOCITY_MOUSE_DOWN_FACTOR : this.VELOCITY_MOUSE_UP_FACTOR;
                this._velocity = this._velocity * loc1;
                loc2 = this._direction != HORIZONTAL ? this._scrollList.height : this._scrollList.width;
                loc3 = this._direction != HORIZONTAL ? this._bounds.height : this._bounds.width;
                loc4 = this._direction != HORIZONTAL ? this._scrollList.y : this._scrollList.x;
                if (!this._mouseDown) 
                {
                    loc5 = 0;
                    if (loc4 >= 0 || loc2 <= loc3) 
                    {
                        if (this.elasticity) 
                        {
                            loc5 = (-loc4) * this.BOUNCE_FACTOR;
                            this._position = loc4 + this._velocity + loc5;
                        }
                        else 
                        {
                            this._position = 0;
                        }
                    }
                    else if (loc4 + loc2 <= loc3) 
                    {
                        if (this.elasticity) 
                        {
                            loc5 = (loc3 - loc2 - loc4) * this.BOUNCE_FACTOR;
                            this._position = loc4 + this._velocity + loc5;
                        }
                        else 
                        {
                            this._position = loc3 - loc2;
                        }
                    }
                    else 
                    {
                        this._position = loc4 + this._velocity;
                    }
                    if (Math.abs(this._velocity) > this.EPSILON || !(loc5 == 0)) 
                    {
                        if (this._direction != HORIZONTAL) 
                        {
                            this._scrollList.y = this._position;
                        }
                        else 
                        {
                            this._scrollList.x = this._position;
                        }
                        this.setDataOnVisibleRenderers();
                    }
                }
                if (this._prevIndicator) 
                {
                    this._prevIndicator.visible = loc4 < 0;
                }
                if (this._nextIndicator) 
                {
                    this._nextIndicator.visible = loc4 > loc3 - loc2;
                }
            }
            return;
        }

        public function set backgroundColor(arg1:int):void
        {
            this._backgroundColor = arg1;
            return;
        }

        public function get noScrollShortList():Boolean
        {
            return this._noScrollShortList;
        }

        public function set noScrollShortList(arg1:Boolean):void
        {
            this._noScrollShortList = arg1;
            return;
        }

        protected const VELOCITY_MOVE_FACTOR:Number=0.4;

        protected const EPSILON:Number=0.01;

        public static const VERTICAL:uint=1;

        protected const VELOCITY_MOUSE_DOWN_FACTOR:Number=0.5;

        protected const VELOCITY_MOUSE_UP_FACTOR:Number=0.8;

        protected const BOUNCE_FACTOR:Number=0.6;

        internal const DELTA_MOUSE_POS:int=15;

        protected const RESISTANCE_OUT_BOUNDS:Number=0.15;

        public static const ITEM_SELECT:String="itemSelect";

        public static const ITEM_RELEASE:String="itemRelease";

        public static const ITEM_RELEASE_OUTSIDE:String="itemReleaseOutside";

        public static const HORIZONTAL:uint=0;

        internal var _itemRendererLinkageId:String="MobileListItemRendererMc";

        protected var _selectedIndex:int;

        protected var _background:flash.display.Sprite;

        protected var _scrollList:flash.display.Sprite;

        protected var _scrollMask:flash.display.Sprite;

        protected var _touchZone:flash.display.Sprite;

        protected var _nextIndicator:flash.display.MovieClip;

        protected var _mouseDown:Boolean=false;

        protected var _velocity:Number=0;

        protected var _deltaBetweenButtons:Number;

        internal var _availableRenderers:__AS3__.vec.Vector.<Mobile.ScrollList.MobileListItemRenderer>;

        protected var _data:__AS3__.vec.Vector.<Object>;

        protected var _rendererRect:flash.geom.Rectangle;

        protected var _mouseDownPos:Number=0;

        protected var _mouseDownPoint:flash.geom.Point;

        protected var _prevMouseDownPoint:flash.geom.Point;

        internal var _mousePressPos:Number;

        protected var _bounds:flash.geom.Rectangle;

        protected var _hasBackground:Boolean=false;

        protected var _backgroundColor:int=15658734;

        protected var _scrollDim:Number;

        protected var _textOption:String;

        protected var _endListAlign:Boolean=false;

        protected var _noScrollShortList:Boolean=false;

        internal var _elasticity:Boolean=true;

        protected var _prevIndicator:flash.display.MovieClip;

        protected var _renderers:__AS3__.vec.Vector.<Mobile.ScrollList.MobileListItemRenderer>;

        protected var _tempSelectedIndex:int;

        protected var _position:Number;

        protected var _direction:uint;

        protected var _clickable:Boolean=true;
    }
}
