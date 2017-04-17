#include "BodyGen.h"

#include "f4se/GameData.h"
#include "f4se/GameStreams.h"
#include "f4se/GameTypes.h"
#include "f4se/GameReferences.h"
#include "f4se/GameRTTI.h"
#include "f4se/GameObjects.h"

#include "f4se/NiNodes.h"
#include "f4se/NiObjects.h"
#include "f4se/NiExtraData.h"
#include "f4se/BSGeometry.h"

#include "f4se/PluginAPI.h"
#include "Utilities.h"

#include <regex>
#include <algorithm>
#include <ppl.h>
#include <atomic>
#include <chrono>

extern BodyGen g_bodyGen;
extern StringTable g_stringTable;
extern bool g_bEnableBodygen;
extern bool g_bUseTaskInterface;
extern bool g_bParallelShapes;
extern F4SETaskInterface * g_task;

using namespace Serialization;

void TriShapeFullVertexData::ApplyMorph(UInt16 vertCount, NiPoint3 * vertices, float factor)
{
	if (!vertices)
		return;

	UInt32 size = m_vertexDeltas.size();
	for (UInt32 i = 0; i < size; i++)
	{
		TriShapeVertexDelta * vert = &m_vertexDeltas.at(i);
		UInt16 vertexIndex = vert->index;
		NiPoint3 * vertexDiff = &vert->diff;
		if (vertexIndex < vertCount)
		{
			vertices[vertexIndex].x += vertexDiff->x * factor;
			vertices[vertexIndex].y += vertexDiff->y * factor;
			vertices[vertexIndex].z += vertexDiff->z * factor;
		}
		else {
			_DMESSAGE("%s - Vertex %d out of bounds X:%f Y:%f Z:%f", __FUNCTION__, vertexIndex, vertexDiff->x, vertexDiff->y, vertexDiff->z);
		}
	}
}

void TriShapePackedVertexData::ApplyMorph(UInt16 vertCount, NiPoint3 * vertices, float factor)
{
	if (!vertices)
		return;

	UInt32 size = m_vertexDeltas.size();
	for (UInt32 i = 0; i < size; i++)
	{
		TriShapePackedVertexDelta * vert = &m_vertexDeltas.at(i);

		UInt16 vertexIndex = vert->index;
		if (vertexIndex < vertCount)
		{
			vertices[vertexIndex].x += (float)vert->x * m_multiplier * factor;
			vertices[vertexIndex].y += (float)vert->y * m_multiplier * factor;
			vertices[vertexIndex].z += (float)vert->z * m_multiplier * factor;
		}
		else {
			_DMESSAGE("%s - Vertex %d out of bounds X:%f Y:%f Z:%f", __FUNCTION__, vertexIndex, vert->x, vert->y, vert->z);
		}
	}
}

TriShapeVertexDataPtr BodyMorphMap::GetVertexData(const F4EEFixedString & name)
{
	SimpleLocker locker(&m_morphLock);
	auto it = find(name);
	if(it != end()) {
		return it->second;
	}

	return nullptr;
}

BodyMorphMapPtr TriShapeMap::GetMorphData(const F4EEFixedString & name)
{
	SimpleLocker locker(&m_morphLock);
	auto it = find(name);
	if(it != end()) {
		return it->second;
	}

	return nullptr;
}

