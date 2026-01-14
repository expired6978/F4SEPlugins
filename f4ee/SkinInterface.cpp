#include "SkinInterface.h"

#include "Utilities.h"

#include "json/json.h"
#include "common/IDirectoryIterator.h"
#include <set>

#include "f4se/GameData.h"
#include "f4se/GameForms.h"
#include "f4se/GameRTTI.h"
#include "f4se/GameStreams.h"
#include "f4se/GameReferences.h"

#include "ActorUpdateManager.h"

extern SkinInterface g_skinInterface;
extern ActorUpdateManager g_actorUpdateManager;
extern StringTable g_stringTable;

extern F4SETaskInterface			* g_task;

F4EESkinUpdate::F4EESkinUpdate(TESForm * form, bool doFace)
{
	m_formId = form->formID;
	m_doFace = doFace;
}

void F4EESkinUpdate::Run()
{
	TESForm * form = LookupFormByID(m_formId);
	if(!form)
		return;

	Actor * actor = DYNAMIC_CAST(form, TESForm, Actor);
	if(!actor)
		return;

	TESNPC * npc =  DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
	if(!npc)
		return;

	UInt32 updateFlags = g_skinInterface.ApplyOverride(actor, npc, m_doFace);
	if (updateFlags != 0)
	{
		auto middleProcess = actor->middleProcess;
		if (middleProcess) {
			middleProcess->Set3DUpdateFlag(updateFlags);
			middleProcess->Update3DModel(actor, true);
		}
	}
}

// Should already be guaranteed to be TXST or ARMO if it's non-zero
BGSTextureSet * SkinTemplate::GetTextureSet(bool isFemale)
{
	return face[isFemale ? 1 : 0] != 0 ? (BGSTextureSet*)LookupFormByID(face[isFemale ? 1 : 0]) : nullptr;
}

TESObjectARMO * SkinTemplate::GetSkinArmor()
{
	return skin != 0 ? (TESObjectARMO*)LookupFormByID(skin) : nullptr;
}

BGSHeadPart * SkinTemplate::GetRearHead(bool isFemale)
{
	return rear[isFemale ? 1 : 0] != 0 ? (BGSHeadPart*)LookupFormByID(rear[isFemale ? 1 : 0]) : nullptr;
}

BGSHeadPart * SkinTemplate::GetHead(bool isFemale)
{
	return head[isFemale ? 1 : 0] != 0 ? (BGSHeadPart*)LookupFormByID(head[isFemale ? 1 : 0]) : nullptr;
}

bool SkinInterface::AddSkinOverride(Actor * actor, const F4EEFixedString & id, bool isFemale)
{
	if(!actor)
		return false;

	TESNPC * npc =  DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
	if(!npc)
		return false;

	auto it = m_skinTemplates.find(id);
	if(it != m_skinTemplates.end()) {
		m_skinOverride.Lock();

		// We do not have a backup yet, lets use what we have
		// Store the skin armor backup
		m_skinBackup.Lock();
		auto bit = m_skinBackup.m_data.find(actor->formID);
		if(bit == m_skinBackup.m_data.end()) {
			m_skinBackup.m_data.emplace(npc->formID, npc->skinForm.skin ? npc->skinForm.skin->formID : 0);
		}
		m_skinBackup.Release();

		// We do not have a backup yet, lets use what we have
		// Store the face texture backup
		/*UInt8 gender = isFemale ? 1 : 0;
		m_faceBackup[gender].Lock();
		auto fit = m_faceBackup[gender].m_data.find(npc->formID);
		if(fit == m_faceBackup[gender].m_data.end()) {
			m_faceBackup[gender].m_data.emplace(npc->formID, npc->headData && npc->headData->faceTextures ? npc->headData->faceTextures->formID : 0);
		}
		m_faceBackup[gender].Release();*/

		// Store the override
		m_skinOverride.m_data[actor->formID] = g_stringTable.GetString(id);
		m_skinOverride.Release();
		return true;
	}

	return false;
}

TESObjectARMO * SkinInterface::GetBackupSkin(TESNPC * npc, bool & exists)
{
	TESObjectARMO * skinArmor = nullptr;
	if(npc) {
		// Revert to the backup if we remove the override
		m_skinBackup.Lock();
		auto bit = m_skinBackup.m_data.find(npc->formID);
		if(bit != m_skinBackup.m_data.end()) {
			skinArmor = bit->second != 0 ? (TESObjectARMO*)LookupFormByID(bit->second) : nullptr;
			exists = true;
		} else {
			exists = false;
		}
		m_skinBackup.Release();
	}

	return skinArmor;
}

