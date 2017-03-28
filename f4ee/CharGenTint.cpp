#include "CharGenTint.h"

#include "f4se/GameAPI.h"
#include "f4se/GameData.h"
#include "f4se/GameRTTI.h"

#include "CharGen.h"
#include "Utilities.h"

#include <set>

std::map<std::string, UInt32> g_flagMap;
std::map<std::string, UInt32> g_slotMap;
std::map<std::string, UInt32> g_blendMap;

void InitFlagMap()
{
	g_flagMap.emplace(std::make_pair("OnOff", (UInt32)BGSCharacterTint::Template::Entry::kFlagOnOff));
	g_flagMap.emplace(std::make_pair("ChargenDetail", (UInt32)BGSCharacterTint::Template::Entry::kFlagChargenDetail));
	g_flagMap.emplace(std::make_pair("TakesSkinTone", (UInt32)BGSCharacterTint::Template::Entry::kFlagTakesSkinTone));
}

void InitSlotMap()
{
	g_slotMap.emplace(std::make_pair("ForeheadMask", (UInt32)BGSCharacterTint::Template::Entry::kSlotForeheadMask));
	g_slotMap.emplace(std::make_pair("EyesMask", (UInt32)BGSCharacterTint::Template::Entry::kSlotEyesMask));
	g_slotMap.emplace(std::make_pair("NoseMask", (UInt32)BGSCharacterTint::Template::Entry::kSlotNoseMask));
	g_slotMap.emplace(std::make_pair("EarsMask", (UInt32)BGSCharacterTint::Template::Entry::kSlotEarsMask));
	g_slotMap.emplace(std::make_pair("CheeksMask", (UInt32)BGSCharacterTint::Template::Entry::kSlotCheeksMask));
	g_slotMap.emplace(std::make_pair("MouthMask", (UInt32)BGSCharacterTint::Template::Entry::kSlotMouthMask));
	g_slotMap.emplace(std::make_pair("NeckMask", (UInt32)BGSCharacterTint::Template::Entry::kSlotNeckMask));
	g_slotMap.emplace(std::make_pair("LipColor", (UInt32)BGSCharacterTint::Template::Entry::kSlotLipColor));
	g_slotMap.emplace(std::make_pair("CheekColor", (UInt32)BGSCharacterTint::Template::Entry::kSlotCheekColor));
	g_slotMap.emplace(std::make_pair("Eyeliner", (UInt32)BGSCharacterTint::Template::Entry::kSlotEyeliner));
	g_slotMap.emplace(std::make_pair("EyeSocketUpper", (UInt32)BGSCharacterTint::Template::Entry::kSlotEyeSocketUpper));
	g_slotMap.emplace(std::make_pair("EyeSocketLower", (UInt32)BGSCharacterTint::Template::Entry::kSlotEyeSocketLower));
	g_slotMap.emplace(std::make_pair("SkinTone", (UInt32)BGSCharacterTint::Template::Entry::kSlotSkinTone));
	g_slotMap.emplace(std::make_pair("Paint", (UInt32)BGSCharacterTint::Template::Entry::kSlotPaint));
	g_slotMap.emplace(std::make_pair("LaughLines", (UInt32)BGSCharacterTint::Template::Entry::kSlotLaughLines));
	g_slotMap.emplace(std::make_pair("CheekColorLower", (UInt32)BGSCharacterTint::Template::Entry::kSlotCheekColorLower));
	g_slotMap.emplace(std::make_pair("Nose", (UInt32)BGSCharacterTint::Template::Entry::kSlotNose));
	g_slotMap.emplace(std::make_pair("Chin", (UInt32)BGSCharacterTint::Template::Entry::kSlotChin));
	g_slotMap.emplace(std::make_pair("Neck", (UInt32)BGSCharacterTint::Template::Entry::kSlotNeck));
	g_slotMap.emplace(std::make_pair("Forehead", (UInt32)BGSCharacterTint::Template::Entry::kSlotForehead));
	g_slotMap.emplace(std::make_pair("Dirt", (UInt32)BGSCharacterTint::Template::Entry::kSlotDirt));
	g_slotMap.emplace(std::make_pair("Scar", (UInt32)BGSCharacterTint::Template::Entry::kSlotScar));
	g_slotMap.emplace(std::make_pair("FaceDetail", (UInt32)BGSCharacterTint::Template::Entry::kSlotFaceDetail));
	g_slotMap.emplace(std::make_pair("Brows", (UInt32)BGSCharacterTint::Template::Entry::kSlotBrows));
}