TriShapeMapPtr BodyGen::GetTrishapeMap(const char * relativePath)
{
	BSFixedString filePath(relativePath);
	if(relativePath == "")
		return nullptr;

	m_morphCacheLock.Lock();
	auto & it = m_morphCache.find(filePath);
	if (it != m_morphCache.end()) {
		it->second->accessed = std::time(nullptr);
		m_morphCacheLock.Release();
		return it->second;
	}
	m_morphCacheLock.Release();

#ifdef _DEBUG
	_MESSAGE("%s - Parsing: %s", __FUNCTION__, filePath.c_str());
#endif

	BSResourceNiBinaryStream binaryStream(filePath);
	if(binaryStream.IsValid())
	{
		TriShapeMapPtr trishapeMap = std::make_shared<TriShapeMap>();

		UInt32 fileFormat = 0;
		trishapeMap->memoryUsage += binaryStream.Read((char *)&fileFormat, sizeof(UInt32));

		bool packed = false;
		if (fileFormat != 'TRI\0' && fileFormat != 'TRIP')
			return nullptr;

		if (fileFormat == 'TRIP')
			packed = true;

		UInt32 trishapeCount = 0;
		if (!packed)
			trishapeMap->memoryUsage += binaryStream.Read((char *)&trishapeCount, sizeof(UInt32));
		else
			trishapeMap->memoryUsage += binaryStream.Read((char *)&trishapeCount, sizeof(UInt16));

		char trishapeNameRaw[MAX_PATH];
		for (UInt32 i = 0; i < trishapeCount; i++)
		{
			memset(trishapeNameRaw, 0, MAX_PATH);

			UInt8 size = 0;
			trishapeMap->memoryUsage += binaryStream.Read((char *)&size, sizeof(UInt8));
			trishapeMap->memoryUsage += binaryStream.Read(trishapeNameRaw, size);
			F4EEFixedString trishapeName(trishapeNameRaw);

#ifdef _DEBUG
			_MESSAGE("%s - Reading TriShape %s", __FUNCTION__, trishapeName.c_str());
#endif

			if (!packed) {
				UInt32 trishapeBlockSize = 0;
				trishapeMap->memoryUsage += binaryStream.Read((char *)&trishapeBlockSize, sizeof(UInt32));
			}

			char morphNameRaw[MAX_PATH];

			BodyMorphMapPtr morphMap = std::make_shared<BodyMorphMap>();

			UInt32 morphCount = 0;
			if (!packed)
				trishapeMap->memoryUsage += binaryStream.Read((char *)&morphCount, sizeof(UInt32));
			else
				trishapeMap->memoryUsage += binaryStream.Read((char *)&morphCount, sizeof(UInt16));

			for (UInt32 j = 0; j < morphCount; j++)
			{
				memset(morphNameRaw, 0, MAX_PATH);

				UInt8 tsize = 0;
				trishapeMap->memoryUsage += binaryStream.Read((char *)&tsize, sizeof(UInt8));
				trishapeMap->memoryUsage += binaryStream.Read(morphNameRaw, tsize);
				F4EEFixedString morphName(morphNameRaw);

#ifdef _DEBUG
				_MESSAGE("%s - Reading Morph %s at (%08X)", __FUNCTION__, morphName.c_str(), binaryStream.GetOffset());
#endif
				if (tsize == 0) {
					_WARNING("%s - %s - Read empty name morph at (%08X)", __FUNCTION__, filePath.c_str(), binaryStream.GetOffset());
				}

				if (!packed) {
					UInt32 morphBlockSize = 0;
					trishapeMap->memoryUsage += binaryStream.Read((char *)&morphBlockSize, sizeof(UInt32));
				}

				UInt32 vertexNum = 0;
				float multiplier = 0.0f;
				if(!packed) {
					trishapeMap->memoryUsage += binaryStream.Read((char *)&vertexNum, sizeof(UInt32));
				}
				else {
					trishapeMap->memoryUsage += binaryStream.Read((char *)&multiplier, sizeof(float));
					trishapeMap->memoryUsage += binaryStream.Read((char *)&vertexNum, sizeof(UInt16));
				}

				if (vertexNum == 0) {
					_WARNING("%s - %s - Read morph %s on %s with no vertices at (%08X)", __FUNCTION__, filePath.c_str(), morphName.c_str(), trishapeName.c_str(), binaryStream.GetOffset());
				}
				if (multiplier == 0.0f) {
					_WARNING("%s - %s - Read morph %s on %s with zero multiplier at (%08X)", __FUNCTION__, filePath.c_str(), morphName.c_str(), trishapeName.c_str(), binaryStream.GetOffset());
				}

#ifdef _DEBUG
				_MESSAGE("%s - Total Vertices read: %d at (%08X)", __FUNCTION__, vertexNum, binaryStream.GetOffset());
#endif
				if (vertexNum > (std::numeric_limits<UInt16>::max)())
				{
					_ERROR("%s - %s - Too many vertices for %s on %s read: %d at (%08X)", __FUNCTION__, filePath.c_str(), morphName.c_str(), vertexNum, trishapeName.c_str(), binaryStream.GetOffset());
					return nullptr;
				}

				TriShapeVertexDataPtr vertexData;
				TriShapeFullVertexDataPtr fullVertexData;
				TriShapePackedVertexDataPtr packedVertexData;
				if (!packed)
				{
					fullVertexData = std::make_shared<TriShapeFullVertexData>();
					for (UInt32 k = 0; k < vertexNum; k++)
					{
						TriShapeVertexDelta vertexDelta;
						trishapeMap->memoryUsage += binaryStream.Read((char *)&vertexDelta.index, sizeof(UInt32));
						trishapeMap->memoryUsage += binaryStream.Read((char *)&vertexDelta.diff, sizeof(NiPoint3));
						fullVertexData->m_vertexDeltas.push_back(vertexDelta);
					}

					vertexData = fullVertexData;
				}
				else
				{
					packedVertexData = std::make_shared<TriShapePackedVertexData>();
					packedVertexData->m_multiplier = multiplier;

					for (UInt32 k = 0; k < vertexNum; k++)
					{
						TriShapePackedVertexDelta vertexDelta;
						trishapeMap->memoryUsage += binaryStream.Read((char *)&vertexDelta.index, sizeof(UInt16));
						trishapeMap->memoryUsage += binaryStream.Read((char *)&vertexDelta.x, sizeof(SInt16));
						trishapeMap->memoryUsage += binaryStream.Read((char *)&vertexDelta.y, sizeof(SInt16));
						trishapeMap->memoryUsage += binaryStream.Read((char *)&vertexDelta.z, sizeof(SInt16));

						packedVertexData->m_vertexDeltas.push_back(vertexDelta);
					}

					vertexData = packedVertexData;
				}

				morphMap->emplace(morphName, vertexData);
			}

			trishapeMap->emplace(trishapeName, morphMap);
			
		}

		trishapeMap->accessed = std::time(nullptr);

		m_morphCacheLock.Lock();
		m_morphCache.emplace(relativePath, trishapeMap);
		m_morphCacheLock.Release();

		m_totalMemory += trishapeMap->memoryUsage;

		_DMESSAGE("%s - Loaded %s (%d bytes)", __FUNCTION__, relativePath, trishapeMap->memoryUsage);
		return trishapeMap;
	}
	else
	{
		_ERROR("%s - Failed to load %s", __FUNCTION__, relativePath);
	}


	return nullptr;
}

