#include "OverlayInterface.h"

#include "f4se/NiNodes.h"
#include "f4se/GameReferences.h"

#include "f4se/GameData.h"
#include "f4se/GameObjects.h"
#include "f4se/GameRTTI.h"

#include "f4se/GameStreams.h"

#include "f4se/BSGeometry.h"
#include "f4se/NiExtraData.h"
#include "f4se/NiProperties.h"
#include "f4se/NiMaterials.h"
#include "f4se/NiCloningProcess.h"
#include "f4se/NiRTTI.h"

#include "f4se/PapyrusUtilities.h"

#include "json\json.h"
#include "Utilities.h"

#include <stdio.h>
#include <set>
#include <unordered_map>

extern OverlayInterface g_overlayInterface;
extern StringTable g_stringTable;
extern bool g_bEnableOverlays;
extern bool g_bEnableBodyMorphs;
extern F4SETaskInterface * g_task;

NiNode * OverlayInterface::GetOverlayRoot(Actor * actor, NiNode * rootNode, bool createIfNecessary)
{
	BSFixedString overlayName("[Overlays]");
	NiAVObject * overlayNode = rootNode->GetObjectByName(&overlayName);
	if(!overlayNode && createIfNecessary) {
		overlayNode = NiNode::Create(0);
		overlayNode->m_name = "[Overlays]";
		rootNode->AttachChild(overlayNode, false);
	}

	return overlayNode->GetAsNiNode();
}

void OverlayInterface::DestroyOverlaySlot(Actor * actor, NiNode * overlayHolder, UInt32 slotIndex)
{
	std::set<NiAVObject*> nodesToDelete;
	for(UInt32 i = 0; i < overlayHolder->m_children.m_emptyRunStart; i++)
	{
		UInt32 targetSlot = 0;
		NiAVObject * childObject = overlayHolder->m_children.m_data[i];
		if(childObject) {
			if(sscanf_s(childObject->m_name.c_str(), "[%d][", &targetSlot) && targetSlot == slotIndex)
				nodesToDelete.insert(childObject);
		}
	}

	for(auto & node : nodesToDelete)
		overlayHolder->Remove(node);
}

bool OverlayInterface::UpdateOverlays(Actor * actor, NiNode * rootNode, NiAVObject * object, UInt32 slotIndex)
{
	TESNPC * npc =  DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
	if(!npc)
		return false;

	UInt64 gender = CALL_MEMBER_FN(npc, GetSex)();
	bool isFemale = gender == 1 ? true : false;

	NiNode * overlayHolder = GetOverlayRoot(actor, rootNode);
	if(overlayHolder)
	{
		DestroyOverlaySlot(actor, overlayHolder, slotIndex);

		// Check the overlay table before we add any overlays
		UInt64 handle = PapyrusVM::GetHandleFromObject(actor, Actor::kTypeID);

		auto hit = m_overlays[isFemale ? 1 : 0].find(handle);
		if(hit == m_overlays[isFemale ? 1 : 0].end()) {
			return false;
		}

		std::unordered_multimap<UInt32, BSTriShape*> candidates;
		VisitObjects(object, [&](NiAVObject * node)
		{
			BSTriShape * trishape = node->GetAsBSTriShape();
			if(trishape) {
				BSLightingShaderProperty * shaderProperty = ni_cast(trishape->shaderProperty, BSLightingShaderProperty);
				if(shaderProperty) {
					BSLightingShaderMaterialBase * newMaterial = static_cast<BSLightingShaderMaterialBase *>(shaderProperty->shaderMaterial);
					if(newMaterial->GetType() == BSLightingShaderMaterialBase::kType_SkinTint) {
						candidates.emplace(slotIndex, trishape);
					}
				}
			}
			return false;
		});

		char buff[MAX_PATH];
		for(auto & item : candidates)
		{
			sprintf_s(buff, MAX_PATH, "[%d][%s]", item.first, item.second->m_name.c_str());
			NiNode * overlayRoot = NiNode::Create(0);
			overlayRoot->m_name = buff;
			overlayHolder->AttachChild(overlayRoot, true);

			UInt32 layerIndex = 0;

			std::vector<std::pair<std::string, std::pair<OverlayDataPtr, F4EEFixedString>>> overlays;

			ForEachOverlayBySlot(actor, isFemale, slotIndex, [&](SInt32, const OverlayDataPtr & overlay, const F4EEFixedString & material)
			{
				sprintf_s(buff, MAX_PATH, "Layer [%d][%d][%d]", layerIndex, item.first, overlay->uid);
				overlays.push_back(std::make_pair(buff, std::make_pair(overlay, material)));
				layerIndex++;
			});

			for(auto & ovl : overlays)
			{
				NiCloningProcess cp;
				memset(&cp, 0, sizeof(NiCloningProcess));
				cp.unk60 = 1;

				BSTriShape * cloned = (BSTriShape*)item.second->CreateClone(&cp);

				auto extraDataList = cloned->m_extraData;
				if(extraDataList)
				{
					extraDataList->lock.Lock();
					for(UInt32 i = 0; i < extraDataList->count; i++)
					{
						extraDataList->entries[i]->DecRef();
					}
					extraDataList->Clear();
					extraDataList->lock.Release();
				}
				cloned->skinInstance = item.second->skinInstance;
				cloned->m_name = ovl.first.c_str();
				overlayRoot->AttachChild(cloned, true);

				LoadMaterialData(cloned, ovl.second.second, ovl.second.first);
			}
		}
	}

	return true;
}

