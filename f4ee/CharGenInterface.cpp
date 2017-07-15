#include "CharGenInterface.h"
#include "f4se/GameAPI.h"
#include "f4se/GameCustomization.h"
#include "f4se/GameReferences.h"
#include "f4se/GameObjects.h"
#include "f4se/GameRTTI.h"
#include "f4se/GameData.h"
#include "f4se_common/Utilities.h"
#include "f4se/GameStreams.h"

#include "f4se/NiRTTI.h"
#include "f4se/BSGeometry.h"
#include "f4se/NiMaterials.h"
#include "f4se/NiProperties.h"

#include "common/IFileStream.h"

#include <fstream>
#include <memory>

#include <ppl.h>
#include <atomic>
#include <chrono>
#include <regex>

#include "CharGenTint.h"
#include "BodyMorphInterface.h"
#include "OverlayInterface.h"
#include "SkinInterface.h"
#include "Utilities.h"

extern bool g_bExportRace;
extern std::string g_strExportRace;
extern UInt32 g_uExportIdMin;
extern UInt32 g_uExportIdMax;

extern BodyMorphInterface g_bodyMorphInterface;
extern OverlayInterface g_overlayInterface;
extern SkinInterface	g_skinInterface;

extern bool g_bIgnoreTintPalettes;
extern bool g_bIgnoreTintTextures;
extern bool g_bIgnoreTintMasks;

extern const std::string & GetRuntimeDirectory(void);

static const char * HairGradientPalette = "actors\\character\\hair\\haircolor_lgrad_d.dds";

DWORD CharGenInterface::SavePreset(const std::string & filePath)
{
	DataHandler * dataHandler = (*g_dataHandler);
	if(!dataHandler)
		return ERROR_INVALID_ADDRESS;

	Actor * actor = GetCurrentActor();
	if(!actor)
		return ERROR_INVALID_ADDRESS;

	TESNPC * npc = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
	if(!npc)
		return ERROR_INVALID_ADDRESS;

	TESRace * race = GetActorRace(actor);
	if(!race)
		return ERROR_INVALID_ADDRESS;

	UInt8 gender = CALL_MEMBER_FN(npc, GetSex)();

	IFileStream		currentFile;
	IFileStream::MakeAllDirs(filePath.c_str());
	if (!currentFile.Create(filePath.c_str()))
	{
		return GetLastError();
	}

	Json::StyledWriter writer;
	Json::Value root;
	
	UInt32 numHeadParts = 0;
	BGSHeadPart ** headParts = NULL;
	if (CALL_MEMBER_FN(npc, HasOverlays)()) {
		numHeadParts = CALL_MEMBER_FN(npc, GetNumOverlayHeadParts)();
		headParts = CALL_MEMBER_FN(npc, GetOverlayHeadParts)();
	}
	else {
		numHeadParts = npc->numHeadParts;
		headParts = npc->headParts;
	}

	root["Gender"] = (Json::UInt)gender;

	Json::Value headPartInfo;
	for (UInt32 i = 0; i < numHeadParts; i++) // Acquire all unique parts
	{
		BGSHeadPart * headPart = headParts[i];
		if (headPart && !headPart->IsExtraPart()) {
			std::string formIdentifier = GetFormIdentifier(headPart);
			if (!formIdentifier.empty()) {
				headPartInfo.append(formIdentifier);
			}
		}
	}
	root["HeadParts"] = headPartInfo;

	auto headData = npc->headData;
	if(headData) {
		auto hairColor = headData->hairColor;
		if(hairColor) {
			root["HairColor"] = GetFormIdentifier(hairColor);
		}
	}

	root["Weight"][0] = npc->weightThin;
	root["Weight"][1] = npc->weightMuscular;
	root["Weight"][2] = npc->weightLarge;

	Json::Value morphSetData;
	Json::Value morphRegionData;

	if(npc->morphSetValue)
	{
		for(int i = 0; i < npc->morphSetValue->count; i++)
		{
			root["Morphs"]["Values"][i] = (*npc->morphSetValue)[i];
		}
	}

	char keyName[MAX_PATH];
	if(npc->morphSetData)
	{
		npc->morphSetData->ForEach([&npc, &keyName, &morphSetData](TESNPC::MorphSetData * region)
		{
			sprintf_s(keyName, "%X", region->key);
			morphSetData[keyName] = region->value;
			return true;
		});

		root["Morphs"]["Presets"] = morphSetData;
	}
		

	if(npc->morphRegionData)
	{
		npc->morphRegionData->ForEach([&npc, &keyName, &morphRegionData](TESNPC::FaceMorphRegion * region)
		{
			Json::Value values;
			for(UInt32 f = 0; f < 8; f++)
			{
				values.append(region->value[f]);
			}

			sprintf_s(keyName, "%X", region->index);
			morphRegionData[keyName] = values;
			return true;
		});

		root["Morphs"]["Regions"] = morphRegionData;
	}

	CharacterCreation::MorphIntensity * intensity = g_morphIntensityMap->Find(&npc);
	if(intensity) {
		root["Morphs"]["Intensity"] = intensity->morphIntensity;
	}

	Json::Value tintData;

	tArray<BGSCharacterTint::Entry*> * tints = npc->tints;

	PlayerCharacter * pPC = DYNAMIC_CAST(actor, Actor, PlayerCharacter);
	if(pPC && pPC->tints)
		tints = pPC->tints;

	if(tints)
	{
		for(UInt32 i = 0; i < tints->count; i++)
		{
			BGSCharacterTint::Entry * entry;
			tints->GetNthItem(i, entry);

			if(entry->percent == 0)
				continue;

			sprintf_s(keyName, "%X", entry->tintIndex);

			UInt32 type = entry->GetType();
			tintData[keyName]["Type"] = (Json::Int)type;
			tintData[keyName]["Percent"] = (Json::Int)entry->percent;

			switch(type)
			{
			case BGSCharacterTint::Entry::kTypePalette:
				BGSCharacterTint::PaletteEntry * palette = static_cast<BGSCharacterTint::PaletteEntry*>(entry);
				tintData[keyName]["Color"] = (Json::Int)palette->color.bgra;
				tintData[keyName]["ColorID"] = palette->colorID;
				break;
			}

			root["TintOrder"].append(keyName);
		}

		root["Tints"] = tintData;
	}

	Json::Value morphData;
	auto morphMap = g_bodyMorphInterface.GetMorphMap(actor, gender == 1 ? true : false);
	if(morphMap) {
		for(auto & morph : *morphMap) {
			auto it = morph.second->find(0);
			if(it != morph.second->end()) {
				morphData[morph.first->c_str()] = it->second;
			}
		}

		root["BodyMorphs"] = morphData;
	}

	Json::Value overlayData;
	if(g_overlayInterface.ForEachOverlay(actor, gender == 1 ? true : false, [&](SInt32 priority, const OverlayInterface::OverlayDataPtr & pOverlay)
	{
		Json::Value overlay;
		overlay["template"] = pOverlay->templateName ? pOverlay->templateName->c_str() : "";
		overlay["priority"] = priority;

		if((pOverlay->flags & OverlayInterface::OverlayData::kHasTintColor) == OverlayInterface::OverlayData::kHasTintColor)
		{
			overlay["tint"].append(pOverlay->tintColor.r);
			overlay["tint"].append(pOverlay->tintColor.g);
			overlay["tint"].append(pOverlay->tintColor.b);
			overlay["tint"].append(pOverlay->tintColor.a);
		}
		if((pOverlay->flags & OverlayInterface::OverlayData::kHasOffsetUV) == OverlayInterface::OverlayData::kHasOffsetUV)
		{
			overlay["offsetUV"].append(pOverlay->offsetUV.x);
			overlay["offsetUV"].append(pOverlay->offsetUV.y);
		}
		if((pOverlay->flags & OverlayInterface::OverlayData::kHasScaleUV) == OverlayInterface::OverlayData::kHasScaleUV)
		{
			overlay["scaleUV"].append(pOverlay->offsetUV.x);
			overlay["scaleUV"].append(pOverlay->offsetUV.y);
		}
		
		overlayData.append(overlay);
	})) {
		root["Overlays"] = overlayData;
	}

	auto skin = g_skinInterface.GetSkinOverride(actor);
	if(skin.length() > 0) {
		root["Skin"] = skin.c_str();
	}
		

	std::string data = writer.write(root);
	currentFile.WriteBuf(data.c_str(), data.length());
	currentFile.Close();
	return ERROR_SUCCESS;
}

