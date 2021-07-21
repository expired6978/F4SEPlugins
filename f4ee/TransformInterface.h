#pragma once

#include <mutex>
#include <functional>

#include "StringTable.h"
#include "f4se/NiTypes.h"
#include "f4se/GameThreads.h"
#include "f4se/BSModelDB.h"

class Actor;
class BGSKeyword;
class NiAVObject;

class TransformData
{
public:
	TransformData() : mFlags(kProperty_None) { }

	enum PropertyFlags
	{
		kProperty_None	= 0,
		kProperty_X		= (1 << 0),
		kProperty_Y		= (1 << 1),
		kProperty_Z		= (1 << 2),
		kProperty_Rot	= (1 << 3),
		kProperty_Scale	= (1 << 4)
	};

	bool IsEmpty() const { return mFlags == kProperty_None; }
	void SetX(float x) { if (x != 0.0f) { SetFlag(kProperty_X); } else { ClearFlag(kProperty_X); } mTransform.pos.x = x; }
	void SetY(float y) { if (y != 0.0f) { SetFlag(kProperty_Y); } else { ClearFlag(kProperty_Y); } mTransform.pos.y = y; }
	void SetZ(float z) { if (z != 0.0f) { SetFlag(kProperty_Z); } else { ClearFlag(kProperty_Z); } mTransform.pos.z = z; }
	void SetRotation(float pitch, float roll, float yaw) { if (pitch != 0.0f || roll != 0.0f || yaw != 0.0f) { SetFlag(kProperty_Rot); } else { ClearFlag(kProperty_Rot); } mTransform.rot.SetEulerAngles(pitch, roll, yaw); }
	void SetScale(float scale) { if (scale != 0.0f) { SetFlag(kProperty_Scale); } else { ClearFlag(kProperty_Scale); } mTransform.scale = scale; }

	NiTransform & GetTransform() { return mTransform; }

	void UpdateFlags()
	{
		if (mTransform.pos.x != 0.0f) { SetFlag(kProperty_X); } else { ClearFlag(kProperty_X); }
		if (mTransform.pos.y != 0.0f) { SetFlag(kProperty_Y); } else { ClearFlag(kProperty_Y); }
		if (mTransform.pos.z != 0.0f) { SetFlag(kProperty_Z); } else { ClearFlag(kProperty_Z); }
		bool isNonZero = false; for (int i = 0; i < 3 && !isNonZero; ++i) { for (int j = 0; j < 3 && !isNonZero; ++j) { if (mTransform.rot.data[i][j] != 0.0f) { isNonZero = true; break; } } }
		if (isNonZero) { SetFlag(kProperty_Rot); } else { ClearFlag(kProperty_Rot); }
		if (mTransform.scale != 0.0f) { SetFlag(kProperty_Scale); } else { ClearFlag(kProperty_Scale); }
	}

protected:
	void SetFlag(UInt16 flag) { mFlags |= flag; }
	void ClearFlag(UInt16 flag) { mFlags &= ~flag; }

private:
	UInt16 mFlags; // Determine what properties to write
	NiTransform mTransform;
};
typedef std::shared_ptr<TransformData> TransformDataPtr;

// Reflects Papyrus output
struct TransformIdentifier
{
	TransformIdentifier(bool _isFemale, bool _isFirstPerson, const F4EEFixedString & _node, BGSKeyword * _keyword, TransformDataPtr _transform)
		: isFemale(_isFemale)
		, isFirstPerson(_isFirstPerson)
		, node(_node)
		, keyword(_keyword)
		, transform(_transform)
	{}

	bool isFemale;
	bool isFirstPerson;
	F4EEFixedString node;
	BGSKeyword * keyword;
	TransformDataPtr transform;
};

// [Male/Female][FP/3P]
class ActorData
{
public:
	enum Genders
	{
		kGender_Male = 0,
		kGender_Female,
		kGender_Num
	};
	enum Perspectives
	{
		kPerspective_First = 0,
		kPerspective_Third,
		kPerspective_Num
	};

	TransformDataPtr SetTransformData(bool isFemale, bool isFirstPerson, const TransformData & data);
	void GetTransformData(bool isFemale, bool isFirstPerson, TransformDataPtr & data);
	bool ClearTransform(bool isFemale, bool isFirstPerson);

	bool IsEmpty() const {
		for (int i = 0; i < kGender_Num; ++i)
			for (int j = 0; j < kPerspective_Num; ++j)
				if (mTransform[i][j])
					return false;
		return true;
	}

private:
	TransformDataPtr mTransform[kGender_Num][kPerspective_Num];
};

// [FormID,TransformData]
class TransformMap : protected std::unordered_map<UInt32, ActorData>
{
public:
	void SetTransformData(bool isFemale, bool isFirstPerson, BGSKeyword * keyword, const TransformData & data);
	void GetTransformData(bool isFemale, bool isFirstPerson, BGSKeyword * keyword, TransformDataPtr & data);

	void ForEachTransform(bool isFemale, bool isFirstPerson, std::function<void(BGSKeyword*, TransformDataPtr&)> functor);

	bool ClearTransform(bool isFemale, bool isFirstPerson, BGSKeyword * keyword);
	void ClearTransforms(bool isFemale, bool isFirstPerson);

	bool IsEmpty() const { return empty(); }

	void GetMergedTransformData(bool isFemale, bool isFirstPerson, TransformDataPtr & data);

protected:
	// Flattens all transforms in this map to a single transform
	void CalculateMergedTransform(bool isFemale, bool isFirstPerson);

private:
	ActorData mMerged; // This is the merged result that is calculated whenever transforms are added/removed
};

