#include "OverlayInterface.h"

#include "f4se/NiNodes.h"
#include "f4se/GameReferences.h"

#include "f4se/GameObjects.h"
#include "f4se/GameRTTI.h"

#include "f4se/BSGeometry.h"
#include "f4se/NiExtraData.h"
#include "f4se/NiProperties.h"
#include "f4se/NiMaterials.h"
#include "f4se/NiCloningProcess.h"
#include "f4se/NiRTTI.h"

#include "f4se/PapyrusUtilities.h"

#include "Utilities.h"

#include <stdio.h>
#include <set>
#include <unordered_map>

extern OverlayInterface g_overlayInterface;
extern StringTable g_stringTable;
extern bool g_bEnableOverlays;
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

			std::vector<std::pair<std::string, OverlayDataPtr>> overlays;

			ForEachOverlayBySlot(actor, isFemale, slotIndex, [&](UInt32 slotIndex, SInt32 priority, const OverlayDataPtr & overlay)
			{
				sprintf_s(buff, MAX_PATH, "Layer [%d]", layerIndex);
				overlays.push_back(std::make_pair(buff, overlay));
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

				BSFixedString materialPath(ovl.second->m_materialPath->c_str());
				// Setup the Overlays materials
				BSMaterialBlock matBlock;
				if(!LoadMaterialFile(materialPath.c_str(), &matBlock, 0))
					CALL_MEMBER_FN(&matBlock, SetShaderProperties)(cloned, true);

				// Change tinting if applicable
				if(ovl.second->m_tintColor.a > 0)
				{
					BSLightingShaderProperty * newShader = ni_cast(cloned->shaderProperty, BSLightingShaderProperty);
					BSLightingShaderMaterialBase * newMaterial = static_cast<BSLightingShaderMaterialBase *>(newShader->shaderMaterial);
					if(newMaterial->GetType() == BSLightingShaderMaterialBase::kType_SkinTint) {
						BSLightingShaderMaterialSkinTint * skinTint = static_cast<BSLightingShaderMaterialSkinTint *>(newShader->shaderMaterial);
						skinTint->kTintColor = ovl.second->m_tintColor;
					}
				}
			}
		}
	}

	return true;
}

void OverlayInterface::AddOverlay(Actor * actor, bool isFemale, UInt32 slotIndex, SInt32 priority, BGSKeyword * keyword, const F4EEFixedString & materialPath, NiColorA tintColor)
{
	UInt64 handle = PapyrusVM::GetHandleFromObject(actor, Actor::kTypeID);
	UInt64 keywordHandle = PapyrusVM::GetHandleFromObject(keyword, BGSKeyword::kTypeID);

	OverlayDataPtr overlayData = std::make_shared<OverlayData>();
	overlayData->m_materialPath = g_stringTable.GetString(materialPath);
	overlayData->m_tintColor = tintColor;
	overlayData->m_keyword = keywordHandle;

	SimpleLocker locker(&m_overlayLock);

	SlotMapPtr slotMap;
	auto hit = m_overlays[isFemale ? 1 : 0].find(handle);
	if(hit != m_overlays[isFemale ? 1 : 0].end()) {
		slotMap = hit->second;
	} else {
		slotMap = std::make_shared<SlotMap>();
		m_overlays[isFemale ? 1 : 0].emplace(handle, slotMap);
	}

	PriorityMapPtr priorityMap;
	auto pit = slotMap->find(slotIndex);
	if(pit != slotMap->end()) {
		priorityMap = pit->second;
	} else {
		priorityMap = std::make_shared<PriorityMap>();
		slotMap->emplace(slotIndex, priorityMap);
	}

	priorityMap->emplace(priority, overlayData);
}

