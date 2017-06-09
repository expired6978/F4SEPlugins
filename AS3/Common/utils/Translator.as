package utils
{
	import flash.display.*;
	import flash.text.*;
	
	/* PRIVATE VARIABLES */
	public class Translator extends Object
	{
		public function Translator()
		{
			super();
		}
		
		private static var _translator:TextField = null;
	
		public static function Create(parentClip: MovieClip)
		{
			if (_translator == null) {
				_translator = new TextField();
				_translator.name = "_translator";
				_translator.visible = false;
				var format:TextFormat = new TextFormat();
				format.font = "$MAIN_Font";
				_translator.setTextFormat(format);
				parentClip.addChild(_translator);
			}
		}
		/* PUBLIC FUNCTIONS */
  
		// Translate simple strings
		public static function translate(a_str: String): String
		{
			if(!_translator) {
				trace("Translator not initialized");
				return a_str;
			}
			
			if (a_str == "")
				return "";
			
			if (a_str.charAt(0) != "$")
				return a_str;
			
			_translator.text = a_str;
			
			return _translator.text;
		}
		
		// Translate with support for substrings, i.e. "$My text, default value {$Default value}" 
		public static function translateNested(a_str: String): String
		{
			if (a_str == "")
				return "";
			
			// Quick test to decide if we can skip
			if (a_str.indexOf("{") == -1)
				return translate(a_str);
			
			// Collect substrings
			var subStrings: Array = [];
			var offset = 0;
			
			do {
				var startIndex = a_str.indexOf("{", offset);
				var endIndex = a_str.indexOf("}", offset);
				
				if (startIndex != -1 && endIndex != -1 && startIndex < endIndex) {
					var s = a_str.slice(startIndex+1, endIndex);
					subStrings.push(translate(s));
					
					a_str = a_str.slice(0, startIndex+1) + a_str.slice(endIndex);
					
					offset = startIndex + 2;
				} else {
					break;
				}
				
			} while (true);
			
			// Translate the base string
			a_str = translate(a_str);
			
			// Split at {} and interleave with the substrings
			var t = a_str.split("{}");
			var result = "";
			
			var i;
			for (i=0; i<subStrings.length; i++)
				result += t[i] + subStrings[i];
				
			if (i < (t.length))
				result += t[i];
			
			return result;
		}
	}
}