DWORD CharGenInterface::LoadPreset(const std::string & filePath)
{
	Json::Reader reader;
	Json::Value root;

	std::ifstream in(filePath);
	if(!in) {
		return GetLastError();
	}

	if(!reader.parse(in, root)) {
		return ERROR_INVALID_TOKEN;
	}

	DataHandler * dataHandler = (*g_dataHandler);
	if(!dataHandler)
		return ERROR_INVALID_ADDRESS;

	Actor * actor = GetCurrentActor();
	if(!actor)
		return ERROR_INVALID_ADDRESS;

	TESNPC * npc = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
	if(!npc)
		return ERROR_INVALID_ADDRESS;

	TESRace * race = GetActorRace(actor);
	if(!race)
		return ERROR_INVALID_ADDRESS;

	UInt8 gender = CALL_MEMBER_FN(npc, GetSex)();

	UInt8 loadedGender = 0;
	try
	{
		loadedGender = root["Gender"].asUInt();
	}
	catch( ... )
	{
		loadedGender = gender;
	}

	if(gender != loadedGender)
	{
		return ERROR_INVALID_TOKEN;
	}

	bool isFemale = gender == 1 ? true : false;

	// Wipe the HeadPart list and replace it with the default race list
	auto chargenData = race->chargenData[gender];
	if (chargenData) {
		BGSHeadPart ** headParts = npc->headParts;
		tArray<BGSHeadPart*> * headPartList = race->chargenData[gender]->headParts;
		if (headParts && headPartList) {
			Heap_Free(headParts);
			npc->numHeadParts = headPartList->count;
			headParts = (BGSHeadPart **)Heap_Allocate(npc->numHeadParts * sizeof(BGSHeadPart*));
			for (UInt32 i = 0; i < headPartList->count; i++)
				headPartList->GetNthItem(i, headParts[i]);
			npc->headParts = headParts;
		}
	}

	try
	{
		Json::Value parts = root["HeadParts"];
		for(auto & part : parts)
		{
			TESForm * form = GetFormFromIdentifier(part.asString());
			if(!form) // Not a valid form
				continue;

			BGSHeadPart * newPart = DYNAMIC_CAST(form, TESForm, BGSHeadPart);
			if(!newPart) // Not a head part type
				continue;

			npc->ChangeHeadPart(newPart, false, false);
		}
	}
	catch(const std::exception& e)
	{
		_ERROR(e.what());
	}
	
	try
	{
		TESForm * form = GetFormFromIdentifier(root["HairColor"].asString());
		if(form) {
			BGSColorForm * colorForm = DYNAMIC_CAST(form, TESForm, BGSColorForm);
			if(colorForm) {
				if(!npc->headData)
					npc->headData = new TESNPC::HeadData();
				npc->headData->hairColor = colorForm;
			}
		}
	}
	catch(const std::exception& e)
	{
		_ERROR(e.what());
	}
	
	try
	{
		Json::Value values = root["Morphs"]["Values"];
		std::vector<float> fValues;
		for(auto & value : values)
		{
			fValues.push_back(value.asFloat());
		}

		if(!npc->morphSetValue && fValues.size() > 0)
			npc->morphSetValue = new tArray<float>();
		if(npc->morphSetValue) {
			npc->morphSetValue->Clear();
			npc->morphSetValue->Allocate(fValues.size());
			for(int i = 0; i < fValues.size(); i++)
			{
				(*npc->morphSetValue)[i] = fValues.at(i);
			}
		}
	}
	catch(const std::exception& e)
	{
		_ERROR(e.what());
	}
	
	try
	{
		Json::Value regions = root["Morphs"]["Regions"];
		auto members = regions.getMemberNames();

		bool bClear = true;
		if(!npc->morphRegionData && members.size() > 0) {
			npc->morphRegionData = new tHashSet<TESNPC::FaceMorphRegion, UInt32>();
			bClear = false;
		}
		if(npc->morphRegionData) {
			if(bClear)
				npc->morphRegionData->Clear();
			
			for(auto key : members)
			{
				UInt32 keyValue = 0;
				sscanf_s(key.c_str(), "%X", &keyValue);

				TESNPC::FaceMorphRegion regionData;
				regionData.index = keyValue;
				for(int i = 0; i < 8; i++)
				{
					regionData.value[i] = regions[key][i].asFloat();
				}
				npc->morphRegionData->Add(&regionData);
			}
		}
	}
	catch(const std::exception& e)
	{
		_ERROR(e.what());
	}
	
	try
	{
		Json::Value presets = root["Morphs"]["Presets"];
		auto members = presets.getMemberNames();

		bool bClear = true;
		if(!npc->morphSetData && members.size() > 0) {
			npc->morphSetData = new tHashSet<TESNPC::MorphSetData, UInt32>();
			bClear = false;
		}
		if(npc->morphSetData) {
			if(bClear)
				npc->morphSetData->Clear();

			for(auto key : members)
			{
				UInt32 keyValue = 0;
				sscanf_s(key.c_str(), "%X", &keyValue);

				TESNPC::MorphSetData morphSet;
				morphSet.key = keyValue;
				morphSet.value = presets[key].asFloat();
				npc->morphSetData->Add(&morphSet);
			}
		}
	}
	catch(const std::exception& e)
	{
		_ERROR(e.what());
	}

	if(actor != (*g_player)) // Don't apply morph intensity to the player.
	{
		try
		{
			bool hasIntensity = root["Morphs"].isMember("Intensity");
			float intensity = hasIntensity ? root["Morphs"]["Intensity"].asFloat() : 1.0f;

			if(CharacterCreation::MorphIntensity * found = g_morphIntensityMap->Find(&npc))
			{
				if(hasIntensity)
					found->morphIntensity = root["Morphs"]["Intensity"].asFloat();
				else
					g_morphIntensityMap->Remove(&npc);
			}
			else if(hasIntensity)
			{
				CharacterCreation::MorphIntensity mi;
				mi.npc = npc;
				mi.morphIntensity = intensity;
				g_morphIntensityMap->Add(&mi);
			}
		}
		catch(const std::exception& e)
		{
			_ERROR(e.what());
		}
	}
	
	try
	{
		npc->weightThin = root["Weight"][0].asFloat();
		npc->weightMuscular = root["Weight"][1].asFloat();
		npc->weightLarge = root["Weight"][2].asFloat();
	}
	catch(const std::exception& e)
	{
		_ERROR(e.what());
	}

	try
	{
		Json::Value tints = root["Tints"];
		auto members = tints.getMemberNames();
		bool bClear = true;
		if(!npc->tints && members.size() > 0) {
			npc->tints = new tArray<BGSCharacterTint::Entry*>();
			bClear = false;
		}
		if(npc->tints) {
			if(bClear)
				ClearCharacterTints(npc->tints);

			std::map<UInt32, BGSCharacterTint::Entry*> tintMap;
			for(auto key : members)
			{
				UInt32 keyValue = 0;
				sscanf_s(key.c_str(), "%X", &keyValue);

				UInt32 type = tints[key]["Type"].asInt();
				if((type == BGSCharacterTint::Entry::kTypePalette && g_bIgnoreTintPalettes) ||
					(type == BGSCharacterTint::Entry::kTypeTexture && g_bIgnoreTintTextures) ||
					(type == BGSCharacterTint::Entry::kTypeMask && g_bIgnoreTintMasks))
					continue;

				BGSCharacterTint::Template::Entry * templateEntry = chargenData->GetTemplateByIndex(keyValue); // Validate the tint index
				if(templateEntry) {
					BGSCharacterTint::Entry* newEntry = CreateCharacterTintEntry((keyValue << 16) | type);
					if(newEntry) {
						if(newEntry->GetType() == BGSCharacterTint::Entry::kTypePalette) {
							BGSCharacterTint::PaletteEntry * palette = static_cast<BGSCharacterTint::PaletteEntry*>(newEntry);
							BGSCharacterTint::Template::Palette * paletteTemplate = static_cast<BGSCharacterTint::Template::Palette*>(templateEntry);
							palette->color.bgra = tints[key]["Color"].asUInt();
							SInt16 colorID = tints[key]["ColorID"].asInt();
							auto colorData = paletteTemplate->GetColorDataByID(colorID); // Validate the color index
							if(colorData)
								palette->colorID = colorID;
							else if(paletteTemplate->colors.count)
								palette->colorID = paletteTemplate->colors[0].colorID;
							else
								palette->colorID = 0;
						}
						newEntry->percent = tints[key]["Percent"].asInt();

						tintMap.emplace(keyValue, newEntry);
					}
				}
			}

			// We have a defined ordering, find them in the map and push them in and erase them
			Json::Value tintOrder = root["TintOrder"];
			if(root.isMember("TintOrder"))
			{
				for(auto & tintEntry : tintOrder)
				{
					UInt32 keyValue = 0;
					sscanf_s(tintEntry.asCString(), "%X", &keyValue);

					auto it = tintMap.find(keyValue);
					if(it != tintMap.end()) {
						npc->tints->Push(it->second);
						tintMap.erase(it);
					}
				}

			}

			// We either have remaining tints not part of the ordering, or we didn't have an ordering at all
			for(auto & tintEntry : tintMap)
				npc->tints->Push(tintEntry.second);

			if(actor == (*g_player)) {
				CopyCharacterTints((*g_player)->tints, npc->tints);
			}
		}
	}
	catch(const std::exception& e)
	{
		_ERROR(e.what());
	}

	try
	{
		Json::Value morphData = root["BodyMorphs"];
		auto members = morphData.getMemberNames();
		if(members.size() > 0 || (root.isMember("BodyMorphs") && morphData.isNull())) {
			g_bodyMorphInterface.RemoveMorphsByKeyword(actor, isFemale, nullptr);
		}

		for(auto key : members)
		{
			float value = morphData[key].asFloat();
			g_bodyMorphInterface.SetMorph(actor, isFemale, key.c_str(), nullptr, value);
		}
	}
	catch(const std::exception& e)
	{
		_ERROR(e.what());
	}

	g_overlayInterface.RemoveAll(actor, isFemale);
	if(root.isMember("Overlays"))
	{
		Json::Value overlays = root["Overlays"];
		for(auto & overlay : overlays)
		{
			try
			{
				SInt32 priority = overlay["priority"].asInt();
				const char * templateName = overlay["template"].asCString();
				NiColorA color;
				color.r = 0.0f;
				color.g = 0.0f;
				color.b = 0.0f;
				color.a = 0.0f;
				if(overlay.isMember("tint")) {
					color.r = overlay["tint"][0].asFloat();
					color.g = overlay["tint"][1].asFloat();
					color.b = overlay["tint"][2].asFloat();
					color.a = overlay["tint"][3].asFloat();
				}
				NiPoint2 offsetUV;
				offsetUV.x = 0.0f;
				offsetUV.y = 0.0f;
				if(overlay.isMember("offsetUV")) {
					offsetUV.x = overlay["offsetUV"][0].asFloat();
					offsetUV.y = overlay["offsetUV"][1].asFloat();
				}
				NiPoint2 scaleUV;
				scaleUV.x = 1.0f;
				scaleUV.y = 1.0f;
				if(overlay.isMember("scaleUV")) {
					scaleUV.x = overlay["scaleUV"][0].asFloat();
					scaleUV.y = overlay["scaleUV"][1].asFloat();
				}

				g_overlayInterface.AddOverlay(actor, gender == 1 ? true : false, priority, templateName, color, offsetUV, scaleUV);
			}
			catch(const std::exception& e)
			{
				_ERROR(e.what());
			}
		}
	}

	g_skinInterface.RemoveSkinOverride(actor);
	if(root.isMember("Skin"))
	{
		g_skinInterface.AddSkinOverride(actor, root["Skin"].asString(), gender == 1 ? true : false);
	}
	

	npc->MarkChanged(0x800); // Save FaceData
	npc->MarkChanged(0x4000); // Save weights

	return ERROR_SUCCESS;
}

