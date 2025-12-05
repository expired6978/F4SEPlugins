#pragma once

#include <vector>
#include <memory>
#include <unordered_map>

#include "f4se/GameTypes.h"
#include "f4se/GameMenus.h"
#include "f4se/ScaleformAPI.h"

class ActorValueInfo;

class HUDNameplate
{
public:
	HUDNameplate(UInt32 refrHandle, GFxValue * nameplate);
	virtual ~HUDNameplate();

	UInt32				m_refrHandle;
	BSGFxShaderFXTarget	* m_nameplate;
};

class HUDExtensionMenu : public GameMenuBase
{
public:
	HUDExtensionMenu();
	virtual ~HUDExtensionMenu();

	static const char * sMenuName;

	static void OpenMenu()
	{
		CALL_MEMBER_FN((*g_uiMessageManager), SendUIMessage)(BSFixedString(sMenuName), kMessage_Open);
	}

	static void CloseMenu()
	{
		CALL_MEMBER_FN((*g_uiMessageManager), SendUIMessage)(BSFixedString(sMenuName), kMessage_Close);
	}

	virtual void AdvanceMovie(float unk0, void * unk1) override final;
	virtual void RegisterFunctions() override final;

	void AddNameplate(GFxValue * parent, const std::shared_ptr<HUDNameplate> & object);
	void UpdateNameplate(double stageWidth, double stageHeight, const std::shared_ptr<HUDNameplate> & object);
	double GetHealthPercent(TESObjectREFR * refr);
	void PopulateNameplateData(GFxValue * args, TESObjectREFR * refr);

	DEFINE_STATIC_HEAP(ScaleformHeap_Allocate, ScaleformHeap_Free)

	virtual bool AddNameplate(TESObjectREFR * refr);
	virtual bool RemoveNameplate(TESObjectREFR * refr);

	virtual void ClearNameplates();
	virtual void ForceClear();

protected:
	SimpleLock m_lock;
	std::unordered_map<UInt32, std::shared_ptr<HUDNameplate>> m_qAdd;
	std::unordered_map<UInt32, std::shared_ptr<HUDNameplate>> m_qRemove;
	std::unordered_map<UInt32, std::shared_ptr<HUDNameplate>> m_mapPlates;
};

struct HUDSettings
{
	HUDSettings() : applyFilter(true), barFlags(0), colorNonHostile(-1), colorTeammate(-1), colorLover(-1), colorEnemy(-1), avHealth(nullptr), avRomanced(nullptr), avInvisibility(nullptr), avFlags(nullptr), fMaxPercent(125.0f), fScaleFactor(20.0f) { }

	enum Flags
	{
		kFlag_HideLover		  = (1 << 0),
		kFlag_HideTeammate	  = (1 << 1),
		kFlag_HideNonHostile  = (1 << 2),
		kFlag_HideEnemy		  = (1 << 3),
		kFlag_HideUserDefined = (1 << 4),
		kFlag_ShowLevel		  = (1 << 5),
		kFlag_HideName        = (1 << 6)
	};

	bool applyFilter;
	UInt64 barFlags;
	UInt32 colorLover;
	UInt32 colorTeammate;
	UInt32 colorNonHostile;
	UInt32 colorEnemy;
	float fScaleFactor;
	float fMaxPercent;

	ActorValueInfo * avHealth;
	ActorValueInfo * avRomanced;
	ActorValueInfo * avInvisibility;
	ActorValueInfo * avUserDefined;
	ActorValueInfo * avFlags;
};

extern HUDSettings  g_hudSettings;