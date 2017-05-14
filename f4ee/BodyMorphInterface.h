#pragma once

#include "f4se/BSModelDB.h"
#include "f4se/GameTypes.h"
#include "f4se/GameEvents.h"

#include "f4se/NiTypes.h"
#include "f4se/NiExtraData.h"

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <ctime>
#include <map>
#include <functional>

#include <json/json.h>

#include "StringTable.h"
#include "f4se/PapyrusVM.h"
#include "f4se/PapyrusUtilities.h"
#include "f4se/GameThreads.h"

#include "common/ICriticalSection.h"

class Actor;
class BGSKeyword;
struct F4SESerializationInterface;
class TESModel;

class TriShapeVertexDelta
{
public:
	UInt16		index;
	NiPoint3	diff;
};

class TriShapePackedVertexDelta
{
public:
	UInt16	index;
	SInt16	x;
	SInt16	y;
	SInt16	z;
};

class TriShapeVertexData
{
public:
	virtual void ApplyMorph(UInt16 vertCount, NiPoint3 * vertices, float factor) = 0;
};
typedef std::shared_ptr<TriShapeVertexData> TriShapeVertexDataPtr;

class TriShapeFullVertexData : public TriShapeVertexData
{
public:
	virtual void ApplyMorph(UInt16 vertCount, NiPoint3 * vertices, float factor);

	std::vector<TriShapeVertexDelta> m_vertexDeltas;
};
typedef std::shared_ptr<TriShapeFullVertexData> TriShapeFullVertexDataPtr;

class TriShapePackedVertexData : public TriShapeVertexData
{
public:
	virtual void ApplyMorph(UInt16 vertCount, NiPoint3 * vertices, float factor);

	float									m_multiplier;
	std::vector<TriShapePackedVertexDelta>	m_vertexDeltas;
};
typedef std::shared_ptr<TriShapePackedVertexData> TriShapePackedVertexDataPtr;

class BodyMorphMap : public std::unordered_map<F4EEFixedString, TriShapeVertexDataPtr>
{
public:
	virtual TriShapeVertexDataPtr GetVertexData(const F4EEFixedString & name);

protected:
	SimpleLock	m_morphLock;
};
typedef std::shared_ptr<BodyMorphMap> BodyMorphMapPtr;

// Maps Shape name to Morphs
class TriShapeMap : public std::unordered_map<F4EEFixedString, BodyMorphMapPtr>
{
public:
	TriShapeMap()
	{
		memoryUsage = sizeof(TriShapeMap);
		accessed = 0;
	}
	virtual BodyMorphMapPtr GetMorphData(const F4EEFixedString & name);

	SimpleLock	m_morphLock;
	UInt32 memoryUsage;
	std::time_t accessed;
};
typedef std::shared_ptr<TriShapeMap> TriShapeMapPtr;


// Maps keyword to value
class UserValues : public std::unordered_map<UInt64, float>
{
public:
	float GetValue(BGSKeyword * keyword);
	void SetValue(BGSKeyword * keyword, float value);

	float GetEffectiveValue();

	void RemoveKeyword(BGSKeyword * keyword);

	void Revert()
	{
		SimpleLocker locker(&m_morphLock);
		clear();
	}

protected:
	SimpleLock	m_morphLock;
};
typedef std::shared_ptr<UserValues> UserValuesPtr;

// Maps morph name to user values
class MorphValueMap : public std::unordered_map<StringTableItem, UserValuesPtr>
{
public:
	virtual void Save(const F4SESerializationInterface * intfc, UInt32 kVersion);
	virtual bool Load(const F4SESerializationInterface * intfc, UInt32 kVersion, const std::unordered_map<UInt32, StringTableItem> & stringTable);

	void SetMorph(const BSFixedString &morph, BGSKeyword * keyword, float value);
	float GetMorph(const BSFixedString & morph, BGSKeyword * keyword);

	void GetKeywords(const BSFixedString & morph, std::vector<BGSKeyword*> & keywords);

	void RemoveMorphsByName(const BSFixedString & morph);
	void RemoveMorphsByKeyword(BGSKeyword * keyword);

	void Revert()
	{
		SimpleLocker locker(&m_morphLock);
		clear();
	}

protected:
	SimpleLock	m_morphLock;
};
typedef std::shared_ptr<MorphValueMap> MorphValueMapPtr;

class BodySlider
{
public:
	virtual bool Parse(const Json::Value & entry);

	F4EEFixedString	morph;
	F4EEFixedString	name;
	UInt8			gender;
	UInt32			sort;
	float			minimum;
	float			maximum;
	float			interval;
};
typedef std::shared_ptr<BodySlider> BodySliderPtr;

class MorphableShape
{
public:
	MorphableShape(NiAVObject * _object, const F4EEFixedString & _morphPath, const F4EEFixedString & _shapeName) : object(_object), morphPath(_morphPath), shapeName(_shapeName) { }

	NiPointer<NiAVObject>	object;
	F4EEFixedString			morphPath;
	F4EEFixedString			shapeName;
};
typedef std::shared_ptr<MorphableShape> MorphableShapePtr;