void OverlayInterface::LoadMaterialData(BSTriShape * shape, const F4EEFixedString & material, const OverlayDataPtr & overlayData)
{
	// Setup the Overlays materials
	BSMaterialBlock matBlock;
	if(!LoadMaterialFile(material.c_str(), &matBlock, 0))
		CALL_MEMBER_FN(&matBlock, SetShaderProperties)(shape, true);

	BSLightingShaderProperty * newShader = ni_cast(shape->shaderProperty, BSLightingShaderProperty);
	if(newShader) {
		BSLightingShaderMaterialBase * newMaterial = static_cast<BSLightingShaderMaterialBase *>(newShader->shaderMaterial);
		if(newMaterial) {
			if(overlayData->tintColor.a > 0 && newMaterial->GetType() == BSLightingShaderMaterialBase::kType_SkinTint) {
				BSLightingShaderMaterialSkinTint * skinTint = static_cast<BSLightingShaderMaterialSkinTint *>(newShader->shaderMaterial);
				skinTint->kTintColor = overlayData->tintColor;
			}

			// Transform the UV
			float sU, sV, oU, oV;
			newMaterial->GetOffsetUV(&oU, &oV);
			newMaterial->GetScaleUV(&sU, &sV);

			oU += overlayData->offsetUV.x;
			oV += overlayData->offsetUV.y;

			sU *= overlayData->scaleUV.x;
			sV *= overlayData->scaleUV.y;

			newMaterial->SetOffsetUV(oU, oV);
			newMaterial->SetScaleUV(sU, sV);
		}
	}
}

OverlayInterface::UniqueID OverlayInterface::AddOverlay(Actor * actor, bool isFemale, SInt32 priority, const F4EEFixedString & templateName, const NiColorA & tintColor, const NiPoint2 & offsetUV, const NiPoint2 & scaleUV)
{
	auto it = g_overlayInterface.m_overlayTemplates[isFemale ? 1 : 0].find(templateName);
	if(it == g_overlayInterface.m_overlayTemplates[isFemale ? 1 : 0].end())
	{
		return 0;
	}

	UInt64 handle = PapyrusVM::GetHandleFromObject(actor, Actor::kTypeID);

	SimpleLocker locker(&m_overlayLock);
	PriorityMapPtr priorityMap;
	auto hit = m_overlays[isFemale ? 1 : 0].find(handle);
	if(hit != m_overlays[isFemale ? 1 : 0].end()) {
		priorityMap = hit->second;
	} else {
		priorityMap = std::make_shared<PriorityMap>();
		m_overlays[isFemale ? 1 : 0].emplace(handle, priorityMap);
	}

	UniqueID uid = GetNextUID();
	OverlayDataPtr overlayData = std::make_shared<OverlayData>();
	overlayData->uid = uid;
	overlayData->templateName = g_stringTable.GetString(templateName);
	overlayData->tintColor = tintColor;
	overlayData->offsetUV = offsetUV;
	overlayData->scaleUV = scaleUV;
	priorityMap->emplace(priority, overlayData);
	m_dataMap.emplace(uid, overlayData);
	return uid;
}

