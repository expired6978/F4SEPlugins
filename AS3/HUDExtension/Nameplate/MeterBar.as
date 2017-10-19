package Nameplate
{
   import flash.display.MovieClip;
   import flash.events.Event;
   
   public class MeterBar extends MovieClip
   {
      public var MeterBarInternal_mc:MovieClip;
      private var _startingX:Number;
      private var _startingWidth:Number;
      private var _percent:Number;
      
      public function MeterBar()
      {
         super();
         addEventListener(Event.ADDED_TO_STAGE, onAddedToStageEvent);
      }
      
      public function onAddedToStageEvent(param1:Event)
      {
         _startingX = x;
         _startingWidth = width;
      }
      
      public function set percent(a_number:Number)
      {
         _percent = a_number;
         width = _startingWidth * a_number / 100;
      }
   }
}