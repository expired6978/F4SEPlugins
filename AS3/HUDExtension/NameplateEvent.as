package {
	import flash.events.Event;
	
	public class NameplateEvent extends Event {
		
		public static const ADDED:String = "NameplateEvent::ADDED";
		public static const REMOVED:String = "NameplateEvent::REMOVED";
		
		public var objectId: int = -1;
		
		public function NameplateEvent(type:String, bubbles:Boolean=false, cancelable:Boolean=true, id:int=-1) {
 			super(type, bubbles, cancelable);
			objectId = id;
        }
	}
}