OverlayInterface::UniqueID OverlayInterface::GetNextUID()
{
	SimpleLocker locker(&m_overlayLock);

	OverlayInterface::UniqueID nextUID = 0;
	if(!m_freeIndices.empty()) {
		nextUID = m_freeIndices.back();
		m_freeIndices.pop_back();
	} else {
		nextUID = m_dataMap.size() + 1;
	}

	return nextUID;
}

bool OverlayInterface::ReorderOverlay(Actor * actor, bool isFemale, UniqueID uid, SInt32 newPriority)
{
	UInt64 handle = PapyrusVM::GetHandleFromObject(actor, Actor::kTypeID);

	SimpleLocker locker(&m_overlayLock);
	auto hit = m_overlays[isFemale ? 1 : 0].find(handle);
	if(hit == m_overlays[isFemale ? 1 : 0].end())
		return false;

	PriorityMapPtr priorityMap = hit->second;
	if(!priorityMap)
		return false;

	OverlayDataPtr overlayPtr = nullptr;
	for (auto it = priorityMap->begin(); it!= priorityMap->end(); ++it)
	{
		if(it->second && it->second->uid == uid) {
			overlayPtr = it->second;
			priorityMap->erase(it);
			break;
		}
	}

	if(overlayPtr) {
		priorityMap->emplace(newPriority, overlayPtr);
		return true;
	}

	return false;
}

bool OverlayInterface::RemoveOverlay(Actor * actor, bool isFemale, UniqueID uid)
{
	UInt64 handle = PapyrusVM::GetHandleFromObject(actor, Actor::kTypeID);

	SimpleLocker locker(&m_overlayLock);
	auto hit = m_overlays[isFemale ? 1 : 0].find(handle);
	if(hit == m_overlays[isFemale ? 1 : 0].end())
		return false;

	PriorityMapPtr priorityMap = hit->second;
	if(!priorityMap)
		return false;

	for (auto it = priorityMap->begin(); it != priorityMap->end(); ++it)
	{
		OverlayDataPtr overlayPtr = it->second;
		if(!overlayPtr)
			continue;
		
		if(overlayPtr->uid == uid) {
			m_dataMap.erase(overlayPtr->uid);
			priorityMap->erase(it);
			return true;
		}
	}

	return false;
}

std::pair<SInt32, OverlayInterface::OverlayDataPtr> OverlayInterface::GetActorOverlayByUID(Actor * actor, bool isFemale, UniqueID uid)
{
	UInt64 handle = PapyrusVM::GetHandleFromObject(actor, Actor::kTypeID);

	SimpleLocker locker(&m_overlayLock);
	auto hit = m_overlays[isFemale ? 1 : 0].find(handle);
	if(hit == m_overlays[isFemale ? 1 : 0].end())
		return std::make_pair<SInt32, OverlayDataPtr>(0, nullptr);

	PriorityMapPtr priorityMap = hit->second;
	if(!priorityMap)
		return std::make_pair<SInt32, OverlayDataPtr>(0, nullptr);

	for (auto it = priorityMap->begin(); it != priorityMap->end(); ++it)
	{
		OverlayDataPtr overlayPtr = it->second;
		if(!overlayPtr)
			continue;

		if(overlayPtr->uid == uid) {
			return std::pair<SInt32, OverlayDataPtr>(it->first, overlayPtr);
		}
	}

	return std::make_pair<SInt32, OverlayDataPtr>(0, nullptr);
}

bool OverlayInterface::RemoveAll(Actor * actor, bool isFemale)
{
	UInt64 handle = PapyrusVM::GetHandleFromObject(actor, Actor::kTypeID);

	SimpleLocker locker(&m_overlayLock);
	auto hit = m_overlays[isFemale ? 1 : 0].find(handle);
	if(hit == m_overlays[isFemale ? 1 : 0].end())
		return false;

	PriorityMapPtr priorityMap = hit->second;
	if(!priorityMap)
		return false;

	// Grab all the UIDs for this actor first
	for (auto it= priorityMap->begin(); it!= priorityMap->end(); ++it)
	{
		OverlayDataPtr overlayPtr = it->second;
		if(!overlayPtr)
			continue;

		m_freeIndices.push_back(overlayPtr->uid);
		m_dataMap.erase(overlayPtr->uid);
	}

	m_overlays[isFemale ? 1 : 0].erase(hit);
	return true;
}

