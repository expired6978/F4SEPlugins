package 
{
	import flash.display.*;
	import flash.events.*;
	import flash.geom.*;
	import flash.ui.*;
	import utils.ColorFunctions;
	import Shared.AS3.BSUIComponent;
	
	public class HSVGroup extends MovieClip
	{
		public var hSlider: Option_Scrollbar;
		public var sSlider: Option_Scrollbar;
		public var vSlider: Option_Scrollbar;
		public var aSlider: Option_Scrollbar;
		
		public static const COLOR_CHANGED:String="HSVGroup::ColorChanged";
		
		private var _currentColor: Array;
		private var _alphaValue: Number;
		
		private var _hsv:Array;
	
		public function HSVGroup()
		{
			super();
			
			hSlider.minimum = 0;
			hSlider.maximum = 360;
			hSlider.StepSize = 1;
	
			sSlider.minimum = 0;
			sSlider.maximum = 100;
			sSlider.StepSize = 1;
	
			vSlider.minimum = 0;
			vSlider.maximum = 100;
			vSlider.StepSize = 1;
			
			aSlider.minimum = 0;
			aSlider.maximum = 100;
			aSlider.StepSize = 1;
			
			setupGradients();
			setColor([0, 0, 0], 0);

			hSlider.addEventListener(Option_Scrollbar.VALUE_CHANGE, onValueChange);
			sSlider.addEventListener(Option_Scrollbar.VALUE_CHANGE, onValueChange);
			vSlider.addEventListener(Option_Scrollbar.VALUE_CHANGE, onValueChange);
			aSlider.addEventListener(Option_Scrollbar.VALUE_CHANGE, onValueChange);
		}
		
		public function setType(a_type)
		{
			hSlider.visible = hSlider.enabled = false;
			sSlider.visible = sSlider.enabled = false;
			vSlider.visible = vSlider.enabled = false;
			aSlider.visible = aSlider.enabled = false;
			switch(a_type) {
				case "Hue":
					hSlider.visible = hSlider.enabled = true;
					break;
				case "Saturation":
					sSlider.visible = sSlider.enabled = true;
					break;
				case "Value":
					vSlider.visible = vSlider.enabled = true;
					break;
				case "Alpha":
					aSlider.visible = aSlider.enabled = true;
					break;
				default:
					break;
			}
		}
		
		public function setHSV(a_hsv:Array, a_alpha:Number)
		{
			var alphaHex:Number = (a_alpha * 255.0) / 100.0;
			_hsv = [a_hsv[0], a_hsv[1], a_hsv[2]];

			var satRGB: Array = ColorFunctions.hsvToRgb([_hsv[0], 100, _hsv[2]]);
			var valRGB: Array = ColorFunctions.hsvToRgb([_hsv[0], _hsv[1], 100]);
			var colRGB: Array = ColorFunctions.hsvToRgb([_hsv[0], _hsv[1], _hsv[2]]);
			
			hSlider.Track_mc.trackColor.transform.colorTransform = new ColorTransform(1, 1, 1, 1, 0, 0, 0, 0);
			hSlider.Track_mc.trackWhite.transform.colorTransform = new ColorTransform(0, 0, 0, 0, 255, 255, 255, (100 - _hsv[1]) * 2.55);
			hSlider.Track_mc.trackBlack.transform.colorTransform = new ColorTransform(0, 0, 0, 0, 0, 0, 0, (100 - _hsv[2]) * 2.55);
			
			sSlider.Track_mc.trackColor.transform.colorTransform = new ColorTransform(0, 0, 0, 0, satRGB[0], satRGB[1], satRGB[2], 255);
			sSlider.Track_mc.trackWhite.transform.colorTransform = new ColorTransform(1, 1, 1, 1, 0, 0, 0, 0);
			sSlider.Track_mc.trackBlack.transform.colorTransform = new ColorTransform(0, 0, 0, 0, 0, 0, 0, (100 - _hsv[2]) * 2.55);
			
			vSlider.Track_mc.trackColor.transform.colorTransform = new ColorTransform(0, 0, 0, 0, valRGB[0], valRGB[1], valRGB[2], 255);
			vSlider.Track_mc.trackWhite.transform.colorTransform = new ColorTransform(0, 0, 0, 0, 255, 255, 255, (100 - _hsv[1]) * 2.55);
			vSlider.Track_mc.trackBlack.transform.colorTransform = new ColorTransform(1, 1, 1, 1, 0, 0, 0, 0);
			
			aSlider.Track_mc.trackColor.transform.colorTransform = new ColorTransform(0, 0, 0, 0, colRGB[0], colRGB[1], colRGB[2], alphaHex);
			aSlider.Track_mc.trackAlpha.transform.colorTransform = new ColorTransform(1, 1, 1, 1, 0, 0, 0, 0);
			
			hSlider.position = _hsv[0];
			sSlider.position = _hsv[1];
			vSlider.position = _hsv[2];
			aSlider.position = a_alpha;
			
			_currentColor = colRGB;
			_alphaValue = alphaHex;
		}
				
		public function setColor(a_color: Array, a_alpha: Number)
		{
			var alphaNormal = (a_alpha / 255) * 100;
			
			_hsv = ColorFunctions.rgbToHsv(a_color);
			var satRGB: Array = ColorFunctions.hsvToRgb([_hsv[0], 100, _hsv[2]]);
			var valRGB: Array = ColorFunctions.hsvToRgb([_hsv[0], _hsv[1], 100]);
							
			hSlider.Track_mc.trackColor.transform.colorTransform = new ColorTransform(1, 1, 1, 1, 0, 0, 0, 0);
			hSlider.Track_mc.trackWhite.transform.colorTransform = new ColorTransform(0, 0, 0, 0, 255, 255, 255, (100 - _hsv[1]) * 2.55);
			hSlider.Track_mc.trackBlack.transform.colorTransform = new ColorTransform(0, 0, 0, 0, 0, 0, 0, (100 - _hsv[2]) * 2.55);
			
			sSlider.Track_mc.trackColor.transform.colorTransform = new ColorTransform(0, 0, 0, 0, satRGB[0], satRGB[1], satRGB[2], 255);
			sSlider.Track_mc.trackWhite.transform.colorTransform = new ColorTransform(1, 1, 1, 1, 0, 0, 0, 0);
			sSlider.Track_mc.trackBlack.transform.colorTransform = new ColorTransform(0, 0, 0, 0, 0, 0, 0, (100 - _hsv[2]) * 2.55);
			
			vSlider.Track_mc.trackColor.transform.colorTransform = new ColorTransform(0, 0, 0, 0, valRGB[0], valRGB[1], valRGB[2], 255);
			vSlider.Track_mc.trackWhite.transform.colorTransform = new ColorTransform(0, 0, 0, 0, 255, 255, 255, (100 - _hsv[1]) * 2.55);
			vSlider.Track_mc.trackBlack.transform.colorTransform = new ColorTransform(1, 1, 1, 1, 0, 0, 0, 0);
			
			aSlider.Track_mc.trackColor.transform.colorTransform = new ColorTransform(0, 0, 0, 0, a_color[0], a_color[1], a_color[2], a_alpha);
			aSlider.Track_mc.trackAlpha.transform.colorTransform = new ColorTransform(1, 1, 1, 1, 0, 0, 0, 0);
			
			hSlider.position = _hsv[0];
			sSlider.position = _hsv[1];
			vSlider.position = _hsv[2];
			aSlider.position = alphaNormal;
			
			_currentColor = a_color;
			_alphaValue = a_alpha;
		}
		
		public function onValueChange(event:flash.events.Event)
		{
			var newRGB: Array;
			var valRGB: Array;
			var satRGB: Array;
						
			if(event.target == hSlider) {
				var newHue: Number = event.target.value;
				_hsv[0] = newHue;
		
				newRGB = ColorFunctions.hsvToRgb(_hsv);
				satRGB = ColorFunctions.hsvToRgb([_hsv[0], 100, _hsv[2]]);
				valRGB = ColorFunctions.hsvToRgb([_hsv[0], _hsv[1], 100]);
				
				sSlider.Track_mc.trackColor.transform.colorTransform = new ColorTransform(0, 0, 0, 0, newRGB[0], newRGB[1], newRGB[2]);
				vSlider.Track_mc.trackColor.transform.colorTransform = new ColorTransform(0, 0, 0, 0, valRGB[0], valRGB[1], valRGB[2], 255);
				aSlider.Track_mc.trackColor.transform.colorTransform = new ColorTransform(0, 0, 0, 0, newRGB[0], newRGB[1], newRGB[2], _alphaValue);
				_currentColor = newRGB;
			} else if(event.target == sSlider) {
				var newSat: Number = event.target.value;
				_hsv[1] = newSat;
		
				newRGB = ColorFunctions.hsvToRgb(_hsv);
				valRGB = ColorFunctions.hsvToRgb([_hsv[0], _hsv[1], 100]);
				
				hSlider.Track_mc.trackWhite.transform.colorTransform = new ColorTransform(0, 0, 0, 0, 255, 255, 255, (100 - _hsv[1]) * 2.55);
				vSlider.Track_mc.trackWhite.transform.colorTransform = new ColorTransform(0, 0, 0, 0, 255, 255, 255, (100 - _hsv[1]) * 2.55);
				
				vSlider.Track_mc.trackColor.transform.colorTransform = new ColorTransform(0, 0, 0, 0, valRGB[0], valRGB[1], valRGB[2], 255);
				aSlider.Track_mc.trackColor.transform.colorTransform = new ColorTransform(0, 0, 0, 0, newRGB[0], newRGB[1], newRGB[2], _alphaValue);
				_currentColor = newRGB;
			} else if(event.target == vSlider) {
				var newVal: Number = event.target.value;
				_hsv[2] = newVal;
		
				newRGB = ColorFunctions.hsvToRgb(_hsv);
				satRGB = ColorFunctions.hsvToRgb([_hsv[0], 100, _hsv[2]]);
				
				hSlider.Track_mc.trackBlack.transform.colorTransform = new ColorTransform(0, 0, 0, 0, 0, 0, 0, (100 - _hsv[2]) * 2.55);
				sSlider.Track_mc.trackBlack.transform.colorTransform = new ColorTransform(0, 0, 0, 0, 0, 0, 0, (100 - _hsv[2]) * 2.55);
		
				sSlider.Track_mc.trackColor.transform.colorTransform = new ColorTransform(0, 0, 0, 0, satRGB[0], satRGB[1], satRGB[2], 255);
				aSlider.Track_mc.trackColor.transform.colorTransform = new ColorTransform(0, 0, 0, 0, newRGB[0], newRGB[1], newRGB[2], _alphaValue);
				_currentColor = newRGB;
			} else if(event.target == aSlider) {
				var newAlpha = event.target.value;
				newRGB = ColorFunctions.hsvToRgb(_hsv);
				var alphaValue = (newAlpha / 100) * 255;
				
				aSlider.Track_mc.trackColor.transform.colorTransform = new ColorTransform(0, 0, 0, 0, newRGB[0], newRGB[1], newRGB[2], alphaValue);
				_currentColor = newRGB;
				_alphaValue = alphaValue;
			}
			
			var colorRGB = _currentColor;
			var colorNorm = ColorFunctions.normalize(_currentColor, _alphaValue);
			var hsva = [_hsv[0], _hsv[1], _hsv[2], (_alphaValue / 255.0) * 100];
						
			dispatchEvent(new HSVEvent(COLOR_CHANGED, colorNorm, hsva, true, true));
		}
		
		  /* PRIVATE FUNCTIONS */
		private function setupGradients()
		{
			var hTrack: MovieClip = hSlider.Track_mc;
			var sTrack: MovieClip = sSlider.Track_mc;
			var vTrack: MovieClip = vSlider.Track_mc;
			var aTrack: MovieClip = aSlider.Track_mc;
	
			var sWidth: Number;
			var sHeight: Number;
	
			var colors: Array;
			var alphas: Array;
			var ratios: Array;
			var matrix: Matrix;
	
			var trackColor: MovieClip;
			var trackWhite: MovieClip;
			var trackBlack: MovieClip;
			var trackAlpha: MovieClip;
				
			//------------------------------------------------------------------------------------------
			// Hue Slider
			var sOffsetX = 1;
			var sOffsetY = 0.5;
			var sMarginX = -2;
			var sMarginY = -2;
			
			sWidth = hTrack.width + sMarginX;
			sHeight = hTrack.height + sMarginY;
	
			colors = [0xFF0000, 0xFFFF00, 0x00FF00, 0x00FFFF, 0x0000FF, 0xFF00FF, 0xFF0000];
			alphas = [100, 100, 100, 100, 100, 100, 100];
			ratios = [0, 42.5, 85, 127.5, 170, 212.5, 255];
			matrix = new Matrix();
			matrix.createGradientBox(sWidth, sHeight);
	
			trackColor = new MovieClip();
			trackColor.name = "trackColor";
			trackColor.graphics.beginGradientFill(GradientType.LINEAR, colors, alphas, ratios, matrix);
			trackColor.graphics.drawRect(sOffsetX, sOffsetY, sWidth, sHeight);
			trackColor.graphics.endFill();
			trackColor.width = sWidth;
			trackColor.height = sHeight;
			hTrack.trackColor = hTrack.addChild(trackColor);
	
			trackWhite = new MovieClip();
			trackWhite.name = "trackWhite";
			trackWhite.graphics.beginFill(0xFFFFFF);
			trackWhite.graphics.drawRect(sOffsetX, sOffsetY, sWidth, sHeight);
			trackWhite.graphics.endFill();
			trackWhite.width = sWidth;
			trackWhite.height = sHeight;
			hTrack.trackWhite = hTrack.addChild(trackWhite);
	
			trackBlack = new MovieClip();
			trackBlack.name = "trackBlack";
			trackBlack.graphics.beginFill(0x000000);
			trackBlack.graphics.drawRect(sOffsetX, sOffsetY, sWidth, sHeight);
			trackBlack.graphics.endFill();
			trackBlack.width = sWidth;
			trackBlack.height = sHeight;
			hTrack.trackBlack = hTrack.addChild(trackBlack);
				
			//------------------------------------------------------------------------------------------
			// Saturation Slider
			sWidth = sTrack.width + sMarginX;
			sHeight = sTrack.height + sMarginY;
	
			trackColor = new MovieClip();
			trackColor.name = "trackColor";
			trackColor.graphics.beginFill(0x000000);
			trackColor.graphics.drawRect(sOffsetX, sOffsetY, sWidth, sHeight);
			trackColor.graphics.endFill();
			trackColor.width = sWidth;
			trackColor.height = sHeight;
			sTrack.trackColor = sTrack.addChild(trackColor);
	
			colors = [0xFFFFFF, 0x000000];
			alphas = [100, 0];
			ratios = [0, 255];
			matrix = new Matrix();
			matrix.createGradientBox(sWidth, sHeight);
	
			trackWhite = new MovieClip();
			trackWhite.name = "trackWhite";
			trackWhite.graphics.beginGradientFill(GradientType.LINEAR, colors, alphas, ratios, matrix);
			trackWhite.graphics.drawRect(sOffsetX, sOffsetY, sWidth, sHeight);
			trackWhite.graphics.endFill();
			trackWhite.width = sWidth;
			trackWhite.height = sHeight;
			sTrack.trackWhite = sTrack.addChild(trackWhite);
			
			trackBlack = new MovieClip();
			trackBlack.name = "trackBlack";
			trackBlack.graphics.beginFill(0x000000);
			trackBlack.graphics.drawRect(sOffsetX, sOffsetY, sWidth, sHeight);
			trackBlack.graphics.endFill();
			trackBlack.width = sWidth;
			trackBlack.height = sHeight;
			sTrack.trackBlack = sTrack.addChild(trackBlack);
	
			//------------------------------------------------------------------------------------------
			// Value Slider
			sWidth = vTrack.width + sMarginX;
			sHeight = vTrack.height + sMarginY;
	
			trackColor = new MovieClip();
			trackColor.name = "trackColor";
			trackColor.graphics.beginFill(0x000000);
			trackColor.graphics.drawRect(sOffsetX, sOffsetY, sWidth, sHeight);
			trackColor.graphics.endFill();
			trackColor.width = sWidth;
			trackColor.height = sHeight;
			vTrack.trackColor = vTrack.addChild(trackColor);
			
			trackWhite = new MovieClip();
			trackWhite.name = "trackWhite";
			trackWhite.graphics.beginFill(0xFFFFFF);
			trackWhite.graphics.drawRect(sOffsetX, sOffsetY, sWidth, sHeight);
			trackWhite.graphics.endFill();
			trackWhite.width = sWidth;
			trackWhite.height = sHeight;
			vTrack.trackWhite = vTrack.addChild(trackWhite);
	
			colors = [0x000000, 0x000000];
			alphas = [100, 0];
			ratios = [0, 255];
			matrix = new Matrix();
			matrix.createGradientBox(sWidth, sHeight);
	
			trackBlack = new MovieClip();
			trackBlack.name = "trackBlack";
			trackBlack.graphics.beginGradientFill(GradientType.LINEAR, colors, alphas, ratios, matrix);
			trackBlack.graphics.drawRect(sOffsetX, sOffsetY, sWidth, sHeight);
			trackBlack.graphics.endFill();
			trackBlack.width = sWidth;
			trackBlack.height = sHeight;
			vTrack.trackBlack = vTrack.addChild(trackBlack);
			
			//------------------------------------------------------------------------------------------
			// Alpha Slider
			
			sWidth = aTrack.width + sMarginX;
			sHeight = aTrack.height + sMarginY;
			
			//var repeat: Boolean = true;
			matrix = new Matrix();
			//var backgroundBitmap: BitmapData = new AlphaBackground();
			trackAlpha = new MovieClip();
			trackAlpha.name = "trackAlpha";
			//trackAlpha.graphics.beginBitmapFill(backgroundBitmap, matrix, repeat);
			trackColor.graphics.beginFill(0xCCCCCC);
			trackAlpha.graphics.drawRect(sOffsetX, sOffsetY, sWidth, sHeight);
			trackAlpha.graphics.endFill();
			trackAlpha.width = sWidth;
			trackAlpha.height = sHeight;
			aTrack.trackAlpha = aTrack.addChild(trackAlpha);
			
			trackColor = new MovieClip();
			trackColor.name = "trackColor";
			trackColor.graphics.beginFill(0x000000);
			trackColor.graphics.drawRect(sOffsetX, sOffsetY, sWidth, sHeight);
			trackColor.graphics.endFill();
			trackColor.width = sWidth;
			trackColor.height = sHeight;
			aTrack.trackColor = aTrack.addChild(trackColor);
		}
	}
}