BGSTextureSet * SkinInterface::GetBackupFace(Actor * actor, TESNPC * npc, bool isFemale, bool & exists)
{
	BGSTextureSet * faceTexture = nullptr;
	if(npc) {
		// Revert to the backup if we remove the override
		UInt8 gender = isFemale ? 1 : 0;

		BGSHeadPart * facePart = npc->GetHeadPartByType(BGSHeadPart::kTypeFace);
		if(facePart) {
			if(!facePart->textureSet) {
				facePart = GetBackupHeadPart(actor, npc, isFemale, BGSHeadPart::kTypeFace);
			}

			if(facePart)
				return facePart->textureSet;
		}

		/*m_faceBackup[gender].Lock();
		auto bit = m_faceBackup[gender].m_data.find(npc->formID);
		if(bit != m_faceBackup[gender].m_data.end()) {
			faceTexture = bit->second != 0 ? (BGSTextureSet*)LookupFormByID(bit->second) : nullptr;
			exists = true;
		} else {
			exists = false;
		}
		m_faceBackup[gender].Release();*/
	}

	return faceTexture;
}

BGSHeadPart * SkinInterface::GetBackupHeadPart(Actor * actor, TESNPC * npc, bool isFemale, UInt32 partType)
{
	TESRace * race = actor->race;
	if(!race)
		race = npc->race.race;

	if(!race)
		return nullptr;

	auto chargenData = race->chargenData[isFemale ? 1 : 0];
	if(!chargenData)
		return nullptr;

	auto headParts = chargenData->headParts;
	if(!headParts)
		return nullptr;

	for(UInt32 i = 0; i < headParts->count; i++)
	{
		BGSHeadPart * headPart;
		headParts->GetNthItem(i, headPart);
		if(headPart->type == partType)
			return headPart;
	}

	return nullptr;
}

bool SkinInterface::RemoveSkinOverride(Actor * actor)
{
	if(actor) {
		m_skinOverride.Lock();
		auto sit = m_skinOverride.m_data.find(actor->formID);
		if(sit != m_skinOverride.m_data.end())
		{
			m_skinOverride.m_data.erase(sit);
			m_skinOverride.Release();
			return true;
		}
		m_skinOverride.Release();
	}

	return false;
}

F4EEFixedString SkinInterface::GetSkinOverride(Actor * actor)
{
	if(actor) {
		m_skinOverride.Lock();
		auto sit = m_skinOverride.m_data.find(actor->formID);
		if(sit != m_skinOverride.m_data.end())
		{
			F4EEFixedString id = *sit->second;
			m_skinOverride.Release();
			return id;
		}

		m_skinOverride.Release();
	}

	return "";
}

SkinTemplatePtr SkinInterface::GetSkinTemplate(Actor * actor)
{
	if(actor) {
		m_skinOverride.Lock();
		auto sit = m_skinOverride.m_data.find(actor->formID);
		if(sit != m_skinOverride.m_data.end())
		{
			auto it = m_skinTemplates.find(*sit->second);
			if(it != m_skinTemplates.end()) {
				auto skinTemplate = it->second;
				m_skinOverride.Release();
				return skinTemplate;
			}
		}

		m_skinOverride.Release();
	}

	return nullptr;
}