bool OverlayInterface::ForEachOverlay(Actor * actor, bool isFemale, std::function<void(SInt32, const OverlayDataPtr&)> functor)
{
	UInt64 handle = PapyrusVM::GetHandleFromObject(actor, Actor::kTypeID);

	SimpleLocker locker(&m_overlayLock);
	auto hit = m_overlays[isFemale ? 1 : 0].find(handle);
	if(hit == m_overlays[isFemale ? 1 : 0].end())
		return false;

	PriorityMapPtr priorityMap = hit->second;
	if(!priorityMap)
		return false;

	for(auto & it : *priorityMap)
	{
		if(it.second)
			functor(it.first, it.second);
	}

	return true;
}

bool OverlayInterface::ForEachOverlayBySlot(Actor * actor, bool isFemale, UInt32 slotIndex, std::function<void(SInt32, const OverlayDataPtr&, const F4EEFixedString &)> functor)
{
	UInt64 handle = PapyrusVM::GetHandleFromObject(actor, Actor::kTypeID);
	
	SimpleLocker locker(&m_overlayLock);
	auto hit = m_overlays[isFemale ? 1 : 0].find(handle);
	if(hit == m_overlays[isFemale ? 1 : 0].end())
		return false;

	PriorityMapPtr priorityMap = hit->second;
	if(!priorityMap)
		return false;

	for(auto & it : *priorityMap)
	{
		OverlayDataPtr pOverlay = it.second;
		if(pOverlay)
		{
			if(pOverlay->templateName)
			{
				auto pTemplate = GetTemplateByName(isFemale, *pOverlay->templateName);
				if(pTemplate) {
					auto slotIter = pTemplate->slotMaterial.find(slotIndex);
					if(slotIter != pTemplate->slotMaterial.end())
						functor(it.first, pOverlay, slotIter->second);
				}
			}
		}
	}

	return true;
}

const OverlayInterface::OverlayTemplatePtr OverlayInterface::GetTemplateByName(bool isFemale, const F4EEFixedString& name)
{
	auto tmpIter = m_overlayTemplates[isFemale ? 1 : 0].find(name);
	if(tmpIter != m_overlayTemplates[isFemale ? 1 : 0].end()) {
		return tmpIter->second;
	}

	return nullptr;
}

void OverlayInterface::OverlayData::Save(const F4SESerializationInterface * intfc, UInt32 kVersion)
{
	Serialization::WriteData<UniqueID>(intfc, &uid);

	UInt32 stringId = g_stringTable.GetStringID(templateName);
	Serialization::WriteData<UInt32>(intfc, &stringId);

	UInt32 a = min(0, max(tintColor.a * 255, 255));
	UInt32 r = min(0, max(tintColor.r * 255, 255));
	UInt32 g = min(0, max(tintColor.g * 255, 255));
	UInt32 b = min(0, max(tintColor.b * 255, 255));

	UInt32 tintARGB = (a << 24) | (r << 16) | (g << 8) | b;

	Serialization::WriteData<UInt32>(intfc, &tintARGB);
	Serialization::WriteData<float>(intfc, &offsetUV.x);
	Serialization::WriteData<float>(intfc, &offsetUV.y);
	Serialization::WriteData<float>(intfc, &scaleUV.x);
	Serialization::WriteData<float>(intfc, &scaleUV.y);
}

