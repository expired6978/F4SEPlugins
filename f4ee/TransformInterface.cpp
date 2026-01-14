#include "TransformInterface.h"

#include "f4se/PluginAPI.h"

#include "f4se/GameReferences.h"
#include "f4se/GameObjects.h"
#include "f4se/GameRTTI.h"
#include "f4se/GameForms.h"
#include "f4se/GameSettings.h"
#include "f4se/GameData.h"

#include "f4se/NiNodes.h"

#include "Utilities.h"

extern StringTable g_stringTable;
#ifdef _TRANSFORMS
extern NiTransformInterface g_transformInterface;
#endif
extern F4SETaskInterface * g_task;

TransformDataPtr ActorData::SetTransformData(bool isFemale, bool isFirstPerson, const TransformData & data)
{
	TransformDataPtr & transformPtr = mTransform[isFemale ? 1 : 0][isFirstPerson ? 0 : 1];
	if (transformPtr)
	{
		if (data.IsEmpty())
		{
			transformPtr = nullptr;
		}
		else
		{
			*transformPtr = data;
		}
	}
	else
	{
		transformPtr = std::make_shared<TransformData>(data);
	}

	return transformPtr;
}

void ActorData::GetTransformData(bool isFemale, bool isFirstPerson, TransformDataPtr & data)
{
	data = mTransform[isFemale ? 1 : 0][isFirstPerson ? 0 : 1];
}

bool ActorData::ClearTransform(bool isFemale, bool isFirstPerson)
{
	TransformDataPtr & transformPtr = mTransform[isFemale ? 1 : 0][isFirstPerson ? 0 : 1];
	if (transformPtr)
	{
		transformPtr = nullptr;
		return true;
	}

	return false;
}

void TransformMap::SetTransformData(bool isFemale, bool isFirstPerson, BGSKeyword * keyword, const TransformData & data)
{
	auto actorIt = find(keyword ? keyword->formID : 0);
	if (actorIt != end())
	{
		actorIt->second.SetTransformData(isFemale, isFirstPerson, data);
		if (actorIt->second.IsEmpty())
		{
			erase(actorIt);
		}
	}
	else
	{
		(*this)[keyword ? keyword->formID : 0].SetTransformData(isFemale, isFirstPerson, data);
	}
	CalculateMergedTransform(isFemale, isFirstPerson);
}

void TransformMap::GetTransformData(bool isFemale, bool isFirstPerson, BGSKeyword * keyword, TransformDataPtr & data)
{
	auto actorIt = find(keyword ? keyword->formID : 0);
	if (actorIt != end())
	{
		actorIt->second.GetTransformData(isFemale, isFirstPerson, data);
	}
}

void TransformMap::ForEachTransform(bool isFemale, bool isFirstPerson, std::function<void(BGSKeyword*, TransformDataPtr&)> functor)
{
	for (auto & xForm : *this)
	{
		TESForm * form = xForm.first ? LookupFormByID(xForm.first) : nullptr;
		BGSKeyword * keyword = form ? DYNAMIC_CAST(form, TESForm, BGSKeyword) : nullptr;
		
		TransformDataPtr transformData;
		xForm.second.GetTransformData(isFemale, isFirstPerson, transformData);
		if (transformData) {
			functor(keyword, transformData);
		}
	}
}

bool TransformMap::ClearTransform(bool isFemale, bool isFirstPerson, BGSKeyword * keyword)
{
	auto dataIt = find(keyword ? keyword->formID : 0);
	if (dataIt != end())
	{
		bool res = dataIt->second.ClearTransform(isFemale, isFirstPerson);
		if (res && dataIt->second.IsEmpty()) {
			erase(dataIt);
		}
		CalculateMergedTransform(isFemale, isFirstPerson);
		return res;
	}

	return false;
}

void TransformMap::ClearTransforms(bool isFemale, bool isFirstPerson)
{
	for (TransformMap::iterator it = begin(); it != end(); ++it)
	{
		it->second.ClearTransform(isFemale, isFirstPerson);
		if (it->second.IsEmpty())
		{
			it = erase(it);
		}
	}
	CalculateMergedTransform(isFemale, isFirstPerson);
}

