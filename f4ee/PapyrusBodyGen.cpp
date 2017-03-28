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

#include "BodyGen.h"

extern BodyGen g_bodyGen;

namespace papyrusBodyGen
{
	void SetMorph(StaticFunctionTag*, Actor * actor, bool isFemale, BSFixedString morph, BGSKeyword * keyword, float value)
	{
		g_bodyGen.SetMorph(actor, isFemale, morph, keyword, value);
	}

	float GetMorph(StaticFunctionTag*, Actor * actor, bool isFemale, BSFixedString morph, BGSKeyword * keyword)
	{
		return g_bodyGen.GetMorph(actor, isFemale, morph, keyword);
	}

	void UpdateMorphs(StaticFunctionTag*, Actor * actor)
	{
		g_bodyGen.UpdateMorphs(actor);
	}
};

void papyrusBodyGen::RegisterFuncs(VirtualMachine* vm)
{
	vm->RegisterFunction(
		new NativeFunction5<StaticFunctionTag, void, Actor*, bool, BSFixedString, BGSKeyword*, float>("SetMorph", "BodyGen", papyrusBodyGen::SetMorph, vm));

	vm->RegisterFunction(
		new NativeFunction4<StaticFunctionTag, float, Actor*, bool, BSFixedString, BGSKeyword*>("GetMorph", "BodyGen", papyrusBodyGen::GetMorph, vm));

	vm->RegisterFunction(
		new NativeFunction1<StaticFunctionTag, void, Actor*>("UpdateMorphs", "BodyGen", papyrusBodyGen::UpdateMorphs, vm));
}