bool OverlayInterface::OverlayData::Load(const F4SESerializationInterface * intfc, UInt32 kVersion, const std::unordered_map<UInt32, StringTableItem> & stringTable)
{
	if (!Serialization::ReadData<UniqueID>(intfc, &uid))
	{
		_ERROR("%s - Error loading overlay uid", __FUNCTION__);
		return false;
	}

	UInt32 stringId;
	if (!Serialization::ReadData<UInt32>(intfc, &stringId))
	{
		_ERROR("%s - Error loading string id", __FUNCTION__);
		return false;
	}

	auto it = stringTable.find(stringId);
	if(it == stringTable.end())
	{
		_ERROR("%s - Error loading overlay material path string from table", __FUNCTION__);
		return false;
	}

	templateName = it->second;

	UInt32 tintARGB;

	if (!Serialization::ReadData<UInt32>(intfc, &tintARGB))
	{
		_ERROR("%s - Error loading overlay tint color", __FUNCTION__);
		return false;
	}

	tintColor.a = min(0, max((float)tintColor.a / 255.0f, 1.0f));
	tintColor.r = min(0, max((float)tintColor.r / 255.0f, 1.0f));
	tintColor.g = min(0, max((float)tintColor.g / 255.0f, 1.0f));
	tintColor.b = min(0, max((float)tintColor.b / 255.0f, 1.0f));

	if (!Serialization::ReadData<float>(intfc, &offsetUV.x))
	{
		_ERROR("%s - Error loading overlay offset U", __FUNCTION__);
		return false;
	}

	if (!Serialization::ReadData<float>(intfc, &offsetUV.y))
	{
		_ERROR("%s - Error loading overlay offset V", __FUNCTION__);
		return false;
	}

	if (!Serialization::ReadData<float>(intfc, &scaleUV.x))
	{
		_ERROR("%s - Error loading overlay scale U", __FUNCTION__);
		return false;
	}

	if (!Serialization::ReadData<float>(intfc, &scaleUV.y))
	{
		_ERROR("%s - Error loading overlay scale V", __FUNCTION__);
		return false;
	}
		
	return true;
}

void OverlayInterface::PriorityMap::Save(const F4SESerializationInterface * intfc, UInt32 kVersion)
{
	intfc->OpenRecord('OIPM', kVersion);

	UInt32 priorityCount = size();
	Serialization::WriteData<UInt32>(intfc, &priorityCount);

	for(auto & slot : *this)
	{
		SInt32 priority = slot.first;
		Serialization::WriteData<SInt32>(intfc, &priority);

		slot.second->Save(intfc, kVersion);
	}
}

bool OverlayInterface::PriorityMap::Load(const F4SESerializationInterface * intfc, bool isFemale, UInt32 kVersion, const std::unordered_map<UInt32, StringTableItem> & stringTable)
{
	UInt32 type, length, version;

	if(intfc->GetNextRecordInfo(&type, &version, &length))
	{
		switch (type)
		{
		case 'OIPM':
			{
				// Override Count
				UInt32 priorityCount = 0;
				if (!Serialization::ReadData<UInt32>(intfc, &priorityCount))
				{
					_ERROR("%s - Error loading priority count", __FUNCTION__);
					return false;
				}

				for (UInt32 i = 0; i < priorityCount; i++)
				{
					SInt32 priority;
					if (!Serialization::ReadData<SInt32>(intfc, &priority))
					{
						_ERROR("%s - Error loading priority", __FUNCTION__);
						return false;
					}

					OverlayDataPtr overlayData = std::make_shared<OverlayData>();
					if (!overlayData->Load(intfc, kVersion, stringTable))
					{
						_ERROR("%s - Error loading overlay data with priority %d", __FUNCTION__, priorityCount);
						return false;
					}

					// Don't add entries we dont have
					auto it = g_overlayInterface.m_overlayTemplates[isFemale ? 1 : 0].find(*overlayData->templateName);
					if(it != g_overlayInterface.m_overlayTemplates[isFemale ? 1 : 0].end())
					{
						emplace(priority, overlayData);
						g_overlayInterface.m_dataMap.emplace(overlayData->uid, overlayData);
					}
				}

				break;
			}
		default:
			{
				_ERROR("%s - Error loading unexpected chunk type %08X (%.4s)", __FUNCTION__, type, &type);
				return false;
			}
		}
	}

	return true;
}

void OverlayInterface::OverlayMap::Save(const F4SESerializationInterface * intfc, UInt32 kVersion)
{
	intfc->OpenRecord('OIPM', kVersion);

	UInt32 numOverlays = size();
	Serialization::WriteData<UInt32>(intfc, &numOverlays);

	// Male handles
	for(auto & overlay : *this)
	{
		// Key
		Serialization::WriteData<UInt64>(intfc, &overlay.first);

		// Value
		overlay.second->Save(intfc, kVersion);
	}
}

