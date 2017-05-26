package Mobile.ScrollList 
{
    import flash.display.*;
    import flash.utils.*;
    
    public class FlashUtil extends Object
    {
        public function FlashUtil()
        {
            super();
            return;
        }

        public static function getLibraryItem(arg1:flash.display.MovieClip, arg2:String):flash.display.DisplayObject
        {
            var loc1:*=getLibraryClass(arg1, arg2);
            if (flash.utils.getQualifiedSuperclassName(loc1) == BITMAP_DATA_CLASS_NAME) 
            {
                return new flash.display.Bitmap(new loc1(), "auto", true);
            }
            return new loc1();
        }

        public static function getLibraryClass(arg1:flash.display.MovieClip, arg2:String):Class
        {
            var loc1:*=arg1.loaderInfo.applicationDomain.getDefinition(arg2) as Class;
            return loc1;
        }

        internal static const BITMAP_DATA_CLASS_NAME:String=flash.utils.getQualifiedClassName(flash.display.BitmapData);
    }
}
