package Shared.AS3 
{
    import flash.events.*;
    
    public class ListFilterer extends flash.events.EventDispatcher
    {
		public static const FILTER_CHANGE:String="ListFilterer::filter_change";
        internal var iItemFilter:int;
        internal var _filterArray:Array;
		
        public function ListFilterer()
        {
            super();
            iItemFilter = 4294967295;
        }

        public function get itemFilter():int
        {
            return this.iItemFilter;
        }

        public function set itemFilter(arg1:int):*
        {
            var loc1:*=!(this.iItemFilter == arg1);
            iItemFilter = arg1;
            if (loc1 == true) 
            {
                dispatchEvent(new flash.events.Event(FILTER_CHANGE, true, true));
            }
            return;
        }

        public function get filterArray():Array
        {
            return _filterArray;
        }

        public function set filterArray(arg1:Array):*
        {
            _filterArray = arg1;
        }

        public function EntryMatchesFilter(entry:Object):Boolean
        {
            return entry != null && (!entry.hasOwnProperty("filterFlag") || !((entry.filterFlag & iItemFilter) == 0));
        }

        public function GetPrevFilterMatch(currentIndex:int):int
        {
            var foundIndex:int = int.MAX_VALUE;
            if (currentIndex != int.MAX_VALUE && _filterArray != null) 
            {
				var previousIndex:int = currentIndex - 1;
                while (previousIndex >= 0) 
                {
                    if (EntryMatchesFilter(_filterArray[previousIndex])) 
                    {
                        foundIndex = previousIndex;
						break;
                    }
                    --previousIndex;
                }
            }
            return foundIndex;
        }

        public function GetNextFilterMatch(currentIndex:int):int
        {
            var foundIndex:int = int.MAX_VALUE;
            if (currentIndex != int.MAX_VALUE && _filterArray != null) 
            {
				var nextIndex:int = currentIndex + 1;
                while (nextIndex < _filterArray.length)
                {
                    if (EntryMatchesFilter(_filterArray[nextIndex])) 
                    {
                        foundIndex = nextIndex;
						break;
                    }
                    ++nextIndex;
                }
            }
            return foundIndex;
        }

        public function ClampIndex(arg1:int):int
        {
            var loc2:*=0;
            var loc3:*=0;
            var loc1:*=arg1;
            if (!(arg1 == int.MAX_VALUE) && !(this._filterArray == null) && !this.EntryMatchesFilter(this._filterArray[loc1])) 
            {
                loc2 = this.GetNextFilterMatch(loc1);
                loc3 = this.GetPrevFilterMatch(loc1);
                if (loc2 == int.MAX_VALUE) 
                {
                    if (loc3 == int.MAX_VALUE) 
                    {
                        loc1 = int.MAX_VALUE;
                    }
                    else 
                    {
                        loc1 = loc3;
                    }
                }
                else 
                {
                    loc1 = loc2;
                }
                if (!(loc2 == int.MAX_VALUE) && !(loc3 == int.MAX_VALUE) && !(loc3 == loc2) && loc1 == loc2 && this._filterArray[loc3].text == this._filterArray[arg1].text) 
                {
                    loc1 = loc3;
                }
            }
            return loc1;
        }

        public function IsFilterEmpty(arg1:int):Boolean
        {
            var loc2:*=false;
            var loc1:*=iItemFilter;
            iItemFilter = arg1;
            loc2 = ClampIndex(0) == int.MAX_VALUE;
            iItemFilter = loc1;
            return loc2;
        }
    }
}
