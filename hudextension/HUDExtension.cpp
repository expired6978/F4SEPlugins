#include "HUDExtension.h"

#include <algorithm>

#include "f4se/ScaleformLoader.h"
#include "f4se/GameReferences.h"
#include "f4se/GameObjects.h"
#include "f4se/GameData.h"

#include "f4se/NiNodes.h"

HUDSettings  g_hudSettings;

const char * HUDExtensionMenu::sMenuName = "HUDExtensionMenu";

HUDExtensionMenu::HUDExtensionMenu() : GameMenuBase()
{
	flags = kFlag_DoNotPreventGameSave | kFlag_DisableInteractive | kFlag_Unk800000;
	if (CALL_MEMBER_FN((*g_scaleformManager), LoadMovie)(this, movie, "HUDExtension", "root1", 2))
	{
		if (g_hudSettings.applyFilter)
			flags |= kFlag_ApplyDropDownFilter;
		else
			stage.SetMember("showShadowEffect", &GFxValue(true));
		CreateBaseShaderTarget(shaderTarget, stage);

		if (flags & kFlag_ApplyDropDownFilter)
		{
			subcomponents.Push(shaderTarget);
		}
	}
}

HUDExtensionMenu::~HUDExtensionMenu()
{
	ForceClear();
}

double HUDExtensionMenu::GetHealthPercent(TESObjectREFR * refr)
{
	float current = 0.0f;
	float maximum = 0.0f;

	if(g_hudSettings.avHealth)
	{
		maximum = refr->actorValueOwner.GetMaximum(g_hudSettings.avHealth);
		current = refr->actorValueOwner.GetValue(g_hudSettings.avHealth);
	}

	return max(0.0, min((current / maximum) * 100.0, 100.0));
}

void HUDExtensionMenu::PopulateNameplateData(GFxValue * args, TESObjectREFR * refr)
{
	bool showName = (g_hudSettings.barFlags & HUDSettings::kFlag_HideName) != HUDSettings::kFlag_HideName;
	args[0].SetString(showName ? CALL_MEMBER_FN(refr, GetReferenceName)() : "");
	args[1].SetNumber(GetHealthPercent(refr));

	UInt16 level = 0;
	if(refr->formType == Actor::kTypeID)
		level = CALL_MEMBER_FN(&static_cast<TESNPC*>(refr->baseForm)->actorData, GetLevel)();

	args[2].SetUInt(level);

	bool showLevel = (g_hudSettings.barFlags & HUDSettings::kFlag_ShowLevel) == HUDSettings::kFlag_ShowLevel;
	args[3].SetBool(showLevel);
}

void HUDExtensionMenu::AddNameplate(GFxValue * parent, const std::shared_ptr<HUDNameplate> & object)
{
	TESObjectREFR * refr = nullptr;
	LookupREFRByHandle(&object->m_refrHandle, &refr);
	if(refr)
	{
		if(parent->IsDisplayObject())
		{
			GFxValue args[5];
			args[0].SetInt(object->m_refrHandle);
			PopulateNameplateData(&args[1], refr);

			GFxValue nameplate;
			parent->Invoke("AddNameplate", &nameplate, args, 5);
			if(nameplate.IsDisplayObject()) {
				object->m_nameplate = new BSGFxShaderFXTarget(&nameplate);
			}
		}
		refr->handleRefObject.DecRefHandle();
	}
}

