#include "ActorUpdateManager.h"
#include "BodyGenInterface.h"
#include "BodyMorphInterface.h"
#include "OverlayInterface.h"

#include "f4se/GameRTTI.h"
#include "f4se/GameObjects.h"
#include "f4se/GameReferences.h"

extern BodyGenInterface		g_bodyGenInterface;
extern BodyMorphInterface	g_bodyMorphInterface;
extern OverlayInterface		g_overlayInterface;

extern bool g_bEnableBodygen;
extern bool g_bEnableBodyMorphs;
extern bool g_bEnableOverlays;

EventResult	ActorUpdateManager::ReceiveEvent(TESObjectLoadedEvent * evn, void * dispatcher)
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

		m_pendingLock.Lock();
		if(m_loading) // We're mid-load, lets just push these to pending
		{
			m_pendingActors.insert((gender << 32) | form->formID);
		}
		else
		{
			// We've loaded the game, we can just generate and apply morphs if we don't already have any and we meet the outlined criteria for generation
			auto morphMap = g_bodyMorphInterface.GetMorphMap(actor, isFemale);
			if(!morphMap && g_bEnableBodygen)
			{
				if(g_bodyGenInterface.EvaluateBodyMorphs(actor, isFemale))
					morphMap = g_bodyMorphInterface.GetMorphMap(actor, isFemale);
			}
			if(morphMap && g_bEnableBodyMorphs)
			{
				g_bodyMorphInterface.UpdateMorphs(actor);
			}
			if(g_bEnableOverlays)
			{
				g_overlayInterface.UpdateOverlays(actor);
			}
		}
		m_pendingLock.Release();
	}

	return kEvent_Continue;
};

EventResult	ActorUpdateManager::ReceiveEvent(TESInitScriptEvent * evn, void * dispatcher)
{
	// Don't do any generation if BodyGen not enabled
	if(!g_bEnableBodygen)
		return kEvent_Continue;

	// We need to collect pending loads because these will fire before the load game event
	Actor * actor = DYNAMIC_CAST(evn->reference, TESForm, Actor);
	if(!actor)
		return kEvent_Continue;

	TESNPC * npc =  DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
	if(!npc)
		return kEvent_Continue;

	UInt64 gender = CALL_MEMBER_FN(npc, GetSex)();
	bool isFemale = gender == 1 ? true : false;

	if(!m_loading)
	{
		// We've spawned a new Reference, can we morph it?
		auto morphMap = g_bodyMorphInterface.GetMorphMap(actor, isFemale);
		if(!morphMap)
		{
			if(g_bodyGenInterface.EvaluateBodyMorphs(actor, isFemale))
				g_bodyMorphInterface.UpdateMorphs(actor);
		}
	}
	return kEvent_Continue;
}

void ActorUpdateManager::ResolvePendingBodyGen()
{
	// We don't need to generate any new morphs at this time
	if(g_bEnableBodygen)
	{
		m_pendingLock.Lock();
		for(auto & uid : m_pendingActors)
		{
			UInt8 gender = uid >> 32;
			UInt32 formID = uid & 0xFFFFFFFF;
			bool isFemale = gender == 1 ? true : false;

			TESForm * form = LookupFormByID(formID);
			if(form && form->formType == Actor::kTypeID)
			{
				Actor * actor = static_cast<Actor*>(form);
				auto morphMap = g_bodyMorphInterface.GetMorphMap(actor, isFemale);
				if(!morphMap)
				{
					if(g_bodyGenInterface.EvaluateBodyMorphs(actor, isFemale))
						m_pendingUpdates.emplace(formID);
				}
			}
		}

		m_pendingActors.clear();
		m_pendingLock.Release();
	}
}

EventResult ActorUpdateManager::ReceiveEvent(TESLoadGameEvent * evn, void * dispatcher)
{
	if(m_loading)
		m_loading = false;

	m_pendingLock.Lock();
	for(auto & uid : m_pendingUpdates)
	{
		TESForm * form = LookupFormByID(uid);
		if(form && form->formType == Actor::kTypeID)
		{
			Actor * actor = static_cast<Actor*>(form);
			if(g_bEnableBodyMorphs)
				g_bodyMorphInterface.UpdateMorphs(actor);
			if(g_bEnableOverlays)
				g_overlayInterface.UpdateOverlays(actor);
		}
	}

	m_pendingUpdates.clear();
	m_pendingLock.Release();

	return kEvent_Continue;
}

void ActorUpdateManager::PushUpdate(Actor * actor)
{
	m_pendingLock.Lock();
	m_pendingUpdates.emplace(actor->formID);
	m_pendingLock.Release();
}

void ActorUpdateManager::Revert()
{
	m_pendingLock.Lock();
	m_pendingActors.clear();
	m_pendingUpdates.clear();
	m_pendingLock.Release();
}