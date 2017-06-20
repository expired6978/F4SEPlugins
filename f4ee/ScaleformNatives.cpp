#include "ScaleformNatives.h"

#include "f4se/GameCustomization.h"
#include "f4se/ScaleformValue.h"
#include "f4se/GameMenus.h"
#include "f4se/GameReferences.h"
#include "f4se/GameObjects.h"
#include "f4se/GameInput.h"
#include "f4se/GameRTTI.h"

#include "CharGenInterface.h"
#include "BodyMorphInterface.h"
#include "OverlayInterface.h"
#include "StringTable.h"

#include <set>

extern std::set<UInt32> g_presetNPCs;
extern CharGenInterface g_charGenInterface;
extern BodyMorphInterface g_bodyMorphInterface;
extern OverlayInterface g_overlayInterface;
extern StringTable g_stringTable;
extern bool g_bEnableBodyMorphs;
extern bool g_bEnableOverlays;

extern F4SETaskInterface * g_task;

template<typename T>
inline void Register(GFxValue * dst, const char * name, T value)
{

}

template<>
inline void Register(GFxValue * dst, const char * name, SInt32 value)
{
	GFxValue	fxValue;
	fxValue.SetInt(value);
	dst->SetMember(name, &fxValue);
}

template<>
inline void Register(GFxValue * dst, const char * name, UInt32 value)
{
	GFxValue	fxValue;
	fxValue.SetUInt(value);
	dst->SetMember(name, &fxValue);
}

template<>
inline void Register(GFxValue * dst, const char * name, double value)
{
	GFxValue	fxValue;
	fxValue.SetNumber(value);
	dst->SetMember(name, &fxValue);
}

template<>
inline void Register(GFxValue * dst, const char * name, bool value)
{
	GFxValue	fxValue;
	fxValue.SetBool(value);
	dst->SetMember(name, &fxValue);
}

inline void RegisterUnmanagedString(GFxValue * dst, const char * name, const char * str)
{
	GFxValue	fxValue;
	fxValue.SetString(str);
	dst->SetMember(name, &fxValue);
}

inline void RegisterString(GFxValue * dst,  GFxMovieRoot * view, const char * name, const char * str)
{
	GFxValue	fxValue;
	view->CreateString(&fxValue, str);
	dst->SetMember(name, &fxValue);
}

void F4EEScaleform_LoadPreset::Invoke(Args * args)
{
	ASSERT(args->numArgs >= 1);
	ASSERT(args->args[0].GetType() == GFxValue::kType_String);

	args->result->SetInt(g_charGenInterface.LoadPreset(args->args[0].GetString()));

	CharacterCreation * characterCreation = g_characterCreation[*g_characterIndex];
	if(characterCreation) {
		characterCreation->unk516 = 1;
		characterCreation->unk517 = 1;

		characterCreation->dirty = 1;
	}

	IMenu * menu = (*g_ui)->GetMenuByMovie(args->movie);
	if(menu) {
		LooksMenu * looksMenu = static_cast<LooksMenu*>(menu);
		CALL_MEMBER_FN(looksMenu, LoadCharacterParameters)();
	}
}

void F4EEScaleform_SetCurrentBoneRegionID::Invoke(Args * args)
{
	ASSERT(args->numArgs >= 2);
	ASSERT(args->args[0].GetType() == GFxValue::kType_UInt || args->args[0].GetType() == GFxValue::kType_Int);
	ASSERT(args->args[1].GetType() == GFxValue::kType_UInt || args->args[1].GetType() == GFxValue::kType_Int);

	IMenu * menu = (*g_ui)->GetMenuByMovie(args->movie);
	if(menu) {
		LooksMenu * looksMenu = static_cast<LooksMenu*>(menu);
		looksMenu->nextBoneID = args->args[0].GetUInt();
		looksMenu->currentBoneID = args->args[1].GetUInt();
	}
}

