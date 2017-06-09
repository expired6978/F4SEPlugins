#include "PapyrusOverlays.h"

#include "OverlayInterface.h"

#include "f4se/GameReferences.h"

#include "f4se/PapyrusVM.h"
#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/PapyrusStruct.h"
#include "f4se/PapyrusUtilities.h"

extern OverlayInterface g_overlayInterface;

namespace papyrusOverlays
{
	DECLARE_STRUCT(Entry, "Overlays");

	UInt32 Add(StaticFunctionTag *, Actor * actor, bool isFemale, Entry overlay)
	{
		if(actor) {
			SInt32 priority;
			BSFixedString id;
			NiColorA color;
			NiPoint2 offsetUV;
			NiPoint2 scaleUV;
			overlay.Get("priority", &priority);
			overlay.Get("template", &id);
			overlay.Get("red", &color.r);
			overlay.Get("green", &color.g);
			overlay.Get("blue", &color.b);
			overlay.Get("alpha", &color.a);
			overlay.Get("offset_u", &offsetUV.x);
			overlay.Get("offset_v", &offsetUV.y);
			overlay.Get("scale_u", &scaleUV.x);
			overlay.Get("scale_v", &scaleUV.y);

			UInt32 uid = g_overlayInterface.AddOverlay(actor, isFemale, priority, id, color, offsetUV, scaleUV);
			overlay.Set("uid", uid);
			return uid;
		}
		return 0;
	}

	bool Set(StaticFunctionTag *, Actor * actor, bool isFemale, UInt32 uid, Entry overlay)
	{
		auto pOverlay = g_overlayInterface.GetActorOverlayByUID(actor, isFemale, uid);
		if(pOverlay.second) {
			SInt32 priority;
			BSFixedString id;
			NiColorA color;
			NiPoint2 offsetUV;
			NiPoint2 scaleUV;
			overlay.Get("priority", &priority);
			overlay.Get("template", &id);
			overlay.Get("red", &color.r);
			overlay.Get("green", &color.g);
			overlay.Get("blue", &color.b);
			overlay.Get("alpha", &color.a);
			overlay.Get("offset_u", &offsetUV.x);
			overlay.Get("offset_v", &offsetUV.y);
			overlay.Get("scale_u", &scaleUV.x);
			overlay.Get("scale_v", &scaleUV.y);

			if(pOverlay.first != priority) {
				g_overlayInterface.ReorderOverlay(actor, isFemale, uid, priority);
			}

			pOverlay.second->tintColor = color;
			pOverlay.second->offsetUV = offsetUV;
			pOverlay.second->scaleUV = scaleUV;
			return true;
		}

		return false;
	}

	Entry Get(StaticFunctionTag *, Actor * actor, bool isFemale, UInt32 uid)
	{
		Entry overlay;
		overlay.SetNone(true);
		auto pOverlay = g_overlayInterface.GetActorOverlayByUID(actor, isFemale, uid);
		if(pOverlay.second) {

			BSFixedString templateName = pOverlay.second->templateName ? pOverlay.second->templateName->c_str() : "";

			overlay.Set("uid", uid);
			overlay.Set("priority", pOverlay.first);
			overlay.Set("template", templateName);
			overlay.Set("red", pOverlay.second->tintColor.r);
			overlay.Set("green", pOverlay.second->tintColor.g);
			overlay.Set("blue", pOverlay.second->tintColor.b);
			overlay.Set("alpha", pOverlay.second->tintColor.a);
			overlay.Set("offset_u", pOverlay.second->offsetUV.x);
			overlay.Set("offset_v", pOverlay.second->offsetUV.y);
			overlay.Set("scale_u", pOverlay.second->scaleUV.x);
			overlay.Set("scale_v", pOverlay.second->scaleUV.y);

			overlay.SetNone(false);
		}

		return overlay;
	}

	// Only looks at slot, priority, owner, and material to remove an entry
	bool Remove(StaticFunctionTag *, Actor * actor, bool isFemale, UInt32 uid)
	{
		if(actor) {
			return g_overlayInterface.RemoveOverlay(actor, isFemale, uid);
		}

		return false;
	}