void CharGenInterface::LoadHairColorMods()
{
	// Load all hair color mods
	for(int i = 0; i < (*g_dataHandler)->modList.loadedModCount; i++)
	{
		ModInfo * modInfo = (*g_dataHandler)->modList.loadedMods[i];
		std::string modName = modInfo->name;
		std::string templatesPath = std::string("F4SE\\Plugins\\F4EE\\LUTs\\") + modName + "\\haircolors.json";
		LoadHairColorData(templatesPath, modInfo);
	}
}

void CharGenInterface::LoadTintTemplateMods()
{
	// Load all categories first
	for(int i = 0; i < (*g_dataHandler)->modList.loadedModCount; i++)
	{
		ModInfo * modInfo = (*g_dataHandler)->modList.loadedMods[i];
		std::string templatesPath = std::string("F4SE\\Plugins\\F4EE\\Tints\\") + std::string(modInfo->name) + "\\categories.json";
		LoadTintCategories(templatesPath);
	}

	// Load templates
	for(int i = 0; i < (*g_dataHandler)->modList.loadedModCount; i++)
	{
		ModInfo * modInfo = (*g_dataHandler)->modList.loadedMods[i];
					
		std::string templatesPath = std::string("F4SE\\Plugins\\F4EE\\Tints\\") + std::string(modInfo->name) + "\\templates.json";
		LoadTintTemplates(templatesPath);
	}

	if(g_bExportRace)
	{
		TESRace * race = GetRaceByName(g_strExportRace);
		if(race)
		{
			std::string exportPath = "Data\\F4SE\\Plugins\\F4EE\\Exported\\Tints";
			std::string categories = GetRuntimeDirectory() + exportPath + std::string("\\categories.json");
			SaveTintCategories(race, categories);
			std::string templatesPath = GetRuntimeDirectory() + exportPath + std::string("\\templates.json");
			SaveTintTemplates(race, templatesPath);
		}
	}
}

