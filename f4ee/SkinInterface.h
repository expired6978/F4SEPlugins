#pragma once

#include "StringTable.h"
#include <unordered_map>
#include <memory>
#include <functional>

#include "f4se/GameThreads.h"

struct F4SESerializationInterface;

class Actor;
class TESNPC;
class TESObjectARMO;
class BGSTextureSet;
class BGSHeadPart;

class SkinTemplate
{
public:
	SkinTemplate() : skin(0), sort(0)
	{
		face[0] = 0;
		face[1] = 0;
		rear[0] = 0;
		rear[1] = 0;
		head[0] = 0;
		head[1] = 0;
		gender = 2;
	}

	F4EEFixedString	name;
	UInt32			face[2];
	UInt32			head[2];
	UInt32			rear[2];
	UInt32			skin;
	SInt32			sort;
	UInt8			gender;

	BGSTextureSet * GetTextureSet(bool isFemale);
	TESObjectARMO * GetSkinArmor();
	BGSHeadPart * GetHead(bool isFemale);
	BGSHeadPart * GetRearHead(bool isFemale);
};
typedef std::shared_ptr<SkinTemplate> SkinTemplatePtr;

class F4EESkinUpdate : public ITaskDelegate
{
public:
	F4EESkinUpdate(TESForm * form, bool doFace);
	virtual ~F4EESkinUpdate() { };
	virtual void Run() override;

protected:
	UInt32					m_formId;
	bool					m_doFace;
};

class SkinInterface
{
public:
	enum
	{
		kVersion1 = 1,
		kSerializationVersion = kVersion1,
	};

	virtual void Save(const F4SESerializationInterface * intfc, UInt32 kVersion);
	virtual bool Load(const F4SESerializationInterface * intfc, UInt32 kVersion, const std::unordered_map<UInt32, StringTableItem> & stringTable);
	virtual void Revert();

	virtual void LoadSkinMods();
	virtual bool LoadSkinTemplates(const std::string & filePath);

	virtual bool AddSkinOverride(Actor * actor, const F4EEFixedString & id, bool isFemale);
	virtual F4EEFixedString GetSkinOverride(Actor * actor);
	virtual SkinTemplatePtr GetSkinTemplate(Actor * actor);
	virtual bool RemoveSkinOverride(Actor * actor);
	virtual void CloneSkinOverride(Actor * source, Actor * target);
	virtual bool UpdateSkinOverride(Actor * actor, bool doFace);

	TESObjectARMO * GetBackupSkin(TESNPC * npc, bool & exists);
	BGSTextureSet * GetBackupFace(Actor * actor, TESNPC * npc, bool isFemale, bool & exists);
	BGSHeadPart * GetBackupHeadPart(Actor * actor, TESNPC * npc, bool isFemale, UInt32 partType);

	virtual void ForEachSkinTemplate(std::function<void(const F4EEFixedString&, const SkinTemplatePtr&)> functor);

	

	virtual void ClearMods();

protected:
	SafeDataHolder<std::unordered_map<UInt32, UInt32>>			m_skinBackup;		// Stores a mapping of TESNPC formid to the previous TESObjectARMO
	//SafeDataHolder<std::unordered_map<UInt32, UInt32>>			m_faceBackup[2];	// Stores a mapping of TESNPC formid to the previous BGSTextureSet
	std::unordered_map<F4EEFixedString, SkinTemplatePtr>		m_skinTemplates;	// Mapping of template id to template object
	SafeDataHolder<std::unordered_map<UInt32, StringTableItem>>	m_skinOverride;		// Maps Actor to specific Override id
};