void HUDExtensionMenu::UpdateNameplate(double stageWidth, double stageHeight, const std::shared_ptr<HUDNameplate> & object)
{
	TESObjectREFR * refr = nullptr;
	LookupREFRByHandle(&object->m_refrHandle, &refr);
	if(refr)
	{
		NiPoint3 markerPos;
		refr->GetMarkerPosition(&markerPos);

		markerPos.z += 25;

		NiNode * rootNode = refr->GetObjectRootNode();
		if(rootNode) {
			// Put the bar in the center of the object, but raised to marker height
			markerPos.x = rootNode->m_worldTransform.pos.x;
			markerPos.y = rootNode->m_worldTransform.pos.y;
		}

		NiPoint3 posOut;
		WorldToScreen_Internal(&markerPos, &posOut);
		posOut.y = 1.0 - posOut.y;
		posOut.y = stageHeight * posOut.y;
		posOut.x = stageWidth * posOut.x;

		BSGFxShaderFXTarget * nameplate = object->m_nameplate;
		if(!nameplate) {
			return;
		}

		if(nameplate->IsObject()) {
			GFxValue args[5];
			PopulateNameplateData(args, refr); // Populates 0-3
			args[4].SetNumber(posOut.z); // Populates 4
			nameplate->Invoke("SetData", nullptr, args, 5);
		}

		double scale = min(((100 - posOut.z * 100) * g_hudSettings.fScaleFactor), g_hudSettings.fMaxPercent);
		if(nameplate->IsDisplayObject()) {
			GFxValue::DisplayInfo dInfo;
			nameplate->GetDisplayInfo(&dInfo);
			dInfo.SetPosition(posOut.x, posOut.y);
			dInfo.SetScale(scale, scale);
			nameplate->SetDisplayInfo(&dInfo);

			bool isHostile = false;
			bool isVisible = true;
			UInt32 clrBar = -1;

			UInt32 visibilityFlags = g_hudSettings.barFlags;
			if(g_hudSettings.avFlags) { // avFlags work opposite to bar flags
				visibilityFlags = ~UInt32((*g_player)->actorValueOwner.GetValue(g_hudSettings.avFlags));
			}

			if (refr->formType == Actor::kTypeID) // Type check is faster than RTTI cast
			{
				Actor * actor = (Actor*)refr;
				isHostile = CALL_MEMBER_FN((*g_player), IsHostileToActor)(actor);

				if(g_hudSettings.avInvisibility && actor->actorValueOwner.GetValue(g_hudSettings.avInvisibility) > 0.0f) {
					isVisible = false; // Actor is invisible, NO CHEATING!
				}

				if(isHostile) {
					clrBar = g_hudSettings.colorEnemy;
					isVisible &= !((visibilityFlags & HUDSettings::kFlag_HideEnemy) ==  HUDSettings::kFlag_HideEnemy);
				} else if(g_hudSettings.avUserDefined && actor->actorValueOwner.GetValue(g_hudSettings.avUserDefined) > 0.0f) {
					clrBar = (UInt32)actor->actorValueOwner.GetValue(g_hudSettings.avUserDefined);
					isVisible &= !((visibilityFlags & HUDSettings::kFlag_HideUserDefined) ==  HUDSettings::kFlag_HideUserDefined);
				} else if(g_hudSettings.avRomanced && actor->actorValueOwner.GetValue(g_hudSettings.avRomanced) >= 1.0f) {
					clrBar = g_hudSettings.colorLover;
					isVisible &= !((visibilityFlags & HUDSettings::kFlag_HideLover) ==  HUDSettings::kFlag_HideLover);
				} else if(actor->IsPlayerTeammate()) {
					clrBar = g_hudSettings.colorTeammate;
					isVisible &= !((visibilityFlags & HUDSettings::kFlag_HideTeammate) ==  HUDSettings::kFlag_HideTeammate);
				} else if(!isHostile) {
					clrBar = g_hudSettings.colorNonHostile;
					isVisible &= !((visibilityFlags & HUDSettings::kFlag_HideNonHostile) ==  HUDSettings::kFlag_HideNonHostile);
				}
			} else if(g_hudSettings.avUserDefined && refr->actorValueOwner.GetValue(g_hudSettings.avUserDefined) > 0.0f) {  // Non-actors cant have bars yet, but we will account for it
				clrBar = (UInt32)refr->actorValueOwner.GetValue(g_hudSettings.avUserDefined);
				isVisible &= !((visibilityFlags & HUDSettings::kFlag_HideUserDefined) ==  HUDSettings::kFlag_HideUserDefined);
			} else {
				clrBar = g_hudSettings.colorNonHostile;
				isVisible &= !((visibilityFlags & HUDSettings::kFlag_HideNonHostile) ==  HUDSettings::kFlag_HideNonHostile);
			}

			UInt8 unk1 = 0;
			bool hasLOS = isVisible && (*g_player) != refr ? HasDetectionLOS((*g_player), refr, &unk1) : true;

			isVisible &= hasLOS;
			if (isVisible)
			{
				//BSGFxShaderFXTarget target(nameplate);
				// Default color processing
				if(clrBar == -1)
					nameplate->SetFilterColor(isHostile);
				else
				{
					FilterColor color; // RRGGBB
					color.r = float((clrBar >> 16) & 0xFF) / 255.0f;
					color.g = float((clrBar >> 8) & 0xFF) / 255.0f;
					color.b = float(clrBar & 0xFF) / 255.0f;
					ApplyColorFilter(nameplate, &color, 1.0f);
				}
			}

			nameplate->SetMember("visible", &GFxValue(isVisible)); // Visibility flag doesnt seem to work from DisplayInfo...?
		}

		refr->handleRefObject.DecRefHandle();
	}
}

