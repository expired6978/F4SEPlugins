#pragma once

class Actor;
class NiAVObject;
class NiNode;
class BGSKeyword;

#include <map>
#include <unordered_map>
#include <functional>

#include "f4se/GameTypes.h"
#include "f4se/NiTypes.h"
#include "StringTable.h"

#include "f4se/GameThreads.h"
#include "f4se/GameEvents.h"

class F4EEOverlayUpdate : public ITaskDelegate
{
public:
	F4EEOverlayUpdate(TESForm * form, UInt32 slot);
	virtual ~F4EEOverlayUpdate() { };
	virtual void Run() override;

protected:
	UInt32					m_formId;
	UInt32					m_slot;
};

class F4EEUpdateOverlays : public ITaskDelegate
{
public:
	F4EEUpdateOverlays(TESForm * form);
	virtual ~F4EEUpdateOverlays() { };
	virtual void Run() override;

protected:
	UInt32					m_formId;
};

class OverlayInterface : public BSTEventSink<TESObjectLoadedEvent>,
						 public BSTEventSink<TESLoadGameEvent>
{
public:
	virtual	EventResult	ReceiveEvent(TESObjectLoadedEvent * evn, void * dispatcher);
	virtual	EventResult	ReceiveEvent(TESLoadGameEvent * evn, void * dispatcher);

	enum
	{
		kSerializationVersion = 1,
	};

	class OverlayData
	{
	public:
		StringTableItem	m_materialPath;
		UInt64			m_keyword;
		NiColorA		m_tintColor;

		void Save(const F4SESerializationInterface * intfc, UInt32 kVersion);
		bool Load(const F4SESerializationInterface * intfc, UInt32 kVersion, const std::unordered_map<UInt32, StringTableItem> & stringTable);
	};
	typedef std::shared_ptr<OverlayData> OverlayDataPtr;

	class PriorityMap : public std::multimap<SInt32, OverlayDataPtr>
	{
	public:
		void Save(const F4SESerializationInterface * intfc, UInt32 kVersion);
		bool Load(const F4SESerializationInterface * intfc, UInt32 kVersion, const std::unordered_map<UInt32, StringTableItem> & stringTable);
	};
	typedef std::shared_ptr<PriorityMap> PriorityMapPtr;

	class SlotMap : public std::unordered_map<UInt32, PriorityMapPtr>
	{
	public:
		void Save(const F4SESerializationInterface * intfc, UInt32 kVersion);
		bool Load(const F4SESerializationInterface * intfc, UInt32 kVersion, const std::unordered_map<UInt32, StringTableItem> & stringTable);
	};
	typedef std::shared_ptr<SlotMap> SlotMapPtr;

	typedef std::unordered_map<UInt64, SlotMapPtr> OverlayMap;

	virtual void Save(const F4SESerializationInterface * intfc, UInt32 kVersion);
	virtual bool Load(const F4SESerializationInterface * intfc, bool isFemale, UInt32 kVersion, const std::unordered_map<UInt32, StringTableItem> & stringTable);
	virtual void Revert();

	virtual void AddOverlay(Actor * actor, bool isFemale, UInt32 slotIndex, SInt32 priority, BGSKeyword * keyword, const F4EEFixedString & materialPath, NiColorA tintColor);
	virtual bool RemoveOverlay(Actor * actor, bool isFemale, UInt32 slotIndex, SInt32 priority, BGSKeyword * keyword, BSFixedString materialPath);
	virtual bool RemoveAll(Actor * actor, bool isFemale);
	virtual bool ReorderOverlay(Actor * actor, bool isFemale, UInt32 slotIndex, SInt32 priority, BGSKeyword * keyword, BSFixedString materialPath, SInt32 newPriority);
	
	virtual bool ForEachOverlay(Actor * actor, bool isFemale, std::function<void(UInt32, SInt32, const OverlayDataPtr&)> functor);
	virtual bool ForEachOverlayBySlot(Actor * actor, bool isFemale, UInt32 slotIndex, std::function<void(UInt32, SInt32, const OverlayDataPtr&)> functor);
	virtual bool ForEachOverlayByPriority(Actor * actor, bool isFemale, SInt32 priority, std::function<void(UInt32, SInt32, const OverlayDataPtr&)> functor);
	virtual bool ForEachOverlayByKeyword(Actor * actor, bool isFemale, BGSKeyword * keyword, std::function<void(UInt32, SInt32, const OverlayDataPtr&)> functor);

	virtual bool UpdateOverlays(Actor * actor);
	virtual bool UpdateOverlay(Actor * actor, UInt32 slotIndex);



	NiNode * GetOverlayRoot(Actor * actor, NiNode * rootNode, bool createIfNecessary = true);

	bool HasSkinChildren(NiAVObject * slot);

	void DestroyOverlaySlot(Actor * actor, NiNode * overlayHolder, UInt32 slotIndex);
	bool UpdateOverlays(Actor * actor, NiNode * rootNode, NiAVObject * object, UInt32 slotIndex);

	SimpleLock	m_overlayLock;
	OverlayMap	m_overlays[2];


};