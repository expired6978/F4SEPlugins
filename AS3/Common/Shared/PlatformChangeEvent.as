package Shared 
{
    import flash.events.*;
    
    public class PlatformChangeEvent extends flash.events.Event
    {
        public function PlatformChangeEvent(arg1:uint, arg2:Boolean)
        {
            super(PLATFORM_CHANGE, true, true);
            this.uiPlatform = arg1;
            this.bPS3Switch = arg2;
        }

        public function get uiPlatform():*
        {
            return this._uiPlatform;
        }

        public function set uiPlatform(arg1:uint):*
        {
            this._uiPlatform = arg1;
        }

        public function get bPS3Switch():*
        {
            return this._bPS3Switch;
        }

        public function set bPS3Switch(arg1:Boolean):*
        {
            this._bPS3Switch = arg1;
        }

        public static const PLATFORM_PC_KB_MOUSE:uint=0;
        public static const PLATFORM_PC_GAMEPAD:uint=1;
        public static const PLATFORM_XB1:uint=2;
        public static const PLATFORM_PS4:uint=3;
        public static const PLATFORM_MOBILE:uint=4;
        public static const PLATFORM_INVALID:uint=uint.MAX_VALUE;
        public static const PLATFORM_CHANGE:String="SetPlatform";
        var _uiPlatform:uint=4294967295;
        var _bPS3Switch:Boolean=false;
    }
}
