package Mobile.ScrollList 
{
    import flash.display.*;
    import flash.events.*;
    import flash.text.*;
    
    public class MobileListItemRenderer extends flash.display.MovieClip
    {
        public function MobileListItemRenderer()
        {
            super();
            return;
        }

        public function get data():Object
        {
            return this._data;
        }

        protected function get isClickable():Boolean
        {
            return !(this.data == null) && !(!(this.data.clickable == null) && !this.data.clickable);
        }

        public function reset():void
        {
            this._data = null;
            addEventListener(flash.events.Event.REMOVED_FROM_STAGE, this.destroy, false, 0, true);
            addEventListener(flash.events.MouseEvent.MOUSE_DOWN, this.itemPressHandler, false, 0, true);
            return;
        }

        public function setData(arg1:Object):*
        {
            this._data = arg1;
            if (arg1 != null) 
            {
                this.setVisual();
            }
            return;
        }

        protected function setVisual():void
        {
            return;
        }

        protected function itemPressHandler(arg1:flash.events.MouseEvent):void
        {
            if (this.isClickable) 
            {
                addEventListener(flash.events.Event.ENTER_FRAME, this.onEnterFrame, false, 0, true);
                addEventListener(flash.events.MouseEvent.MOUSE_UP, this.itemReleaseHandler, false, 0, true);
                this.dispatchEvent(new Mobile.ScrollList.EventWithParams(Mobile.ScrollList.MobileScrollList.ITEM_SELECT, {"renderer":this}));
            }
            return;
        }

        protected function itemReleaseHandler(arg1:flash.events.MouseEvent):void
        {
            if (this.isClickable) 
            {
                removeEventListener(flash.events.Event.ENTER_FRAME, this.onEnterFrame);
                removeEventListener(flash.events.MouseEvent.MOUSE_UP, this.itemReleaseHandler);
                this.dispatchEvent(new Mobile.ScrollList.EventWithParams(Mobile.ScrollList.MobileScrollList.ITEM_RELEASE, {"renderer":this}));
            }
            return;
        }

        protected function onEnterFrame(arg1:flash.events.Event):void
        {
            if (mouseY < 0 || mouseY > this.height || mouseX < 0 || mouseX > this.width) 
            {
                removeEventListener(flash.events.Event.ENTER_FRAME, this.onEnterFrame);
                removeEventListener(flash.events.MouseEvent.MOUSE_UP, this.itemReleaseHandler);
                this.dispatchEvent(new Mobile.ScrollList.EventWithParams(Mobile.ScrollList.MobileScrollList.ITEM_RELEASE_OUTSIDE, {"renderer":this}));
            }
            return;
        }

        public function selectItem():void
        {
            return;
        }

        public function unselectItem():void
        {
            return;
        }

        public function pressItem():void
        {
            return;
        }

        protected function destroy(arg1:flash.events.Event):void
        {
            removeEventListener(flash.events.Event.REMOVED_FROM_STAGE, this.destroy);
            removeEventListener(flash.events.MouseEvent.MOUSE_DOWN, this.itemPressHandler);
            return;
        }

        internal const DELTA_MOUSE_POS:int=15;

        public var textField:flash.text.TextField;

        protected var _data:Object;

        internal var _mouseDownPos:Number;

        internal var _mouseUpPos:Number;
    }
}
