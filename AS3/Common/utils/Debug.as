package utils
{
	import Date;

	public class Debug extends Object
	{
		public function Debug()
        {
            super();
        }
		
	  /* PRIVATE VARIABLES */
		private static var _buffer: Array = [];
		private static var _traversed: Array = [];

	  /* PUBLIC STATIC FUNCTIONS */
		public static function log(/* a_text: String , a_text2: String ... */)
		{
			var date: Date = new Date;
			var hh: String = String(date.getHours());
			var mm: String = String(date.getMinutes());
			var ss: String = String(date.getSeconds());
			var ff: String = String(date.getMilliseconds());

			var dateTime: String = "[" + ((hh.length < 2) ? "0" + hh : hh);
			dateTime += ":" + ((mm.length < 2) ? "0" + mm : mm);
			dateTime += ":" + ((ss.length < 2) ? "0" + ss : ss);
			dateTime += "." + ((ff.length < 2) ? "00" + ff : (ff.length < 3) ? "0" + ff : ff);
			dateTime += "]";

			// Flush buffer
			if (_buffer.length > 0) {
				for(var i:uint = 0; i < _buffer.length; i++)
					trace(_buffer[i]);			
				_buffer.splice(0);
			}

			for(var j:uint = 0; j < arguments.length; j++) {
				var str = dateTime + " " + arguments[j];
				trace(str);
			}
		}

		public static function logNT(/* a_text: String , a_text2: String ... */)
		{
			// Flush buffer
			if (_buffer.length > 0) {
				for(var j:uint = 0; j < _buffer.length; j++)
					trace(_buffer[j]);			
				_buffer.splice(0);
			}

			for(var k:uint = 0; i < arguments.length; k++) {
				var str = arguments[k];
				trace(str);
			}
		}

		public static function dump(a_name: String, a_obj, a_noTimestamp: Boolean, a_padLevel: Number, a_getMembers: Function)
		{
			var pad: String = "";
			var padLevel: Number = a_padLevel;
			var logFn: Function = a_noTimestamp ? logNT : log;
			
			for(var i:uint = 0; i < padLevel; i++)
				pad += "    ";
							
			switch(typeof(a_obj))
			{
				case "object":
				case "function":
				case "movieclip":
				{
					var suffix = ": (" + typeof(a_obj) + ")";
					if(_traversed.indexOf(a_obj) != -1)
						suffix += " [Completed]";
						
					logFn(pad + a_name + suffix);
					
					if(_traversed.indexOf(a_obj) != -1)
						return;
					
					_traversed.push(a_obj);
					
					var members = a_getMembers(a_obj);
					for (var k = 0; k < members.length; k++)
					{
						try
						{
							dump(members[k], a_obj[members[k]], a_noTimestamp, padLevel + 1, a_getMembers);
						}
						catch(e:Error)
						{
							logFn(pad + "Error: " + members[k]);
						}
					}
				}
				break;
				case "array":
				{
					logFn(pad + a_name);
					for(var j:uint = 0; j < a_obj.length; j++)
						dump(j, a_obj[j], a_noTimestamp, padLevel + 1, a_getMembers);
				}
				break;
				default:
				{
					logFn(pad + a_name + ": " + a_obj + " (" + typeof(a_obj) + ")");
				}
				break;
			}
			
			_traversed.splice(0, _traversed.length);
		}
	}
}