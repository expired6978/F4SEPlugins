package 
{
	import Shared.AS3.*;
	import flash.display.*;
	import flash.geom.*;
	import scaleform.clik.controls.Slider;
	import scaleform.clik.events.SliderEvent;
	
	public class FeatureListEntry extends Shared.AS3.BSScrollingListEntry
	{
		public static const VALUE_CHANGE:String="FeatureListEntry::VALUE_CHANGE";
		
		public function FeatureListEntry()
		{
			super();
			return;
		}

		public override function SetEntryText(object:Object, arg2:String):*
		{
			super.SetEntryText(object, arg2);
			this.EquipIcon_mc.visible = object.applied;
			var colorTrans:*=this.EquipIcon_mc.transform.colorTransform;
			colorTrans.redOffset = this.selected ? -255 : 0;
			colorTrans.greenOffset = this.selected ? -255 : 0;
			colorTrans.blueOffset = this.selected ? -255 : 0;
			this.EquipIcon_mc.transform.colorTransform = colorTrans;
			this.Slider_mc.transform.colorTransform = colorTrans;
			
			Slider_mc.visible = Slider_mc.enabled = false;
			if(object.value != undefined) {
				Slider_mc.visible = Slider_mc.enabled = true;
				Slider_mc.focusable = false;
				Slider_mc.minimum = object.min;
				Slider_mc.maximum = object.max;
				Slider_mc.snapping = false;
				Slider_mc.snapInterval = object.interval;
				Slider_mc.liveDragging = true;
				Slider_mc.position = object.value;
				Slider_mc.offsetLeft = 2;
				Slider_mc.offsetRight = 20;
				Slider_mc["data"] = object;
				Slider_mc.addEventListener(SliderEvent.VALUE_CHANGE, onSliderChanged);
			}
		}
		
		public function onSliderChanged(event:SliderEvent)
		{
			// Bubble the received event back up
			event.target.dispatchEvent(new SliderEvent(FeatureListEntry.VALUE_CHANGE, true, true, event.value));
		}

		public var EquipIcon_mc:flash.display.MovieClip;
		public var Slider_mc:Slider;
	}
}
