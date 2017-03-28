#pragma once

#include "json\json.h"
#include <functional>

#include "f4se/GameCustomization.h"

class TESRace;

class CharGenTintObject
{
public:
	CharGenTintObject(UInt32 id) : categoryId(id), modified(0) { }
	
	virtual bool Apply(const TESRace * race, UInt8 gender);
	virtual bool Parse(const Json::Value & entry);

	virtual CharacterCreation::TintData * CreateCategory();
	virtual void SetCategory(CharacterCreation::TintData * category);

	virtual BGSCharacterTint::Template::Entry* Create() { return nullptr; }
	virtual void Set(BGSCharacterTint::Template::Entry* entry) { }

	static void ForEachGender(const TESRace * race, UInt8 gender, std::function<void(tArray<CharacterCreation::TintData *>*, UInt8)> func);
	static CharacterCreation::TintData * GetCategoryByID(const tArray<CharacterCreation::TintData*> * data, UInt32 id);
	static CharacterCreation::TintData * GetCategoryByName(const tArray<CharacterCreation::TintData*> * data, const char * name);
	static BGSCharacterTint::Template::Entry * GetTemplateByID(const tArray<BGSCharacterTint::Template::Entry*> * data, UInt32 id);

	static UInt32 ParseSlot(const std::string & slotName);
	static std::string WriteSlot(UInt32 blendOp);
	
	static UInt32 ParseBlendOp(const std::string & blendName);
	static std::string WriteBlendOp(UInt32 blendOp);

	static UInt32 ParseFlags(const Json::Value & value);
	static void WriteFlags(UInt32 flags, Json::Value & value);

	enum ModifiedFlag
	{
		kModified_Name = (1 << 0),
		kModified_Slot = (1 << 1),
		kModified_Flags = (1 << 2),
		kModified_BlendOp = (1 << 3),
		kModified_Texture = (1 << 4),
		kModified_Diffuse = (1 << 5),
		kModified_Normal = (1 << 6),
		kModified_Specular = (1 << 7),
		kModified_Default = (1 << 8),
	};

	bool IsFlagSet(UInt32 flag) { return (modified & flag) == flag; }

	UInt32 modified;
	std::string name;
	UInt32 categoryId;
	std::string fixedCategory;
};

class CharGenTintBase : public CharGenTintObject
{
public:
	CharGenTintBase(UInt32 id) : CharGenTintObject(0), templateId(id), slot(0), flags(0) { }
	virtual ~CharGenTintBase() { }

	virtual CharacterCreation::TintData * CreateCategory() { return nullptr; }
	virtual void SetCategory(CharacterCreation::TintData * category) { }

	virtual void Set(BGSCharacterTint::Template::Entry* entry);

	virtual bool Apply(const TESRace * race, UInt8 gender);
	virtual bool Parse(const Json::Value & entry);

	UInt32 templateId;
	UInt32 slot;
	UInt32 flags;
};

class CharGenTintMask : public CharGenTintBase
{
public:
	CharGenTintMask(UInt32 id) : CharGenTintBase(id), blendOp(0) { }
	virtual ~CharGenTintMask() { }

	virtual bool Parse(const Json::Value & entry);
	virtual BGSCharacterTint::Template::Entry* Create();
	virtual void Set(BGSCharacterTint::Template::Entry* entry);

	std::string		texture;
	UInt32			blendOp;
};

class CharGenTintPalette : public CharGenTintBase
{
public:
	CharGenTintPalette(UInt32 id) : CharGenTintBase(id) { }
	virtual ~CharGenTintPalette() { }

	virtual bool Parse(const Json::Value & entry);
	virtual BGSCharacterTint::Template::Entry* Create();
	virtual void Set(BGSCharacterTint::Template::Entry* entry);

	std::string		texture;
	std::vector<BGSCharacterTint::Template::Palette::ColorData> colors;
};

class CharGenTintTextureSet : public CharGenTintBase
{
public:
	CharGenTintTextureSet(UInt32 id) : CharGenTintBase(id), blendOp(0), defaultValue(0.0f) { }
	virtual ~CharGenTintTextureSet() { }

	virtual bool Parse(const Json::Value & entry);
	virtual BGSCharacterTint::Template::Entry* Create();
	virtual void Set(BGSCharacterTint::Template::Entry* entry);

	std::string diffuse;
	std::string normal;
	std::string specular;
	UInt32	blendOp;
	float	defaultValue;
};