void TransformMap::GetMergedTransformData(bool isFemale, bool isFirstPerson, TransformDataPtr & data)
{
	mMerged.GetTransformData(isFemale, isFirstPerson, data);
}

void TransformMap::CalculateMergedTransform(bool isFemale, bool isFirstPerson)
{
	TransformData base;
	for (auto & it : *this)
	{
		TransformDataPtr data;
		it.second.GetTransformData(isFemale, isFirstPerson, data);
		base.GetTransform() = base.GetTransform() * data->GetTransform();

		// Check Scale merging mode here
	}
	mMerged.SetTransformData(isFemale, isFirstPerson, base);
}

void TransformNodeMap::SetNodeTransform(bool isFemale, bool isFirstPerson, const F4EEFixedString & node, BGSKeyword * keyword, const TransformData & data)
{
	auto nodeIt = find(g_stringTable.GetString(node));
	if (nodeIt != end())
	{
		nodeIt->second.SetTransformData(isFemale, isFirstPerson, keyword, data);
		if (nodeIt->second.IsEmpty())
		{
			erase(nodeIt);
		}
	}
	else
	{
		(*this)[g_stringTable.GetString(node)].SetTransformData(isFemale, isFirstPerson, keyword, data);
	}
}

void TransformNodeMap::GetNodeTransform(bool isFemale, bool isFirstPerson, const F4EEFixedString & node, BGSKeyword * keyword, TransformDataPtr & data)
{
	auto nodeIt = find(g_stringTable.GetString(node));
	if (nodeIt != end())
	{
		nodeIt->second.GetTransformData(isFemale, isFirstPerson, keyword, data);
	}
}

void TransformNodeMap::ForEachTransform(bool isFemale, bool isFirstPerson, const F4EEFixedString & node, std::function<void(BGSKeyword*, TransformDataPtr&)> functor)
{
	auto nodeIt = find(g_stringTable.GetString(node));
	if (nodeIt != end())
	{
		nodeIt->second.ForEachTransform(isFemale, isFirstPerson, functor);
	}
}

void TransformNodeMap::ForEachNode(bool isFemale, bool isFirstPerson, std::function<void(const F4EEFixedString&, BGSKeyword*, TransformDataPtr&)> functor)
{
	for (auto & it : *this)
	{
		it.second.ForEachTransform(isFemale, isFirstPerson, [&](BGSKeyword* keyword, TransformDataPtr& data)
		{
			functor(*it.first, keyword, data);
		});
	}
}

void TransformNodeMap::ForEachNodeResult(bool isFemale, bool isFirstPerson, std::function<void(const F4EEFixedString&, TransformDataPtr&)> functor)
{
	for (auto & it : *this)
	{
		TransformDataPtr data;
		it.second.GetMergedTransformData(isFemale, isFirstPerson, data);
		if(data)
			functor(*it.first, data);
	}
}

bool TransformNodeMap::ClearTransform(bool isFemale, bool isFirstPerson, const F4EEFixedString & node, BGSKeyword * keyword)
{
	auto nodeIt = find(g_stringTable.GetString(node));
	if (nodeIt != end())
	{
		bool res = nodeIt->second.ClearTransform(isFemale, isFirstPerson, keyword);
		if (res && nodeIt->second.IsEmpty())
		{
			erase(nodeIt);
		}

		return res;
	}

	return false;
}

void TransformNodeMap::ClearNodeTransforms(bool isFemale, bool isFirstPerson, const F4EEFixedString & node)
{
	auto nodeIt = find(g_stringTable.GetString(node));
	if (nodeIt != end())
	{
		nodeIt->second.ClearTransforms(isFemale, isFirstPerson);
		if (nodeIt->second.IsEmpty())
		{
			erase(nodeIt);
		}
	}
}

