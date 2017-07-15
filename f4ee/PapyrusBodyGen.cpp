#include "PapyrusBodyGen.h"

#include "f4se/PapyrusVM.h"
#include "f4se/PapyrusNativeFunctions.h"

#include "f4se/GameReferences.h"
#include "f4se/GameRTTI.h"

#include <functional>
#include <algorithm>

#include "f4se/NiNodes.h"
#include "f4se/NiExtraData.h"
#include "f4se/BSGeometry.h"

#include "BodyMorphInterface.h"
#include "BodyGenInterface.h"
#include "SkinInterface.h"

#include "f4se/GameObjects.h"

extern BodyMorphInterface g_bodyMorphInterface;
extern BodyGenInterface g_bodyGenInterface;
extern SkinInterface g_skinInterface;

namespace papyrusBodyGen
{
	void SetMorph(StaticFunctionTag*, Actor * actor, bool isFemale, BSFixedString morph, BGSKeyword * keyword, float value)
	{
		g_bodyMorphInterface.SetMorph(actor, isFemale, morph, keyword, value);
	}

	float GetMorph(StaticFunctionTag*, Actor * actor, bool isFemale, BSFixedString morph, BGSKeyword * keyword)
	{
		return g_bodyMorphInterface.GetMorph(actor, isFemale, morph, keyword);
	}

	void RemoveMorphsByName(StaticFunctionTag*, Actor * actor, bool isFemale, BSFixedString morph)
	{
		g_bodyMorphInterface.RemoveMorphsByName(actor, isFemale, morph);
	}

	void RemoveMorphsByKeyword(StaticFunctionTag*, Actor * actor, bool isFemale, BGSKeyword * keyword)
	{
		g_bodyMorphInterface.RemoveMorphsByKeyword(actor, isFemale, keyword);
	}

	void RemoveAllMorphs(StaticFunctionTag*, Actor * actor, bool isFemale)
	{
		g_bodyMorphInterface.ClearMorphs(actor, isFemale);
	}

	VMArray<BGSKeyword*> GetKeywords(StaticFunctionTag*, Actor * actor, bool isFemale, BSFixedString morph)
	{
		std::vector<BGSKeyword*> keywords;
		g_bodyMorphInterface.GetKeywords(actor, isFemale, morph, keywords);
		return VMArray<BGSKeyword*>(keywords);
	}

	VMArray<BSFixedString> GetMorphs(StaticFunctionTag*, Actor * actor, bool isFemale)
	{
		std::vector<BSFixedString> morphs;
		g_bodyMorphInterface.GetMorphs(actor, isFemale, morphs);
		return VMArray<BSFixedString>(morphs);
	}

	void ClearAll(StaticFunctionTag*)
	{
		g_bodyMorphInterface.Revert();
	}

	void RegenerateMorphs(StaticFunctionTag*, Actor * actor, bool update)
	{
		if(!actor)
			return;

		TESNPC * npc = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
		if(!npc)
			return;

		UInt64 gender = CALL_MEMBER_FN(npc, GetSex)();
		bool isFemale = gender == 1 ? true : false;

		g_bodyMorphInterface.ClearMorphs(actor, isFemale);
		g_bodyGenInterface.EvaluateBodyMorphs(actor, isFemale);

		if(update)
			g_bodyMorphInterface.UpdateMorphs(actor);
	}

	void UpdateMorphs(StaticFunctionTag*, Actor * actor)
	{
		g_bodyMorphInterface.UpdateMorphs(actor);
	}

	bool SetSkinOverride(StaticFunctionTag*, Actor * actor, BSFixedString id)
	{
		if(!actor)
			return false;

		TESNPC * npc = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
		if(!npc)
			return false;

		UInt8 gender = CALL_MEMBER_FN(npc, GetSex)();
		bool isFemale = gender == 1 ? true : false;

		bool ret = g_skinInterface.AddSkinOverride(actor, id, isFemale);
		if(ret)
			g_skinInterface.UpdateSkinOverride(actor, true);
		return ret;
	}

	bool RemoveSkinOverride(StaticFunctionTag*, Actor * actor)
	{
		if(!actor)
			return false;

		TESNPC * npc = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
		if(!npc)
			return false;

		UInt8 gender = CALL_MEMBER_FN(npc, GetSex)();
		bool isFemale = gender == 1 ? true : false;

		bool ret = g_skinInterface.RemoveSkinOverride(actor);
		if(ret)
			g_skinInterface.UpdateSkinOverride(actor, true);
		return ret;
	}
};

void papyrusBodyGen::RegisterFuncs(VirtualMachine* vm)
{
	vm->RegisterFunction(
		new NativeFunction5<StaticFunctionTag, void, Actor*, bool, BSFixedString, BGSKeyword*, float>("SetMorph", "BodyGen", papyrusBodyGen::SetMorph, vm));

	vm->RegisterFunction(
		new NativeFunction4<StaticFunctionTag, float, Actor*, bool, BSFixedString, BGSKeyword*>("GetMorph", "BodyGen", papyrusBodyGen::GetMorph, vm));

	vm->RegisterFunction(
		new NativeFunction3<StaticFunctionTag, void, Actor*, bool, BSFixedString>("RemoveMorphsByName", "BodyGen", papyrusBodyGen::RemoveMorphsByName, vm));

	vm->RegisterFunction(
		new NativeFunction3<StaticFunctionTag, void, Actor*, bool, BGSKeyword*>("RemoveMorphsByKeyword", "BodyGen", papyrusBodyGen::RemoveMorphsByKeyword, vm));

	vm->RegisterFunction(
		new NativeFunction2<StaticFunctionTag, void, Actor*, bool>("RemoveAllMorphs", "BodyGen", papyrusBodyGen::RemoveAllMorphs, vm));

	vm->RegisterFunction(
		new NativeFunction3<StaticFunctionTag, VMArray<BGSKeyword*>, Actor*, bool, BSFixedString>("GetKeywords", "BodyGen", papyrusBodyGen::GetKeywords, vm));

	vm->RegisterFunction(
		new NativeFunction2<StaticFunctionTag, VMArray<BSFixedString>, Actor*, bool>("GetMorphs", "BodyGen", papyrusBodyGen::GetMorphs, vm));

	vm->RegisterFunction(
		new NativeFunction2<StaticFunctionTag, void, Actor*, bool>("RegenerateMorphs", "BodyGen", papyrusBodyGen::RegenerateMorphs, vm));

	vm->RegisterFunction(
		new NativeFunction1<StaticFunctionTag, void, Actor*>("UpdateMorphs", "BodyGen", papyrusBodyGen::UpdateMorphs, vm));

	vm->RegisterFunction(
		new NativeFunction0<StaticFunctionTag, void>("ClearAll", "BodyGen", papyrusBodyGen::ClearAll, vm));

	vm->RegisterFunction(
		new NativeFunction2<StaticFunctionTag, bool, Actor*, BSFixedString>("SetSkinOverride", "BodyGen", papyrusBodyGen::SetSkinOverride, vm));

	vm->RegisterFunction(
		new NativeFunction1<StaticFunctionTag, bool, Actor*>("RemoveSkinOverride", "BodyGen", papyrusBodyGen::RemoveSkinOverride, vm));
}