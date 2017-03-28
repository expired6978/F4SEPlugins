#pragma once

#include "f4se/PluginAPI.h"
#include "f4se/GameTypes.h"
#include "StringTable.h"

#include <functional>

class BSResourceNiBinaryStream;
class NiAVObject;
class TESRace;

namespace Serialization
{
	template <typename T>
	bool WriteData(const F4SESerializationInterface * intfc, const T * data)
	{
		return intfc->WriteRecordData(data, sizeof(T));
	}

	template <typename T>
	bool ReadData(const F4SESerializationInterface * intfc, T * data)
	{
		return intfc->ReadRecordData(data, sizeof(T)) > 0;
	}

	template <> bool WriteData<BSFixedString>(const F4SESerializationInterface * intfc, const BSFixedString * data);
	template <> bool ReadData<BSFixedString>(const F4SESerializationInterface * intfc, BSFixedString * data);

	template <> bool WriteData<F4EEFixedString>(const F4SESerializationInterface * intfc, const F4EEFixedString * data);
	template <> bool ReadData<F4EEFixedString>(const F4SESerializationInterface * intfc, F4EEFixedString * data);

	template <> bool WriteData<std::string>(const F4SESerializationInterface * intfc, const std::string * data);
	template <> bool ReadData<std::string>(const F4SESerializationInterface * intfc, std::string * data);
};

TESRace * GetRaceByName(const std::string & name);
std::string GetFormIdentifier(TESForm * form);
TESForm * GetFormFromIdentifier(const std::string & formIdentifier);

void BSReadAll(BSResourceNiBinaryStream* fin, std::string* str);
bool VisitObjects(NiAVObject * parent, std::function<bool(NiAVObject*)> functor);