void F4EEScaleform_SavePreset::Invoke(Args * args)
{
	ASSERT(args->numArgs >= 1);
	ASSERT(args->args[0].GetType() == GFxValue::kType_String);

	args->result->SetInt(g_charGenInterface.SavePreset(args->args[0].GetString()));
}

void F4EEScaleform_ReadPreset::Invoke(Args * args)
{
	ASSERT(args->numArgs >= 1);
	ASSERT(args->args[0].GetType() == GFxValue::kType_String);

	const char	* strPath = args->args[0].GetString();
}

void F4EEScaleform_AllowTextInput::Invoke(Args * args)
{
	ASSERT(args->numArgs >= 1);
	ASSERT(args->args[0].GetType() == GFxValue::kType_Bool);
	
	(*g_inputMgr)->AllowTextInput(args->args[0].GetBool());
}

void F4EEScaleform_GetBodySliders::Invoke(Args * args)
{	
	CharacterCreation * characterCreation = g_characterCreation[*g_characterIndex];
	if(characterCreation) {
		Actor * actor = characterCreation->actor;
		TESNPC * npc = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
		if(npc) {
			UInt8 gender = CALL_MEMBER_FN(npc, GetSex)();

			args->movie->movieRoot->CreateArray(args->result);

			std::vector<BodySliderPtr> sliders;
			g_bodyMorphInterface.ForEachSlider(gender, [&sliders](const BodySliderPtr & slider) {
				sliders.push_back(slider);
			});

			std::sort(sliders.begin(), sliders.end(), [&](const BodySliderPtr & a, const BodySliderPtr & b)
			{
				if(a->sort == b->sort)
					return std::string(a->name.c_str()) < std::string(b->name.c_str());
				else
					return a->sort < b->sort;
			});

			for(const BodySliderPtr & slider : sliders) {
				GFxValue sliderInfo;
				args->movie->movieRoot->CreateObject(&sliderInfo);
				RegisterString(&sliderInfo, args->movie->movieRoot, "name", slider->name);
				RegisterString(&sliderInfo, args->movie->movieRoot, "morph", slider->morph);
				Register<double>(&sliderInfo, "value", g_bodyMorphInterface.GetMorph(actor, gender == 1 ? true : false, slider->morph, nullptr));
				Register<double>(&sliderInfo, "minimum", slider->minimum);
				Register<double>(&sliderInfo, "maximum", slider->maximum);
				Register<double>(&sliderInfo, "interval", slider->interval);
				args->result->PushBack(&sliderInfo);
			}
		}
	}
}

void F4EEScaleform_SetBodyMorph::Invoke(Args * args)
{
	ASSERT(args->numArgs >= 2);
	ASSERT(args->args[0].GetType() == GFxValue::kType_String);
	ASSERT(args->args[1].GetType() == GFxValue::kType_Number);

	CharacterCreation * characterCreation = g_characterCreation[*g_characterIndex];
	if(characterCreation) {
		Actor * actor = characterCreation->actor;
		TESNPC * npc = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
		if(npc) {
			UInt8 gender = CALL_MEMBER_FN(npc, GetSex)();
			g_bodyMorphInterface.SetMorph(actor, gender == 1 ? true : false, args->args[0].GetString(), nullptr, args->args[1].GetNumber());
		}
		
	}
}

void F4EEScaleform_CloneBodyMorphs::Invoke(Args * args)
{
	bool bVerify = true;
	if(args->numArgs >= 1) {
		bVerify = args->args[0].GetBool(); // Used to verify whether the target is the starting character's dummy actor
	}
	CharacterCreation * characterCreation = g_characterCreation[*g_characterIndex];
	if(characterCreation) {
		Actor * actor = characterCreation->actor;
		TESNPC * npc = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);

		if(actor != (*g_player) && ((bVerify && (npc == (*g_customizationDummy1) || npc == (*g_customizationDummy2))) || !bVerify)) {
			if(g_bEnableBodyMorphs) {
				g_bodyMorphInterface.CloneMorphs(actor, (*g_player));
				g_bodyMorphInterface.UpdateMorphs(*g_player);
			}
		}
	}
}