void InitBlendMap()
{
	g_blendMap.emplace(std::make_pair("Default", (UInt32)BGSCharacterTint::Template::Entry::kBlendOpDefault));
	g_blendMap.emplace(std::make_pair("Multiply", (UInt32)BGSCharacterTint::Template::Entry::kBlendOpMultiply));
	g_blendMap.emplace(std::make_pair("Overlay", (UInt32)BGSCharacterTint::Template::Entry::kBlendOpOverlay));
	g_blendMap.emplace(std::make_pair("SoftLight", (UInt32)BGSCharacterTint::Template::Entry::kBlendOpSoftLight));
	g_blendMap.emplace(std::make_pair("HardLight", (UInt32)BGSCharacterTint::Template::Entry::kBlendOpHardLight));
}

UInt32 CharGenTintObject::ParseSlot(const std::string & slotName)
{
	if(g_slotMap.empty())
		InitSlotMap();

	auto iter = g_slotMap.find(slotName.c_str());
	if(iter != g_slotMap.end())
	{
		return iter->second;
	}

	_ERROR("Unknown slot name: %s", slotName.c_str());
	return BGSCharacterTint::Template::Entry::kSlotFaceDetail;
}

std::string CharGenTintObject::WriteSlot(UInt32 slotId)
{
	if(g_slotMap.empty())
		InitSlotMap();

	for(auto & slot : g_slotMap)
	{
		if(slotId == slot.second)
		{
			return slot.first;
		}
	}

	return "";
}

UInt32 CharGenTintObject::ParseBlendOp(const std::string & blendName)
{
	if(g_blendMap.empty())
		InitBlendMap();

	auto iter = g_blendMap.find(blendName);
	if(iter != g_blendMap.end())
	{
		return iter->second;
	}

	_ERROR("Unknown blend operation: %s", blendName.c_str());
	return BGSCharacterTint::Template::Entry::kBlendOpDefault;
}

std::string CharGenTintObject::WriteBlendOp(UInt32 blendOp)
{
	if(g_blendMap.empty())
		InitBlendMap();

	for(auto & blend : g_blendMap)
	{
		if(blendOp == blend.second)
		{
			return blend.first;
		}
	}

	return "";
}

UInt32 CharGenTintObject::ParseFlags(const Json::Value & value)
{
	if(g_flagMap.empty())
		InitFlagMap();

	UInt32 flags = 0;
	for(auto & str : value)
	{
		auto iter = g_flagMap.find(str.asString());
		if(iter != g_flagMap.end())
		{
			flags |= iter->second;
		}
	}

	return flags;
}

void CharGenTintObject::WriteFlags(UInt32 flags, Json::Value & value)
{
	if(g_flagMap.empty())
		InitFlagMap();

	for(auto & flag : g_flagMap)
	{
		if((flags & flag.second) == flag.second)
			value.append(flag.first);
	}
}

void CharGenTintObject::ForEachGender(const TESRace * race, UInt8 gender, std::function<void(tArray<CharacterCreation::TintData *>*, UInt8)> func)
{
	if(gender == 2)
	{
		for(UInt8 i = 0; i <= 1; i++)
		{
			auto chargenData = race->chargenData[i];
			if(chargenData)
			{
				auto tints = chargenData->tintData;
				if(tints)
				{
					func(tints, i);
				}
			}
		}
	}
	else
	{
		auto chargenData = race->chargenData[gender];
		if(chargenData)
		{
			auto tints = chargenData->tintData;
			if(tints)
			{
				func(tints, gender);
			}
		}
	}
}

CharacterCreation::TintData * CharGenTintObject::GetCategoryByName(const tArray<CharacterCreation::TintData*> * data, const char * name)
{
	for(UInt32 i = 0; i < data->count; i++)
	{
		CharacterCreation::TintData * tintData;
		data->GetNthItem(i, tintData);
		if(_stricmp(tintData->category.c_str(), name) == 0)
			return tintData;
	}

	return nullptr;
}

CharacterCreation::TintData * CharGenTintObject::GetCategoryByID(const tArray<CharacterCreation::TintData*> * data, UInt32 id)
{
	for(UInt32 i = 0; i < data->count; i++)
	{
		CharacterCreation::TintData * tintData;
		data->GetNthItem(i, tintData);
		if(tintData->type == id)
			return tintData;
	}

	return nullptr;
}

BGSCharacterTint::Template::Entry * CharGenTintObject::GetTemplateByID(const tArray<BGSCharacterTint::Template::Entry*> * data, UInt32 id)
{
	for(UInt32 i = 0; i < data->count; i++)
	{
		BGSCharacterTint::Template::Entry * tintData;
		data->GetNthItem(i, tintData);
		if(tintData->templateIndex == id)
			return tintData;
	}

	return nullptr;
}

