package Shared.AS3.COMPANIONAPP 
{
    import Mobile.ScrollList.*;
    
    public class BSScrollingListInterface extends Object
    {
        public function BSScrollingListInterface()
        {
            super();
            return;
        }

        public static function GetMobileScrollListProperties(arg1:String):Shared.AS3.COMPANIONAPP.MobileScrollListProperties
        {
            var loc1:*=new Shared.AS3.COMPANIONAPP.MobileScrollListProperties();
            var loc2:*=arg1;
            switch (loc2) 
            {
                case STATS_SPECIAL_ENTRY_LINKAGE_ID:
                {
                    loc1.linkageId = STATS_SPECIAL_RENDERER_LINKAGE_ID;
                    loc1.maskDimension = 450;
                    loc1.scrollDirection = Mobile.ScrollList.MobileScrollList.VERTICAL;
                    loc1.spaceBetweenButtons = 0;
                    loc1.clickable = true;
                    loc1.reversed = false;
                    break;
                }
                case STATS_PERKS_ENTRY_LINKAGE_ID:
                {
                    loc1.linkageId = STATS_PERKS_RENDERER_LINKAGE_ID;
                    loc1.maskDimension = 400;
                    loc1.scrollDirection = Mobile.ScrollList.MobileScrollList.VERTICAL;
                    loc1.spaceBetweenButtons = 0;
                    loc1.clickable = true;
                    loc1.reversed = false;
                    break;
                }
                case INVENTORY_COMPONENT_ENTRY_LINKAGE_ID:
                case INVENTORY_ENTRY_LINKAGE_ID:
                {
                    loc1.linkageId = INVENTORY_RENDERER_LINKAGE_ID;
                    loc1.maskDimension = 405;
                    loc1.scrollDirection = Mobile.ScrollList.MobileScrollList.VERTICAL;
                    loc1.spaceBetweenButtons = 0;
                    loc1.clickable = true;
                    loc1.reversed = false;
                    break;
                }
                case INVENTORY_COMPONENT_OWNERS_ENTRY_LINKAGE_ID:
                {
                    loc1.linkageId = INVENTORY_COMPONENT_OWNERS_RENDERER_LINKAGE_ID;
                    loc1.maskDimension = 400;
                    loc1.scrollDirection = Mobile.ScrollList.MobileScrollList.VERTICAL;
                    loc1.spaceBetweenButtons = 0;
                    loc1.clickable = true;
                    loc1.reversed = false;
                    break;
                }
                case DATA_STATS_CATEGORIES_ENTRY_LINKAGE_ID:
                {
                    loc1.linkageId = DATA_STATS_CATEGORIES_RENDERER_LINKAGE_ID;
                    loc1.maskDimension = 400;
                    loc1.scrollDirection = Mobile.ScrollList.MobileScrollList.VERTICAL;
                    loc1.spaceBetweenButtons = 2.25;
                    loc1.clickable = true;
                    loc1.reversed = false;
                    break;
                }
                case DATA_STATS_VALUES_ENTRY_LINKAGE_ID:
                {
                    loc1.linkageId = DATA_STATS_VALUES_RENDERER_LINKAGE_ID;
                    loc1.maskDimension = 400;
                    loc1.scrollDirection = Mobile.ScrollList.MobileScrollList.VERTICAL;
                    loc1.spaceBetweenButtons = 2.75;
                    loc1.clickable = false;
                    loc1.reversed = false;
                    break;
                }
                case DATA_WORKSHOPS_ENTRY_LINKAGE_ID:
                {
                    loc1.linkageId = DATA_WORKSHOPS_RENDERER_LINKAGE_ID;
                    loc1.maskDimension = 400;
                    loc1.scrollDirection = Mobile.ScrollList.MobileScrollList.VERTICAL;
                    loc1.spaceBetweenButtons = 2.75;
                    loc1.clickable = true;
                    loc1.reversed = false;
                    break;
                }
                case QUEST_ENTRY_LINKAGE_ID:
                {
                    loc1.linkageId = QUEST_RENDERER_LINKAGE_ID;
                    loc1.maskDimension = 400;
                    loc1.scrollDirection = Mobile.ScrollList.MobileScrollList.VERTICAL;
                    loc1.spaceBetweenButtons = 1.4;
                    loc1.clickable = true;
                    loc1.reversed = false;
                    break;
                }
                case QUEST_OBJECTIVES_ENTRY_LINKAGE_ID:
                {
                    loc1.linkageId = QUEST_OBJECTIVES_RENDERER_LINKAGE_ID;
                    loc1.maskDimension = 200;
                    loc1.scrollDirection = Mobile.ScrollList.MobileScrollList.VERTICAL;
                    loc1.spaceBetweenButtons = 1.75;
                    loc1.clickable = false;
                    loc1.reversed = false;
                    break;
                }
                case RADIO_ENTRY_LINKAGE_ID:
                {
                    loc1.linkageId = RADIO_RENDERER_LINKAGE_ID;
                    loc1.maskDimension = 400;
                    loc1.scrollDirection = Mobile.ScrollList.MobileScrollList.VERTICAL;
                    loc1.spaceBetweenButtons = 1.4;
                    loc1.clickable = true;
                    loc1.reversed = false;
                    break;
                }
                case PIPBOY_MESSAGE_ENTRY_LINKAGE_ID:
                {
                    loc1.linkageId = PIPBOY_MESSAGE_RENDERER_LINKAGE_ID;
                    loc1.maskDimension = 150;
                    loc1.scrollDirection = Mobile.ScrollList.MobileScrollList.VERTICAL;
                    loc1.spaceBetweenButtons = 4;
                    loc1.clickable = true;
                    loc1.reversed = false;
                    break;
                }
                default:
                {
                    trace("Error: No mapping found between ListItemRenderer \'" + arg1 + "\' used InGame and mobile ListItemRenderer");
                    break;
                }
            }
            return loc1;
        }

        public static const STATS_SPECIAL_ENTRY_LINKAGE_ID:String="SPECIALListEntry";

        public static const STATS_PERKS_ENTRY_LINKAGE_ID:String="PerksListEntry";

        public static const INVENTORY_ENTRY_LINKAGE_ID:String="InvListEntry";

        public static const INVENTORY_COMPONENT_ENTRY_LINKAGE_ID:String="ComponentListEntry";

        public static const INVENTORY_COMPONENT_OWNERS_ENTRY_LINKAGE_ID:String="ComponentOwnersListEntry";

        public static const DATA_STATS_CATEGORIES_ENTRY_LINKAGE_ID:String="Stats_CategoriesListEntry";

        public static const DATA_STATS_VALUES_ENTRY_LINKAGE_ID:String="Stats_ValuesListEntry";

        public static const DATA_WORKSHOPS_ENTRY_LINKAGE_ID:String="WorkshopsListEntry";

        public static const QUEST_ENTRY_LINKAGE_ID:String="QuestsListEntry";

        public static const QUEST_OBJECTIVES_ENTRY_LINKAGE_ID:String="ObjectivesListEntry";

        public static const RADIO_ENTRY_LINKAGE_ID:String="RadioListEntry";

        public static const PIPBOY_MESSAGE_ENTRY_LINKAGE_ID:String="MessageBoxButtonEntry";

        public static const STATS_SPECIAL_RENDERER_LINKAGE_ID:String="SPECIALItemRendererMc";

        public static const STATS_PERKS_RENDERER_LINKAGE_ID:String="PerksItemRendererMc";

        public static const INVENTORY_RENDERER_LINKAGE_ID:String="InventoryItemRendererMc";

        public static const INVENTORY_COMPONENT_OWNERS_RENDERER_LINKAGE_ID:String="ComponentOwnersItemRendererMc";

        public static const DATA_STATS_CATEGORIES_RENDERER_LINKAGE_ID:String="StatsCategoriesItemRendererMc";

        public static const DATA_STATS_VALUES_RENDERER_LINKAGE_ID:String="StatsValuesItemRendererMc";

        public static const DATA_WORKSHOPS_RENDERER_LINKAGE_ID:String="WorkshopsItemRendererMc";

        public static const QUEST_RENDERER_LINKAGE_ID:String="QuestsItemRendererMc";

        public static const QUEST_OBJECTIVES_RENDERER_LINKAGE_ID:String="QuestsObjectivesItemRendererMc";

        public static const RADIO_RENDERER_LINKAGE_ID:String="RadioItemRendererMc";

        public static const PIPBOY_MESSAGE_RENDERER_LINKAGE_ID:String="PipboyMessageItemRenderer";
    }
}