void F4EEScaleform_UpdateBodyMorphs::Invoke(Args * args)
{
	CharacterCreation * characterCreation = g_characterCreation[*g_characterIndex];
	if(characterCreation && g_bEnableBodyMorphs) {
		g_bodyMorphInterface.UpdateMorphs(characterCreation->actor);
	}
}

#include <Shlwapi.h>
#include <functional>

void ReadFileDirectory(const char * lpFolder, const char ** lpFilePattern, UInt32 numPatterns, bool recursive, std::function<void(char*, WIN32_FIND_DATA &, bool)> file)
{
	char szFullPattern[MAX_PATH];
	WIN32_FIND_DATA FindFileData;
	HANDLE hFindFile;

	if(recursive)
	{
		// first we are going to process any subdirectories
		PathCombine(szFullPattern, lpFolder, "*");
		hFindFile = FindFirstFile(szFullPattern, &FindFileData);
		if (hFindFile != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					// found a subdirectory; recurse into it
					PathCombine(szFullPattern, lpFolder, FindFileData.cFileName);
					if (FindFileData.cFileName[0] == '.')
						continue;

					file(szFullPattern, FindFileData, true);
				}
			} while (FindNextFile(hFindFile, &FindFileData));
			FindClose(hFindFile);
		}
	}
	// now we are going to look for the matching files
	for (UInt32 i = 0; i < numPatterns; i++)
	{
		PathCombine(szFullPattern, lpFolder, lpFilePattern[i]);
		hFindFile = FindFirstFile(szFullPattern, &FindFileData);
		if (hFindFile != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					// found a file; do something with it
					PathCombine(szFullPattern, lpFolder, FindFileData.cFileName);
					file(szFullPattern, FindFileData, false);
				}
			} while (FindNextFile(hFindFile, &FindFileData));
			FindClose(hFindFile);
		}
	}
}

void F4EEScaleform_GetExternalFiles::Invoke(Args * args)
{
	ASSERT(args->numArgs >= 3);
	ASSERT(args->args[0].GetType() == GFxValue::kType_String);
	ASSERT(args->args[1].GetType() == GFxValue::kType_Array);
	ASSERT(args->args[2].GetType() == GFxValue::kType_Bool);

	const char * path = args->args[0].GetString();
	
	UInt32 numPatterns = args->args[1].GetArraySize();

	const char ** patterns = (const char **)ScaleformHeap_Allocate(numPatterns * sizeof(const char*));
	for (UInt32 i = 0; i < numPatterns; i++) {
		GFxValue str;
		args->args[1].GetElement(i, &str);
		patterns[i] = str.GetString();
	}

	args->movie->movieRoot->CreateArray(args->result);

	ReadFileDirectory(path, patterns, numPatterns, args->args[2].GetBool(), [args](char* dirPath, WIN32_FIND_DATA & fileData, bool dir)
	{
		GFxValue fileInfo;
		args->movie->movieRoot->CreateObject(&fileInfo);
		RegisterString(&fileInfo, args->movie->movieRoot, "path", dirPath);
		RegisterString(&fileInfo, args->movie->movieRoot, "name", fileData.cFileName);
		UInt64 fileSize = (UInt64)fileData.nFileSizeHigh << 32 | fileData.nFileSizeLow;
		Register<UInt32>(&fileInfo, "size", fileSize);
		SYSTEMTIME sysTime;
		FileTimeToSystemTime(&fileData.ftLastWriteTime, &sysTime);
		GFxValue date;
		GFxValue params[7];
		params[0].SetNumber(sysTime.wYear);
		params[1].SetNumber(sysTime.wMonth - 1); // Flash Month is 0-11, System time is 1-12
		params[2].SetNumber(sysTime.wDay);
		params[3].SetNumber(sysTime.wHour);
		params[4].SetNumber(sysTime.wMinute);
		params[5].SetNumber(sysTime.wSecond);
		params[6].SetNumber(sysTime.wMilliseconds);
		args->movie->movieRoot->CreateObject(&date, "Date", params, 7);
		fileInfo.SetMember("lastModified", &date);
		Register<bool>(&fileInfo, "directory", dir);
		args->result->PushBack(&fileInfo);
	});

	ScaleformHeap_Free(patterns);
}



