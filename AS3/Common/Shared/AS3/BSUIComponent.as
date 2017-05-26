package Shared.AS3
{
   import flash.display.MovieClip;
   import flash.events.Event;
   import flash.geom.Rectangle;
   import Shared.PlatformChangeEvent;
   import flash.geom.Point;
   import Shared.PlatformRequestEvent;
   
   public dynamic class BSUIComponent extends MovieClip
   {
       
      private var _bIsDirty:Boolean;
      
      private var _iPlatform:Number;
      
      private var _bPS3Switch:Boolean;
      
      private var _bAcquiredByNativeCode:Boolean;
      
      private var _bShowBrackets:Boolean = false;
      
      private var _bUseShadedBackground:Boolean = false;
      
      private var _shadedBackgroundType:String = "normal";
      
      private var _shadedBackgroundMethod:String = "Shader";
      
      private var _bracketPair:Shared.AS3.BSBracketClip;
      
      private var _bracketLineWidth:Number = 1.5;
      
      private var _bracketCornerLength:Number = 6;
      
      private var _bracketPaddingX:Number = 0;
      
      private var _bracketPaddingY:Number = 0;
      
      private var _bracketStyle:String = "horizontal";
      
      public function BSUIComponent()
      {
         super();
         this._bIsDirty = false;
         this._iPlatform = PlatformChangeEvent.PLATFORM_INVALID;
         this._bPS3Switch = false;
         this._bAcquiredByNativeCode = false;
         this._bracketPair = new Shared.AS3.BSBracketClip();
         addEventListener(Event.ADDED_TO_STAGE,this.onAddedToStageEvent);
      }
      
      public function get bIsDirty() : Boolean
      {
         return this._bIsDirty;
      }
      
      public function get iPlatform() : Number
      {
         return this._iPlatform;
      }
      
      public function get bPS3Switch() : Boolean
      {
         return this._bPS3Switch;
      }
      
      public function get bAcquiredByNativeCode() : Boolean
      {
         return this._bAcquiredByNativeCode;
      }
      
      public function get bShowBrackets() : Boolean
      {
         return this._bShowBrackets;
      }
      
      public function set bShowBrackets(abShowBrackets:Boolean) : *
      {
         if(this.bShowBrackets != abShowBrackets)
         {
            this._bShowBrackets = abShowBrackets;
            this.SetIsDirty();
         }
      }
      
      public function get bracketLineWidth() : Number
      {
         return this._bracketLineWidth;
      }
      
      public function set bracketLineWidth(aBracketLineWidth:Number) : void
      {
         if(this._bracketLineWidth != aBracketLineWidth)
         {
            this._bracketLineWidth = aBracketLineWidth;
            this.SetIsDirty();
         }
      }
      
      public function get bracketCornerLength() : Number
      {
         return this._bracketCornerLength;
      }
      
      public function set bracketCornerLength(aBracketCornerLength:Number) : void
      {
         if(this._bracketCornerLength != aBracketCornerLength)
         {
            this._bracketCornerLength = aBracketCornerLength;
            this.SetIsDirty();
         }
      }
      
      public function get bracketPaddingX() : Number
      {
         return this._bracketPaddingX;
      }
      
      public function set bracketPaddingX(aBracketPaddingX:Number) : void
      {
         if(this._bracketPaddingX != aBracketPaddingX)
         {
            this._bracketPaddingX = aBracketPaddingX;
            this.SetIsDirty();
         }
      }
      
      public function get bracketPaddingY() : Number
      {
         return this._bracketPaddingY;
      }
      
      public function set bracketPaddingY(aBracketPaddingY:Number) : void
      {
         if(this._bracketPaddingY != aBracketPaddingY)
         {
            this._bracketPaddingY = aBracketPaddingY;
            this.SetIsDirty();
         }
      }
      
      public function get BracketStyle() : String
      {
         return this._bracketStyle;
      }
      
      public function set BracketStyle(aBracketStyle:String) : *
      {
         if(this._bracketStyle != aBracketStyle)
         {
            this._bracketStyle = aBracketStyle;
            this.SetIsDirty();
         }
      }
      
      public function get bUseShadedBackground() : Boolean
      {
         return this._bUseShadedBackground;
      }
      
      public function set bUseShadedBackground(abUseShadedBackground:Boolean) : *
      {
         if(this._bUseShadedBackground != abUseShadedBackground)
         {
            this._bUseShadedBackground = abUseShadedBackground;
            this.SetIsDirty();
         }
      }
      
      public function get ShadedBackgroundType() : String
      {
         return this._shadedBackgroundType;
      }
      
      public function set ShadedBackgroundType(aShadedBackgroundType:String) : *
      {
         if(this._shadedBackgroundType != aShadedBackgroundType)
         {
            this._shadedBackgroundType = aShadedBackgroundType;
            this.SetIsDirty();
         }
      }
      
      public function get ShadedBackgroundMethod() : String
      {
         return this._shadedBackgroundMethod;
      }
      
      public function set ShadedBackgroundMethod(aShadedBackgroundMethod:String) : *
      {
         if(this._shadedBackgroundMethod != aShadedBackgroundMethod)
         {
            this._shadedBackgroundMethod = aShadedBackgroundMethod;
            this.SetIsDirty();
         }
      }
      
      public function SetIsDirty() : void
      {
         this._bIsDirty = true;
         this.requestRedraw();
      }
      
      private final function ClearIsDirty() : void
      {
         this._bIsDirty = false;
      }
      
      public function onAcquiredByNativeCode() : *
      {
         this._bAcquiredByNativeCode = true;
      }
      
      private final function onEnterFrameEvent(arEvent:Event) : void
      {
         removeEventListener(Event.ENTER_FRAME,this.onEnterFrameEvent,false);
         if(this.bIsDirty)
         {
            this.requestRedraw();
         }
      }
      
      private final function onAddedToStageEvent(arEvent:Event) : void
      {
         removeEventListener(Event.ADDED_TO_STAGE,this.onAddedToStageEvent);
         this.onAddedToStage();
         addEventListener(Event.REMOVED_FROM_STAGE,this.onRemovedFromStageEvent);
      }
      
      private function requestRedraw() : void
      {
         if(stage)
         {
            stage.addEventListener(Event.RENDER,this.onRenderEvent);
            addEventListener(Event.ENTER_FRAME,this.onEnterFrameEvent,false,0,true);
            stage.invalidate();
         }
      }
      
      private final function onRemovedFromStageEvent(arEvent:Event) : void
      {
         removeEventListener(Event.REMOVED_FROM_STAGE,this.onRemovedFromStageEvent);
         this.onRemovedFromStage();
         addEventListener(Event.ADDED_TO_STAGE,this.onAddedToStageEvent);
      }
      
      private final function onRenderEvent(arEvent:Event) : void
      {
         var bBracketsDrawn:* = undefined;
         var preDrawBounds:Rectangle = null;
         var postDrawBounds:Rectangle = null;
         removeEventListener(Event.ENTER_FRAME,this.onEnterFrameEvent,false);
         if(stage)
         {
            stage.removeEventListener(Event.RENDER,this.onRenderEvent);
         }
         if(this.bIsDirty)
         {
            this.ClearIsDirty();
            try
            {
               bBracketsDrawn = contains(this._bracketPair);
               if(bBracketsDrawn)
               {
                  removeChild(this._bracketPair);
               }
               preDrawBounds = getBounds(this);
               this.redrawUIComponent();
               postDrawBounds = getBounds(this);
               this.UpdateBrackets(!bBracketsDrawn || preDrawBounds != postDrawBounds);
            }
            catch(e:Error)
            {
               trace(this + " " + this.name + ": " + e.getStackTrace());
            }
         }
         if(this.bIsDirty)
         {
            addEventListener(Event.ENTER_FRAME,this.onEnterFrameEvent,false,0,true);
         }
      }
      
      private final function onSetPlatformEvent(event:Event) : *
      {
         var e:PlatformChangeEvent = event as PlatformChangeEvent;
         this.SetPlatform(e.uiPlatform,e.bPS3Switch);
      }
      
      public function UpdateBrackets(bRedrawBrackets:Boolean) : *
      {
         if(this._bShowBrackets && width > this.bracketCornerLength && height > this._bracketCornerLength)
         {
            if(bRedrawBrackets)
            {
               this._bracketPair.redrawUIComponent(this,this.bracketLineWidth,this.bracketCornerLength,new Point(this._bracketPaddingX,this.bracketPaddingY),this.BracketStyle);
            }
            addChild(this._bracketPair);
         }
         else
         {
            this._bracketPair.ClearBrackets();
         }
      }
      
      public function onAddedToStage() : void
      {
         dispatchEvent(new PlatformRequestEvent(this));
         if(stage)
         {
            stage.addEventListener(PlatformChangeEvent.PLATFORM_CHANGE,this.onSetPlatformEvent);
         }
         if(this.bIsDirty)
         {
            this.requestRedraw();
         }
      }
      
      public function onRemovedFromStage() : void
      {
         if(stage)
         {
            stage.removeEventListener(PlatformChangeEvent.PLATFORM_CHANGE,this.onSetPlatformEvent);
            stage.removeEventListener(Event.RENDER,this.onRenderEvent);
         }
      }
      
      public function redrawUIComponent() : void
      {
      }
      
      public function SetPlatform(aiPlatform:Number, abPS3Switch:Boolean) : void
      {
         if(this._iPlatform != aiPlatform || this._bPS3Switch != abPS3Switch)
         {
            this._iPlatform = aiPlatform;
            this._bPS3Switch = abPS3Switch;
            this.SetIsDirty();
         }
      }
   }
}