void TransformNodeMap::ClearTransforms(bool isFemale, bool isFirstPerson)
{
	for (TransformNodeMap::iterator it = begin(); it != end(); ++it)
	{
		it->second.ClearTransforms(isFemale, isFirstPerson);
		if (it->second.IsEmpty())
		{
			it = erase(it);
		}
	}
}

void NiTransformInterface::SetTransform(Actor * actor, bool isFemale, bool isFirstPerson, const F4EEFixedString & node, BGSKeyword * keyword, const TransformData & data)
{
	std::lock_guard<std::mutex> locker(mMutex);
	auto actorIt = find(actor->formID);
	if (actorIt != end())
	{
		actorIt->second.SetNodeTransform(isFemale, isFirstPerson, node, keyword, data);
		if (actorIt->second.IsEmpty())
		{
			erase(actorIt);
		}
	}
	else if(!data.IsEmpty())
	{
		(*this)[actor->formID].SetNodeTransform(isFemale, isFirstPerson, node, keyword, data);
	}
}

void NiTransformInterface::GetTransform(Actor * actor, bool isFemale, bool isFirstPerson, const F4EEFixedString & node, BGSKeyword * keyword, TransformDataPtr & data)
{
	std::lock_guard<std::mutex> locker(mMutex);
	auto actorIt = find(actor->formID);
	if (actorIt != end())
	{
		actorIt->second.GetNodeTransform(isFemale, isFirstPerson, node, keyword, data);
	}
}

void NiTransformInterface::GetActorNodeTransforms(Actor * actor, bool isFemale, bool isFirstPerson, const F4EEFixedString & node, std::vector<TransformIdentifier>& result)
{
	std::lock_guard<std::mutex> locker(mMutex);
	auto actorIt = find(actor->formID);
	if (actorIt != end())
	{
		actorIt->second.ForEachTransform(isFemale, isFirstPerson, node, [&](BGSKeyword* keyword, TransformDataPtr& data)
		{
			result.push_back(TransformIdentifier(isFemale, isFirstPerson, node, keyword, data));
		});
	}
}

void NiTransformInterface::GetActorGenderAndPerspectiveTransforms(Actor * actor, bool isFemale, bool isFirstPerson, std::vector<TransformIdentifier>& result)
{
	std::lock_guard<std::mutex> locker(mMutex);
	auto actorIt = find(actor->formID);
	if (actorIt != end())
	{
		actorIt->second.ForEachNode(isFemale, isFirstPerson, [&](const F4EEFixedString& node, BGSKeyword* keyword, TransformDataPtr& data)
		{
			result.push_back(TransformIdentifier(isFemale, isFirstPerson, node, keyword, data));
		});
	}
}

void NiTransformInterface::GetActorGenderTransforms(Actor * actor, bool isFemale, std::vector<TransformIdentifier>& result)
{
	std::lock_guard<std::mutex> locker(mMutex);
	auto actorIt = find(actor->formID);
	if (actorIt != end())
	{
		for (int i = 0; i <= 1; ++i)
		{
			actorIt->second.ForEachNode(isFemale, i == 0, [&](const F4EEFixedString& node, BGSKeyword* keyword, TransformDataPtr& data)
			{
				result.push_back(TransformIdentifier(isFemale, i == 0, node, keyword, data));
			});
		}
	}
}

void NiTransformInterface::GetActorTransforms(Actor * actor, std::vector<TransformIdentifier>& result)
{
	std::lock_guard<std::mutex> locker(mMutex);
	auto actorIt = find(actor->formID);
	if (actorIt != end())
	{
		for (int i = 0; i <= 1; ++i)
		{
			for (int j = 0; j <= 1; ++j)
			{
				actorIt->second.ForEachNode(i == 0, j == 0, [&](const F4EEFixedString& node, BGSKeyword* keyword, TransformDataPtr& data)
				{
					result.push_back(TransformIdentifier(i == 0, j == 0, node, keyword, data));
				});
			}
		}
	}
}

