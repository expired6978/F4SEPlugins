package
{
	import flash.display.MovieClip;
	import flash.text.TextField;
	import flash.events.Event;
	import Nameplate.MeterBar;
	import flash.geom.Point;
	
	public class Nameplate extends IHUDObject
	{
		public var DisplayText_tf:TextField;
		public var HPBar_mc:MovieClip;
		public var Background:MovieClip;
		
		public function Nameplate()
		{
			addEventListener(Event.ADDED_TO_STAGE,onAddedToStage); 
		}
		
		public function onAddedToStage(evt:Event)
		{
			if(objectData) {
				DisplayText_tf.autoSize = "center";
				percent = objectData.percent;
				objectName = objectData.name;
			}
			else
			{
				trace("[Nameplate]: No objectData");
			}
		}

		
		public function set percent(num: Number)
		{
			HPBar_mc.percent = num;
		}
		
		public function set objectName(a_name: String)
		{
			DisplayText_tf.text = a_name;
		}
	}
}