void BodyGen::ShrinkMorphCache()
{
	m_morphCacheLock.Lock();
	while (m_totalMemory > m_memoryLimit && m_morphCache.size() > 0)
	{
		auto & it = std::min_element(m_morphCache.begin(), m_morphCache.end(), [](std::pair<F4EEFixedString, TriShapeMapPtr> a, std::pair<F4EEFixedString, TriShapeMapPtr> b)
		{
			return (a.second->accessed < b.second->accessed);
		});

		UInt32 size = it->second->memoryUsage;
		m_morphCache.erase(it);
		m_totalMemory -= size;
	}

	if (m_morphCache.size() == 0) // Just in case we erased but messed up
		m_totalMemory = sizeof(std::unordered_map<F4EEFixedString, TriShapeMapPtr>);
	m_morphCacheLock.Release();
}

void BodyGen::SetCacheLimit(UInt64 limit)
{
	m_memoryLimit = limit;
}

void BodyGen::LoadBodyGenSliderMods()
{
	for(int i = 0; i < (*g_dataHandler)->modList.loadedModCount; i++)
	{
		ModInfo * modInfo = (*g_dataHandler)->modList.loadedMods[i];
		std::string templatesPath = std::string("Data\\F4SE\\Plugins\\F4EE\\Sliders\\") + std::string(modInfo->name) + "\\sliders.json";
		LoadBodyGenSliders(templatesPath);
	}
}

bool BodyGen::LoadBodyGenSliders(const std::string & filePath)
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

		for(auto & item : root)
		{
			BodySliderPtr pBodySlider = std::make_shared<BodySlider>();
			if(pBodySlider->Parse(item)) {
				if(pBodySlider->gender == 0 || pBodySlider->gender == 1) {
					m_sliderMap[pBodySlider->gender][pBodySlider->morph] = pBodySlider;
				}
			}
		}
	}

	return true;
}

