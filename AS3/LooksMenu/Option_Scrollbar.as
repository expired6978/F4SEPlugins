package 
{
	import flash.display.*;
	import flash.events.*;
	import flash.geom.*;
	import flash.ui.*;
	
	public class Option_Scrollbar extends flash.display.MovieClip
	{
		public static const VALUE_CHANGE:String="Option_Scrollbar::VALUE_CHANGE";
		public var Track_mc:flash.display.MovieClip;
		public var Thumb_mc:flash.display.MovieClip;
		public var LeftArrow_mc:flash.display.MovieClip;
		public var RightArrow_mc:flash.display.MovieClip;
		public var LeftCatcher_mc:flash.display.MovieClip;
		public var RightCatcher_mc:flash.display.MovieClip;
		public var BarCatcher_mc:flash.display.MovieClip;
		public var TrackBorder_mc:flash.display.MovieClip;
		internal var fValue:Number;
		protected var fMinThumbX:Number;
		protected var fMaxThumbX:Number;
		internal var fMinValue:Number=0;
		internal var fMaxValue:Number=1;
		internal var fStepSize:Number=0.05;
		internal var iStartDragThumb:int;
		internal var fStartValue:Number;
		internal var dragStart:Point;
        public var offsetLeft:Number = 0;
        public var offsetRight:Number = 0;
		
		public function Option_Scrollbar()
		{
			super();
			fMinThumbX = Track_mc.x;
			fMaxThumbX = Track_mc.x + Track_mc.width - Thumb_mc.width;
			addEventListener(flash.events.MouseEvent.CLICK, onClick);
			Thumb_mc.addEventListener(flash.events.MouseEvent.MOUSE_DOWN, onThumbMouseDown);
		}
		
        public function get maximum():Number { return fMaxValue; }
        public function set maximum(value:Number):void { fMaxValue = value; }

        public function get minimum():Number { return fMinValue; }
        public function set minimum(value:Number):void { fMinValue = value; }
		
		public function get snapInterval():Number { return fStepSize; }
        public function set snapInterval(value:Number):void { fStepSize = value; }

		public function get MinValue():Number
		{
			return fMinValue;
		}

		public function set MinValue(arg1:Number):*
		{
			fMinValue = arg1;
		}

		public function get MaxValue():Number
		{
			return fMaxValue;
		}

		public function set MaxValue(arg1:Number):*
		{
			fMaxValue = arg1;
		}

		public function get StepSize():Number
		{
			return fStepSize;
		}

		public function set StepSize(arg1:Number):*
		{
			fStepSize = arg1;

		}
		
		public function get position():Number
		{
			return fValue;
		}
        public function set position(value:Number):void
		{
			fValue = Math.min(Math.max(value, this.fMinValue), this.fMaxValue);
			var xOffset = (fValue - fMinValue) / (fMaxValue - fMinValue);
			Thumb_mc.x = fMinThumbX + xOffset * (fMaxThumbX - fMinThumbX);
		}

		public function get value():Number
		{
			return fValue;
		}

		public function set value(val:Number):*
		{
			position = val;
			dispatchEvent(new flash.events.Event(VALUE_CHANGE, true, true));
		}

		public function Decrement():*
		{
			value = value - fStepSize;
		}

		public function Increment():*
		{
			value = value + fStepSize;
		}

		public function HandleKeyboardInput(arg1:flash.events.KeyboardEvent):*
		{
			if (arg1.keyCode == flash.ui.Keyboard.LEFT && this.value > 0) 
			{
				Decrement();
			}
			else if (arg1.keyCode == flash.ui.Keyboard.RIGHT && this.value < 1) 
			{
				Increment();
			}
		}

		public function onClick(event:flash.events.MouseEvent):*
		{
			if (event.target != this.LeftCatcher_mc) 
			{
				if (event.target != this.RightCatcher_mc) 
				{
					if (event.target == this.BarCatcher_mc) 
					{
						var trackWidth:Number = (Track_mc.width - offsetLeft - offsetRight);
           				var newValue:Number = (event.localX * scaleX - offsetLeft) / trackWidth * (fMaxValue - fMinValue) + fMinValue;
												
						this.value = Math.max(fMinValue, Math.min(fMaxValue, newValue));//arg1.currentTarget.mouseX / this.BarCatcher_mc.width * (this.fMaxValue - this.fMinValue);
						dispatchEvent(new flash.events.Event(VALUE_CHANGE, true, true));
					}
				}
				else 
				{
					Increment();
				}
			}
			else 
			{
				Decrement();
			}
		}

		internal function onThumbMouseDown(arg1:flash.events.MouseEvent):*
		{
			Thumb_mc.startDrag(false, new flash.geom.Rectangle(0, Thumb_mc.y, fMaxThumbX, 0));
			stage.addEventListener(flash.events.MouseEvent.MOUSE_UP, onThumbMouseUp);
			stage.addEventListener(flash.events.MouseEvent.MOUSE_MOVE, onThumbMouseMove);
		}

		internal function onThumbMouseMove(event:flash.events.MouseEvent):*
		{
			var thumbX = Thumb_mc.x - fMinThumbX;
			value = thumbX / (fMaxThumbX - fMinThumbX) * (fMaxValue - fMinValue);
		}

		internal function onThumbMouseUp(arg1:flash.events.MouseEvent):*
		{
			value = value;
			Thumb_mc.stopDrag();
			stage.removeEventListener(flash.events.MouseEvent.MOUSE_UP, onThumbMouseUp);
			stage.removeEventListener(flash.events.MouseEvent.MOUSE_MOVE, onThumbMouseMove);
		}
	}
}