void F4EEScaleform_GetOverlays::Invoke(Args * args)
{	
	CharacterCreation * characterCreation = g_characterCreation[*g_characterIndex];
	if(characterCreation) {
		Actor * actor = characterCreation->actor;
		TESNPC * npc = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
		if(npc) {
			UInt8 gender = CALL_MEMBER_FN(npc, GetSex)();
			bool isFemale = gender == 1 ? true : false;

			args->movie->movieRoot->CreateArray(args->result);

			g_overlayInterface.ForEachOverlay(actor, isFemale, [&](SInt32 priority, const OverlayInterface::OverlayDataPtr & slider) {
				GFxValue sliderInfo;
				args->movie->movieRoot->CreateObject(&sliderInfo);
				Register<SInt32>(&sliderInfo, "priority", priority);
				Register<UInt32>(&sliderInfo, "uid", slider->uid);
				RegisterString(&sliderInfo, args->movie->movieRoot, "id", slider->templateName->c_str());
				Register<double>(&sliderInfo, "red", slider->tintColor.r);
				Register<double>(&sliderInfo, "green", slider->tintColor.g);
				Register<double>(&sliderInfo, "blue", slider->tintColor.b);
				Register<double>(&sliderInfo, "alpha", slider->tintColor.a);
				Register<double>(&sliderInfo, "offsetU", slider->offsetUV.x);
				Register<double>(&sliderInfo, "offsetV", slider->offsetUV.y);
				Register<double>(&sliderInfo, "scaleU", slider->scaleUV.x);
				Register<double>(&sliderInfo, "scaleV", slider->scaleUV.y);

				auto pTemplate = g_overlayInterface.GetTemplateByName(isFemale, *slider->templateName);
				if(pTemplate) {
					RegisterString(&sliderInfo, args->movie->movieRoot, "name", pTemplate->displayName);
					Register<bool>(&sliderInfo, "playable", pTemplate->playable);
				}

				args->result->PushBack(&sliderInfo);
			});
		}
	}
}

void F4EEScaleform_GetOverlayTemplates::Invoke(Args * args)
{	
	CharacterCreation * characterCreation = g_characterCreation[*g_characterIndex];
	if(characterCreation) {
		Actor * actor = characterCreation->actor;
		TESNPC * npc = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
		if(npc) {
			UInt8 gender = CALL_MEMBER_FN(npc, GetSex)();
			bool isFemale = gender == 1 ? true : false;

			args->movie->movieRoot->CreateArray(args->result);

			std::vector<std::pair<F4EEFixedString, OverlayInterface::OverlayTemplatePtr>> overlays;
			g_overlayInterface.ForEachOverlayTemplate(gender == 1 ? true : false, [&overlays](const F4EEFixedString & name, const OverlayInterface::OverlayTemplatePtr & slider) {
				overlays.push_back(std::make_pair(name, slider));
			});

			std::sort(overlays.begin(), overlays.end(), [&](const std::pair<F4EEFixedString, OverlayInterface::OverlayTemplatePtr> & a, const std::pair<F4EEFixedString, OverlayInterface::OverlayTemplatePtr> & b)
			{
				if(a.second->sort == b.second->sort)
					return std::string(a.second->displayName.c_str()) < std::string(b.second->displayName.c_str());
				else
					return a.second->sort < b.second->sort;
			});

			for(auto & slider : overlays)
			{
				if(slider.second->playable) {
					GFxValue sliderInfo;
					args->movie->movieRoot->CreateObject(&sliderInfo);
					RegisterString(&sliderInfo, args->movie->movieRoot, "id", slider.first);
					RegisterString(&sliderInfo, args->movie->movieRoot, "name", slider.second->displayName.c_str());
					Register<bool>(&sliderInfo, "transformable", slider.second->transformable);
					Register<bool>(&sliderInfo, "tintable", slider.second->tintable);
					args->result->PushBack(&sliderInfo);
				}
			}
		}
	}
}