bool CharGenInterface::LoadTintCategories(const std::string & filePath)
{
	BSResourceNiBinaryStream binaryStream(filePath.c_str());
	if(!binaryStream.IsValid())
		return false;

	std::string strFile;
	BSReadAll(&binaryStream, &strFile);

	Json::Reader reader;
	Json::Value root;

	UInt32 loadedCategories = 0;

	if(!reader.parse(strFile, root)) {
		return false;
	}

	try
	{
		// traverse instances
		for(auto & item : root)
		{
			std::string raceName = item["Race"].asString();

			TESRace * race = GetRaceByName(raceName);
			if(race)
			{
				// First pass for category registration
				auto entries = item["Entries"];
				for(auto & entry : entries)
				{
					std::string type = entry["Type"].asString();
					UInt8 gender = entry["Gender"].asUInt();
					UInt32 identifier = entry["Id"].asUInt();
					switch(gender)
					{
					case 0:
					case 1:
					case 2:
						break;
					default:
						throw std::exception("Invalid gender specified");
						break;
					}

					if(_strnicmp(type.c_str(), "Category", 8) == 0)
					{
						std::shared_ptr<CharGenTintObject> pCategory = std::make_shared<CharGenTintObject>(identifier);
						if(pCategory->Parse(entry)) {
							pCategory->Apply(race, gender);
							loadedCategories++;
						}
					}
				}
			}
		}
	}
	catch(const std::exception& e)
	{
		_ERROR(e.what());
		return false;
	}

	_MESSAGE("%s - Info - Loaded %d tint category(s).\t[%s]", __FUNCTION__, loadedCategories, filePath.c_str());
	return true;
}