bool NiTransformInterface::ClearTransform(Actor * actor, bool isFemale, bool isFirstPerson, const F4EEFixedString & node, BGSKeyword * keyword)
{
	std::lock_guard<std::mutex> locker(mMutex);
	auto actorIt = find(actor->formID);
	if (actorIt != end())
	{
		bool res = actorIt->second.ClearTransform(isFemale, isFirstPerson, node, keyword);
		if (res && actorIt->second.IsEmpty())
		{
			erase(actorIt);
		}
		return res;
	}
	return false;
}

void NiTransformInterface::ClearActorNodeTransforms(Actor * actor, bool isFemale, bool isFirstPerson, const F4EEFixedString & node)
{
	std::lock_guard<std::mutex> locker(mMutex);
	auto actorIt = find(actor->formID);
	if (actorIt != end())
	{
		actorIt->second.ClearNodeTransforms(isFemale, isFirstPerson, node);
		if (actorIt->second.IsEmpty())
		{
			erase(actorIt);
		}
	}
}

void NiTransformInterface::ClearActorGenderAndPerspectiveTransforms(Actor * actor, bool isFemale, bool isFirstPerson)
{
	std::lock_guard<std::mutex> locker(mMutex);
	auto actorIt = find(actor->formID);
	if (actorIt != end())
	{
		actorIt->second.ClearTransforms(isFemale, isFirstPerson);
		if (actorIt->second.IsEmpty())
		{
			erase(actorIt);
		}
	}
}

void NiTransformInterface::ClearActorGenderTransforms(Actor * actor, bool isFemale)
{
	std::lock_guard<std::mutex> locker(mMutex);
	auto actorIt = find(actor->formID);
	if (actorIt != end())
	{
		for (int i = 0; i <= 1; ++i)
		{
			actorIt->second.ClearTransforms(isFemale, i == 0);
		}
		if (actorIt->second.IsEmpty())
		{
			erase(actorIt);
		}
	}
}

void NiTransformInterface::ClearActorTransforms(Actor * actor)
{
	std::lock_guard<std::mutex> locker(mMutex);
	auto actorIt = find(actor->formID);
	if (actorIt != end())
	{
		erase(actorIt);
	}
}

void NiTransformInterface::UpdateActorTransforms(Actor * actor)
{
	if (!actor)
		return;

	if (g_task)
		g_task->AddTask(new F4EETransformUpdate(actor));
}

void NiTransformInterface::ForEachActorNodeTransform(Actor * actor, bool isFemale, bool isFirstPerson, std::function<void(const F4EEFixedString&, TransformDataPtr&)> functor)
{
	std::lock_guard<std::mutex> locker(mMutex);
	auto actorIt = find(actor->formID);
	if (actorIt != end())
	{
		actorIt->second.ForEachNodeResult(isFemale, isFirstPerson, functor);
	}
}

F4EEFixedString NiTransformInterface::GetRootModelPath(Actor * refr, bool firstPerson, bool isFemale)
{
	TESModel * model = NULL;
	Actor * character = DYNAMIC_CAST(refr, TESObjectREFR, Actor);
	if (character) {
		if (firstPerson) {
			Setting	* setting = GetGameSetting("sFirstPersonSkeleton");
			if (setting && setting->GetType() == Setting::kType_String)
				return F4EEFixedString(setting->data.s);
		}

		TESRace * race = character->race;
		if (!race) {
			TESNPC * actorBase = DYNAMIC_CAST(refr->baseForm, TESForm, TESNPC);
			if (actorBase)
				race = actorBase->race.race;
		}

		if (race)
			model = &race->models[isFemale ? 1 : 0];
	}
	else
		model = DYNAMIC_CAST(refr->baseForm, TESForm, TESModel);

	if (model)
		return F4EEFixedString(model->GetModelName());

	return F4EEFixedString("");
}

F4EETransformUpdate::F4EETransformUpdate(TESForm * form)
{
	m_formId = form ? form->formID : 0;
}