bool OverlayInterface::OverlayMap::Load(const F4SESerializationInterface * intfc, bool isFemale, UInt32 kVersion, const std::unordered_map<UInt32, StringTableItem> & stringTable)
{
	UInt32 type, length, version;

	if(intfc->GetNextRecordInfo(&type, &version, &length))
	{
		switch (type)
		{
		case 'OIPM':
			{

				UInt32 overlays = 0;
				if (!Serialization::ReadData<UInt32>(intfc, &overlays))
				{
					_ERROR("%s - Error loading male overlay count", __FUNCTION__);
					return false;
				}

				for(UInt32 i = 0; i < overlays; i++)
				{
					UInt64 handle;
					// Key
					if (!Serialization::ReadData<UInt64>(intfc, &handle))
					{
						_ERROR("%s - Error loading actor key", __FUNCTION__);
						return false;
					}

					PriorityMapPtr priorityMap = std::make_shared<PriorityMap>();
					if (!priorityMap->Load(intfc, isFemale, kVersion, stringTable))
					{
						_ERROR("%s - Error loading overlay priority map for actor %016llX", __FUNCTION__, handle);
						return false;
					}

					UInt64 newHandle = 0;

					if(!g_bEnableOverlays)
						continue;

					// Skip if handle is no longer valid.
					if (!intfc->ResolveHandle(handle, &newHandle))
						continue;

					if(priorityMap->empty())
						continue;

					emplace(newHandle, priorityMap);
				}

				return true;
			}
		}
	}

	return false;
}

void OverlayInterface::Save(const F4SESerializationInterface * intfc, UInt32 kVersion)
{
	SimpleLocker locker(&m_overlayLock);

	intfc->OpenRecord('OVRL', kVersion);

	for(UInt32 g = 0; g <= 1; g++)
	{
		m_overlays[g].Save(intfc, kVersion);
	}
}

bool OverlayInterface::Load(const F4SESerializationInterface * intfc, UInt32 kVersion, const std::unordered_map<UInt32, StringTableItem> & stringTable)
{
	SimpleLocker locker(&m_overlayLock);

	UInt32 type, length, version;
	if(intfc->GetNextRecordInfo(&type, &version, &length))
	{
		switch (type)
		{
		case 'OIPM':
			{
				m_overlays[0].Load(intfc, false, kVersion, stringTable);
				break;
			}
		default:
			break;
		}
	}

	if(intfc->GetNextRecordInfo(&type, &version, &length))
	{
		switch (type)
		{
		case 'OIPM':
			{
				m_overlays[1].Load(intfc, true, kVersion, stringTable);
				break;
			}
		default:
			break;
		}
	}

	// Build the free index list
	UInt32 dataSize = m_dataMap.size();
	for(UInt32 uid = 0; uid < dataSize; uid++)
	{
		auto it = m_dataMap.find(uid);
		if(it == m_dataMap.end()) {
			m_freeIndices.push_back(uid);
		}
	}
	
	return true;
}

void OverlayInterface::Revert()
{
	SimpleLocker locker(&m_overlayLock);
	m_overlays[0].clear();
	m_overlays[1].clear();
	m_freeIndices.clear();
	m_dataMap.clear();
}

const OverlayInterface::OverlayDataPtr OverlayInterface::GetOverlayByUID(UniqueID uid)
{
	SimpleLocker locker(&m_overlayLock);
	auto it = m_dataMap.find(uid);
	if(it != m_dataMap.end()) {
		return it->second;
	}

	return nullptr;
}

EventResult	OverlayInterface::ReceiveEvent(TESObjectLoadedEvent * evn, void * dispatcher)
{
	// Don't need to push pending updates if we have BodyMorphs enabled, those will do this for us
	if(!g_bEnableBodyMorphs)
	{
		if(evn->loaded)
		{
			// We need to collect pending loads because these will fire before the load game event
			TESForm * form = LookupFormByID(evn->formId);
			if(!form)
				return kEvent_Continue;

			Actor * actor = DYNAMIC_CAST(form, TESForm, Actor);
			if(!actor)
				return kEvent_Continue;

			TESNPC * npc =  DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
			if(!npc)
				return kEvent_Continue;

			UInt64 gender = CALL_MEMBER_FN(npc, GetSex)();
			bool isFemale = gender == 1 ? true : false;

			if(m_loading) // We're mid-load, lets just push these to pending
			{
				m_pendingUpdates.insert((gender << 32) | form->formID);
			}
			else
			{
				UpdateOverlays(actor);
			}
		}
	}

	return kEvent_Continue;
}

