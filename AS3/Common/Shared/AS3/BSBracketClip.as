package Shared.AS3
{
   import flash.display.MovieClip;
   import flash.geom.Point;
   import flash.geom.Rectangle;
   import flash.display.CapsStyle;
   import flash.display.JointStyle;
   
   public dynamic class BSBracketClip extends MovieClip
   {
      
      static const BR_HORIZONTAL:String = "horizontal";
      
      static const BR_VERTICAL:String = "vertical";
      
      static const BR_CORNERS:String = "corners";
      
      static const BR_FULL:String = "full";
       
      private var _drawPos:Point;
      
      private var _clipRect:Rectangle;
      
      private var _lineThickness:Number;
      
      private var _cornerLength:Number;
      
      private var _padding:Point;
      
      private var _style:String;
      
      public function BSBracketClip()
      {
         super();
      }
      
      public function BracketPair() : *
      {
      }
      
      public function ClearBrackets() : *
      {
         graphics.clear();
      }
      
      public function redrawUIComponent(aDrawClip:BSUIComponent, aLineThickness:Number, aCornerLength:Number, aPadding:Point, aStyle:String) : *
      {
         this._clipRect = aDrawClip.getBounds(aDrawClip);
         this._lineThickness = aLineThickness;
         this._cornerLength = aCornerLength;
         this._padding = aPadding;
         this._clipRect.inflatePoint(this._padding);
         this._style = aStyle;
         this.ClearBrackets();
         graphics.lineStyle(this._lineThickness,16777215,1,false,"normal",CapsStyle.SQUARE,JointStyle.MITER,3);
         switch(this._style)
         {
            case BR_HORIZONTAL:
               this.doHorizontal();
               break;
            case BR_VERTICAL:
               this.doVertical();
               break;
            case BR_CORNERS:
               this.doCorners();
               break;
            case BR_FULL:
               this.doFull();
               break;
         }
      }
      
      private function doHorizontal() : *
      {
         this._drawPos = new Point(this._clipRect.left + this._cornerLength,this._clipRect.top);
         this.moveTo();
         this.LineX(this._clipRect.left);
         this.LineY(this._clipRect.bottom);
         this.LineX(this._clipRect.left + this._cornerLength);
         this.MoveX(this._clipRect.right - this._cornerLength);
         this.LineX(this._clipRect.right);
         this.LineY(this._clipRect.top);
         this.LineX(this._clipRect.right - this._cornerLength);
      }
      
      private function doVertical() : *
      {
         this._drawPos = new Point(this._clipRect.left,this._clipRect.top + this._cornerLength);
         this.moveTo();
         this.LineY(this._clipRect.top);
         this.LineX(this._clipRect.right);
         this.LineY(this._clipRect.top + this._cornerLength);
         this.MoveY(this._clipRect.bottom - this._cornerLength);
         this.LineY(this._clipRect.bottom);
         this.LineX(this._clipRect.left);
         this.LineY(this._clipRect.bottom - this._cornerLength);
      }
      
      private function doCorners() : *
      {
         this._drawPos = new Point(this._clipRect.left + this._cornerLength,this._clipRect.top);
         this.moveTo();
         this.LineX(this._clipRect.left);
         this.LineY(this._clipRect.top + this._cornerLength);
         this.MoveY(this._clipRect.bottom - this._cornerLength);
         this.LineY(this._clipRect.bottom);
         this.LineX(this._clipRect.left + this._cornerLength);
         this.MoveX(this._clipRect.right - this._cornerLength);
         this.LineX(this._clipRect.right);
         this.LineY(this._clipRect.bottom - this._cornerLength);
         this.MoveY(this._clipRect.top + this._cornerLength);
         this.LineY(this._clipRect.top);
         this.LineX(this._clipRect.right - this._cornerLength);
      }
      
      private function doFull() : *
      {
         this._drawPos = new Point(this._clipRect.left,this._clipRect.top);
         this.moveTo();
         this.LineY(this._clipRect.bottom);
         this.LineX(this._clipRect.right);
         this.LineY(this._clipRect.top);
         this.LineX(this._clipRect.left);
      }
      
      private function LineX(newX:Number) : *
      {
         this._drawPos.x = newX;
         this.lineTo();
      }
      
      private function LineY(newY:Number) : *
      {
         this._drawPos.y = newY;
         this.lineTo();
      }
      
      private function MoveX(newX:Number) : *
      {
         this._drawPos.x = newX;
         this.moveTo();
      }
      
      private function MoveY(newY:Number) : *
      {
         this._drawPos.y = newY;
         this.moveTo();
      }
      
      private function lineTo() : *
      {
         graphics.lineTo(this._drawPos.x,this._drawPos.y);
      }
      
      private function moveTo() : *
      {
         graphics.moveTo(this._drawPos.x,this._drawPos.y);
      }
   }
}