UInt32 SkinInterface::ApplyOverride(Actor* actor, TESNPC* npc, bool doFace)
{
	UInt64 gender = CALL_MEMBER_FN(npc, GetSex)();
	bool isFemale = gender == 1 ? true : false;

	BGSTextureSet* faceTexture = nullptr;
	TESObjectARMO* skinArmor = nullptr;
	BGSHeadPart* headRearPart = nullptr;
	BGSHeadPart* headPart = nullptr;

	SkinTemplatePtr pTemplate = GetSkinTemplate(actor);
	if (pTemplate) {
		faceTexture = pTemplate->GetTextureSet(isFemale);
		skinArmor = pTemplate->GetSkinArmor();
		headPart = pTemplate->GetHead(isFemale);
		headRearPart = pTemplate->GetRearHead(isFemale);
	}

	// We have a new texture to assign, create head data if we don't have one
	if (doFace && faceTexture) {
		if (!npc->headData)
			npc->headData = new TESNPC::HeadData();
	}

	bool faceBackupExists = false;
	if (doFace) {
		if (!faceTexture) // Couldn't find an override, choose the backup
			faceTexture = GetBackupFace(actor, npc, isFemale, faceBackupExists);
		//if(!faceTexture && !faceBackupExists) // There was no backup, lets take what the NPC has
		//	faceTexture = npc->headData ? npc->headData->faceTextures : nullptr;
	}

	bool skinBackupExists = false;
	if (!skinArmor) // Couldn't find an override, choose the backup
		skinArmor = GetBackupSkin(npc, skinBackupExists);
	if (!skinArmor && !skinBackupExists) // There was no backup, lets take what the NPC has
		skinArmor = npc->skinForm.skin;

	bool doHeadUpdate = false;
	if (doFace) {
		// Change the face part
		BGSHeadPart* pCurrentHead = npc->GetHeadPartByType(BGSHeadPart::kTypeFace);
		if (headPart && pCurrentHead && headPart != pCurrentHead) {
			npc->ChangeHeadPart(headPart, false, false);
			doHeadUpdate = true;
			npc->MarkChanged(0x800); // Save FaceData
		}
		// We changed the head rear HeadPart, we need to invoke an update
		BGSHeadPart* pCurrentHeadRear = npc->GetHeadPartByType(BGSHeadPart::kTypeHeadRear);
		if (headRearPart && pCurrentHeadRear && headRearPart != pCurrentHeadRear) {
			npc->ChangeHeadPart(headRearPart, false, false);
			doHeadUpdate = true;
			npc->MarkChanged(0x800); // Save FaceData
		}
	}

	// We changed base face textures, clear our morph groups and do a facegen update
	bool doFaceUpdate = doFace && npc->headData && npc->headData->faceTextures != faceTexture;
	if (doFaceUpdate) {
		npc->headData->faceTextures = faceTexture;
		auto morphData = npc->morphSetData;
		if (morphData)
			morphData->Clear();
		npc->MarkChanged(0x800);
	}

	// We changed the skin, assign the new skin and perform an update
	bool doSkinUpdate = npc->skinForm.skin != skinArmor;
	if (doSkinUpdate) {
		npc->skinForm.skin = skinArmor;
	}

	UInt32 updateFlags = 0;
	if (doSkinUpdate)
		updateFlags |= Actor::AIProcess::RESET_SKIN | Actor::AIProcess::RESET_MODEL;
	if (doFaceUpdate)
		updateFlags |= Actor::AIProcess::RESET_FACE;
	if (doHeadUpdate)
		updateFlags |= Actor::AIProcess::RESET_HEAD;
	else if (doSkinUpdate || doFaceUpdate)
		updateFlags |= Actor::AIProcess::RESET_KEEP_HEAD;
	return updateFlags;
}

void SkinInterface::RevertOverride(Actor* actor, TESNPC* npc)
{
	UInt64 gender = CALL_MEMBER_FN(npc, GetSex)();
	bool isFemale = gender == 1 ? true : false;

	bool faceBackupExists = false;
	bool skinBackupExists = false;
	auto faceBackup = GetBackupFace(actor, npc, isFemale, faceBackupExists);

	if(npc->headData)
		npc->headData->faceTextures = faceBackup; // Backup, or null

	auto skinBackup = GetBackupSkin(npc, skinBackupExists);
	if (skinBackupExists) // Backup could be null, in which case use the Race
	{
		if (npc->skinForm.skin != skinBackup)
			npc->skinForm.skin = skinBackup;
	}
}

void SkinInterface::Save(const F4SESerializationInterface * intfc, UInt32 kVersion)
{
	m_skinOverride.Lock();
	intfc->OpenRecord('SOVR', kVersion);

	// Key
	UInt32 length = m_skinOverride.m_data.size();
	Serialization::WriteData<UInt32>(intfc, &length);

	for(auto & ovr : m_skinOverride.m_data)
	{
		// Key
		Serialization::WriteData<UInt32>(intfc, &ovr.first);

		// Value
		UInt32 stringId = g_stringTable.GetStringID(ovr.second);
		Serialization::WriteData<UInt32>(intfc, &stringId);
	}

	m_skinOverride.Release();
}