void BodyGen::ClearBodyGenSliders()
{
	m_sliderMap[0].clear();
	m_sliderMap[1].clear();
}

void BodyGen::ForEachSlider(UInt8 gender, std::function<void(const BodySliderPtr & slider)> func)
{
	if(gender == 0 || gender == 1)
	{
		for(auto & it : m_sliderMap[gender])
		{
			func(it.second);
		}
	}
}

bool BodySlider::Parse(const Json::Value & entry)
{
	try
	{
		name = entry["name"].asCString();
		morph = entry["morph"].asCString();
		gender = entry["gender"].asInt();
		minimum = entry["minimum"].asFloat();
		maximum = entry["maximum"].asFloat();
		interval = entry["interval"].asFloat();
		sort = entry["sort"].asInt();
	}
	catch(const std::exception& e)
	{
		_ERROR(e.what());
		return false;
	}

	return true;
}

void BodyGen::GetMorphableShapes(NiAVObject * rootNode, std::vector<MorphableShapePtr> & shapes)
{
	VisitObjects(rootNode, [&](NiAVObject * node)
	{
		BSTriShape * trishape = node->GetAsBSTriShape();
		if(trishape)
		{
			NiPointer<NiStringExtraData> bodyMorph(DYNAMIC_CAST(trishape->GetExtraData("MORPH_SHAPE"), NiExtraData, NiStringExtraData));
			if(!bodyMorph)
				return false;

			NiPointer<NiStringExtraData> morphPath(DYNAMIC_CAST(trishape->GetExtraData("MORPH_FILE"), NiExtraData, NiStringExtraData));
			if(!morphPath)
				return false;

			shapes.push_back(std::make_shared<MorphableShape>(trishape, morphPath->m_string, bodyMorph->m_string));
		}
		return false;
	});
}

F4EEBodyGenUpdate::F4EEBodyGenUpdate(TESForm * form)
{
	m_formId = form ? form->formID : 0;
}

void F4EEBodyGenUpdate::Run()
{
	TESForm * form = LookupFormByID(m_formId);
	if(form) {
		Actor * actor = DYNAMIC_CAST(form, TESForm, Actor);
		if(actor) {
			CALL_MEMBER_FN(actor, QueueUpdate)(true, 0, false, 0x0C);
		}
	}
}

#include "f4se/BSGraphics.h"
#include "Morpher.h"

bool BodyGen::ApplyMorphsToShape(Actor * actor, const MorphableShapePtr & morphableShape)
{
	// Don't allow dynamic shapes
	BSDynamicTriShape * dynamicShape = morphableShape->object->GetAsBSDynamicTriShape();
	if(dynamicShape)
		return false;

	BSTriShape * geometry = morphableShape->object->GetAsBSTriShape();
	if(geometry) {

		// Lookup the TRI file from the parsed path
		auto triMap = GetTrishapeMap(morphableShape->morphPath);
		if(!triMap) {
			return false;
		}

		ShrinkMorphCache();

		// Lookup the particular morph set for this shape
		auto morphMap = triMap->GetMorphData(morphableShape->shapeName);
		if(!morphMap) {
			return false;
		}

		bool isFemale = false;
		TESNPC * npc = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
		if(npc)
			isFemale = CALL_MEMBER_FN(npc, GetSex)() == 1 ? true : false;

		auto actorMorphs = GetMorphMap(actor, isFemale); // Get the actor's list of morphs
		if(!actorMorphs) // There's nothing to morph, lets just use the base mesh
			return false;

		UInt64 vertexDesc = geometry->vertexDesc;
		UInt32 vertexSize = geometry->GetVertexSize();
		UInt32 blockSize = geometry->numVertices * vertexSize;

		BSGeometryData * baseData = geometry->geometryData;
		BSGeometryData * geomData = nullptr;
		if(!baseData)
			return false;

		if(!(vertexDesc & BSGeometry::kFlag_Vertex)) // What kind of dumbass mesh doesn't have verts
			return false;

		// Create the cloned copy since we don't have a MORPH_DATA
		geomData = CALL_MEMBER_FN(g_renderManager, CreateBSGeometryData)(&blockSize, baseData->vertexData, geometry->vertexDesc, baseData->triangleData);
		if(!geomData)
			return false;

		// Copy from the base data to the newly created block
		memcpy(geomData->vertexData->vertexBlock, baseData->vertexData->vertexBlock, geometry->numVertices * vertexSize);

		// We don't want to delete the original copy, but we'll release because we are forking
		if(baseData->refCount > 1)
			InterlockedDecrement(&baseData->refCount);

		geometry->geometryData = geomData;

		MorphApplicator morpher(geometry, [&](std::vector<Morpher::Vector3> & verts)
		{
			SimpleLocker locker(&m_morphLock);
			for(auto & actorMorph : *actorMorphs)
			{
				float effectiveValue = actorMorph.second->GetEffectiveValue();
				if(effectiveValue == 0.0f)
					continue;

				auto morph = morphMap->GetVertexData(*actorMorph.first);
				if(!morph)
					continue;

				morph->ApplyMorph(geometry->numVertices, (NiPoint3*)&verts.at(0), effectiveValue);
			}
		});
	}

	return false;
}

