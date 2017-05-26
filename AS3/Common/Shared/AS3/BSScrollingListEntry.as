package Shared.AS3 
{
    import Shared.*;
    import flash.display.*;
    import flash.text.*;
    import scaleform.gfx.*;
    
    public class BSScrollingListEntry extends flash.display.MovieClip
    {
        public function BSScrollingListEntry()
        {
            super();
            scaleform.gfx.Extensions.enabled = true;
            this.ORIG_BORDER_HEIGHT = this.border == null ? 0 : this.border.height;
            return;
        }

        public function get clipIndex():uint
        {
            return this._clipIndex;
        }

        public function set clipIndex(arg1:uint):*
        {
            this._clipIndex = arg1;
            return;
        }

        public function get itemIndex():uint
        {
            return this._itemIndex;
        }

        public function set itemIndex(arg1:uint):*
        {
            this._itemIndex = arg1;
            return;
        }

        public function get selected():Boolean
        {
            return this._selected;
        }

        public function set selected(arg1:Boolean):*
        {
            this._selected = arg1;
            return;
        }

        public function SetEntryText(arg1:Object, arg2:String):*
        {
            var loc1:*=NaN;
            if (!(this.textField == null) && !(arg1 == null) && arg1.hasOwnProperty("text")) 
            {
                if (arg2 != Shared.AS3.BSScrollingList.TEXT_OPTION_SHRINK_TO_FIT) 
                {
                    if (arg2 == Shared.AS3.BSScrollingList.TEXT_OPTION_MULTILINE) 
                    {
                        this.textField.autoSize = flash.text.TextFieldAutoSize.LEFT;
                        this.textField.multiline = true;
                        this.textField.wordWrap = true;
                    }
                }
                else 
                {
                    scaleform.gfx.TextFieldEx.setTextAutoSize(this.textField, "shrink");
                }
                if (arg1.text == undefined) 
                {
                    Shared.GlobalFunc.SetText(this.textField, " ", true);
                }
                else 
                {
                    Shared.GlobalFunc.SetText(this.textField, arg1.text, true);
                }
                this.textField.textColor = this.selected ? 0 : 16777215;
            }
            if (this.border != null) 
            {
                this.border.alpha = this.selected ? Shared.GlobalFunc.SELECTED_RECT_ALPHA : 0;
                if (!(this.textField == null) && arg2 == Shared.AS3.BSScrollingList.TEXT_OPTION_MULTILINE && this.textField.numLines > 1) 
                {
                    loc1 = this.textField.y - this.border.y;
                    this.border.height = this.textField.textHeight + loc1 * 2 + 5;
                }
                else 
                {
                    this.border.height = this.ORIG_BORDER_HEIGHT;
                }
            }
            return;
        }

        public var border:flash.display.MovieClip;

        public var textField:flash.text.TextField;

        protected var _clipIndex:uint;

        protected var _itemIndex:uint;

        protected var _selected:Boolean;

        public var ORIG_BORDER_HEIGHT:Number;
    }
}