bool CharGenInterface::LoadTintTemplates(const std::string & filePath)
{
	BSResourceNiBinaryStream binaryStream(filePath.c_str());
	if(!binaryStream.IsValid())
		return false;

	std::string strFile;
	BSReadAll(&binaryStream, &strFile);

	Json::Reader reader;
	Json::Value root;

	UInt32 loadedTemplates = 0;

	if(!reader.parse(strFile, root)) {
		return false;
	}

	try
	{
		// traverse instances
		for(auto & item : root)
		{
			std::string raceName = item["Race"].asString();

			TESRace * race = GetRaceByName(raceName);
			if(race)
			{
				auto entries = item["Entries"];
				for(auto & entry : entries)
				{
					std::string type = entry["Type"].asString();
					UInt8 gender = entry["Gender"].asUInt();
					UInt32 identifier = entry["Id"].asUInt();

					switch(gender)
					{
					case 0:
					case 1:
					case 2:
						break;
					default:
						throw std::exception("Invalid gender specified");
						break;
					}

					if(_strnicmp(type.c_str(), "Mask", 4) == 0)
					{
						std::shared_ptr<CharGenTintMask> pMask = std::make_shared<CharGenTintMask>(identifier);
						if(pMask->Parse(entry)) {
							pMask->Apply(race, gender);
							loadedTemplates++;
						}
					}
					else if(_strnicmp(type.c_str(), "Palette", 7) == 0)
					{
						std::shared_ptr<CharGenTintPalette> pPalette = std::make_shared<CharGenTintPalette>(identifier);
						if(pPalette->Parse(entry)) {
							pPalette->Apply(race, gender);
							loadedTemplates++;
						}
					}
					else if(_strnicmp(type.c_str(), "TextureSet", 10) == 0)
					{
						std::shared_ptr<CharGenTintTextureSet> pTextureSet = std::make_shared<CharGenTintTextureSet>(identifier);
						if(pTextureSet->Parse(entry)) {
							pTextureSet->Apply(race, gender);
							loadedTemplates++;
						}
					}
				}
			}
		}
	}
	catch(const std::exception& e)
	{
		_ERROR(e.what());
		return false;
	}

	_MESSAGE("%s - Info - Loaded %d tint template(s).\t[%s]", __FUNCTION__, loadedTemplates, filePath.c_str());
	return true;
}


bool CharGenInterface::SaveTintCategories(const TESRace * race, const std::string & filePath)
{
	IFileStream		currentFile;
	IFileStream::MakeAllDirs(filePath.c_str());
	if (!currentFile.Create(filePath.c_str()))
	{
		return false;
	}

	Json::StyledWriter writer;
	Json::Value root;

	Json::Value rootEntry;
	rootEntry["Race"] = race->editorId.c_str();

	// Find the last index first by going over all looking for the max
	UInt32 lastIndex = 0;
	CharGenTintObject::ForEachGender(race, 2, [&](tArray<CharacterCreation::TintData *>* tints, UInt8 genderId)
	{
		for(UInt32 i = 0; i < tints->count; i++)
		{
			CharacterCreation::TintData * category;
			tints->GetNthItem(i, category);

			if(category->type > lastIndex)
				lastIndex = category->type;
		}
	});

	CharGenTintObject::ForEachGender(race, 2, [&](tArray<CharacterCreation::TintData *>* tints, UInt8 genderId)
	{
		for(UInt32 i = 0; i < tints->count; i++)
		{
			CharacterCreation::TintData * category;
			tints->GetNthItem(i, category);

			// Correct broken categories
			if(category->type == 0 && category->category == "SkinTints" && category->category != "FaceRegions" && category->category != "Brows") {
				lastIndex++;
				category->type = lastIndex;
			}

			Json::Value jcategory;
			jcategory["Id"] = (Json::UInt)category->type;
			jcategory["Type"] = "Category";
			jcategory["Gender"] = genderId;
			jcategory["Name"] = category->category.c_str();

			rootEntry["Entries"].append(jcategory);
		}
	});

	root.append(rootEntry);

	std::string data = writer.write(root);
	currentFile.WriteBuf(data.c_str(), data.length());
	currentFile.Close();
	return true;
}

