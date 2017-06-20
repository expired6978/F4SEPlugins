#pragma once

#include "f4se/GameEvents.h"

#include <unordered_set>

class Actor;

class ActorUpdateManager :
	public BSTEventSink<TESInitScriptEvent>,
	public BSTEventSink<TESObjectLoadedEvent>,
	public BSTEventSink<TESLoadGameEvent>
{
public:
	ActorUpdateManager() : m_loading(false) { }
	virtual ~ActorUpdateManager() { }

	virtual	EventResult	ReceiveEvent(TESObjectLoadedEvent * evn, void * dispatcher) override;
	virtual	EventResult	ReceiveEvent(TESLoadGameEvent * evn, void * dispatcher) override;
	virtual	EventResult	ReceiveEvent(TESInitScriptEvent * evn, void * dispatcher) override;

	virtual void PushUpdate(Actor * actor);
	virtual void Revert();

	void SetLoading(bool loading) { m_loading = loading; }
	void ResolvePendingBodyGen();

	SimpleLock					m_pendingLock;
	bool						m_loading;			// True when the game is loading, false when the cell has loaded
	std::unordered_set<UInt64>	m_pendingActors;	// Stores the pending actors while loading (Populated while loading, erased during load, remaining actors get new morphs, cleared after)
	std::unordered_set<UInt64>	m_pendingUpdates;	// Stores the actors for update
};