// [String,TransformMap]
class TransformNodeMap : protected std::unordered_map<StringTableItem, TransformMap>
{
public:
	void SetNodeTransform(bool isFemale, bool isFirstPerson, const F4EEFixedString & node, BGSKeyword * keyword, const TransformData & data);
	void GetNodeTransform(bool isFemale, bool isFirstPerson, const F4EEFixedString & node, BGSKeyword * keyword, TransformDataPtr & data);

	// Used to accumulate all transforms on this node
	void ForEachTransform(bool isFemale, bool isFirstPerson, const F4EEFixedString & node, std::function<void(BGSKeyword*, TransformDataPtr&)> functor);
	void ForEachNode(bool isFemale, bool isFirstPerson, std::function<void(const F4EEFixedString &, BGSKeyword*, TransformDataPtr&)> functor);
	void ForEachNodeResult(bool isFemale, bool isFirstPerson, std::function<void(const F4EEFixedString &, TransformDataPtr&)> functor);

	bool ClearTransform(bool isFemale, bool isFirstPerson, const F4EEFixedString & node, BGSKeyword * keyword);
	void ClearNodeTransforms(bool isFemale, bool isFirstPerson, const F4EEFixedString & node);
	void ClearTransforms(bool isFemale, bool isFirstPerson);

	bool IsEmpty() const { return empty(); }
};

class F4EETransformUpdate : public ITaskDelegate
{
public:
	F4EETransformUpdate(TESForm * form);
	virtual ~F4EETransformUpdate() { };
	virtual void Run() override;

protected:
	UInt32					m_formId;
};

class NodeTransformCache : public std::unordered_map<F4EEFixedString, std::unordered_map<F4EEFixedString, NiTransform>>
{
	friend class NiTransformInterface;
public:
	typedef std::unordered_map<F4EEFixedString, NiTransform> NodeMap;
	typedef std::unordered_map<F4EEFixedString, NodeMap> RegMap;

	NiTransform * GetBaseTransform(F4EEFixedString rootModel, F4EEFixedString nodeName, bool relative);
};

class TransformProcessor : public BSModelDB::BSModelProcessor
{
public:
	TransformProcessor(BSModelDB::BSModelProcessor * oldProcessor) : m_oldProcessor(oldProcessor) { }

	virtual void Process(BSModelDB::ModelData * modelData, const char * modelName, NiAVObject ** root, UInt32 * typeOut);

	DEFINE_STATIC_HEAP(Heap_Allocate, Heap_Free)

protected:
	BSModelDB::BSModelProcessor	* m_oldProcessor;
};

class NiTransformInterface : protected std::unordered_map<UInt32, TransformNodeMap>
{
	friend class F4EETransformUpdate;
	friend class TransformProcessor;
public:
	enum
	{
		kVersion1 = 1,
		kSerializationVersion = kVersion1,
	};

	void SetTransform(Actor * actor, bool isFemale, bool isFirstPerson, const F4EEFixedString & node, BGSKeyword * keyword, const TransformData & data);

	// Acquire specific transform by full key
	void GetTransform(Actor * actor, bool isFemale, bool isFirstPerson, const F4EEFixedString & node, BGSKeyword * keyword, TransformDataPtr & data);

	// Acquire all transforms by node
	void GetActorNodeTransforms(Actor * actor, bool isFemale, bool isFirstPerson, const F4EEFixedString & node, std::vector<TransformIdentifier> & data);

	// Acquire all transforms for gender and perspective
	void GetActorGenderAndPerspectiveTransforms(Actor * actor, bool isFemale, bool isFirstPerson, std::vector<TransformIdentifier> & data);

	// Acquire all transforms for gender
	void GetActorGenderTransforms(Actor * actor, bool isFemale, std::vector<TransformIdentifier> & data);

	// Acquire all transforms for actor
	void GetActorTransforms(Actor * actor, std::vector<TransformIdentifier> & data);

	// Clear specific transform
	bool ClearTransform(Actor * actor, bool isFemale, bool isFirstPerson, const F4EEFixedString & node, BGSKeyword * keyword);

	// Clear all transforms for specific node
	void ClearActorNodeTransforms(Actor * actor, bool isFemale, bool isFirstPerson, const F4EEFixedString & node);

	// Clear all transforms for specific actor and perspective
	void ClearActorGenderAndPerspectiveTransforms(Actor * actor, bool isFemale, bool isFirstPerson);

	// Clear all transforms for specific gender
	void ClearActorGenderTransforms(Actor * actor, bool isFemale);

	// Clear all transforms for the entire actor
	void ClearActorTransforms(Actor * actor);

	// Applies all in-memory transforms to the actor
	void UpdateActorTransforms(Actor * actor);

	// Iterates every transform by node name
	void ForEachActorNodeTransform(Actor * actor, bool isFemale, bool isFirstPerson, std::function<void(const F4EEFixedString &, TransformDataPtr&)> functor);

	void SetModelProcessor();

	void LoadAllSkeletons();

	F4EEFixedString GetRootModelPath(Actor * refr, bool firstPerson, bool isFemale);
	void RevertSkeleton(Actor * actor, NiAVObject * rootNode, bool isFemale, bool isFirstPerson);
	void CacheSkeleton(const char * modelName, NiAVObject * root);

private:
	NodeTransformCache mTransformCache;
	mutable std::mutex mMutex;
};
