package Shared 
{
    import flash.display.*;
    import flash.events.*;
    
    public class PlatformRequestEvent extends flash.events.Event
    {
        public function PlatformRequestEvent(arg1:flash.display.MovieClip)
        {
            super(PLATFORM_REQUEST);
            this._target = arg1;
        }

        public function RespondToRequest(arg1:uint, arg2:Boolean):*
        {
            this._target.SetPlatform(arg1, arg2);
        }

        public static const PLATFORM_REQUEST:String="GetPlatform";

        var _target:flash.display.MovieClip;
    }
}
