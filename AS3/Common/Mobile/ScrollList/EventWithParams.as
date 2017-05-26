package Mobile.ScrollList 
{
    import flash.events.*;
    import flash.utils.*;
    
    public class EventWithParams extends flash.events.Event
    {
        public function EventWithParams(arg1:String, arg2:Object=null, arg3:Boolean=false, arg4:Boolean=false)
        {
            super(arg1, arg3, arg4);
            this.params = arg2;
            return;
        }

        public override function clone():flash.events.Event
        {
            return new Mobile.ScrollList.EventWithParams(type, this.params, bubbles, cancelable);
        }

        public override function toString():String
        {
            return formatToString(flash.utils.getQualifiedClassName(this), "type", "bubbles", "cancelable", "eventPhase", "params");
        }

        public var params:Object;
    }
}