bool BodyGen::ApplyMorphsToShapes(Actor * actor, NiAVObject * slotNode)
{
	if(!actor || !slotNode)
		return false;

	std::vector<MorphableShapePtr> shapes;
	GetMorphableShapes(slotNode, shapes);

	if(g_bParallelShapes)
	{
		concurrency::parallel_for_each(begin(shapes), end(shapes), [&](const MorphableShapePtr & shape)
		{
			ApplyMorphsToShape(actor, shape);
		}, concurrency::static_partitioner());
	}
	else
	{
		for(auto & shape : shapes)
		{
			ApplyMorphsToShape(actor, shape);
		}
	}

	return true;
}

bool BodyGen::UpdateMorphs(Actor * actor)
{
	if(!actor)
			return false;

	if(g_task)
		g_task->AddTask(new F4EEBodyGenUpdate(actor));

	return true;
}

BSFixedString PrefixMeshPath(const char * relativePath)
{
	if(relativePath == "")
		return BSFixedString("");

	std::string targetPath = "meshes\\";
	targetPath += std::string(relativePath);
	std::transform(targetPath.begin(), targetPath.end(), targetPath.begin(), ::tolower);
	return BSFixedString(targetPath.c_str());
}

void BodyGen::ProcessModel(BSModelDB::ModelData * modelData, const char * modelName, NiAVObject * object)
{
	NiExtraData * bodyMorphs = object->GetExtraData("BODYTRI");
	if(bodyMorphs)
	{
		NiStringExtraData * stringData = DYNAMIC_CAST(bodyMorphs, NiExtraData, NiStringExtraData);
		if(stringData)
		{
			stringData->IncRef();
			auto triPath = PrefixMeshPath(stringData->m_string);
			stringData->DecRef();

			auto trishapeMap = g_bodyGen.GetTrishapeMap(triPath);
			if(trishapeMap)
			{
				for(auto & shape : *trishapeMap)
				{
					BSFixedString str(shape.first);
					NiAVObject * child = object->GetObjectByName(&str);
					if(child) {
						child->IncRef();
						BSTriShape * childShape = child->GetAsBSTriShape();
						if(childShape) {
							NiPointer<NiExtraData> morphFile = NiStringExtraData::Create("MORPH_FILE", triPath);
							NiPointer<NiExtraData> morphShape = NiStringExtraData::Create("MORPH_SHAPE", shape.first);
							childShape->AddExtraData(morphFile);
							childShape->AddExtraData(morphShape);
						}
						child->DecRef();
					}
				}
			}
			g_bodyGen.ShrinkMorphCache();
		}
	}
}

MorphValueMapPtr BodyGen::GetMorphMap(Actor * actor, bool isFemale)
{
	SimpleLocker locker(&m_morphLock);

	UInt64 handle = GetHandleFromObject(actor);
	auto it = m_morphMap[isFemale ? 1 : 0].find(handle);
	if(it != m_morphMap[isFemale ? 1 : 0].end()) {
		return it->second;
	}

	return nullptr;
}

