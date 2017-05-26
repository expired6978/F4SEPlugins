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
		internal var fValue:Number;
		protected var fMinThumbX:Number;
		protected var fMaxThumbX:Number;
		internal var fMinValue:Number=0;
		internal var fMaxValue:Number=1;
		internal var fStepSize:Number=0.05;
		internal var iStartDragThumb:int;
		internal var fStartValue:Number;
		internal var dragStart:Point;
		
		public function Option_Scrollbar()
		{
			super();
			this.fMinThumbX = this.Track_mc.x;
			this.fMaxThumbX = this.Track_mc.x + this.Track_mc.width - this.Thumb_mc.width;
			addEventListener(flash.events.MouseEvent.CLICK, this.onClick);
			this.Thumb_mc.addEventListener(flash.events.MouseEvent.MOUSE_DOWN, this.onThumbMouseDown);
		}

		public function get MinValue():Number
		{
			return this.fMinValue;
		}

		public function set MinValue(arg1:Number):*
		{
			this.fMinValue = arg1;
		}

		public function get MaxValue():Number
		{
			return this.fMaxValue;
		}

		public function set MaxValue(arg1:Number):*
		{
			this.fMaxValue = arg1;
		}

		public function get StepSize():Number
		{
			return this.fStepSize;
		}

		public function set StepSize(arg1:Number):*
		{
			this.fStepSize = arg1;

		}

		public function get value():Number
		{
			return this.fValue;
		}

		public function set value(arg1:Number):*
		{
			this.fValue = Math.min(Math.max(arg1, this.fMinValue), this.fMaxValue);
			var loc1:*=(this.fValue - this.fMinValue) / (this.fMaxValue - this.fMinValue);
			this.Thumb_mc.x = this.fMinThumbX + loc1 * (this.fMaxThumbX - this.fMinThumbX);
		}

		public function Decrement():*
		{
			this.value = this.value - this.fStepSize;
			dispatchEvent(new flash.events.Event(VALUE_CHANGE, true, true));
		}

		public function Increment():*
		{
			this.value = this.value + this.fStepSize;
			dispatchEvent(new flash.events.Event(VALUE_CHANGE, true, true));
		}

		public function HandleKeyboardInput(arg1:flash.events.KeyboardEvent):*
		{
			if (arg1.keyCode == flash.ui.Keyboard.LEFT && this.value > 0) 
			{
				this.Decrement();
			}
			else if (arg1.keyCode == flash.ui.Keyboard.RIGHT && this.value < 1) 
			{
				this.Increment();
			}
		}

		public function onClick(arg1:flash.events.MouseEvent):*
		{
			if (arg1.target != this.LeftCatcher_mc) 
			{
				if (arg1.target != this.RightCatcher_mc) 
				{
					if (arg1.target == this.BarCatcher_mc) 
					{
						this.value = arg1.currentTarget.mouseX / this.BarCatcher_mc.width * (this.fMaxValue - this.fMinValue);
						dispatchEvent(new flash.events.Event(VALUE_CHANGE, true, true));
					}
				}
				else 
				{
					this.Increment();
				}
			}
			else 
			{
				this.Decrement();
			}
		}

		internal function onThumbMouseDown(arg1:flash.events.MouseEvent):*
		{
			this.Thumb_mc.startDrag(false, new flash.geom.Rectangle(0, this.Thumb_mc.y, this.fMaxThumbX - this.fMinThumbX, 0));
			stage.addEventListener(flash.events.MouseEvent.MOUSE_UP, this.onThumbMouseUp);
			stage.addEventListener(flash.events.MouseEvent.MOUSE_MOVE, this.onThumbMouseMove);
		}

		internal function onThumbMouseMove(arg1:flash.events.MouseEvent):*
		{
			var loc1:*=this.Thumb_mc.x - this.fMinThumbX;
			this.value = loc1 / (this.fMaxThumbX - this.fMinThumbX) * (this.fMaxValue - this.fMinValue);
			dispatchEvent(new flash.events.Event(VALUE_CHANGE, true, true));
		}

		internal function onThumbMouseUp(arg1:flash.events.MouseEvent):*
		{
			this.Thumb_mc.stopDrag();
			stage.removeEventListener(flash.events.MouseEvent.MOUSE_UP, this.onThumbMouseUp);
			stage.removeEventListener(flash.events.MouseEvent.MOUSE_MOVE, this.onThumbMouseMove);
		}
	}
}