void F4EETransformUpdate::Run()
{
	TESForm * form = LookupFormByID(m_formId);
	if (!form)
		return;

	Actor * actor = DYNAMIC_CAST(form, TESForm, Actor);
	if (!actor)
		return;

#ifdef _DEBUG
	_MESSAGE("%s - Activating Transform for %s (%08X)", __FUNCTION__, CALL_MEMBER_FN(actor, GetReferenceName)(), actor->formID);
#endif
	TESNPC * npc = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
	if (!npc)
		return;

	UInt64 gender = CALL_MEMBER_FN(npc, GetSex)();
	bool isFemale = gender == 1 ? true : false;

	NiNode * rootNode[2];
	rootNode[0] = actor->GetActorRootNode(false);
	rootNode[1] = actor == (*g_player) ? actor->GetActorRootNode(true) : nullptr;

	for (int i = 0; i <= 1; ++i)
	{
		if (rootNode[i])
		{
#ifdef _TRANSFORMS
			g_transformInterface.RevertSkeleton(actor, rootNode[i], isFemale, i == 1);
			g_transformInterface.ForEachActorNodeTransform(actor, isFemale, i == 1, [&](const F4EEFixedString & node, TransformDataPtr & data)
			{
				NiAVObject * objectNode = rootNode[i]->GetObjectByName(&BSFixedString(node.c_str()));
				if (objectNode) {
					objectNode->m_localTransform = objectNode->m_localTransform * data->GetTransform();
				}
				objectNode->UpdateTransforms();
			});
#endif
		}
	}
}

void NiTransformInterface::RevertSkeleton(Actor * actor, NiAVObject * rootNode, bool isFemale, bool isFirstPerson)
{
	auto modelPath = GetRootModelPath(actor, isFemale, isFirstPerson);
	auto it = mTransformCache.find(modelPath);
	if (it != mTransformCache.end())
	{
		for (auto & node : it->second)
		{
			auto objectName = BSFixedString(node.first.c_str());
			auto targetNode = rootNode->GetObjectByName(&objectName);
			if (targetNode) {
				targetNode->m_localTransform = node.second;
			}
		}
	}
}

void NiTransformInterface::SetModelProcessor()
{
	(*g_TESProcessor) = new TransformProcessor(*g_TESProcessor);
}

void NiTransformInterface::LoadAllSkeletons()
{
	mTransformCache.clear();

	Setting	* setting = GetGameSetting("sFirstPersonSkeleton");
	if (setting && setting->GetType() == Setting::kType_String)
	{
		mTransformCache.emplace(setting->data.s, NodeTransformCache::NodeMap());
	}
	
	for (UInt32 i = 0; i < (*g_dataHandler)->arrRACE.count; ++i)
	{
		TESRace * race = nullptr;
		(*g_dataHandler)->arrRACE.GetNthItem(i, race);
		if (race) {
			for (int i = 0; i <= 1; ++i)
			{
				F4EEFixedString modelName = race->models[i].GetModelName();
				if (modelName.length() > 0) {
					mTransformCache.emplace(modelName, NodeTransformCache::NodeMap());
				}
			}
		}
	}
}

void NiTransformInterface::CacheSkeleton(const char * modelName, NiAVObject * root)
{
	auto it = mTransformCache.find(modelName);
	if (it != mTransformCache.end())
	{
		NodeTransformCache::NodeMap transformMap;
		VisitObjects(root, [&](NiAVObject* child)
		{
			if (child->m_name == NULL)
				return false;

			F4EEFixedString localName(child->m_name);
			if (localName.length() == 0)
				return false;

			transformMap[localName] = child->m_localTransform;
			return false;
		});

		mTransformCache[modelName] = transformMap;
	}
}

void TransformProcessor::Process(BSModelDB::ModelData * modelData, const char * modelName, NiAVObject ** root, UInt32 * typeOut)
{
#ifdef _TRANSFORMS
	g_transformInterface.CacheSkeleton(modelName, *root);
#endif

	if (m_oldProcessor)
		m_oldProcessor->Process(modelData, modelName, root, typeOut);
}
