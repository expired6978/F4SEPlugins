package Shared 
{
    import flash.display.*;
    import flash.events.*;
    import flash.text.*;
    
    public class IMenu extends flash.display.MovieClip
    {
		internal var _uiPlatform:uint;
        internal var _bPS3Switch:Boolean;
        internal var _bRestoreLostFocus:Boolean;
        internal var safeX:Number=0;
        internal var safeY:Number=0;
        internal var textFieldSizeMap:Object;
		
        public function IMenu()
        {
            this.textFieldSizeMap = new Object();
            super();
            _uiPlatform = Shared.PlatformChangeEvent.PLATFORM_INVALID;
            _bPS3Switch = false;
            _bRestoreLostFocus = false;
            Shared.GlobalFunc.MaintainTextFormat();
            addEventListener(flash.events.Event.ADDED_TO_STAGE, onStageInit);
            addEventListener(flash.events.Event.REMOVED_FROM_STAGE, onStageDestruct);
            addEventListener(Shared.PlatformRequestEvent.PLATFORM_REQUEST, onPlatformRequestEvent, true);
        }

        public function get uiPlatform():uint
        {
            return _uiPlatform;
        }

        public function get bPS3Switch():Boolean
        {
            return _bPS3Switch;
        }

        public function get SafeX():Number
        {
            return safeX;
        }

        public function get SafeY():Number
        {
            return safeY;
        }

        protected function onPlatformRequestEvent(event:flash.events.Event):*
        {
            if (uiPlatform != Shared.PlatformChangeEvent.PLATFORM_INVALID) 
            {
                (event as Shared.PlatformRequestEvent).RespondToRequest(uiPlatform, bPS3Switch);
            }
            return;
        }

        protected function onStageInit(event:flash.events.Event):*
        {
            stage.stageFocusRect = false;
            stage.addEventListener(flash.events.FocusEvent.FOCUS_OUT, onFocusLost);
            stage.addEventListener(flash.events.FocusEvent.MOUSE_FOCUS_CHANGE, onMouseFocus);
        }

        protected function onStageDestruct(event:flash.events.Event):*
        {
            stage.removeEventListener(flash.events.FocusEvent.FOCUS_OUT, onFocusLost);
            stage.removeEventListener(flash.events.FocusEvent.MOUSE_FOCUS_CHANGE, onMouseFocus);
        }

        public function SetPlatform(arg1:uint, arg2:Boolean):*
        {
            _uiPlatform = arg1;
            _bPS3Switch = bPS3Switch;
            dispatchEvent(new Shared.PlatformChangeEvent(uiPlatform, bPS3Switch));
        }

        public function SetSafeRect(arg1:Number, arg2:Number):*
        {
            safeX = arg1;
            safeY = arg2;
            onSetSafeRect();
            return;
        }

        protected function onSetSafeRect():void
        {
            return;
        }

        internal function onFocusLost(event:flash.events.FocusEvent):*
        {
            if (_bRestoreLostFocus) 
            {
                _bRestoreLostFocus = false;
                stage.focus = event.target as flash.display.InteractiveObject;
            }
        }

        protected function onMouseFocus(event:flash.events.FocusEvent):*
        {
            if (event.target == null || !(event.target is flash.display.InteractiveObject)) 
            {
                stage.focus = null;
            }
            else 
            {
                _bRestoreLostFocus = true;
            }
        }

        public function ShrinkFontToFit(arg1:flash.text.TextField, arg2:int):*
        {
            var loc3:*=0;
            var loc1:*=arg1.getTextFormat();
            if (this.textFieldSizeMap[arg1] == null) 
            {
                this.textFieldSizeMap[arg1] = loc1.size;
            }
            loc1.size = this.textFieldSizeMap[arg1];
            arg1.setTextFormat(loc1);
            var loc2:*=arg1.maxScrollV;
            while (loc2 > arg2 && loc1.size > 4) 
            {
                loc3 = loc1.size as int;
                loc1.size = (loc3 - 1);
                arg1.setTextFormat(loc1);
                loc2 = arg1.maxScrollV;
            }
        }
    }
}