void BodyGen::SetMorph(Actor * actor, bool isFemale, const BSFixedString & morph, BGSKeyword * keyword, float value)
{
	if(!actor)
		return;

	SimpleLocker locker(&m_morphLock);

	UInt64 handle = GetHandleFromObject(actor);

	MorphValueMapPtr morphMap = nullptr;
	auto it = m_morphMap[isFemale ? 1 : 0].find(handle);
	if(it == m_morphMap[isFemale ? 1 : 0].end()) {
		morphMap = std::make_shared<MorphValueMap>();
		m_morphMap[isFemale ? 1 : 0].emplace(handle, morphMap);
	}
	else
		morphMap = it->second;

	morphMap->SetMorph(morph, keyword, value);
}

void BodyGen::GetKeywords(Actor * actor, bool isFemale, const BSFixedString & morph, std::vector<BGSKeyword*> & keywords)
{
	if(!actor)
		return;

	SimpleLocker locker(&m_morphLock);

	UInt64 handle = GetHandleFromObject(actor);

	auto it = m_morphMap[isFemale ? 1 : 0].find(handle);
	if(it != m_morphMap[isFemale ? 1 : 0].end()) {
		it->second->GetKeywords(morph, keywords);
	}
}

void BodyGen::GetMorphs(Actor * actor, bool isFemale, std::vector<BSFixedString> & morphs)
{
	if(!actor)
		return;

	SimpleLocker locker(&m_morphLock);

	UInt64 handle = GetHandleFromObject(actor);

	auto it = m_morphMap[isFemale ? 1 : 0].find(handle);
	if(it != m_morphMap[isFemale ? 1 : 0].end()) {
		for(auto & morph : *it->second) {
			morphs.push_back(morph.first->c_str());
		}
	}
}

void BodyGen::RemoveMorphsByName(Actor * actor, bool isFemale, const BSFixedString & morph)
{
	if(!actor)
		return;

	SimpleLocker locker(&m_morphLock);

	UInt64 handle = GetHandleFromObject(actor);

	auto it = m_morphMap[isFemale ? 1 : 0].find(handle);
	if(it != m_morphMap[isFemale ? 1 : 0].end()) {
		it->second->RemoveMorphsByName(morph);
	}
}

void BodyGen::RemoveMorphsByKeyword(Actor * actor, bool isFemale, BGSKeyword * keyword)
{
	if(!actor)
		return;

	SimpleLocker locker(&m_morphLock);

	UInt64 handle = GetHandleFromObject(actor);

	auto it = m_morphMap[isFemale ? 1 : 0].find(handle);
	if(it != m_morphMap[isFemale ? 1 : 0].end()) {
		it->second->RemoveMorphsByKeyword(keyword);
	}	
}

void BodyGen::ClearMorphs(Actor * actor, bool isFemale)
{
	if(!actor)
		return;

	SimpleLocker locker(&m_morphLock);
	UInt64 handle = GetHandleFromObject(actor);
	auto it = m_morphMap[isFemale ? 1 : 0].find(handle);
	if(it != m_morphMap[isFemale ? 1 : 0].end()) {
		m_morphMap[isFemale ? 1 : 0].erase(it);
	}
}

void BodyGen::CloneMorphs(Actor * source, Actor * target)
{
	if(!source || !target)
		return;

	SimpleLocker locker(&m_morphLock);
	UInt64 handleSrc = GetHandleFromObject(source);
	UInt64 handleDst = GetHandleFromObject(target);
	
	bool isFemale = false;
	TESNPC * npc = DYNAMIC_CAST(source->baseForm, TESForm, TESNPC);
	if(npc)
		isFemale = CALL_MEMBER_FN(npc, GetSex)() == 1 ? true : false;

	auto it = m_morphMap[isFemale ? 1 : 0].find(handleSrc);
	if(it != m_morphMap[isFemale ? 1 : 0].end()) {
		m_morphMap[isFemale ? 1 : 0][handleDst] = it->second;
	}
}

float BodyGen::GetMorph(Actor * actor, bool isFemale, const BSFixedString & morph, BGSKeyword * keyword)
{
	SimpleLocker locker(&m_morphLock);

	UInt64 handle = GetHandleFromObject(actor);

	MorphValueMapPtr morphMap = nullptr;
	auto it = m_morphMap[isFemale ? 1 : 0].find(handle);
	if(it != m_morphMap[isFemale ? 1 : 0].end())
		return it->second->GetMorph(morph, keyword);

	return 0.0f;
}