EventResult	OverlayInterface::ReceiveEvent(TESLoadGameEvent * evn, void * dispatcher)
{
	if(m_loading)
		m_loading = false;

	// We don't need to do this if BodyMorphs are enabled, that will update for us
	if(!g_bEnableBodyMorphs)
	{
		for(auto & uid : m_pendingUpdates)
		{
			TESForm * form = LookupFormByID(uid);
			if(form && form->formType == Actor::kTypeID)
			{
				Actor * actor = static_cast<Actor*>(form);
				UpdateOverlays(actor);
			}
		}

		m_pendingUpdates.clear();
	}
	
	return kEvent_Continue;
}

bool OverlayInterface::HasSkinChildren(NiAVObject * slot)
{
	return VisitObjects(slot, [&](NiAVObject * node)
	{
		BSTriShape * trishape = node->GetAsBSTriShape();
		if(trishape) {
			BSLightingShaderProperty * shaderProperty = ni_cast(trishape->shaderProperty, BSLightingShaderProperty);
			if(shaderProperty) {
				BSLightingShaderMaterialBase * newMaterial = static_cast<BSLightingShaderMaterialBase *>(shaderProperty->shaderMaterial);
				if(newMaterial->GetType() == BSLightingShaderMaterialBase::kType_SkinTint) {
					return true;
				}
			}
		}
		return false;
	});
}


F4EEUpdateOverlays::F4EEUpdateOverlays(TESForm * form)
{
	m_formId = form ? form->formID : 0;
}

void F4EEUpdateOverlays::Run()
{
	TESForm * form = LookupFormByID(m_formId);
	if(form) {
		Actor * actor = DYNAMIC_CAST(form, TESForm, Actor);
		if(actor) {
			// Delete all overlays
			NiNode * overlayRoot = g_overlayInterface.GetOverlayRoot(actor, actor->GetActorRootNode(false), false);
			if(overlayRoot) {
				NiNode * parent = overlayRoot->m_parent;
				if(parent)
					parent->Remove(overlayRoot);

				// Delete the first person Overlays
				if(actor == (*g_player)) {
					NiNode * overlayRoot = g_overlayInterface.GetOverlayRoot(actor, actor->GetActorRootNode(true), false);
					if(overlayRoot) {
						NiNode * parent = overlayRoot->m_parent;
						if(parent)
							parent->Remove(overlayRoot);
					}
				}
			}


			if(actor->unk300) {
				// Detaching the node will cause the game to regenerate when UpdateEquipment is called
				// We only need to detach armor, and armor that's even eligible for morphing
				ActorEquipData * equipData[2];
				equipData[0] = actor->equipData;
				equipData[1] = actor == (*g_player) ? (*g_player)->playerEquipData : nullptr;

				for(UInt32 s = 0; s < (actor == (*g_player) ? 2 : 1); s++)
				{
					if(!equipData[s])
						continue;

					for(UInt32 i = 0; i < 31; ++i)
					{
						NiPointer<NiAVObject> slotNode(equipData[s]->slots[i].node);
						if(!slotNode)
							continue;

						// See if this node has any skin candidates first
						if(!g_overlayInterface.HasSkinChildren(slotNode))
							continue;

						NiNode * rootNode = GetRootNode(actor, slotNode);
						if(rootNode)
							g_overlayInterface.UpdateOverlays(actor, rootNode, slotNode, i);
					}
				}
			}
		}
	}
}


F4EEOverlayUpdate::F4EEOverlayUpdate(TESForm * form, UInt32 uid)
{
	m_formId = form ? form->formID : 0;
	m_uid = uid;
}