CharacterCreation::TintData * CharGenTintObject::CreateCategory()
{
	CharacterCreation::TintData* data = (CharacterCreation::TintData*)Heap_Allocate(sizeof(CharacterCreation::TintData));
	memset(data, 0, sizeof(CharacterCreation::TintData));
	return data;
}

void CharGenTintObject::SetCategory(CharacterCreation::TintData * tintData)
{
	tintData->category = name.c_str();
	tintData->type = categoryId;
	tintData->unk08 = -1;
}

bool CharGenTintObject::Apply(const TESRace * race, UInt8 gender)
{
	try
	{
		ForEachGender(race, gender, [&](tArray<CharacterCreation::TintData *>* tints, UInt8 genderId)
		{
			CharacterCreation::TintData * tintData = GetCategoryByID(tints, categoryId);
			if(!tintData)
			{
				if(name.empty())
				{
					throw std::exception("Invalid category name for identifier");
				}


				tintData = CreateCategory();
				SetCategory(tintData);
				tints->Push(tintData);
			}
			else
			{
				// Assign new category data
				SetCategory(tintData);
			}
		});

		return true;
	}
	catch(const std::exception& e)
	{
		_ERROR(e.what());
	}

	return false;
}

bool CharGenTintObject::Parse(const Json::Value & entry)
{
	try
	{
		if(entry.isMember("Name")) {
			name = entry["Name"].asString();
			modified |= kModified_Name;
		}
		return true;
	}
	catch(const std::exception& e)
	{
		_ERROR(e.what());
	}

	return false;
}

bool CharGenTintBase::Parse(const Json::Value & entry)
{
	__super::Parse(entry);
	try
	{
		if(entry.isMember("Category")) {
			categoryId = entry["Category"].asUInt();
		}
		if(entry.isMember("FixedCategory")) {
			fixedCategory = entry["FixedCategory"].asString();
		}
		if(entry.isMember("Slot")) {
			slot = ParseSlot(entry["Slot"].asString());
			modified |= kModified_Slot;
		}

		if(entry.isMember("Flags")) {
			flags = ParseFlags(entry["Flags"]);
			modified |= kModified_Flags;
		}
		return true;
	}
	catch(const std::exception& e)
	{
		_ERROR(e.what());
	}

	return false;
}

void CharGenTintBase::Set(BGSCharacterTint::Template::Entry* entry)
{
	if(IsFlagSet(kModified_Name))
		entry->name = name.c_str();
	entry->templateIndex = templateId;

	if(IsFlagSet(kModified_Slot))
		entry->slot = slot;

	if(IsFlagSet(kModified_Flags))
		entry->flags = flags;
}

bool CharGenTintBase::Apply(const TESRace * race, UInt8 gender)
{
	try
	{
		ForEachGender(race, gender, [&](tArray<CharacterCreation::TintData *>* tints, UInt8 genderId)
		{
			CharacterCreation::TintData * tintData = nullptr;
			if(categoryId != 0)
				tintData = GetCategoryByID(tints, categoryId);
			else
				tintData = GetCategoryByName(tints, fixedCategory.c_str());

			if(tintData)
			{
				if(name.empty())
				{
					throw std::exception("Invalid template name");
				}

				BGSCharacterTint::Template::Entry* tintTemplate = GetTemplateByID(&tintData->entry, templateId);
				if(!tintTemplate)
				{
					// Create new entry
					tintTemplate = Create();
					Set(tintTemplate);
					tintData->entry.Push(tintTemplate);
				}
				else
				{
					// Assign to existing template
					Set(tintTemplate);
				}
			}
		});

		return true;
	}
	catch(const std::exception& e)
	{
		_ERROR(e.what());
	}

	return false;
}

bool CharGenTintMask::Parse(const Json::Value & entry)
{
	__super::Parse(entry);
	try
	{
		if(entry.isMember("Texture"))
		{
			texture = entry["Texture"].asString();
			modified |= kModified_Texture;
		}
		if(entry.isMember("BlendOp"))
		{
			blendOp = ParseBlendOp(entry["BlendOp"].asString());
			modified |= kModified_BlendOp;
		}
		return true;
	}
	catch(const std::exception& e)
	{
		_ERROR(e.what());
	}

	return false;
}

BGSCharacterTint::Template::Entry* CharGenTintMask::Create()
{
	auto pMask = BGSCharacterTint::Template::Mask::Create();
	pMask->texture = "";
	return pMask;
}

void CharGenTintMask::Set(BGSCharacterTint::Template::Entry* entry)
{
	__super::Set(entry);

	BGSCharacterTint::Template::Mask* pMask = (BGSCharacterTint::Template::Mask*)Runtime_DynamicCast(entry, RTTI_BGSCharacterTint__Template__Entry, RTTI_BGSCharacterTint__Template__Mask);
	if(pMask)
	{
		if(IsFlagSet(kModified_BlendOp))
			pMask->blendOp = blendOp;

		if(IsFlagSet(kModified_Texture))
			pMask->texture = texture.c_str();
	}
}