bool OverlayInterface::ReorderOverlay(Actor * actor, bool isFemale, UInt32 slotIndex, SInt32 priority, BGSKeyword * keyword, BSFixedString materialPath, SInt32 newPriority)
{
	UInt64 handle = PapyrusVM::GetHandleFromObject(actor, Actor::kTypeID);
	UInt64 keywordHandle = PapyrusVM::GetHandleFromObject(keyword, BGSKeyword::kTypeID);

	SimpleLocker locker(&m_overlayLock);
	auto hit = m_overlays[isFemale ? 1 : 0].find(handle);
	if(hit == m_overlays[isFemale ? 1 : 0].end())
		return false;

	SlotMapPtr slotMap = hit->second;
	if(!slotMap)
		return false;

	auto pit = slotMap->find(slotIndex);
	if(pit == slotMap->end())
		return false;

	PriorityMapPtr priorityMap = pit->second;
	if(!priorityMap)
		return false;

	OverlayDataPtr overlayPtr = nullptr;
	for (auto it= priorityMap->begin(); it!= priorityMap->end(); ++it)
	{
		overlayPtr = it->second;
		if(!overlayPtr)
			continue;

		if(overlayPtr->m_materialPath && overlayPtr->m_materialPath->AsBSFixedString() == materialPath && it->first == priority && overlayPtr->m_keyword == keywordHandle) {
			priorityMap->erase(it);
			return true;
		}
	}

	if(overlayPtr) {
		priorityMap->emplace(newPriority, overlayPtr);
		return true;
	}

	return false;
}

bool OverlayInterface::RemoveOverlay(Actor * actor, bool isFemale, UInt32 slotIndex, SInt32 priority, BGSKeyword * keyword, BSFixedString materialPath)
{
	UInt64 handle = PapyrusVM::GetHandleFromObject(actor, Actor::kTypeID);
	UInt64 keywordHandle = PapyrusVM::GetHandleFromObject(keyword, BGSKeyword::kTypeID);

	SimpleLocker locker(&m_overlayLock);
	auto hit = m_overlays[isFemale ? 1 : 0].find(handle);
	if(hit == m_overlays[isFemale ? 1 : 0].end())
		return false;

	SlotMapPtr slotMap = hit->second;
	if(!slotMap)
		return false;

	auto pit = slotMap->find(slotIndex);
	if(pit == slotMap->end())
		return false;

	PriorityMapPtr priorityMap = pit->second;
	if(!priorityMap)
		return false;

	for (auto it= priorityMap->begin(); it!= priorityMap->end(); ++it)
	{
		OverlayDataPtr overlayPtr = it->second;
		if(!overlayPtr)
			continue;
		
		if(overlayPtr->m_materialPath && overlayPtr->m_materialPath->AsBSFixedString() == materialPath && it->first == priority && overlayPtr->m_keyword == keywordHandle) {
			priorityMap->erase(it);
			return true;
		}
	}

	return false;
}

bool OverlayInterface::RemoveAll(Actor * actor, bool isFemale)
{
	UInt64 handle = PapyrusVM::GetHandleFromObject(actor, Actor::kTypeID);

	SimpleLocker locker(&m_overlayLock);
	auto hit = m_overlays[isFemale ? 1 : 0].find(handle);
	if(hit == m_overlays[isFemale ? 1 : 0].end())
		return false;

	m_overlays[isFemale ? 1 : 0].erase(hit);
	return true;
}

bool OverlayInterface::ForEachOverlay(Actor * actor, bool isFemale, std::function<void(UInt32, SInt32, const OverlayDataPtr&)> functor)
{
	UInt64 handle = PapyrusVM::GetHandleFromObject(actor, Actor::kTypeID);

	SimpleLocker locker(&m_overlayLock);
	auto hit = m_overlays[isFemale ? 1 : 0].find(handle);
	if(hit == m_overlays[isFemale ? 1 : 0].end())
		return false;

	SlotMapPtr slotMap = hit->second;
	if(!slotMap)
		return false;

	for(auto & pit : *slotMap)
	{
		PriorityMapPtr priorityMap = pit.second;
		if(!priorityMap)
			continue;

		for(auto & it : *priorityMap)
		{
			if(it.second)
				functor(pit.first, it.first, it.second);
		}
	}

	return true;
}

