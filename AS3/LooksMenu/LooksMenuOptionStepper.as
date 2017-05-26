package 
{
	import Shared.*;
	import flash.display.*;
	import flash.text.*;
	
	public class LooksMenuOptionStepper extends flash.display.MovieClip
	{
		public function LooksMenuOptionStepper()
		{
			super();
			return;
		}

		internal function UpdateButtonHelp():*
		{
			Shared.GlobalFunc.SetText(this.LeftIcon_tf, this.Buttons != TRIGGERS ? this.Platform == Shared.PlatformChangeEvent.PLATFORM_PC_GAMEPAD || Shared.PlatformChangeEvent.PLATFORM_XB1 ? "G" : "g" : this.Platform == Shared.PlatformChangeEvent.PLATFORM_PC_GAMEPAD || Shared.PlatformChangeEvent.PLATFORM_XB1 ? "Y" : "y", false);
			Shared.GlobalFunc.SetText(this.RightIcon_tf, this.Buttons != TRIGGERS ? this.Platform == Shared.PlatformChangeEvent.PLATFORM_PC_GAMEPAD || Shared.PlatformChangeEvent.PLATFORM_XB1 ? "L" : "m" : this.Platform == Shared.PlatformChangeEvent.PLATFORM_PC_GAMEPAD || Shared.PlatformChangeEvent.PLATFORM_XB1 ? "X" : "x", false);
			return;
		}

		public function set buttons(arg1:uint):*
		{
			this.Buttons = arg1;
			this.UpdateButtonHelp();
			return;
		}

		public function set getDispInfoFunc(arg1:Function):*
		{
			this.GetDispInfoFunc = arg1;
			return;
		}

		public function set max(arg1:uint):*
		{
			this.MaxIndex = arg1;
			return;
		}

		public function set platform(arg1:uint):*
		{
			this.Platform = arg1;
			this.UpdateButtonHelp();
			return;
		}

		public function set value(arg1:uint):*
		{
			var loc2:*=null;
			var loc3:*=undefined;
			var loc4:*=undefined;
			var loc5:*=undefined;
			this.CurrentIndex = arg1;
			var loc1:*=new Object();
			if (this.GetDispInfoFunc != null) 
			{
				this.GetDispInfoFunc(loc1, arg1);
			}
			else 
			{
				loc1.DisplayIndex = arg1;
				loc1.DisplayLabel = "";
			}
			this.LeftIcon_tf.alpha = this.CurrentIndex > 0 ? 1 : Shared.GlobalFunc.DIMMED_ALPHA;
			this.RightIcon_tf.alpha = this.CurrentIndex < (this.MaxIndex - 1) ? 1 : Shared.GlobalFunc.DIMMED_ALPHA;
			if (loc1.DisplayIndex < uint.MAX_VALUE) 
			{
				loc2 = loc1.DisplayIndex < 10 ? "0" : "";
				loc2 = loc2 + loc1.DisplayIndex.toString();
				Shared.GlobalFunc.SetText(this.Index_tf, loc2, false);
				this.Index_tf.alpha = 1;
			}
			else 
			{
				this.Index_tf.alpha = 0;
			}
			if (loc1.DisplayLabel != this.CurrentLabel) 
			{
				this.CurrentLabel = loc1.DisplayLabel;
				Shared.GlobalFunc.SetText(this.Label_tf, loc1.DisplayLabel, false);
				loc3 = this.Index_tf.textWidth;
				loc4 = this.Label_tf.textWidth + (this.Index_tf.alpha != 0 ? loc3 * 1.5 : 0);
				this.Label_tf.x = (this.Index_tf.alpha != 0 ? -loc4 : -this.Label_tf.textWidth) / 2;
				this.Index_tf.x = loc4 / 2 - loc3;
				this.LeftIcon_tf.x = this.Label_tf.x - loc3 / 2 - this.LeftIcon_tf.textWidth;
				this.RightIcon_tf.x = loc4 / 2;
				loc5 = loc4 + loc3;
				var loc6:*;
				this.Brackets_mc.Bottom_mc.width = loc6 = loc5;
				this.Brackets_mc.Top_mc.width = loc6;
				this.Brackets_mc.x = (-loc5) / 2;
				this.Brackets_mc.Right_mc.x = loc5;
				this.CurrentLabel = loc1.DisplayLabel;
			}
			return;
		}

		public static const TRIGGERS:int=0;
		public static const BUMPERS:int=1;
		public var Label_tf:flash.text.TextField;
		public var Index_tf:flash.text.TextField;
		public var Brackets_mc:flash.display.MovieClip;
		public var LeftIcon_tf:flash.text.TextField;
		public var RightIcon_tf:flash.text.TextField;
		internal var CurrentIndex:uint;
		internal var CurrentLabel:String="";
		internal var GetDispInfoFunc:Function=null;
		internal var MaxIndex:uint;
		internal var Platform:uint;
		internal var Buttons:uint=0;
	}
}