bool CharGenInterface::SaveTintTemplates(const TESRace * race, const std::string & filePath)
{
	IFileStream		currentFile;
	IFileStream::MakeAllDirs(filePath.c_str());
	if (!currentFile.Create(filePath.c_str()))
	{
		return false;
	}

	Json::StyledWriter writer;
	Json::Value root;

	Json::Value rootEntry;
	rootEntry["Race"] = race->editorId.data->Get<char>();

	CharGenTintObject::ForEachGender(race, 2, [&](tArray<CharacterCreation::TintData *>* tints, UInt8 genderId)
	{
		for(UInt32 i = 0; i < tints->count; i++)
		{
			CharacterCreation::TintData * category;
			tints->GetNthItem(i, category);

			for(UInt32 k = 0; k < category->entry.count; k++)
			{
				BGSCharacterTint::Template::Entry* entry;
				category->entry.GetNthItem(k, entry);

				BGSCharacterTint::Template::Mask* mask = (BGSCharacterTint::Template::Mask*)Runtime_DynamicCast(entry, RTTI_BGSCharacterTint__Template__Entry, RTTI_BGSCharacterTint__Template__Mask);
				BGSCharacterTint::Template::Palette* palette = (BGSCharacterTint::Template::Palette*)Runtime_DynamicCast(entry, RTTI_BGSCharacterTint__Template__Entry, RTTI_BGSCharacterTint__Template__Palette);
				BGSCharacterTint::Template::TextureSet* textureSet = (BGSCharacterTint::Template::TextureSet*)Runtime_DynamicCast(entry, RTTI_BGSCharacterTint__Template__Entry, RTTI_BGSCharacterTint__Template__TextureSet);

				if(entry->templateIndex < g_uExportIdMin || entry->templateIndex > g_uExportIdMax)
					continue;

				if(mask)
				{
					Json::Value jentry;
					jentry["Id"] = (Json::UInt)entry->templateIndex;
					jentry["Type"] = "Mask";
					jentry["Gender"] = genderId;
					jentry["Slot"] =  CharGenTintObject::WriteSlot(mask->slot);
					CharGenTintObject::WriteFlags(mask->flags, jentry["Flags"]);
					jentry["Name"] = mask->name.c_str();

					if(category->type == 0 || jentry["Slot"].asString().compare("Brows") == 0)
					{
						jentry["FixedCategory"] = category->category.c_str();
					}
					else
					{
						jentry["Category"] = (Json::UInt)category->type;
					}
					

					std::string texture = mask->texture;
					if(!texture.empty())
						jentry["Texture"] = texture;

					if(mask->blendOp != BGSCharacterTint::Template::Entry::kBlendOpDefault)
						jentry["BlendOp"] = CharGenTintObject::WriteBlendOp(mask->blendOp);

					rootEntry["Entries"].append(jentry);
				}
				else if(palette)
				{
					Json::Value jentry;
					jentry["Id"] = (Json::UInt)entry->templateIndex;
					jentry["Type"] = "Palette";
					jentry["Gender"] = genderId;
					jentry["Slot"] =  CharGenTintObject::WriteSlot(palette->slot);
					CharGenTintObject::WriteFlags(palette->flags, jentry["Flags"]);
					jentry["Name"] = palette->name.c_str();
					if(category->type == 0)
					{
						jentry["FixedCategory"] = category->category.c_str();
					}
					else
					{
						jentry["Category"] = (Json::UInt)category->type;
					}
					jentry["Texture"] = palette->texture.c_str();
					Json::Value colors;
					for(UInt32 j = 0; j < palette->colors.count; j++)
					{
						BGSCharacterTint::Template::Palette::ColorData colorData;
						palette->colors.GetNthItem(j, colorData);

						Json::Value color;
						color["Id"] = (Json::UInt)colorData.colorID;
						color["Form"] = GetFormIdentifier(colorData.colorForm);
						color["Alpha"] = colorData.alpha;
						if(colorData.blendOp != BGSCharacterTint::Template::Entry::kBlendOpDefault)
							color["BlendOp"] = CharGenTintObject::WriteBlendOp(colorData.blendOp);
						colors.append(color);
					}
					jentry["Colors"] = colors;
					rootEntry["Entries"].append(jentry);
				}
				else if(textureSet)
				{
					Json::Value jentry;
					jentry["Id"] = (Json::UInt)entry->templateIndex;
					jentry["Type"] = "TextureSet";
					jentry["Gender"] = genderId;
					jentry["Slot"] =  CharGenTintObject::WriteSlot(textureSet->slot);
					CharGenTintObject::WriteFlags(textureSet->flags, jentry["Flags"]);
					jentry["Name"] = textureSet->name.c_str();
					if(category->type == 0)
					{
						jentry["FixedCategory"] = category->category.c_str();
					}
					else
					{
						jentry["Category"] = (Json::UInt)category->type;
					}

					std::string diffuse = textureSet->diffuse;
					std::string normal = textureSet->normal;
					std::string specular = textureSet->specular;

					if(!diffuse.empty())
						jentry["Diffuse"] = diffuse;
					if(!normal.empty())
						jentry["Normal"] = normal;
					if(!specular.empty())
						jentry["Specular"] = specular;

					if(textureSet->blendOp != BGSCharacterTint::Template::Entry::kBlendOpDefault)
						jentry["BlendOp"] = CharGenTintObject::WriteBlendOp(textureSet->blendOp);

					if(textureSet->defaultValue != 0.0f)
						jentry["Default"] = textureSet->defaultValue;

					rootEntry["Entries"].append(jentry);
				}
			}
		}
	});

	root.append(rootEntry);

	std::string data = writer.write(root);
	currentFile.WriteBuf(data.c_str(), data.length());
	currentFile.Close();
	return true;
}

Actor * CharGenInterface::GetCurrentActor()
{
	if(!g_characterCreation)
		return nullptr;

	CharacterCreation * characterCreation = g_characterCreation[*g_characterIndex];
	if(!characterCreation)
		return nullptr;

	return characterCreation->actor;
}


