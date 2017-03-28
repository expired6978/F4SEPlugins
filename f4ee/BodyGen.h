#pragma once

#include "f4se/BSModelDB.h"
#include "f4se/GameTypes.h"

#include "f4se/NiTypes.h"
#include "f4se/NiExtraData.h"

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <ctime>
#include <map>
#include <functional>

#include <json/json.h>

#include "StringTable.h"
#include "f4se/PapyrusVM.h"
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

	BSFixedString	morph;
	BSFixedString	name;
	UInt8			gender;
	float			minimum;
	float			maximum;
	float			interval;
};
typedef std::shared_ptr<BodySlider> BodySliderPtr;

class MorphableShape
{
public:
	MorphableShape(NiAVObject * _object, const BSFixedString & _morphPath, const BSFixedString & _shapeName) : object(_object), morphPath(_morphPath), shapeName(_shapeName) { }

	NiPointer<NiAVObject>	object;
	BSFixedString			morphPath;
	BSFixedString			shapeName;
};
typedef std::shared_ptr<MorphableShape> MorphableShapePtr;

class F4EEBodyGenUpdateShape : public ITaskDelegate
{
public:
	F4EEBodyGenUpdateShape(TESForm * form, NiAVObject * object);
	virtual ~F4EEBodyGenUpdateShape();
	virtual void Run() override;

protected:
	UInt32					m_formId;
	NiPointer<NiAVObject>	m_object;
};

class BodyGen
{
public:
	BodyGen() : m_totalMemory(0), m_memoryLimit(0x80000000LL) { } // 2GB

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
	virtual bool UpdateMorphs(Actor * actor);
	virtual void SetupDynamicMeshes();
	virtual bool SetModelAsDynamic(TESModel * model);

	virtual void ProcessModel(BSModelDB::ModelData * modelData, const char * modelName, NiAVObject * root);

	void ShrinkMorphCache();
	void SetCacheLimit(UInt64 limit);

	/*void SetDescriptor(const BSFixedString & tri, const BSFixedString & shape, UInt64 vDesc)
	{
		m_descriptorMap[tri][shape] = vDesc;
	}*/

	template<typename T>
	UInt64 GetHandleFromObject(T * src)
	{
		VirtualMachine		* vm =	(*g_gameVM)->m_virtualMachine;
		IObjectHandlePolicy	* policy =		vm->GetHandlePolicy();

		return policy->Create(T::kTypeID, (T*)src);
	}

	template<typename T>
	T * GetObjectFromHandle(UInt64 handle)
	{
		VirtualMachine		* vm =	(*g_gameVM)->m_virtualMachine;
		IObjectHandlePolicy	* policy =		vm->GetHandlePolicy();

		if(handle == policy->GetInvalidHandle()) {
			return NULL;
		}

		return (T*)policy->Resolve(T::kTypeID, handle);
	}

private:
	SimpleLock											m_morphLock;
	std::unordered_map<UInt64, MorphValueMapPtr>		m_morphMap[2];

	SimpleLock											m_morphCacheLock;
	std::unordered_map<F4EEFixedString, TriShapeMapPtr>	m_morphCache;
	UInt64												m_totalMemory;
	UInt64												m_memoryLimit;

	//std::unordered_map<BSFixedString, std::unordered_map<BSFixedString, UInt64>> m_descriptorMap;

	std::map<F4EEFixedString, BodySliderPtr>				m_sliderMap[2];

	ICriticalSection	m_bodyGenLock;
};