float UserValues::GetValue(BGSKeyword * keyword)
{
	SimpleLocker locker(&m_morphLock);

	UInt64 handle = g_bodyGen.GetHandleFromObject(keyword);
	auto it = find(handle);
	if(it != end()) {
		return it->second;
	}

	return 0;
}

void UserValues::SetValue(BGSKeyword * keyword, float value)
{
	SimpleLocker locker(&m_morphLock);

	UInt64 handle = g_bodyGen.GetHandleFromObject(keyword);

	// Erase the value if it isn't present at all
	if(value == 0.0f) {
		auto it = find(handle);
		if(it != end()) {
			erase(it);
		}
	} else {
		(*this)[handle] = value;
	}
}

void UserValues::RemoveKeyword(BGSKeyword * keyword)
{
	SimpleLocker locker(&m_morphLock);
	UInt64 handle = g_bodyGen.GetHandleFromObject(keyword);
	auto it = find(handle);
	if(it != end()) {
		erase(it);
	}
}

float UserValues::GetEffectiveValue()
{
	SimpleLocker locker(&m_morphLock);

	auto maxIt = std::max_element(begin(), end());
	if(maxIt != end()) {
		return maxIt->second;
	}

	return 0.0f;
}

void MorphValueMap::SetMorph(const BSFixedString & morph, BGSKeyword * keyword, float value)
{
	SimpleLocker locker(&m_morphLock);

	UserValuesPtr userValues = nullptr;
	StringTableItem string = g_stringTable.GetString(morph);
	auto it = find(string);
	if(it == end()) {
		userValues = std::make_shared<UserValues>();
		emplace(string, userValues);
	}
	else
		userValues = it->second;

	userValues->SetValue(keyword, value);
}

float MorphValueMap::GetMorph(const BSFixedString & morph, BGSKeyword * keyword)
{
	SimpleLocker locker(&m_morphLock);

	auto it = find(g_stringTable.GetString(morph));
	if(it != end()) {
		return it->second->GetValue(keyword);
	}

	return 0.0f;
}

void MorphValueMap::GetKeywords(const BSFixedString & morph, std::vector<BGSKeyword*> & keywords)
{
	SimpleLocker locker(&m_morphLock);
	auto it = find(g_stringTable.GetString(morph));
	if(it != end()) {
		for(auto & kwds : *it->second) {
			BGSKeyword * keyword = g_bodyGen.GetObjectFromHandle<BGSKeyword>(kwds.first);
			keywords.push_back(keyword);
		}
	}
}

void MorphValueMap::RemoveMorphsByName(const BSFixedString & morph)
{
	SimpleLocker locker(&m_morphLock);

	auto it = find(g_stringTable.GetString(morph));
	if(it != end()) {
		erase(it);
	}
}

void MorphValueMap::RemoveMorphsByKeyword(BGSKeyword * keyword)
{
	SimpleLocker locker(&m_morphLock);

	for(auto & values : *this) {
		values.second->RemoveKeyword(keyword);
	}
}

void BodyGen::Save(const F4SESerializationInterface * intfc, UInt32 kVersion)
{
	SimpleLocker locker(&m_morphLock);

	// Male handles
	for(auto & morph : m_morphMap[0])
	{
		intfc->OpenRecord('MRPM', kVersion);

		// Key
		WriteData<UInt64>(intfc, &morph.first);

#ifdef _DEBUG
		_MESSAGE("%s - Saving Male Morph Handle %016llX", __FUNCTION__, morph.first);
#endif

		// Value
		morph.second->Save(intfc, kVersion);
	}

	// Female handles
	for(auto & morph : m_morphMap[1])
	{
		intfc->OpenRecord('MRPH', kVersion);

		// Key
		WriteData<UInt64>(intfc, &morph.first);

#ifdef _DEBUG
		_MESSAGE("%s - Saving Female Morph Handle %016llX", __FUNCTION__, morph.first);
#endif

		// Value
		morph.second->Save(intfc, kVersion);
	}
}