void CharGenInterface::UnlockHeadParts()
{
	std::atomic<int> total;
	std::chrono::time_point<std::chrono::system_clock> start, end;

	start = std::chrono::system_clock::now();
	concurrency::parallel_for(UInt32(0), (*g_dataHandler)->arrHDPT.count, [&](const UInt32 & i)
	{
		BGSHeadPart * hdpt;
		(*g_dataHandler)->arrHDPT.GetNthItem(i, hdpt);

		if(hdpt->conditions) {
			hdpt->conditions = nullptr;
			total++;
		}
	});
	end = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = end-start;
	_MESSAGE("Completed head part unlock of %d parts in %f seconds", total, elapsed_seconds.count());
}

void CharGenInterface::UnlockTints()
{
	std::atomic<int> total;
	std::chrono::time_point<std::chrono::system_clock> start, end;

	start = std::chrono::system_clock::now();
	concurrency::parallel_for(UInt32(0), (*g_dataHandler)->arrRACE.count, [&](const UInt32 & i)
	{
		TESRace * race;
		(*g_dataHandler)->arrRACE.GetNthItem(i, race);

		for(UInt32 i = 0; i <= 1; i++) {
			auto chargenData = race->chargenData[i];
			if(chargenData) {
				auto tintData = chargenData->tintData;
				if(tintData) {
					for(UInt32 j = 0; j < tintData->count; j++) {
						auto tintCategory = tintData->entries[j];
						for(UInt32 k = 0; k < tintCategory->entry.count; k++) {
							auto tintEntry = tintCategory->entry[k];

							if(tintEntry->conditions) {
								tintEntry->conditions = nullptr;
								total++;
							}
						}
					}
				}
			}
		}
	});
	end = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = end-start;
	_MESSAGE("Completed tint unlock of %d tints in %f seconds", total, elapsed_seconds.count());
}

void CharGenInterface::ProcessHairColor(NiAVObject * node, BGSColorForm * colorForm, BSLightingShaderMaterialBase * shaderMaterial)
{
	VisitObjects(node, [&](NiAVObject* object)
	{
		BSTriShape * trishape = object->GetAsBSTriShape();
		if(trishape) {
			BSLightingShaderProperty * lightingShader = ni_cast(trishape->shaderProperty, BSLightingShaderProperty);
			if(lightingShader && lightingShader->flags & BSLightingShaderProperty::kShaderFlags_GrayscaleToPalette) {
				BSLightingShaderMaterialBase * material = static_cast<BSLightingShaderMaterialBase *>(lightingShader->shaderMaterial);
				if(material && material->spLookupTexture) {

					std::string fullPath = material->spLookupTexture->name.c_str();
					std::transform(fullPath.begin(), fullPath.end(), fullPath.begin(), ::tolower);

					fullPath = std::regex_replace(fullPath, std::regex("/+|\\\\+"), "\\"); // Replace multiple slashes or forward slashes with one backslash
					fullPath = std::regex_replace(fullPath, std::regex("^\\\\+"), ""); // Remove all backslashes from the front
					fullPath = std::regex_replace(fullPath, std::regex(".*?textures\\\\"), ""); // Remove everything before and including the textures path root
					
					

					bool bNeedsCustomLUT = (colorForm->flags & 0x8000) == 0x8000;
					bool bUsingCustomLUT = IsLUTUsed(fullPath.c_str());
					bool bEligibleCustomLUT = fullPath.compare(HairGradientPalette) == 0;

					char destBuff[MAX_PATH];
					F4EEFixedString str;
					const char * pNewPalettePath = nullptr;

					// We don't want the custom LUT anymore
					if(bUsingCustomLUT && !bNeedsCustomLUT)
					{
						strcpy_s(destBuff, MAX_PATH, "DATA\\TEXTURES\\");
						strcat_s(destBuff, MAX_PATH, HairGradientPalette);
						pNewPalettePath = destBuff;
					}

					// We are eligible for a custom LUT or are already using one and we need to change texture
					if((bEligibleCustomLUT || bUsingCustomLUT) && bNeedsCustomLUT)
					{
						if(GetLUTFromColor(colorForm, str))
						{
							strcpy_s(destBuff, MAX_PATH, "DATA\\TEXTURES\\");
							strcat_s(destBuff, MAX_PATH, str.c_str());
							pNewPalettePath = destBuff;
						}
					}

					// Create a new TXST with the new palette path in it
					if(pNewPalettePath)
					{
						BSShaderTextureSet * textureSet = CreateBSShaderTextureSet();
						BSShaderTextureSet * otherSet = (BSShaderTextureSet*)material->spTextureSet.m_pObject;
						for(int i = 0; i < 10; i++)
						{
							const char * path = otherSet->textures[i].c_str();
							if(path && path[0] != 0)
							{
								textureSet->SetTextureFilename(i, otherSet->textures[i].c_str());
							}
						}

						//textureSet->Copy((BSShaderTextureSet*)material->spTextureSet.m_pObject);
						textureSet->SetTextureFilename(3, pNewPalettePath);
						material->spTextureSet = textureSet;
						material->ClearTextures();
						CALL_MEMBER_FN(lightingShader, LoadTextureSet)(0);
					}
				}
			}
		}

		return false;
	});
}