void F4EEScaleform_CreateOverlay::Invoke(Args * args)
{
	CharacterCreation * characterCreation = g_characterCreation[*g_characterIndex];
	if(characterCreation) {
		Actor * actor = characterCreation->actor;
		TESNPC * npc = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
		if(npc) {
			UInt8 gender = CALL_MEMBER_FN(npc, GetSex)();
			bool isFemale = gender == 1 ? true : false;

			SInt32 priority = args->args[0].GetInt();
			const char * templateName = args->args[1].GetString();

			NiColorA color;
			color.r = 0.0f;
			color.g = 0.0f;
			color.b = 0.0f;
			color.a = 0.0f;
			NiPoint2 offsetUV;
			offsetUV.x = 0.0f;
			offsetUV.y = 0.0f;
			NiPoint2 scaleUV;
			scaleUV.x = 1.0f;
			scaleUV.y = 1.0f;

			UInt32 uid = g_overlayInterface.AddOverlay(actor, isFemale, priority, templateName, color, offsetUV, scaleUV);
			args->result->SetUInt(uid);
		}
	}
}

void F4EEScaleform_DeleteOverlay::Invoke(Args * args)
{
	CharacterCreation * characterCreation = g_characterCreation[*g_characterIndex];
	if(characterCreation) {
		Actor * actor = characterCreation->actor;
		TESNPC * npc = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
		if(npc) {
			UInt8 gender = CALL_MEMBER_FN(npc, GetSex)();
			bool isFemale = gender == 1 ? true : false;

			UInt32 uid = args->args[0].GetUInt();
			bool result = g_overlayInterface.RemoveOverlay(actor, isFemale, uid);
			args->result->SetBool(result);
		}
	}
}

void F4EEScaleform_SetOverlayData::Invoke(Args * args)
{
	CharacterCreation * characterCreation = g_characterCreation[*g_characterIndex];
	if(characterCreation) {
		Actor * actor = characterCreation->actor;
		TESNPC * npc = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
		if(npc) {
			UInt8 gender = CALL_MEMBER_FN(npc, GetSex)();
			bool isFemale = gender == 1 ? true : false;

			UInt32 uid = args->args[0].GetUInt();

			bool setParam = false;
			auto pOverlay = g_overlayInterface.GetActorOverlayByUID(actor, isFemale, uid);
			if(pOverlay.second) {
				GFxValue memberData;
				if(args->args[1].HasMember("template")) {
					args->args[1].GetMember("template", &memberData);
					pOverlay.second->templateName = g_stringTable.GetString(memberData.GetString());
					setParam = true;
				}
				if(args->args[1].HasMember("offsetU")) {
					args->args[1].GetMember("offsetU", &memberData);
					pOverlay.second->offsetUV.x = memberData.GetNumber();
					setParam = true;
				}
				if(args->args[1].HasMember("offsetV")) {
					args->args[1].GetMember("offsetV", &memberData);
					pOverlay.second->offsetUV.y = memberData.GetNumber();
					setParam = true;
				}
				if(args->args[1].HasMember("scaleU")) {
					args->args[1].GetMember("scaleU", &memberData);
					pOverlay.second->scaleUV.x = memberData.GetNumber();
					setParam = true;
				}
				if(args->args[1].HasMember("scaleV")) {
					args->args[1].GetMember("scaleV", &memberData);
					pOverlay.second->scaleUV.y = memberData.GetNumber();
					setParam = true;
				}
				if(args->args[1].HasMember("red")) {
					args->args[1].GetMember("red", &memberData);
					pOverlay.second->tintColor.r = memberData.GetNumber();
					setParam = true;
				}
				if(args->args[1].HasMember("green")) {
					args->args[1].GetMember("green", &memberData);
					pOverlay.second->tintColor.g = memberData.GetNumber();
					setParam = true;
				}
				if(args->args[1].HasMember("blue")) {
					args->args[1].GetMember("blue", &memberData);
					pOverlay.second->tintColor.b = memberData.GetNumber();
					setParam = true;
				}
				if(args->args[1].HasMember("alpha")) {
					args->args[1].GetMember("alpha", &memberData);
					pOverlay.second->tintColor.a = memberData.GetNumber();
					setParam = true;
				}

				pOverlay.second->UpdateFlags();
			}

			args->result->SetBool(setParam);
		}
	}
}