bool CharGenTintPalette::Parse(const Json::Value & entry)
{
	__super::Parse(entry);
	try
	{
		if(entry.isMember("Texture"))
		{
			texture = entry["Texture"].asString();
			modified |= kModified_Texture;
		}
		
		auto entries = entry["Colors"];
		for(auto & colorEntry : entries)
		{
			BGSCharacterTint::Template::Palette::ColorData data;
			memset(&data, 0, sizeof(BGSCharacterTint::Template::Palette::ColorData));
			data.colorID = colorEntry["Id"].asUInt();
			data.colorForm = DYNAMIC_CAST(GetFormFromIdentifier(colorEntry["Form"].asString()), TESForm, BGSColorForm);
			data.alpha = colorEntry["Alpha"].asFloat();

			if(colorEntry.isMember("BlendOp"))
				data.blendOp = ParseBlendOp(colorEntry["BlendOp"].asString());

			if(data.colorForm)
				colors.push_back(data);
		}

		return true;
	}
	catch(const std::exception& e)
	{
		_ERROR(e.what());
	}

	return false;
}

BGSCharacterTint::Template::Entry* CharGenTintPalette::Create()
{
	auto pPalette = BGSCharacterTint::Template::Palette::Create();
	pPalette->texture = "";
	return pPalette;
}

void CharGenTintPalette::Set(BGSCharacterTint::Template::Entry* entry)
{
	__super::Set(entry);

	BGSCharacterTint::Template::Palette* pPalette = (BGSCharacterTint::Template::Palette*)Runtime_DynamicCast(entry, RTTI_BGSCharacterTint__Template__Entry, RTTI_BGSCharacterTint__Template__Palette);
	if(pPalette)
	{
		if(IsFlagSet(kModified_Texture))
			pPalette->texture = texture.c_str();

		std::set<UInt16> colorIds;
		for(UInt32 i = 0; i < pPalette->colors.count; i++)
		{
			BGSCharacterTint::Template::Palette::ColorData colorData;
			pPalette->colors.GetNthItem(i, colorData);

			colorIds.insert(colorData.colorID);
		}

		// Merge new colors into the list
		for(auto & colorData : colors)
		{
			// Only add color Ids not in the list
			if(colorIds.find(colorData.colorID) == colorIds.end())
				pPalette->colors.Push(colorData);
		}
	}
}

bool CharGenTintTextureSet::Parse(const Json::Value & entry)
{
	__super::Parse(entry);
	try
	{
		if(entry.isMember("Diffuse"))
		{
			diffuse = entry["Diffuse"].asString();
			modified |= kModified_Diffuse;
		}

		if(entry.isMember("Normal"))
		{
			normal = entry["Normal"].asString();
			modified |= kModified_Normal;
		}

		if(entry.isMember("Specular"))
		{
			specular = entry["Specular"].asString();
			modified |= kModified_Specular;
		}

		if(entry.isMember("BlendOp"))
		{
			blendOp = ParseBlendOp(entry["BlendOp"].asString());
			modified |= kModified_BlendOp;
		}

		if(entry.isMember("Default"))
		{
			defaultValue = entry["Default"].asFloat();
			modified |= kModified_Default;
		}

		return true;
	}
	catch(const std::exception& e)
	{
		_ERROR(e.what());
	}

	return false;
}

BGSCharacterTint::Template::Entry* CharGenTintTextureSet::Create()
{
	auto textureSet = BGSCharacterTint::Template::TextureSet::Create();
	textureSet->diffuse = "";
	textureSet->normal = "";
	textureSet->specular = "";
	return textureSet;
}

void CharGenTintTextureSet::Set(BGSCharacterTint::Template::Entry* entry)
{
	__super::Set(entry);

	BGSCharacterTint::Template::TextureSet* pTextureSet = (BGSCharacterTint::Template::TextureSet*)Runtime_DynamicCast(entry, RTTI_BGSCharacterTint__Template__Entry, RTTI_BGSCharacterTint__Template__TextureSet);
	if(pTextureSet)
	{
		if(IsFlagSet(kModified_Diffuse))
			pTextureSet->diffuse = diffuse.c_str();
		if(IsFlagSet(kModified_Normal))
			pTextureSet->normal = normal.c_str();
		if(IsFlagSet(kModified_Specular))
			pTextureSet->specular = specular.c_str();
		if(IsFlagSet(kModified_BlendOp))
			pTextureSet->blendOp = blendOp;
		if(IsFlagSet(kModified_Default))
			pTextureSet->defaultValue = defaultValue;
	}
}