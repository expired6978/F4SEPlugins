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

extern BodyMorphInterface g_bodyMorphInterface;

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

	void UpdateMorphs(StaticFunctionTag*, Actor * actor)
	{
		g_bodyMorphInterface.UpdateMorphs(actor);
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
		new NativeFunction3<StaticFunctionTag, VMArray<BGSKeyword*>, Actor*, bool, BSFixedString>("GetKeywords", "BodyGen", papyrusBodyGen::GetKeywords, vm));

	vm->RegisterFunction(
		new NativeFunction2<StaticFunctionTag, VMArray<BSFixedString>, Actor*, bool>("GetMorphs", "BodyGen", papyrusBodyGen::GetMorphs, vm));

	vm->RegisterFunction(
		new NativeFunction1<StaticFunctionTag, void, Actor*>("UpdateMorphs", "BodyGen", papyrusBodyGen::UpdateMorphs, vm));
}