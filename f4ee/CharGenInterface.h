#pragma once

#include "f4se/GameCustomization.h"

#include <unordered_map>
#include <functional>
#include <unordered_set>
#include <mutex>

#include "json/json.h"

#include "f4se/GameObjects.h"
#include "f4se/BSModelDB.h"

#include "StringTable.h"

class Actor;
class TESRace;
class BSLightingShaderMaterialBase;
struct ModInfo;

class CharGenInterface
{
public:
	CharGenInterface() { }

	virtual DWORD SavePreset(const std::string & filePath);
	virtual DWORD LoadPreset(const std::string & filePath);

	virtual void LoadTintTemplateMods();

	virtual bool LoadTintCategories(const std::string & filePath);
	virtual bool LoadTintTemplates(const std::string & filePath);

	virtual bool SaveTintCategories(const TESRace * race, const std::string & filePath);
	virtual bool SaveTintTemplates(const TESRace * race, const std::string & filePath);

	virtual void LoadHairColorMods();
	virtual bool LoadHairColorData(const std::string & filePath, ModInfo * modInfo);

	virtual void UnlockHeadParts();
	virtual void UnlockTints();

	virtual void ProcessHairColor(NiAVObject * node, BGSColorForm * colorForm, BSLightingShaderMaterialBase * shaderMaterial);
	virtual const char * ProcessEyebrowPath(TESNPC * npc);
	/*virtual TESNPC::HeadData * ProcessHeadData(TESNPC * npc);

	virtual void SetBaseTintTextureOverride(BGSTextureSet * textureSet);
	void LockBaseTextureOverride();
	void ReleaseBaseTextureOverride();*/

	bool IsLUTUsed(const F4EEFixedString & str);
	bool GetLUTFromColor(BGSColorForm * color, F4EEFixedString & str);

protected:
	std::unordered_set<F4EEFixedString, std::hash<F4EEFixedString>> m_LUTs;
	std::unordered_map<BGSColorForm*, F4EEFixedString> m_LUTMap;

	//TESNPC::HeadData m_faceTextureOverride;
	//std::mutex m_faceTextureLock;

	Actor * GetCurrentActor();
};