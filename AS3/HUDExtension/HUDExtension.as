package  {
	import flash.system.ApplicationDomain;
	import flash.display.MovieClip;
	import flash.system.LoaderContext;
	import flash.display.LoaderInfo;
	import flash.display.DisplayObject;
	import flash.display.DisplayObjectContainer;
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.utils.Dictionary;
	import flash.display.Loader;
	import flash.net.URLRequest;
	import flash.utils.getQualifiedClassName;
	import flash.utils.getDefinitionByName;
	import flash.utils.Timer;
    import flash.events.TimerEvent;
	import flash.geom.Matrix;
	import flash.geom.Point;
	import flash.geom.ColorTransform;
	
	public class HUDExtension extends MovieClip
	{
		private var HUDMenu:MovieClip;
		
		private var ObjectsMap:Dictionary = new Dictionary();
		public var Objects:Sprite;
				
		public function HUDExtension()
		{
			addEventListener(Event.ADDED_TO_STAGE,onAddedToStage); 
		}
						
		public function onAddedToStage(evt:Event)
		{
			removeEventListener(Event.ADDED_TO_STAGE,onAddedToStage);

			HUDMenu = stage.getChildAt(0) as MovieClip;
			
			Objects = new Sprite();
			Objects.name = "HUDSurface";
			
			// This portion should only be executed by the game since HUDExtensionBase wont exist otherwise
			var HUDExtensionBase = (HUDMenu.getChildByName("HUDExtensionBase") as MovieClip);
			if(HUDExtensionBase)
			{
				//CreateTargetSurface(HUDMenu.BottomCenterGroup_mc.CompassWidget_mc.CompassBar_mc);
				CreateTargetSurface(HUDMenu.CenterGroup_mc.HUDCrosshair_mc.CrosshairBase_mc.CrosshairTicks_mc.Up);
				CreateTargetSurface(HUDMenu.CenterGroup_mc.HUDCrosshair_mc.CrosshairBase_mc.CrosshairClips_mc);
				//CreateTargetSurface(HUDMenu.CenterGroup_mc.HUDCrosshair_mc.CrosshairBase_mc.CrosshairTicks_mc);
				
				// Create our placeholder surface
				Objects.graphics.beginFill(0xFF0000, 0.0);   // Second param: alpha = 0
				Objects.graphics.drawRect(0, 0, stage.stageWidth, stage.stageHeight);
				Objects.graphics.endFill();
				
				var globalCoords:Point = HUDMenu.globalToLocal(new Point(0, 0));
				Objects.x = globalCoords.x;
				Objects.y = globalCoords.y;
				HUDMenu.addChildAt(Objects, 0);
				
				parent.removeChild(this); // Reparent so we can rename the clip
				name = "HUDExtension";
				HUDExtensionBase.addChild(this); // Move HUDExtension to right below HUDExtensionBase
				HUDExtensionBase.removeChildAt(0); // Remove the Loader object
			}
			else
			{
				addChild(Objects);
			}

			trace("HUDExtension injected");
		}
				
		public function CreateTargetSurface(targetSurface)
		{
			var bgRC = new Sprite();
			bgRC.name = "HUDExtensionSurface";
			bgRC.graphics.beginFill(0x00FF00, 0.0);
			bgRC.graphics.drawRect(0, 0, stage.stageWidth, stage.stageHeight * 1.25);
			bgRC.graphics.endFill();
			
			targetSurface.addChild(bgRC);
			
			var targetSurfaceMatrix:Matrix = targetSurface.transform.concatenatedMatrix.clone();
			// a and d properties of the matrix affects the positioning of pixels along the x and y axis respectively when scaling or rotating an object.
			if (targetSurfaceMatrix.a != 1 || targetSurfaceMatrix.d != 1) {
				targetSurfaceMatrix.invert();
				bgRC.transform.matrix = targetSurfaceMatrix;
			}
			var globalCoords = targetSurface.globalToLocal(new Point(0, 0));
			bgRC.x = globalCoords.x;
			bgRC.y = globalCoords.y;
		}
				
		// Native code invokes this directly
		public function GetNameplate(objectId: int)
		{
			return ObjectsMap[objectId];
		}
		
		// Native code invokes this directly
		public function AddNameplate(objectId: int, a_name: String, a_health: Number, a_level: uint, a_showLevel: Boolean): MovieClip
		{
			var classRef: Class = getDefinitionByName("Nameplate") as Class;
			var swf = new classRef();
			ObjectsMap[objectId] = swf;
			swf.objectData = {"objectName": a_name, "percent": a_health, "level": a_level, "showLevel": a_showLevel};
			Objects.addChild(swf);
			dispatchEvent(new NameplateEvent(NameplateEvent.ADDED, true, false, objectId));
			return swf;
		}
		
		// Native code invokes this directly
		public function RemoveNameplate(objectId: int):Boolean
		{
			var object = ObjectsMap[objectId];
			if(object) {
				Objects.removeChild(object); // Remove from the object holder
				delete ObjectsMap[objectId]; // Erase from the dictionary
				dispatchEvent(new NameplateEvent(NameplateEvent.REMOVED, true, false, objectId));
				return true;
			}
			
			return false;
		}
		
		// Native code invokes this directly
		public function GetRootPoint(): Point
		{
			return HUDMenu.globalToLocal(new Point(0, 0));
		}
		
		// Native code invokes this directly
		public function GetStageHeight(): Number
		{
			return stage.stageHeight;
		}
		
		// Native code invokes this directly
		public function GetStageWidth(): Number
		{
			return stage.stageWidth;
		}
		
		// Native code invokes this directly		
		function SortChildrenByDepth():void {
			var i:int;
			var childList:Array = new Array();
			
			// first put all children in an array
			i = Objects.numChildren;
			while(i--){
				childList[i] = Objects.getChildAt(i);
			}
			
			// next, sort the array based on the
			// elements' zIndex property
			childList.sortOn("zIndex", Array.NUMERIC);
			
			// now match the arrangement of the array
			// with the arrangement of the display list
			i = Objects.numChildren;
			while(i--){
				
				// if the child at position i in the array
				// does not match the child at position
				// i in the display list, set that child
				// to the i position in the display list
				if (childList[i] != Objects.getChildAt(i)){
					Objects.setChildIndex(childList[i], i);
				}
			}
		}
		
		public static function traceDisplayObject(dOC:DisplayObjectContainer, recursionLevel:int=0) {
            var numCh = dOC.numChildren;
            for(var i = 0; i < numCh; i++)
            {
                var child = dOC.getChildAt(i);
                var indentation:String = "";
                for (var j:int=0; j<recursionLevel; j++) {
                    indentation += "----";
                }
                trace(indentation + "[" + i + "] " + child.name + " Alpha: " + child.alpha + " Visible: " + child.visible + " " + child);
				
				if (getQualifiedClassName(child) == "Object") { 
					traceObj(child);
				}

                if (child is DisplayObjectContainer && child.numChildren > 0)
                {
                    traceDisplayObject(child, recursionLevel+1);
                }
            }
        }
		
		public static function traceObj(obj:Object):void {
            for(var id:String in obj) {
			  var value:Object = obj[id];
			
			  if (getQualifiedClassName(value) == "Object") {
				  trace("-->");
				  traceObj(value);
			  } else {
			      trace(id + " = " + value);
			  }
			}
        }
	}
}
