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

		public var Title:String="";
		public var Level:uint=0;
		public var ShowLevel:Boolean=true;
		
		public function Nameplate()
		{
			addEventListener(Event.ADDED_TO_STAGE,onAddedToStage); 
		}
		
		public function onAddedToStage(evt:Event)
		{
			if(objectData) {
				DisplayText_tf.autoSize = "center";
				percent = objectData.percent;
				Title = objectData.objectName;
				Level = objectData.level;
				ShowLevel = objectData.showLevel;
				UpdateTitle();
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
			if(Title != a_name) {
				Title = a_name;
				UpdateTitle();
			}
		}

		public function set level(a_level: uint)
		{
			if(Level != a_level) {
				Level = a_level;
				UpdateTitle();
			}
		}

		public function set showLevel(a_show: Boolean)
		{
			if(ShowLevel != a_show) {
				ShowLevel = a_show;
				UpdateTitle();
			}
		}
		
		public function SetData(a_name: String, a_health: Number, a_level: uint, a_show: Boolean, a_zIndex: Number)
		{
			HPBar_mc.percent = a_health;
			zIndex = a_zIndex;
			
			var doUpdate: Boolean = false;
			doUpdate ||= Title != a_name;
			Title = a_name;
			doUpdate ||= Level != a_level;
			Level = a_level;
			doUpdate ||= ShowLevel != a_show;
			ShowLevel = a_show;
			if(doUpdate) {
				UpdateTitle();
			}
		}

		public function UpdateTitle()
		{
			DisplayText_tf.text = Title;
			DisplayText_tf.appendText(ShowLevel ? " [" + String(Level) + "]" : "");
		}
	}
}