void HUDExtensionMenu::RegisterFunctions()
{
	
}

void HUDExtensionMenu::DrawNextFrame(float unk0, void * unk1)
{
	if(!stage.IsDisplayObject())
		return;

	GFxValue stageWidth, stageHeight;
	stage.Invoke("GetStageWidth", &stageWidth, nullptr, 0);
	stage.Invoke("GetStageHeight", &stageHeight, nullptr, 0);

	// Deal with all pending removes first
	m_lock.Lock();
	for(auto & removals : m_qRemove)
	{
		GFxValue success;
		GFxValue args[1];
		args[0].SetInt(removals.first);
		stage.Invoke("RemoveNameplate", &success, args, 1);
		auto it = m_mapPlates.find(removals.first);
		if(it != m_mapPlates.end())
		{
			if(removals.second)
			{
				BSGFxDisplayObject * target = removals.second->m_nameplate;
				SInt64 i = subcomponents.GetItemIndex(target);
				if(i != -1) {
					subcomponents.Remove(i);
				}
			}
			m_mapPlates.erase(it);
		}
	}
	m_qRemove.clear();

	for(auto & adds : m_qAdd)
	{
		AddNameplate(&stage, adds.second);
		m_mapPlates.emplace(adds.first, adds.second);
		if(adds.second) {
			subcomponents.Push(adds.second->m_nameplate);
		}
	}
	m_qAdd.clear();

	for(auto & nameplate : m_mapPlates)
	{
		if(nameplate.second)
		{
			UpdateNameplate(stageWidth.GetNumber(), stageHeight.GetNumber(), nameplate.second);
		}
	}

	m_lock.Release();

	stage.Invoke("SortChildrenByDepth", nullptr, nullptr, 0);
	__super::DrawNextFrame(unk0, unk1);
}

HUDNameplate::HUDNameplate(UInt32 refrHandle, GFxValue * nameplate) : m_refrHandle(refrHandle), m_nameplate(nullptr)
{
	
}

HUDNameplate::~HUDNameplate()
{
	if(m_nameplate)
	{
		delete m_nameplate;
	}
}

bool HUDExtensionMenu::AddNameplate(TESObjectREFR * refr)
{
	SimpleLocker locker(&m_lock);

	UInt32 refrHandle = refr->CreateRefHandle();
	if(refrHandle == (*g_invalidRefHandle)) {
		_MESSAGE("Invalid handle on add");
		return false;
	}

	auto adding = m_qAdd.find(refrHandle);
	if(adding != m_qAdd.end())
	{
		return false;
	}

	auto existing = m_mapPlates.find(refrHandle);
	if(existing != m_mapPlates.end())
	{
		return false;
	}

	m_qAdd.emplace(refrHandle, std::make_shared<HUDNameplate>(refrHandle, nullptr));
	return true;
}

bool HUDExtensionMenu::RemoveNameplate(TESObjectREFR * refr)
{
	SimpleLocker locker(&m_lock);

	UInt32 refrHandle = refr->CreateRefHandle();
	if(refrHandle == (*g_invalidRefHandle)) {
		_MESSAGE("Invalid handle on remove");
		return false;
	}

	// We are mid-add, just remove it from the queue
	auto adding = m_qAdd.find(refrHandle);
	if(adding != m_qAdd.end())
	{
		m_qAdd.erase(adding);
		return true;
	}

	// Move it from the handled map to the remove queue
	auto it = m_mapPlates.find(refrHandle);
	if(it != m_mapPlates.end())
	{
		m_qRemove.emplace(it->first, it->second);
		return true;
	}

	return false;
}

void HUDExtensionMenu::ClearNameplates()
{
	SimpleLocker locker(&m_lock);
	m_qAdd.clear();

	// Move everything from the nameplate map to the remove queue
	for(auto & it : m_mapPlates)
	{
		m_qRemove.emplace(it.first, it.second);
	}
}

void HUDExtensionMenu::ForceClear()
{
	SimpleLocker locker(&m_lock);
	m_qAdd.clear();
	m_qRemove.clear();

	for(auto & plate : m_mapPlates)
	{
		if(plate.second)
		{
			BSGFxDisplayObject * target = plate.second->m_nameplate;
			SInt64 i = subcomponents.GetItemIndex(target);
			if(i != -1) {
				subcomponents.Remove(i);
			}
		}
	}

	m_mapPlates.clear();
}