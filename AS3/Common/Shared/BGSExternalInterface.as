package Shared 
{
    public class BGSExternalInterface extends Object
    {
        public function BGSExternalInterface()
        {
            super();
            return;
        }

        public static function call(arg1:Object, ... rest):void
        {
            var loc1:*=null;
            var loc2:*=null;
            if (arg1 == null) 
            {
                trace("BGSExternalInterface::call -- Can\'t call function \'" + loc1 + "\' on BGSCodeObj. BGSCodeObj is null!");
            }
            else 
            {
                loc1 = rest.shift();
                if ((loc2 = arg1[loc1]) == null) 
                {
                    trace("BGSExternalInterface::call -- Can\'t call function \'" + loc1 + "\' on BGSCodeObj. This function doesn\'t exist!");
                }
                else 
                {
                    loc2.apply(null, rest);
                }
            }
        }
    }
}
