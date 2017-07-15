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
		}

		public override function SetEntryText(object:Object, arg2:String):*
		{
			super.SetEntryText(object, arg2);
			this.EquipIcon_mc.visible = object.applied;
			var colorTrans:*=this.EquipIcon_mc.transform.colorTransform;
			colorTrans.redOffset = this.selected ? -255 : 0;
			colorTrans.greenOffset = this.selected ? -255 : 0;
			colorTrans.blueOffset = this.selected ? -255 : 0;
			colorTrans.alphaOffset = 0;
			this.EquipIcon_mc.transform.colorTransform = colorTrans;
			this.Slider_mc.transform.colorTransform = colorTrans;
						
			Slider_mc.visible = Slider_mc.enabled = false;
			HSVGroup_mc.visible = HSVGroup_mc.enabled = false;
			
			textField.width = 220;
			
			if(object.sliderType != undefined) {
				HSVGroup_mc.visible = HSVGroup_mc.enabled = true;
				HSVGroup_mc.setType(object.sliderType);
				HSVGroup_mc.setHSV(object.hsva, object.hsva[3]);
				textField.width = 65;
			}
			
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
				textField.width = 70;
			}
		}
		
		public function onSliderChanged(event:SliderEvent)
		{
			// Bubble the received event back up
			event.target.dispatchEvent(new SliderEvent(FeatureListEntry.VALUE_CHANGE, true, true, event.value));
		}
				
		public var EquipIcon_mc:flash.display.MovieClip;
		public var Slider_mc:Slider;		
		public var HSVGroup_mc:HSVGroup;
	}
}