	bool RemoveAll(StaticFunctionTag *, Actor * actor, bool isFemale)
	{
		if(actor) {
			return g_overlayInterface.RemoveAll(actor, isFemale);
		}

		return false;
	}

	VMArray<Entry> GetAll(StaticFunctionTag *, Actor * actor, bool isFemale)
	{
		VMArray<Entry> results;
		if(!actor)
			return results;

		g_overlayInterface.ForEachOverlay(actor, isFemale, [&](SInt32 priority, const OverlayInterface::OverlayDataPtr & overlay)
		{
			Entry entry;
			entry.Set("priority", priority);
			entry.Set("uid", overlay->uid);

			BSFixedString templateName = overlay->templateName ? overlay->templateName->c_str() : "";
			entry.Set("template", templateName);

			entry.Set("red", overlay->tintColor.r);
			entry.Set("green", overlay->tintColor.g);
			entry.Set("blue", overlay->tintColor.b);
			entry.Set("alpha", overlay->tintColor.a);

			results.Push(&entry);
		});

		return results;
	}
	
	void Update(StaticFunctionTag*, Actor * actor)
	{
		g_overlayInterface.UpdateOverlays(actor);
	}

	/*void UpdateUID(StaticFunctionTag*, Actor * actor, UInt32 uid)
	{
		g_overlayInterface.UpdateOverlay(actor, uid);
	}*/
};

void papyrusOverlays::RegisterFuncs(VirtualMachine* vm)
{
	vm->RegisterFunction(
		new NativeFunction3<StaticFunctionTag, UInt32, Actor*, bool, Entry>("Add", "Overlays", papyrusOverlays::Add, vm));

	vm->RegisterFunction(
		new NativeFunction3<StaticFunctionTag, bool, Actor*, bool, UInt32>("Remove", "Overlays", papyrusOverlays::Remove, vm));

	vm->RegisterFunction(
		new NativeFunction4<StaticFunctionTag, bool, Actor*, bool, UInt32, Entry>("Set", "Overlays", papyrusOverlays::Set, vm));

	vm->RegisterFunction(
		new NativeFunction3<StaticFunctionTag, Entry, Actor*, bool, UInt32>("Get", "Overlays", papyrusOverlays::Get, vm));

	vm->RegisterFunction(
		new NativeFunction2<StaticFunctionTag, bool, Actor*, bool>("RemoveAll", "Overlays", papyrusOverlays::RemoveAll, vm));

	vm->RegisterFunction(
		new NativeFunction2<StaticFunctionTag, VMArray<Entry>, Actor*, bool>("GetAll", "Overlays", papyrusOverlays::GetAll, vm));

	vm->RegisterFunction(
		new NativeFunction1<StaticFunctionTag, void, Actor*>("Update", "Overlays", papyrusOverlays::Update, vm));

	/*vm->RegisterFunction(
		new NativeFunction2<StaticFunctionTag, void, Actor*, UInt32>("UpdateUID", "Overlays", papyrusOverlays::UpdateUID, vm));*/


	vm->SetFunctionFlags("Overlays", "Add", IFunction::kFunctionFlag_NoWait);
	vm->SetFunctionFlags("Overlays", "Remove", IFunction::kFunctionFlag_NoWait);
	vm->SetFunctionFlags("Overlays", "Set", IFunction::kFunctionFlag_NoWait);
	vm->SetFunctionFlags("Overlays", "Get", IFunction::kFunctionFlag_NoWait);
	vm->SetFunctionFlags("Overlays", "RemoveAll", IFunction::kFunctionFlag_NoWait);
	vm->SetFunctionFlags("Overlays", "GetAll", IFunction::kFunctionFlag_NoWait);

	vm->SetFunctionFlags("Overlays", "Update", IFunction::kFunctionFlag_NoWait);
	//vm->SetFunctionFlags("Overlays", "UpdateUID", IFunction::kFunctionFlag_NoWait);
}