bool OverlayInterface::ForEachOverlayBySlot(Actor * actor, bool isFemale, UInt32 slotIndex, std::function<void(UInt32, SInt32, const OverlayDataPtr&)> functor)
{
	UInt64 handle = PapyrusVM::GetHandleFromObject(actor, Actor::kTypeID);
	
	SimpleLocker locker(&m_overlayLock);
	auto hit = m_overlays[isFemale ? 1 : 0].find(handle);
	if(hit == m_overlays[isFemale ? 1 : 0].end())
		return false;

	SlotMapPtr slotMap = hit->second;
	if(!slotMap)
		return false;

	auto pit = slotMap->find(slotIndex);
	if(pit == slotMap->end())
		return false;

	PriorityMapPtr priorityMap = pit->second;
	if(!priorityMap)
		return false;

	for(auto & it : *priorityMap)
	{
		if(it.second)
			functor(pit->first, it.first, it.second);
	}

	return true;
}

bool OverlayInterface::ForEachOverlayByPriority(Actor * actor, bool isFemale, SInt32 priority, std::function<void(UInt32, SInt32, const OverlayDataPtr&)> functor)
{
	UInt64 handle = PapyrusVM::GetHandleFromObject(actor, Actor::kTypeID);

	SimpleLocker locker(&m_overlayLock);
	auto hit = m_overlays[isFemale ? 1 : 0].find(handle);
	if(hit == m_overlays[isFemale ? 1 : 0].end())
		return false;

	SlotMapPtr slotMap = hit->second;
	if(!slotMap)
		return false;

	for(auto & pit : *slotMap)
	{
		PriorityMapPtr priorityMap = pit.second;
		if(!priorityMap)
			continue;

		auto range = priorityMap->equal_range(priority);
		for (PriorityMap::iterator it = range.first; it != range.second; ++it)
		{
			OverlayDataPtr pOverlay = it->second;
			if(pOverlay)
				functor(pit.first, it->first, pOverlay);
		}
	}

	return false;
}

bool OverlayInterface::ForEachOverlayByKeyword(Actor * actor, bool isFemale, BGSKeyword * keyword, std::function<void(UInt32, SInt32, const OverlayDataPtr&)> functor)
{
	UInt64 handle = PapyrusVM::GetHandleFromObject(actor, Actor::kTypeID);
	UInt64 keywordHandle = PapyrusVM::GetHandleFromObject(keyword, BGSKeyword::kTypeID);

	SimpleLocker locker(&m_overlayLock);
	auto hit = m_overlays[isFemale ? 1 : 0].find(handle);
	if(hit == m_overlays[isFemale ? 1 : 0].end())
		return false;

	SlotMapPtr slotMap = hit->second;
	if(!slotMap)
		return false;

	for(auto & pit : *slotMap)
	{
		PriorityMapPtr priorityMap = pit.second;
		if(!priorityMap)
			continue;

		for(auto it : *priorityMap)
		{
			OverlayDataPtr pOverlay = it.second;
			if(pOverlay && pOverlay->m_keyword == keywordHandle)
				functor(pit.first, it.first, pOverlay);
		}
	}

	return true;
}


void OverlayInterface::OverlayData::Save(const F4SESerializationInterface * intfc, UInt32 kVersion)
{
	UInt32 stringId = g_stringTable.GetStringID(m_materialPath);
	Serialization::WriteData<UInt32>(intfc, &stringId);
	Serialization::WriteData<UInt64>(intfc, &m_keyword);
	Serialization::WriteData<float>(intfc, &m_tintColor.a);
	if(m_tintColor.a > 0) {
		Serialization::WriteData<float>(intfc, &m_tintColor.r);
		Serialization::WriteData<float>(intfc, &m_tintColor.g);
		Serialization::WriteData<float>(intfc, &m_tintColor.b);
	}
}

bool OverlayInterface::OverlayData::Load(const F4SESerializationInterface * intfc, UInt32 kVersion, const std::unordered_map<UInt32, StringTableItem> & stringTable)
{
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

	m_materialPath = it->second;

	UInt64 handle;
	if (!Serialization::ReadData<UInt64>(intfc, &handle))
	{
		_ERROR("%s - Error loading overlay data keyword handle", __FUNCTION__);
		return false;
	}

	m_keyword = handle;


	if (!Serialization::ReadData<float>(intfc, &m_tintColor.a))
	{
		_ERROR("%s - Error loading overlay tint data alpha", __FUNCTION__);
		return false;
	}

	if(m_tintColor.a > 0)
	{
		if (!Serialization::ReadData<float>(intfc, &m_tintColor.r))
		{
			_ERROR("%s - Error loading overlay tint data red", __FUNCTION__);
			return false;
		}
		if (!Serialization::ReadData<float>(intfc, &m_tintColor.g))
		{
			_ERROR("%s - Error loading overlay tint data green", __FUNCTION__);
			return false;
		}
		if (!Serialization::ReadData<float>(intfc, &m_tintColor.b))
		{
			_ERROR("%s - Error loading overlay tint data blue", __FUNCTION__);
			return false;
		}
	}
	
	return true;
}

