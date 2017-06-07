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

	void Add(StaticFunctionTag *, Actor * actor, bool isFemale, Entry overlay)
	{
		if(actor) {
			UInt32 slotIndex;
			SInt32 priority;
			BGSKeyword * keyword = nullptr;
			BSFixedString materialPath;
			NiColorA color;
			overlay.Get("slotIndex", &slotIndex);
			overlay.Get("priority", &priority);
			overlay.Get("owner", &keyword);
			overlay.Get("materialPath", &materialPath);
			overlay.Get("red", &color.r);
			overlay.Get("green", &color.g);
			overlay.Get("blue", &color.b);
			overlay.Get("alpha", &color.a);

			g_overlayInterface.AddOverlay(actor, isFemale, slotIndex, priority, keyword, materialPath, color);
		}
	}

	// Only looks at slot, priority, owner, and material to remove an entry
	bool Remove(StaticFunctionTag *, Actor * actor, bool isFemale, Entry overlay)
	{
		if(actor) {
			UInt32 slotIndex;
			SInt32 priority;
			BGSKeyword * keyword = nullptr;
			BSFixedString materialPath;

			overlay.Get("slotIndex", &slotIndex);
			overlay.Get("priority", &priority);
			overlay.Get("owner", &keyword);
			overlay.Get("materialPath", &materialPath);

			return g_overlayInterface.RemoveOverlay(actor, isFemale, slotIndex, priority, keyword, materialPath);
		}

		return false;
	}

	bool ChangePriority(StaticFunctionTag *, Actor * actor, bool isFemale, Entry overlay, SInt32 newPriority)
	{
		if(actor) {
			UInt32 slotIndex;
			SInt32 priority;
			BGSKeyword * keyword = nullptr;
			BSFixedString materialPath;

			overlay.Get("slotIndex", &slotIndex);
			overlay.Get("priority", &priority);
			overlay.Get("owner", &keyword);
			overlay.Get("materialPath", &materialPath);

			if(g_overlayInterface.ReorderOverlay(actor, isFemale, slotIndex, priority, keyword, materialPath, newPriority)) {
				overlay.Set("priority", newPriority);
				return true;
			}
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

		g_overlayInterface.ForEachOverlay(actor, isFemale, [&](UInt32 slotIndex, SInt32 priority, const OverlayInterface::OverlayDataPtr & overlay)
		{
			Entry entry;
			entry.Set("slotIndex", slotIndex);
			entry.Set("priority", priority);

			BGSKeyword * keyword = (BGSKeyword*)PapyrusVM::GetObjectFromHandle(overlay->m_keyword, BGSKeyword::kTypeID);
			entry.Set("owner", keyword);

			BSFixedString materialPath = overlay->m_materialPath ? overlay->m_materialPath->c_str() : "";
			entry.Set("materialPath", materialPath);

			entry.Set("red", overlay->m_tintColor.r);
			entry.Set("green", overlay->m_tintColor.g);
			entry.Set("blue", overlay->m_tintColor.b);
			entry.Set("alpha", overlay->m_tintColor.a);

			results.Push(&entry);
		});

		return results;
	}

	VMArray<Entry> GetBySlot(StaticFunctionTag *, Actor * actor, bool isFemale, UInt32 slotIndex)
	{
		VMArray<Entry> results;
		if(!actor)
			return results;

		g_overlayInterface.ForEachOverlayBySlot(actor, isFemale, slotIndex, [&](UInt32 slotIndex, SInt32 priority, const OverlayInterface::OverlayDataPtr & overlay)
		{
			Entry entry;
			entry.Set("slotIndex", slotIndex);
			entry.Set("priority", priority);

			BGSKeyword * keyword = (BGSKeyword*)PapyrusVM::GetObjectFromHandle(overlay->m_keyword, BGSKeyword::kTypeID);
			entry.Set("owner", keyword);

			BSFixedString materialPath = overlay->m_materialPath ? overlay->m_materialPath->c_str() : "";
			entry.Set("materialPath", materialPath);

			entry.Set("red", overlay->m_tintColor.r);
			entry.Set("green", overlay->m_tintColor.g);
			entry.Set("blue", overlay->m_tintColor.b);
			entry.Set("alpha", overlay->m_tintColor.a);

			results.Push(&entry);
		});

		return results;
	}

	VMArray<Entry> GetByPriority(StaticFunctionTag *, Actor * actor, bool isFemale, SInt32 priority)
	{
		VMArray<Entry> results;
		if(!actor)
			return results;

		g_overlayInterface.ForEachOverlayByPriority(actor, isFemale, priority, [&](UInt32 slotIndex, SInt32 priority, const OverlayInterface::OverlayDataPtr & overlay)
		{
			Entry entry;
			entry.Set("slotIndex", slotIndex);
			entry.Set("priority", priority);

			BGSKeyword * keyword = (BGSKeyword*)PapyrusVM::GetObjectFromHandle(overlay->m_keyword, BGSKeyword::kTypeID);
			entry.Set("owner", keyword);

			BSFixedString materialPath = overlay->m_materialPath ? overlay->m_materialPath->c_str() : "";
			entry.Set("materialPath", materialPath);

			entry.Set("red", overlay->m_tintColor.r);
			entry.Set("green", overlay->m_tintColor.g);
			entry.Set("blue", overlay->m_tintColor.b);
			entry.Set("alpha", overlay->m_tintColor.a);

			results.Push(&entry);
		});

		return results;
	}

	VMArray<Entry> GetByKeyword(StaticFunctionTag *, Actor * actor, bool isFemale, BGSKeyword * keyword)
	{
		VMArray<Entry> results;
		if(!actor)
			return results;

		g_overlayInterface.ForEachOverlayByKeyword(actor, isFemale, keyword, [&](UInt32 slotIndex, SInt32 priority, const OverlayInterface::OverlayDataPtr & overlay)
		{
			Entry entry;
			entry.Set("slotIndex", slotIndex);
			entry.Set("priority", priority);

			BGSKeyword * keyword = (BGSKeyword*)PapyrusVM::GetObjectFromHandle(overlay->m_keyword, BGSKeyword::kTypeID);
			entry.Set("owner", keyword);

			BSFixedString materialPath = overlay->m_materialPath ? overlay->m_materialPath->c_str() : "";
			entry.Set("materialPath", materialPath);

			entry.Set("red", overlay->m_tintColor.r);
			entry.Set("green", overlay->m_tintColor.g);
			entry.Set("blue", overlay->m_tintColor.b);
			entry.Set("alpha", overlay->m_tintColor.a);

			results.Push(&entry);
		});

		return results;
	}

	void Update(StaticFunctionTag*, Actor * actor)
	{
		g_overlayInterface.UpdateOverlays(actor);
	}

	void UpdateSlot(StaticFunctionTag*, Actor * actor, UInt32 slotIndex)
	{
		if(slotIndex >= 0 && slotIndex <= 31)
			g_overlayInterface.UpdateOverlay(actor, slotIndex);
	}
};