void F4EEScaleform_ReorderOverlay::Invoke(Args * args)
{
	CharacterCreation * characterCreation = g_characterCreation[*g_characterIndex];
	if(characterCreation) {
		Actor * actor = characterCreation->actor;
		TESNPC * npc = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
		if(npc) {
			UInt8 gender = CALL_MEMBER_FN(npc, GetSex)();
			bool isFemale = gender == 1 ? true : false;

			UInt32 uid = args->args[0].GetUInt();
			SInt32 priority = args->args[1].GetInt();

			bool result = g_overlayInterface.ReorderOverlay(actor, isFemale, uid, priority);
			args->result->SetBool(result);
		}
	}
}

void F4EEScaleform_UpdateOverlays::Invoke(Args * args)
{
	CharacterCreation * characterCreation = g_characterCreation[*g_characterIndex];
	if(characterCreation && g_bEnableOverlays) {
		g_overlayInterface.UpdateOverlays(characterCreation->actor);
	}
}

void F4EEScaleform_CloneOverlays::Invoke(Args * args)
{
	bool bVerify = true;
	if(args->numArgs >= 1) {
		bVerify = args->args[0].GetBool(); // Used to verify whether the target is the starting character's dummy actor
	}
	CharacterCreation * characterCreation = g_characterCreation[*g_characterIndex];
	if(characterCreation) {
		Actor * actor = characterCreation->actor;
		TESNPC * npc = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);

		if(actor != (*g_player) && ((bVerify && (npc == (*g_customizationDummy1) || npc == (*g_customizationDummy2))) || !bVerify)) {
			if(g_bEnableOverlays) {
				g_overlayInterface.CloneOverlays(actor, (*g_player));
				g_overlayInterface.UpdateOverlays(actor);
			}
		}
	}
}

void F4EEScaleform_GetEquippedItems::Invoke(Args * args)
{
	CharacterCreation * characterCreation = g_characterCreation[*g_characterIndex];
	if(characterCreation) {
		Actor * actor = characterCreation->actor;

		static std::unordered_map<TESForm*, std::pair<SInt32, UInt8>> stackList;
		auto inventory = actor->inventoryList;
		if(inventory)
		{
			inventory->inventoryLock.Lock();
			for(UInt32 i = 0; i < inventory->items.count; i++)
			{
				SInt32 s = 0;
				inventory->items[i].stack->Visit([&](BGSInventoryItem::Stack * stack)
				{
					if(stack->flags & BGSInventoryItem::Stack::kFlagEquipped) {
						stackList.emplace(inventory->items[i].form, std::make_pair(s, stack->flags & 0xF));
					}
					s++;
					return true;
				});
			}
			inventory->inventoryLock.Release();
		}
		if(!stackList.empty())
		{
			args->movie->movieRoot->CreateArray(args->result);

			for(auto & stackItem : stackList)
			{
				GFxValue equippedItem;
				args->movie->movieRoot->CreateObject(&equippedItem);
				Register<UInt32>(&equippedItem, "formId", stackItem.first->formID);
				Register<UInt32>(&equippedItem, "stackIndex", stackItem.second.first);
				Register<UInt32>(&equippedItem, "flags", stackItem.second.second);
				args->result->PushBack(&equippedItem);
			}	
		}
		else
			args->result->SetNull();
	}
}