void OverlayInterface::PriorityMap::Save(const F4SESerializationInterface * intfc, UInt32 kVersion)
{
	intfc->OpenRecord('OIPM', kVersion);

	UInt32 slotCount = size();
	Serialization::WriteData<UInt32>(intfc, &slotCount);

	for(auto & slot : *this)
	{
		SInt32 priority = slot.first;
		Serialization::WriteData<SInt32>(intfc, &priority);

		slot.second->Save(intfc, kVersion);
	}
}

bool OverlayInterface::PriorityMap::Load(const F4SESerializationInterface * intfc, UInt32 kVersion, const std::unordered_map<UInt32, StringTableItem> & stringTable)
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

					UInt64 newHandle = 0;

					// Skip if handle is no longer valid.
					if (!intfc->ResolveHandle(overlayData->m_keyword, &newHandle))
						return true;

					overlayData->m_keyword = newHandle;

					emplace(priority, overlayData);
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

void OverlayInterface::SlotMap::Save(const F4SESerializationInterface * intfc, UInt32 kVersion)
{
	intfc->OpenRecord('OISM', kVersion);

	UInt32 slotCount = size();
	Serialization::WriteData<UInt32>(intfc, &slotCount);

	for(auto & slot : *this)
	{
		UInt32 slotId = slot.first;
		Serialization::WriteData<UInt32>(intfc, &slotId);

		slot.second->Save(intfc, kVersion);
	}
}