void MorphValueMap::Save(const F4SESerializationInterface * intfc, UInt32 kVersion)
{
	intfc->OpenRecord('MRVM', kVersion);

	UInt32 numMorphs = size();

	WriteData<UInt32>(intfc, &numMorphs);

#ifdef _DEBUG
	_MESSAGE("%s - Saving %d morphs", __FUNCTION__, numMorphs);
#endif

	for (auto & morph : *this)
	{
		UInt32 stringId = g_stringTable.GetStringID(morph.first);
		WriteData<UInt32>(intfc, &stringId);

		UInt32 numKeys = morph.second->size();
		WriteData<UInt32>(intfc, &numKeys);

		for (auto & keys : *morph.second)
		{
			WriteData<UInt64>(intfc, &keys.first);
			WriteData<float>(intfc, &keys.second);
		}
	}
}

bool MorphValueMap::Load(const F4SESerializationInterface * intfc, UInt32 kVersion, const std::unordered_map<UInt32, StringTableItem> & stringTable)
{
	UInt32 type, length, version;

	if(intfc->GetNextRecordInfo(&type, &version, &length))
	{
		switch (type)
		{
		case 'MRVM':
			{
				// Override Count
				UInt32 numMorphs = 0;
				if (!ReadData<UInt32>(intfc, &numMorphs))
				{
					_ERROR("%s - Error loading morph set count", __FUNCTION__);
					return false;
				}

				for (UInt32 i = 0; i < numMorphs; i++)
				{
					UInt32 stringId;
					if (!ReadData<UInt32>(intfc, &stringId))
					{
						_ERROR("%s - Error loading string id", __FUNCTION__);
						return false;
					}

					auto it = stringTable.find(stringId);
					if(it == stringTable.end())
					{
						_ERROR("%s - Error loading string from table", __FUNCTION__);
						return false;
					}

					UInt32 numKeys;
					if (!ReadData<UInt32>(intfc, &numKeys))
					{
						_ERROR("%s - Error loading number of morph keys", __FUNCTION__);
						return false;
					}

					UserValuesPtr userValues = std::make_shared<UserValues>();
					for (UInt32 k = 0; k < numKeys; k++)
					{
						UInt64 handle;
						if (!ReadData<UInt64>(intfc, &handle))
						{
							_ERROR("%s - Error loading morph key handle", __FUNCTION__);
							return false;
						}

						float value;
						if (!ReadData<float>(intfc, &value))
						{
							_ERROR("%s - Error loading morph key value", __FUNCTION__);
							return false;
						}

						if(value == 0.0f)
							continue;

						UInt64 newHandle = 0;

						// Skip if handle is no longer valid.
						if (!intfc->ResolveHandle(handle, &newHandle))
							continue;

						userValues->emplace(newHandle, value);
					}

					if(userValues->empty())
						continue;

					m_morphLock.Lock();
					emplace(it->second, userValues);
					m_morphLock.Release();
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

bool BodyGen::Load(const F4SESerializationInterface * intfc, bool isFemale, UInt32 kVersion, const std::unordered_map<UInt32, StringTableItem> & stringTable)
{
	UInt64 handle;
	// Key
	if (!ReadData<UInt64>(intfc, &handle))
	{
		_ERROR("%s - Error loading actor key", __FUNCTION__);
		return false;
	}

	MorphValueMapPtr morphValueMap = std::make_shared<MorphValueMap>();
	if (!morphValueMap->Load(intfc, kVersion, stringTable))
	{
		_ERROR("%s - Error loading value map for actor %016llX", __FUNCTION__, handle);
		return false;
	}

	// Only place the data into the map if bodygen is enabled
	// this allows it to be parsed first, then discarded next save
	if(g_bEnableBodygen)
	{
		UInt64 newHandle = 0;

		// Skip if handle is no longer valid.
		if (!intfc->ResolveHandle(handle, &newHandle))
			return true;

		if(morphValueMap->empty())
			return true;
	
		m_morphLock.Lock();
		m_morphMap[isFemale ? 1 : 0].emplace(newHandle, morphValueMap);
		m_morphLock.Release();


		Actor * actor = GetObjectFromHandle<Actor>(newHandle);
		if(actor) {
			UpdateMorphs(actor);
		}
	}

	return true;
}

void BodyGen::Revert()
{
	SimpleLocker	locker(&m_morphLock);
	m_morphMap[0].clear();
	m_morphMap[1].clear();
}