void papyrusOverlays::RegisterFuncs(VirtualMachine* vm)
{
	vm->RegisterFunction(
		new NativeFunction3<StaticFunctionTag, void, Actor*, bool, Entry>("Add", "Overlays", papyrusOverlays::Add, vm));

	vm->RegisterFunction(
		new NativeFunction3<StaticFunctionTag, bool, Actor*, bool, Entry>("Remove", "Overlays", papyrusOverlays::Remove, vm));

	vm->RegisterFunction(
		new NativeFunction4<StaticFunctionTag, bool, Actor*, bool, Entry, SInt32>("ChangePriority", "Overlays", papyrusOverlays::ChangePriority, vm));

	vm->RegisterFunction(
		new NativeFunction2<StaticFunctionTag, bool, Actor*, bool>("RemoveAll", "Overlays", papyrusOverlays::RemoveAll, vm));

	vm->RegisterFunction(
		new NativeFunction2<StaticFunctionTag, VMArray<Entry>, Actor*, bool>("GetAll", "Overlays", papyrusOverlays::GetAll, vm));

	vm->RegisterFunction(
		new NativeFunction3<StaticFunctionTag, VMArray<Entry>, Actor*, bool, UInt32>("GetBySlot", "Overlays", papyrusOverlays::GetBySlot, vm));

	vm->RegisterFunction(
		new NativeFunction3<StaticFunctionTag, VMArray<Entry>, Actor*, bool, SInt32>("GetByPriority", "Overlays", papyrusOverlays::GetByPriority, vm));

	vm->RegisterFunction(
		new NativeFunction3<StaticFunctionTag, VMArray<Entry>, Actor*, bool, BGSKeyword*>("GetByKeyword", "Overlays", papyrusOverlays::GetByKeyword, vm));

	vm->RegisterFunction(
		new NativeFunction1<StaticFunctionTag, void, Actor*>("Update", "Overlays", papyrusOverlays::Update, vm));

	vm->RegisterFunction(
		new NativeFunction2<StaticFunctionTag, void, Actor*, UInt32>("UpdateSlot", "Overlays", papyrusOverlays::UpdateSlot, vm));


	vm->SetFunctionFlags("Overlays", "Add", IFunction::kFunctionFlag_NoWait);
	vm->SetFunctionFlags("Overlays", "Remove", IFunction::kFunctionFlag_NoWait);
	vm->SetFunctionFlags("Overlays", "RemoveAll", IFunction::kFunctionFlag_NoWait);
	vm->SetFunctionFlags("Overlays", "GetAll", IFunction::kFunctionFlag_NoWait);
	vm->SetFunctionFlags("Overlays", "GetBySlot", IFunction::kFunctionFlag_NoWait);
	vm->SetFunctionFlags("Overlays", "GetByPriority", IFunction::kFunctionFlag_NoWait);
	vm->SetFunctionFlags("Overlays", "GetByKeyword", IFunction::kFunctionFlag_NoWait);

	vm->SetFunctionFlags("Overlays", "Update", IFunction::kFunctionFlag_NoWait);
	vm->SetFunctionFlags("Overlays", "UpdateSlot", IFunction::kFunctionFlag_NoWait);
}