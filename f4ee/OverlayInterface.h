#pragma once

class Actor;
class NiAVObject;
class NiNode;
class BGSKeyword;
class BSTriShape;

#include <map>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <stack>

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
	UInt32					m_uid;
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
	OverlayInterface() : m_loading(false) { }

	virtual	EventResult	ReceiveEvent(TESObjectLoadedEvent * evn, void * dispatcher);
	virtual	EventResult	ReceiveEvent(TESLoadGameEvent * evn, void * dispatcher);

	typedef UInt32 UniqueID;

	enum
	{
		kSerializationVersion = 1,
	};

	class OverlayData
	{
	public:
		OverlayData()
		{
			uid = 0;
			tintColor.r = 0.0f;
			tintColor.g = 0.0f;
			tintColor.b = 0.0f;
			tintColor.a = 0.0f;
			offsetUV.x = 0.0f;
			offsetUV.y = 0.0f;
			scaleUV.x = 1.0f;
			scaleUV.y = 1.0f;
		}

		UniqueID		uid;
		StringTableItem	templateName;
		NiColorA		tintColor;
		NiPoint2		offsetUV;
		NiPoint2		scaleUV;
		

		void Save(const F4SESerializationInterface * intfc, UInt32 kVersion);
		bool Load(const F4SESerializationInterface * intfc, UInt32 kVersion, const std::unordered_map<UInt32, StringTableItem> & stringTable);
	};
	typedef std::shared_ptr<OverlayData> OverlayDataPtr;

	class PriorityMap : public std::multimap<SInt32, OverlayDataPtr>
	{
	public:
		void Save(const F4SESerializationInterface * intfc, UInt32 kVersion);
		bool Load(const F4SESerializationInterface * intfc, bool isFemale, UInt32 kVersion, const std::unordered_map<UInt32, StringTableItem> & stringTable);
	};
	typedef std::shared_ptr<PriorityMap> PriorityMapPtr;

	class OverlayMap : public std::unordered_map<UInt64, PriorityMapPtr>
	{
	public:
		void Save(const F4SESerializationInterface * intfc, UInt32 kVersion);
		bool Load(const F4SESerializationInterface * intfc, bool isFemale, UInt32 kVersion, const std::unordered_map<UInt32, StringTableItem> & stringTable);
	};
	
	class OverlayTemplate
	{
	public:
		OverlayTemplate() : playable(false), sort(0), transformable(false) { }

		F4EEFixedString								displayName;
		std::unordered_map<UInt32, F4EEFixedString> slotMaterial;
		bool										playable;
		bool										transformable;
		SInt32										sort;
	};
	typedef std::shared_ptr<OverlayTemplate> OverlayTemplatePtr;


	virtual void Save(const F4SESerializationInterface * intfc, UInt32 kVersion);
	virtual bool Load(const F4SESerializationInterface * intfc, UInt32 kVersion, const std::unordered_map<UInt32, StringTableItem> & stringTable);
	virtual void Revert();

	virtual void LoadOverlayMods();
	virtual void ClearMods()
	{
		m_overlayTemplates[0].clear();
		m_overlayTemplates[1].clear();
	}
	bool LoadOverlayTemplates(const std::string & filePath);

	virtual UniqueID AddOverlay(Actor * actor, bool isFemale, SInt32 priority, const F4EEFixedString & templateName, const NiColorA & tintColor, const NiPoint2 & offsetUV, const NiPoint2 & scaleUV);
	virtual bool RemoveOverlay(Actor * actor, bool isFemale, UniqueID uid);
	virtual bool RemoveAll(Actor * actor, bool isFemale);
	virtual bool ReorderOverlay(Actor * actor, bool isFemale, UniqueID uid, SInt32 newPriority);
	
	virtual bool ForEachOverlay(Actor * actor, bool isFemale, std::function<void(SInt32, const OverlayDataPtr&)> functor);
	virtual bool ForEachOverlayBySlot(Actor * actor, bool isFemale, UInt32 slotIndex, std::function<void(SInt32, const OverlayDataPtr&, const F4EEFixedString &)> functor);

	virtual void ForEachOverlayTemplate(bool isFemale, std::function<void(const F4EEFixedString&, const OverlayTemplatePtr&)> functor);

	virtual bool UpdateOverlays(Actor * actor);
	virtual bool UpdateOverlay(Actor * actor, UInt32 slotIndex);

	UniqueID GetNextUID();

	NiNode * GetOverlayRoot(Actor * actor, NiNode * rootNode, bool createIfNecessary = true);

	const OverlayTemplatePtr GetTemplateByName(bool isFemale, const F4EEFixedString& name);
	const OverlayDataPtr GetOverlayByUID(UniqueID uid);

	std::pair<SInt32, OverlayDataPtr> GetActorOverlayByUID(Actor * actor, bool isFemale, UniqueID uid);

	bool HasSkinChildren(NiAVObject * slot);
	void LoadMaterialData(BSTriShape * shape, const F4EEFixedString & material, const OverlayDataPtr & overlayData);

	void DestroyOverlaySlot(Actor * actor, NiNode * overlayHolder, UInt32 slotIndex);
	bool UpdateOverlays(Actor * actor, NiNode * rootNode, NiAVObject * object, UInt32 slotIndex);

	void SetLoading(bool loading) { m_loading = true; }

protected:
	friend class OverlayTemplate;
	friend class PriorityMap;
	friend class OverlayData;

	bool						m_loading;
	std::unordered_set<UInt64>	m_pendingUpdates;	// Stores the actors for update

	SimpleLock												m_overlayLock;
	OverlayMap												m_overlays[2];
	std::vector<UniqueID>									m_freeIndices;
	std::unordered_map<UniqueID, OverlayDataPtr>			m_dataMap;
	std::unordered_map<F4EEFixedString, OverlayTemplatePtr> m_overlayTemplates[2];
};