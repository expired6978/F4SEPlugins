package 
{
	import Shared.AS3.*;
	
	public dynamic class FeatureList extends Shared.AS3.BSScrollingList
	{
		public function FeatureList()
		{
			super();
		}
		
		public function GetCurrentSlider()
		{
			if(selectedIndex >= 0) {
				var entryObject = entryList[selectedIndex];
				var entryClip = GetClipByIndex(entryObject.clipIndex);
				if(entryObject.sliderType != undefined) {
					switch(entryObject.sliderType) {
						case "Hue":
							return entryClip.HSVGroup_mc.hSlider;
							break;
						case "Saturation":
							return entryClip.HSVGroup_mc.sSlider;
							break;
						case "Value":
							return entryClip.HSVGroup_mc.vSlider;
							break;
						case "Alpha":
							return entryClip.HSVGroup_mc.aSlider;
							break;
						default:
							break;
					}
				}
				
				return entryClip.Slider_mc;
			}
			return null;
		}
	}
}