class F4EEBodyGenUpdate : public ITaskDelegate
{
public:
	F4EEBodyGenUpdate(TESForm * form, bool doEquipment, bool doQueue);
	virtual ~F4EEBodyGenUpdate() { };
	virtual void Run() override;

protected:
	UInt32					m_formId;
	bool					m_doEquipment;
	bool					m_doQueue;
};

class BodyMorphProcessor : public BSModelDB::BSModelProcessor
{
public:
	BodyMorphProcessor(BSModelDB::BSModelProcessor * oldProcessor) : m_oldProcessor(oldProcessor) { }

	virtual void Process(BSModelDB::ModelData * modelData, const char * modelName, NiAVObject ** root, UInt32 * typeOut);

	DEFINE_STATIC_HEAP(Heap_Allocate, Heap_Free)

protected:
	BSModelDB::BSModelProcessor	* m_oldProcessor;
};

class BodyMorphInterface :	public BSTEventSink<TESObjectLoadedEvent>,
							public BSTEventSink<TESLoadGameEvent>,
							public BSTEventSink<TESInitScriptEvent>
{
public:
	BodyMorphInterface() : m_totalMemory(0), m_memoryLimit(0x80000000LL), m_loading(false) { } // 2GB
	
	virtual	EventResult	ReceiveEvent(TESObjectLoadedEvent * evn, void * dispatcher);
	virtual	EventResult	ReceiveEvent(TESLoadGameEvent * evn, void * dispatcher);
	virtual	EventResult	ReceiveEvent(TESInitScriptEvent * evn, void * dispatcher);

	enum
	{
		kSerializationVersion = 1,
	};

	virtual void Save(const F4SESerializationInterface * intfc, UInt32 kVersion);
	virtual bool Load(const F4SESerializationInterface * intfc, bool isFemale, UInt32 kVersion, const std::unordered_map<UInt32, StringTableItem> & stringTable);
	virtual void Revert();

	virtual void LoadBodyGenSliderMods();
	virtual void ClearBodyGenSliders();

	virtual bool LoadBodyGenSliders(const std::string & filePath);
	virtual void ForEachSlider(UInt8 gender, std::function<void(const BodySliderPtr & slider)> func);

	virtual TriShapeMapPtr GetTrishapeMap(const char * relativePath);
	virtual MorphValueMapPtr GetMorphMap(Actor * actor, bool isFemale);

	virtual void SetMorph(Actor * actor, bool isFemale, const BSFixedString & morph, BGSKeyword * keyword, float value);
	virtual float GetMorph(Actor * actor, bool isFemale, const BSFixedString & morph, BGSKeyword * keyword);

	virtual void GetKeywords(Actor * actor, bool isFemale, const BSFixedString & morph, std::vector<BGSKeyword*> & keywords);
	virtual void GetMorphs(Actor * actor, bool isFemale, std::vector<BSFixedString> & morphs);
	virtual void RemoveMorphsByName(Actor * actor, bool isFemale, const BSFixedString & morph);
	virtual void RemoveMorphsByKeyword(Actor * actor, bool isFemale, BGSKeyword * keyword);
	virtual void ClearMorphs(Actor * actor, bool isFemale);

	// Not a deep copy, will be a shallow copy, editing on the target edits on the source
	virtual void CloneMorphs(Actor * source, Actor * target);

	virtual void GetMorphableShapes(NiAVObject * node, std::vector<MorphableShapePtr> & shapes);
	virtual bool ApplyMorphsToShapes(Actor * actor, NiAVObject * slotNode);
	virtual bool ApplyMorphsToShape(Actor * actor, const MorphableShapePtr & morphableShape);
	virtual bool UpdateMorphs(Actor * actor, bool doEquipment, bool doQueue);

	bool IsNodeMorphable(NiAVObject * rootNode);

	void ShrinkMorphCache();
	void SetCacheLimit(UInt64 limit);
	void SetModelProcessor();
	void SetLoading(bool loading) { m_loading = loading; };
	void ResolvePendingMorphs();

	template<typename T>
	UInt64 GetHandleFromObject(T * src)
	{
		return PapyrusVM::GetHandleFromObject((T*)src, T::kTypeID);
	}

	template<typename T>
	T * GetObjectFromHandle(UInt64 handle)
	{
		return (T*)PapyrusVM::GetObjectFromHandle(handle, T::kTypeID);
	}

private:
	SimpleLock											m_pendingLock;
	bool												m_loading;			// True when the game is loading, false when the cell has loaded
	std::unordered_set<UInt64>							m_pendingMorphs;	// Stores the pending actors while loading (Populated while loading, erased during load, remaining actors get new morphs, cleared after)
	std::unordered_set<UInt64>							m_pendingUpdates;	// Stores the actors for update

	SimpleLock											m_morphLock;
	std::unordered_map<UInt64, MorphValueMapPtr>		m_morphMap[2];

	SimpleLock											m_morphCacheLock;
	std::unordered_map<F4EEFixedString, TriShapeMapPtr>	m_morphCache;
	UInt64												m_totalMemory;
	UInt64												m_memoryLimit;

	std::unordered_map<F4EEFixedString, BodySliderPtr>	m_sliderMap[2];
};