void F4EEScaleform_UnequipItems::Invoke(Args * args)
{
	CharacterCreation * characterCreation = g_characterCreation[*g_characterIndex];
	if(characterCreation) {
		Actor * actor = characterCreation->actor;

		// Convert the GFX data to a stack mapping
		static std::unordered_map<TESForm*, std::pair<SInt32, UInt8>> stackList;
		UInt32 numItems = args->args[0].GetArraySize();
		for (UInt32 i = 0; i < numItems; i++) {
			GFxValue element, formId, stackIndex, flags;
			args->args[0].GetElement(i, &element);

			element.GetMember("formId", &formId);
			element.GetMember("stackIndex", &stackIndex);
			element.GetMember("flags", &flags);			

			TESForm * form = LookupFormByID(formId.GetUInt());
			if(!form)
				continue;

			stackList.emplace(form, std::make_pair(stackIndex.GetUInt(), flags.GetUInt()));
		}

		// Traverse the inventory to unequip the specific stacks
		auto inventory = actor->inventoryList;
		if(inventory)
		{
			inventory->inventoryLock.Lock();

			for(UInt32 i = 0; i < inventory->items.count; i++)
			{
				SInt32 s = 0;
				auto it = stackList.find(inventory->items[i].form);
				if(it != stackList.end())
				{
					inventory->items[i].stack->Visit([&](BGSInventoryItem::Stack * stack)
					{
						if(it->second.first == s) {
							stack->flags &= ~0xF;
							return false;
						}

						s++;
						return true;
					});
				}
			}

			inventory->inventoryLock.Release();
		}

		if(!stackList.empty()) {
			args->movie->movieRoot->CreateArray(args->result);

			for(auto & stackItem : stackList)
			{
				GFxValue equippedItem;
				args->movie->movieRoot->CreateObject(&equippedItem);
				Register<UInt32>(&equippedItem, "formId", stackItem.first->formID);
				Register<UInt32>(&equippedItem, "stackIndex", stackItem.second.first);
				Register<UInt32>(&equippedItem, "flags", stackItem.second.second);
				args->result->PushBack(&equippedItem);
			}

			g_task->AddTask(new F4EEBodyGenUpdate(actor, false));			
		}
		else
			args->result->SetNull();
	}
}

void F4EEScaleform_EquipItems::Invoke(Args * args)
{
	CharacterCreation * characterCreation = g_characterCreation[*g_characterIndex];
	if(characterCreation) {
		Actor * actor = characterCreation->actor;

		// Convert the GFX data
		static std::unordered_map<TESForm*, std::pair<SInt32, UInt8>> stackList;
		UInt32 numItems = args->args[0].GetArraySize();
		for (UInt32 i = 0; i < numItems; i++) {
			GFxValue element, formId, stackIndex, flags;
			args->args[0].GetElement(i, &element);
			
			element.GetMember("formId", &formId);
			element.GetMember("stackIndex", &stackIndex);
			element.GetMember("flags", &flags);			

			TESForm * form = LookupFormByID(formId.GetUInt());
			if(!form)
				continue;

			stackList.emplace(form, std::make_pair(stackIndex.GetUInt(), flags.GetUInt()));
		}

		// Re-equip the specified stacks
		auto inventory = actor->inventoryList;
		if(inventory)
		{
			inventory->inventoryLock.Lock();

			for(UInt32 i = 0; i < inventory->items.count; i++)
			{
				SInt32 s = 0;
				auto it = stackList.find(inventory->items[i].form);
				if(it != stackList.end())
				{
					inventory->items[i].stack->Visit([&](BGSInventoryItem::Stack * stack)
					{
						if(it->second.first == s) {
							stack->flags |= it->second.second & 0xF;
							return false;
						}

						s++;
						return true;
					});
				}
			}

			inventory->inventoryLock.Release();
		}

		if(!stackList.empty()) {
			g_task->AddTask(new F4EEBodyGenUpdate(actor, false));
			args->result->SetBool(true);
		}
		else
			args->result->SetBool(false);
	}
}