const char * CharGenInterface::ProcessEyebrowPath(TESNPC * npc)
{
	auto hairTexturePath = npc->race.race->hairColorLUT.str.c_str();

	BGSColorForm * colorForm = npc->GetHairColor();
	if(colorForm && (colorForm->flags & 0x8000) == 0x8000) {

		std::string fullPath = hairTexturePath;
		std::transform(fullPath.begin(), fullPath.end(), fullPath.begin(), ::tolower);

		fullPath = std::regex_replace(fullPath, std::regex("/+|\\\\+"), "\\"); // Replace multiple slashes or forward slashes with one backslash
		fullPath = std::regex_replace(fullPath, std::regex("^\\\\+"), ""); // Remove all backslashes from the front
		fullPath = std::regex_replace(fullPath, std::regex(".*?textures\\\\"), ""); // Remove everything before and including the textures path


		bool bEligibleCustomLUT = fullPath.compare(HairGradientPalette) == 0;
		if(bEligibleCustomLUT) {
			F4EEFixedString str;
			if(GetLUTFromColor(colorForm, str))
			{
				return str.c_str();
			}
		}
	}

	return hairTexturePath;
}

#include <unordered_set>

bool CharGenInterface::LoadHairColorData(const std::string & filePath, ModInfo * modInfo)
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

		auto colors = root["Colors"];
		if(!colors.isArray())
			return false;

		std::unordered_set<BGSColorForm*> colorForms;

		// traverse instances
		for(auto & item : colors)
		{
			try
			{
				std::vector<BGSColorForm*> entryForms;
				auto formEntry = item["Form"];

				std::function<BGSColorForm*(Json::Value & value)> GetColorForm = [&modInfo, &filePath](Json::Value & value) -> BGSColorForm *
				{
					UInt32 formId = 0;
					sscanf_s(value.asCString(), "%X", &formId);
					UInt32 modIndex = modInfo->modIndex;
					formId |= modIndex << 24;

					TESForm * form = LookupFormByID(formId);
					if(!form) {
						_WARNING("Loading Hair Colors (%s): Could not find form %08X", filePath.c_str(), formId);
						return nullptr;
					}

					BGSColorForm * colorForm = DYNAMIC_CAST(form, TESForm, BGSColorForm);
					if(!colorForm) {
						_WARNING("Loading Hair Colors (%s): Wrong form type %08X", filePath.c_str(), formId);
						return nullptr;
					}

					return colorForm;
				};

				if(formEntry.isString())
				{
					BGSColorForm * colorForm = GetColorForm(formEntry);
					if(colorForm)
						entryForms.push_back(colorForm);
				}
				else if(formEntry.isArray())
				{
					for(auto & formItem : formEntry)
					{
						BGSColorForm * colorForm = GetColorForm(formItem);
						if(colorForm)
							entryForms.push_back(colorForm);
					}
				}
				
				std::string palettePath = item["LUT"].asString();
				UInt32 gender = item["Gender"].asUInt();
				auto raceList = item["Races"];
				
				for(BGSColorForm * colorForm : entryForms)
				{
					colorForm->flags |= 0x8000;
					colorForms.insert(colorForm);
					
					for(auto raceItem : raceList)
					{
						try
						{
							std::string raceName = raceItem.asString();
							TESRace * race = GetRaceByName(raceName);
							if(!race) {
								_WARNING("Loading Hair Colors (%s): Could not find race %s", filePath.c_str(), raceName);
								continue;
							}

							for(UInt32 i = 0; i <= 1; i++)
							{
								if((i == 0 && !(gender & 0x01)) || (i == 1 && !(gender & 0x02)))
									continue;

								auto charGenData = race->chargenData[i];
								if(!charGenData)
									continue;

								if(charGenData->colors)
									charGenData->colors->Push(colorForm);

								auto paletteStr = F4EEFixedString(palettePath.c_str());
								m_LUTMap.emplace(std::make_pair(colorForm, paletteStr));
								m_LUTs.insert(paletteStr);
							}
						}
						catch(const std::exception& e)
						{
							_ERROR("Loading Hair Colors (%s): %s", filePath.c_str(), e.what());
						}
					}
				}
			}
			catch(const std::exception& e)
			{
				_ERROR("Loading Hair Colors (%s): %s", filePath.c_str(), e.what());
			}
		}

		TESForm * form = LookupFormByID(0x1a4ae8); // ChargenOptionsSortList
		if(form) {
			BGSListForm * orderedList = DYNAMIC_CAST(form, TESForm, BGSListForm);
			if(orderedList) {
				TESForm * colorForm = nullptr;
				int i = 0;
				for(i = 0; i < orderedList->forms.count; i++)
				{
					TESForm * entry;
					orderedList->forms.GetNthItem(i, entry);
					BGSColorForm * colorEntry = DYNAMIC_CAST(entry, TESForm, BGSColorForm);
					if(colorEntry && !colorForm) {
						colorForm = colorEntry;
					}
					else if(colorForm && !colorEntry) {
						break;
					}
				}

				for(auto & colorForm : colorForms)
				{
					orderedList->forms.Insert(i, colorForm);
				}
			}
		}
	}

	return true;
}

bool CharGenInterface::IsLUTUsed(const F4EEFixedString & str)
{
	auto it = m_LUTs.find(str);
	if(it != m_LUTs.end())
	{
		return true;
	}

	return false;
}

bool CharGenInterface::GetLUTFromColor(BGSColorForm * color, F4EEFixedString & str)
{
	auto it = m_LUTMap.find(color);
	if(it != m_LUTMap.end())
	{
		str = it->second;
		return true;
	}

	return false;
}
/*
void CharGen::SetBaseTintTextureOverride(BGSTextureSet * textureSet)
{
	m_faceTextureOverride.faceTextures = textureSet;
}

void CharGen::LockBaseTextureOverride()
{
	m_faceTextureLock.lock();
}

void CharGen::ReleaseBaseTextureOverride()
{
	m_faceTextureLock.unlock();
}

TESNPC::HeadData * CharGen::ProcessHeadData(TESNPC * npc)
{
	std::lock_guard<std::mutex> locker(m_faceTextureLock);
	if(m_faceTextureOverride.faceTextures) {
		return &m_faceTextureOverride;
	}

	return npc->headData;
}*/
