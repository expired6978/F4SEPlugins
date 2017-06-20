package
{
    import flash.events.*;
    import flash.utils.*;
    
    public class HSVEvent extends flash.events.Event
    {
		public var rgba:Array;
		public var hsva:Array;
		
        public function HSVEvent(eventName:String, a_rgba:Array, a_hsva:Array, bubbles:Boolean=false, cancelable:Boolean=false)
        {
            super(eventName, bubbles, cancelable);
            rgba = a_rgba;
			hsva = a_hsva;
        }

        public override function clone():flash.events.Event
        {
            return new HSVEvent(type, rgba, hsva, bubbles, cancelable);
        }

        public override function toString():String
        {
            return formatToString(flash.utils.getQualifiedClassName(this), "type", "bubbles", "cancelable", "eventPhase", "rgba", "hsva");
        }
    }
}