void F4EEOverlayUpdate::Run()
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

	UInt64 gender = CALL_MEMBER_FN(npc, GetSex)();
	bool isFemale = gender == 1 ? true : false;

	if(!actor->unk300)
		return;

	// Detaching the node will cause the game to regenerate when UpdateEquipment is called
	// We only need to detach armor, and armor that's even eligible for morphing
	ActorEquipData * equipData[2];
	equipData[0] = actor->equipData;
	equipData[1] = actor == (*g_player) ? (*g_player)->playerEquipData : nullptr;

	for(UInt32 s = 0; s < (actor == (*g_player) ? 2 : 1); s++)
	{
		if(!equipData[s])
			continue;

		for(UInt32 slot = 0; slot < 31; slot++)
		{
			NiPointer<NiAVObject> slotNode(equipData[s]->slots[slot].node);
			if(!slotNode)
				continue;

			// See if this node has any skin candidates first
			if(!g_overlayInterface.HasSkinChildren(slotNode))
				continue;

			NiNode * rootNode = GetRootNode(actor, slotNode);
			if(!rootNode)
				continue;

			NiNode * overlayRoot = g_overlayInterface.GetOverlayRoot(actor, rootNode, false);
			if(!overlayRoot)
				continue;

			VisitObjects(overlayRoot, [&](NiAVObject * object)
			{
				BSTriShape * shape = object->GetAsBSTriShape();
				if(!shape)
					return false;

				UInt32 layerIndex;
				UInt32 slotIndex;
				UInt32 targetUID;
				if(sscanf_s(object->m_name.c_str(), "Layer [%d][%d][%d]", &layerIndex, &slotIndex, &targetUID) && targetUID == m_uid)
				{
					const OverlayInterface::OverlayDataPtr pOverlayData = g_overlayInterface.GetOverlayByUID(m_uid);
					if(!pOverlayData)
						return false;

					const OverlayInterface::OverlayTemplatePtr pTemplate = g_overlayInterface.GetTemplateByName(isFemale, *pOverlayData->templateName);
					if(!pTemplate)
						return false;

					auto material = pTemplate->slotMaterial.find(slotIndex);
					if(material != pTemplate->slotMaterial.end()) {
						g_overlayInterface.LoadMaterialData(shape, material->second, pOverlayData);
					}
				}

				return false;
			});
		}
	}
}

bool OverlayInterface::UpdateOverlays(Actor * actor)
{
	if(!actor)
		return false;

	if(g_task)
		g_task->AddTask(new F4EEUpdateOverlays(actor));

	return true;
}

bool OverlayInterface::UpdateOverlay(Actor * actor, UInt32 uid)
{
	if(!actor)
		return false;

	if(g_task)
		g_task->AddTask(new F4EEOverlayUpdate(actor, uid));

	return true;
}

void OverlayInterface::LoadOverlayMods()
{
	// Load templates
	for(int i = 0; i < (*g_dataHandler)->modList.loadedModCount; i++)
	{
		ModInfo * modInfo = (*g_dataHandler)->modList.loadedMods[i];
		std::string templatesPath = std::string("Data\\F4SE\\Plugins\\F4EE\\Overlays\\") + std::string(modInfo->name) + "\\overlays.json";
		LoadOverlayTemplates(templatesPath.c_str());
	}
}


bool OverlayInterface::LoadOverlayTemplates(const std::string & filePath)
{
	BSResourceNiBinaryStream binaryStream(filePath.c_str());
	if(binaryStream.IsValid())
	{
		std::string strFile;
		BSReadAll(&binaryStream, &strFile);

		Json::Reader reader;
		Json::Value root;

		if(!reader.parse(strFile, root)) {
			return false;
		}

		// traverse instances
		for(auto & item : root)
		{
			try
			{
				OverlayTemplatePtr pOverlayTemplate = nullptr;
				UInt8 gender = max(0, min(item["gender"].asUInt(), 1));

				F4EEFixedString id = item["id"].asCString();

				auto idIter = m_overlayTemplates[gender].find(id);
				if(idIter != m_overlayTemplates[gender].end())
					pOverlayTemplate = idIter->second;
				else {
					pOverlayTemplate = std::make_shared<OverlayTemplate>();
					m_overlayTemplates[gender].emplace(id, pOverlayTemplate);
				}

				if(item.isMember("name"))
					pOverlayTemplate->displayName = item["name"].asCString();

				if(item.isMember("playable"))
					pOverlayTemplate->playable = item["playable"].asBool();

				if(item.isMember("sort"))
					pOverlayTemplate->sort = item["sort"].asInt();

				if(item.isMember("transformable"))
					pOverlayTemplate->transformable = item["transformable"].asBool();

				if(item.isMember("slots"))
				{
					auto slots = item["slots"];
					for(auto & slot : slots)
					{
						pOverlayTemplate->slotMaterial.emplace(slot["slot"].asUInt(), slot["material"].asCString());
					}
				}
			}
			catch(const std::exception& e)
			{
				_ERROR(e.what());
			}
		}
	}

	return true;
}

void OverlayInterface::ForEachOverlayTemplate(bool isFemale, std::function<void(const F4EEFixedString&, const OverlayTemplatePtr&)> functor)
{
	for(auto & overlay : m_overlayTemplates[isFemale ? 1 : 0]) {
		functor(overlay.first, overlay.second);
	}
}