bool SkinInterface::Load(const F4SESerializationInterface * intfc, UInt32 kVersion, const std::unordered_map<UInt32, StringTableItem> & stringTable)
{
	UInt32 overrides = 0;
	if (!Serialization::ReadData<UInt32>(intfc, &overrides))
	{
		_ERROR("%s - Error loading male overlay count", __FUNCTION__);
		return false;
	}

	for(UInt32 i = 0; i < overrides; i++)
	{
		// Key
		UInt32 formId = 0;
		UInt32 newFormId = 0;
		if (!Serialization::ReadData<UInt32>(intfc, &formId))
		{
			_ERROR("%s - Error loading actor formId", __FUNCTION__);
			return false;
		}

		UInt32 stringId;
		if (!Serialization::ReadData<UInt32>(intfc, &stringId))
		{
			_ERROR("%s - Error loading skin override string id", __FUNCTION__);
			return false;
		}

		auto it = stringTable.find(stringId);
		if(it == stringTable.end())
		{
			_ERROR("%s - Error loading skin override string from table", __FUNCTION__);
			continue;
		}

		if(!intfc->ResolveFormId(formId, &newFormId))
			continue;

		Actor * actor = DYNAMIC_CAST(LookupFormByID(newFormId), TESForm, Actor);
		if(!actor)
			continue;

		TESNPC * npc =  DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
		if(!npc)
			continue;

		UInt64 gender = CALL_MEMBER_FN(npc, GetSex)();
		bool isFemale = gender == 1 ? true : false;

		g_skinInterface.AddSkinOverride(actor, *it->second, isFemale);
		g_actorUpdateManager.PushUpdate(actor);
	}

	return true;
}

void SkinInterface::Revert()
{
	for(UInt32 g = 0; g <= 1; g++)
	{
		m_skinBackup.Lock();
		for(auto & skin : m_skinBackup.m_data)
		{
			TESNPC * npc = (TESNPC*)LookupFormByID(skin.first);
			TESObjectARMO * skinArmor = skin.second != 0 ? (TESObjectARMO*)LookupFormByID(skin.second) : nullptr;
			if(npc) {
				npc->skinForm.skin = skinArmor;
			}
		}
		m_skinBackup.m_data.clear();
		m_skinBackup.Release();

		/*m_faceBackup[g].Lock();
		for(auto & face : m_faceBackup[g].m_data)
		{
			TESNPC * npc = (TESNPC*)LookupFormByID(face.first);
			BGSTextureSet * faceTexture = face.second != 0 ? (BGSTextureSet*)LookupFormByID(face.second) : nullptr;
			if(npc && npc->headData) {
				npc->headData->faceTextures = faceTexture;
			}
		}
		m_faceBackup[g].m_data.clear();
		m_faceBackup[g].Release();*/
	}
}

void SkinInterface::LoadSkinMods()
{
	std::string overlayPath("F4SE\\Plugins\\F4EE\\Skin\\");

	// Load templates
	ForEachMod([&](const ModInfo * modInfo)
	{
		std::string templatesPath = overlayPath + std::string(modInfo->name) + "\\skin.json";
		LoadSkinTemplates(templatesPath.c_str());
	});

	std::string loosePath("Data\\");
	loosePath += overlayPath;
	loosePath += "Loose";

	std::set<std::string> templates;
	for(IDirectoryIterator iter(loosePath.c_str(), "*.json"); !iter.Done(); iter.Next())
	{
		std::string	path = iter.GetFullPath();
		std::transform(path.begin(), path.begin(), path.end(), ::tolower);
		templates.insert(path);
	}

	for(auto & path : templates)
	{
		LoadSkinTemplates(path);
	}
}

