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
				return entryClip.Slider_mc;
			}
			return null;
		}
	}
}