bool OverlayInterface::SlotMap::Load(const F4SESerializationInterface * intfc, UInt32 kVersion, const std::unordered_map<UInt32, StringTableItem> & stringTable)
{
	UInt32 type, length, version;

	if(intfc->GetNextRecordInfo(&type, &version, &length))
	{
		switch (type)
		{
		case 'OISM':
			{
				// Override Count
				UInt32 slotCount = 0;
				if (!Serialization::ReadData<UInt32>(intfc, &slotCount))
				{
					_ERROR("%s - Error loading slot map count", __FUNCTION__);
					return false;
				}

				for (UInt32 i = 0; i < slotCount; i++)
				{
					UInt32 slotId;
					if (!Serialization::ReadData<UInt32>(intfc, &slotId))
					{
						_ERROR("%s - Error loading slot id", __FUNCTION__);
						return false;
					}

					PriorityMapPtr priorityMap = std::make_shared<PriorityMap>();
					if (!priorityMap->Load(intfc, kVersion, stringTable))
					{
						_ERROR("%s - Error loading overlay priority map for slot %d", __FUNCTION__, slotId);
						return false;
					}

					emplace(slotId, priorityMap);
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

void OverlayInterface::Save(const F4SESerializationInterface * intfc, UInt32 kVersion)
{
	SimpleLocker locker(&m_overlayLock);

	// Male handles
	for(auto & overlay : m_overlays[0])
	{
		intfc->OpenRecord('OVRM', kVersion);

		// Key
		Serialization::WriteData<UInt64>(intfc, &overlay.first);

		// Value
		overlay.second->Save(intfc, kVersion);
	}

	// Female handles
	for(auto & overlay : m_overlays[1])
	{
		intfc->OpenRecord('OVRF', kVersion);

		// Key
		Serialization::WriteData<UInt64>(intfc, &overlay.first);

		// Value
		overlay.second->Save(intfc, kVersion);
	}
}

bool OverlayInterface::Load(const F4SESerializationInterface * intfc, bool isFemale, UInt32 kVersion, const std::unordered_map<UInt32, StringTableItem> & stringTable)
{
	UInt64 handle;
	// Key
	if (!Serialization::ReadData<UInt64>(intfc, &handle))
	{
		_ERROR("%s - Error loading actor key", __FUNCTION__);
		return false;
	}

	SlotMapPtr slotMap = std::make_shared<SlotMap>();
	if (!slotMap->Load(intfc, kVersion, stringTable))
	{
		_ERROR("%s - Error loading overlay slot map for actor %016llX", __FUNCTION__, handle);
		return false;
	}

	UInt64 newHandle = 0;

	if(!g_bEnableOverlays)
		return true;

	// Skip if handle is no longer valid.
	if (!intfc->ResolveHandle(handle, &newHandle))
		return true;

	if(slotMap->empty())
		return true;

	m_overlayLock.Lock();
	m_overlays[isFemale ? 1 : 0].emplace(newHandle, slotMap);
	m_overlayLock.Release();
	return true;
}

void OverlayInterface::Revert()
{
	SimpleLocker locker(&m_overlayLock);
	m_overlays[0].clear();
	m_overlays[1].clear();
}

EventResult	OverlayInterface::ReceiveEvent(TESObjectLoadedEvent * evn, void * dispatcher)
{
	return kEvent_Continue;
}

EventResult	OverlayInterface::ReceiveEvent(TESLoadGameEvent * evn, void * dispatcher)
{
	/*NiColorA color;
	color.a = 1.0f;
	color.r = 1.0f;
	color.g = 0.0f;
	color.b = 0.0f;
	SetOverlayData(*g_player, true, 3, 0, nullptr, "materials\\test\\test0.bgsm", color);
	color.r = 0.0f;
	color.g = 0.0f;
	color.b = 1.0f;
	SetOverlayData(*g_player, true, 3, 0, nullptr, "materials\\test\\test1.bgsm", color);
	color.r = 0.0f;
	color.g = 1.0f;
	color.b = 0.0f;
	SetOverlayData(*g_player, true, 3, 0, nullptr, "materials\\test\\test2.bgsm", color);*/
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
					if(equipData[s])
					{
						for(UInt32 i = 0; i < 31; ++i)
						{
							NiPointer<NiAVObject> slotNode(equipData[s]->slots[i].node);
							if(slotNode) {
								// See if this node has any skin candidates first
								if(g_overlayInterface.HasSkinChildren(slotNode)) {
									NiNode * rootNode = GetRootNode(actor, slotNode);
									if(rootNode) {
										g_overlayInterface.UpdateOverlays(actor, rootNode, slotNode, i);
									}
								}
							}
						}
					}
				}
			}
		}
	}
}


F4EEOverlayUpdate::F4EEOverlayUpdate(TESForm * form, UInt32 slot)
{
	m_formId = form ? form->formID : 0;
	m_slot = slot;
}

void F4EEOverlayUpdate::Run()
{
	TESForm * form = LookupFormByID(m_formId);
	if(form) {
		Actor * actor = DYNAMIC_CAST(form, TESForm, Actor);
		if(actor) {
			if(actor->unk300) {
				// Detaching the node will cause the game to regenerate when UpdateEquipment is called
				// We only need to detach armor, and armor that's even eligible for morphing
				ActorEquipData * equipData[2];
				equipData[0] = actor->equipData;
				equipData[1] = actor == (*g_player) ? (*g_player)->playerEquipData : nullptr;

				for(UInt32 s = 0; s < (actor == (*g_player) ? 2 : 1); s++)
				{
					if(equipData[s])
					{
						NiPointer<NiAVObject> slotNode(equipData[s]->slots[m_slot].node);
						if(slotNode) {
							// See if this node has any skin candidates first
							if(g_overlayInterface.HasSkinChildren(slotNode)) {
								NiNode * rootNode = GetRootNode(actor, slotNode);
								if(rootNode) {
									g_overlayInterface.UpdateOverlays(actor, rootNode, slotNode, m_slot); // Install the new overlays
								}
							}
						}
					}
				}
			}
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

bool OverlayInterface::UpdateOverlay(Actor * actor, UInt32 slotIndex)
{
	if(!actor)
		return false;

	if(g_task)
		g_task->AddTask(new F4EEOverlayUpdate(actor, slotIndex));

	return true;
}