bool SkinInterface::LoadSkinTemplates(const std::string & filePath)
{
	BSResourceNiBinaryStream binaryStream(filePath.c_str());
	if(!binaryStream.IsValid())
		return false;

	std::string strFile;
	BSReadAll(&binaryStream, &strFile);

	Json::Reader reader;
	Json::Value root;

	UInt32 loadedTemplates = 0;

	if(!reader.parse(strFile, root)) {
		_ERROR("%s - Failed to skin template file\t[%s]", __FUNCTION__, filePath.c_str());
		return false;
	}

	// traverse instances
	for(auto & item : root)
	{
		try
		{
			SkinTemplatePtr pTemplate = nullptr;

			F4EEFixedString id = item["id"].asCString();

			auto idIter = m_skinTemplates.find(id);
			if(idIter != m_skinTemplates.end())
				pTemplate = idIter->second;
			else {
				pTemplate = std::make_shared<SkinTemplate>();
				m_skinTemplates.emplace(id, pTemplate);
				loadedTemplates++;
			}

			if(item.isMember("name"))
				pTemplate->name = item["name"].asCString();

			if(item.isMember("gender"))
				pTemplate->gender = item["gender"].asUInt();

			if(item.isMember("maleFace") && !item["maleFace"].isNull()) {
				TESForm * form = GetFormFromIdentifier(item["maleFace"].asString());
				if(form) {
					BGSTextureSet * textureSet = DYNAMIC_CAST(form, TESForm, BGSTextureSet);
					if(textureSet)
						pTemplate->face[0] = form->formID;
					else
						_ERROR("%s - invalid form type for %s must be TextureSet (PluginFile|FORMID)", __FUNCTION__, id.c_str());
				}
			}

			if(item.isMember("femaleFace") && !item["femaleFace"].isNull()) {
				TESForm * form = GetFormFromIdentifier(item["femaleFace"].asString());
				if(form) {
					BGSTextureSet * textureSet = DYNAMIC_CAST(form, TESForm, BGSTextureSet);
					if(textureSet)
						pTemplate->face[1] = form->formID;
					else
						_ERROR("%s - invalid form type for %s must be TextureSet (PluginFile|FORMID)", __FUNCTION__, id.c_str());
				}
			}

			if(item.isMember("maleHead") && !item["maleHead"].isNull()) {
				TESForm * form = GetFormFromIdentifier(item["maleHead"].asString());
				if(form) {
					BGSHeadPart * headPart = DYNAMIC_CAST(form, TESForm, BGSHeadPart);
					if(headPart)
						pTemplate->head[0] = form->formID;
					else
						_ERROR("%s - invalid form type for %s must be HeadPart (PluginFile|FORMID)", __FUNCTION__, id.c_str());
				}
			}

			if(item.isMember("femaleHead") && !item["femaleHead"].isNull()) {
				TESForm * form = GetFormFromIdentifier(item["femaleHead"].asString());
				if(form) {
					BGSHeadPart * headPart = DYNAMIC_CAST(form, TESForm, BGSHeadPart);
					if(headPart)
						pTemplate->head[1] = form->formID;
					else
						_ERROR("%s - invalid form type for %s must be HeadPart (PluginFile|FORMID)", __FUNCTION__, id.c_str());
				}
			}

			if(item.isMember("maleHeadRear") && !item["maleHeadRear"].isNull()) {
				TESForm * form = GetFormFromIdentifier(item["maleHeadRear"].asString());
				if(form) {
					BGSHeadPart * headPart = DYNAMIC_CAST(form, TESForm, BGSHeadPart);
					if(headPart)
						pTemplate->rear[0] = form->formID;
					else
						_ERROR("%s - invalid form type for %s must be HeadPart (PluginFile|FORMID)", __FUNCTION__, id.c_str());
				}
			}

			if(item.isMember("femaleHeadRear") && !item["femaleHeadRear"].isNull()) {
				TESForm * form = GetFormFromIdentifier(item["femaleHeadRear"].asString());
				if(form) {
					BGSHeadPart * headPart = DYNAMIC_CAST(form, TESForm, BGSHeadPart);
					if(headPart)
						pTemplate->rear[1] = form->formID;
					else
						_ERROR("%s - invalid form type for %s must be HeadPart (PluginFile|FORMID)", __FUNCTION__, id.c_str());
				}
			}

			if(item.isMember("skin") && !item["skin"].isNull()) {
				TESForm * form = GetFormFromIdentifier(item["skin"].asString());
				if(form) {
					TESObjectARMO * skin = DYNAMIC_CAST(form, TESForm, TESObjectARMO);
					if(skin)
						pTemplate->skin = form->formID;
					else
						_ERROR("%s - invalid form type for %s must be Armor (PluginFile|FORMID)", __FUNCTION__, id.c_str());
				}
			}

			if(item.isMember("sort"))
				pTemplate->sort = item["sort"].asInt();
		}
		catch(const std::exception& e)
		{
			_ERROR(e.what());
		}
	}

	_MESSAGE("%s - Info - Loaded %d skin template(s).\t[%s]", __FUNCTION__, loadedTemplates, filePath.c_str());
	return true;
}

void SkinInterface::ClearMods()
{
	m_skinTemplates.clear();
}

bool SkinInterface::UpdateSkinOverride(Actor * actor, bool doFace)
{
	if(!actor)
		return false;

	if(g_task)
		g_task->AddTask(new F4EESkinUpdate(actor, doFace));

	return true;
}

void SkinInterface::ForEachSkinTemplate(std::function<void(const F4EEFixedString&, const SkinTemplatePtr&)> functor)
{
	for(auto & skinIt : m_skinTemplates)
	{
		functor(skinIt.first, skinIt.second);
	}
}

void SkinInterface::CloneSkinOverride(Actor * source, Actor * target)
{
	if(!source || !target)
		return;

	m_skinOverride.Lock();
	auto it = m_skinOverride.m_data.find(source->formID);
	if(it != m_skinOverride.m_data.end()) {
		m_skinOverride.m_data[target->formID] = it->second;
	}
	m_skinOverride.Release();
}