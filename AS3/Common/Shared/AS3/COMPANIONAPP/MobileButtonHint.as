package Shared.AS3.COMPANIONAPP 
{
    import Shared.AS3.*;
    import flash.display.*;
    import flash.events.*;
    import flash.geom.*;
    
    public dynamic class MobileButtonHint extends Shared.AS3.BSButtonHint
    {
        public function MobileButtonHint()
        {
            super();
            addEventListener(flash.events.MouseEvent.MOUSE_DOWN, this.onButtonPress);
            return;
        }

        internal function onButtonPress(arg1:flash.events.MouseEvent):void
        {
            if (!ButtonDisabled && ButtonVisible) 
            {
                this.setPressState();
            }
            return;
        }

        protected override function onMouseOut(arg1:flash.events.MouseEvent):*
        {
            super.onMouseOut(arg1);
            if (!ButtonDisabled && ButtonVisible) 
            {
                this.setNormalState();
            }
            return;
        }

        public override function onTextClick(arg1:flash.events.Event):void
        {
            super.onTextClick(arg1);
            if (!ButtonDisabled && ButtonVisible) 
            {
                this.setNormalState();
            }
            return;
        }

        public override function redrawUIComponent():void
        {
            this.background.width = 1;
            this.background.height = 1;
            super.redrawUIComponent();
            this.background.width = textField_tf.width + this.BUTTON_MARGIN;
            this.background.height = textField_tf.height + this.BUTTON_MARGIN;
            if (Justification == JUSTIFY_RIGHT) 
            {
                this.background.x = 0;
                this.textField_tf.x = 0;
                if (hitArea) 
                {
                    hitArea.x = 0;
                }
            }
            if (hitArea) 
            {
                hitArea.width = this.background.width;
                hitArea.height = this.background.height;
            }
            if (ButtonVisible) 
            {
                if (ButtonDisabled) 
                {
                    this.setDisableState();
                }
                else 
                {
                    this.setNormalState();
                }
            }
            return;
        }

        protected function setNormalState():void
        {
            this.background.gotoAndPlay("normal");
            var loc1:*=textField_tf.transform.colorTransform;
            loc1.redOffset = 0;
            loc1.greenOffset = 0;
            loc1.blueOffset = 0;
            textField_tf.transform.colorTransform = loc1;
            return;
        }

        protected function setDisableState():void
        {
            this.setNormalState();
            this.background.gotoAndPlay("disabled");
            return;
        }

        protected function setPressState():void
        {
            this.background.gotoAndPlay("press");
            var loc1:*=textField_tf.transform.colorTransform;
            loc1.redOffset = 255;
            loc1.greenOffset = 255;
            loc1.blueOffset = 255;
            textField_tf.transform.colorTransform = loc1;
            return;
        }

        internal const BUTTON_MARGIN:Number=4;

        public var background:flash.display.MovieClip;
    }
}
