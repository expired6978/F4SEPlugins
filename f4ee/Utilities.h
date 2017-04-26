#pragma once

#include "f4se/PluginAPI.h"
#include "f4se/GameTypes.h"
#include "StringTable.h"

#include <functional>

class BSResourceNiBinaryStream;
class NiAVObject;
class TESRace;

#include "f4se/Serialization.h"

namespace Serialization
{
	template <> bool WriteData<F4EEFixedString>(const F4SESerializationInterface * intfc, const F4EEFixedString * data);
	template <> bool ReadData<F4EEFixedString>(const F4SESerializationInterface * intfc, F4EEFixedString * data);
};

namespace std
{
	std::string &ltrim(std::string &s);// trim from end
	std::string &rtrim(std::string &s);
	std::string &trim(std::string &s);// trim from both ends
	std::vector<std::string> explode(const std::string& str, const char& ch);
}

TESRace * GetRaceByName(const std::string & name);
std::string GetFormIdentifier(TESForm * form);
TESForm * GetFormFromIdentifier(const std::string & formIdentifier);

bool BSReadLine(BSResourceNiBinaryStream* fin, std::string* str);
void BSReadAll(BSResourceNiBinaryStream* fin, std::string* str);
bool VisitObjects(NiAVObject * parent, std::function<bool(NiAVObject*)> functor);