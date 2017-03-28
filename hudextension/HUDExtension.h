#pragma once

#include <vector>
#include <memory>
#include <unordered_map>

#include "f4se/GameTypes.h"
#include "f4se/GameMenus.h"

class ActorValueInfo;

class HUDNameplate
{
public:
	HUDNameplate(UInt32 refrHandle, GFxValue * nameplate);
	virtual ~HUDNameplate();

	UInt32				m_refrHandle;
	BSGFxShaderFXTarget	* m_nameplate;
};

class HUDExtensionBase : public HUDComponentBase
{
public:
	HUDExtensionBase(GFxValue * parent, const char * componentName, HUDContextArray<BSFixedString> * contextList);
	virtual ~HUDExtensionBase();

	virtual void UpdateComponent() override;

	void AddNameplate(GFxValue * parent, const std::shared_ptr<HUDNameplate> & object);
	void UpdateNameplate(double stageWidth, double stageHeight, const std::shared_ptr<HUDNameplate> & object);
	double GetHealthPercent(TESObjectREFR * refr);
};

class HUDExtension
{
	friend class HUDExtensionBase;
public:
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
	HUDSettings() : barFlags(0), colorNonHostile(-1), colorTeammate(-1), colorLover(-1), colorEnemy(-1), avHealth(nullptr), avRomanced(nullptr), avInvisibility(nullptr), avFlags(nullptr), fMaxPercent(125.0f), fScaleFactor(20.0f) { }

	enum Flags
	{
		kFlag_HideLover		 = (1 << 0),
		kFlag_HideTeammate	 = (1 << 1),
		kFlag_HideNonHostile = (1 << 2),
		kFlag_HideEnemy		 = (1 << 3),
		kFlag_HideUserDefined = (1 << 4)
	};

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
extern HUDExtension g_hudExtension;