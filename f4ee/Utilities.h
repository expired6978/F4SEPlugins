#pragma once

#include "f4se/PluginAPI.h"
#include "f4se/GameTypes.h"
#include "f4se/NiTypes.h"
#include "StringTable.h"

#include <cmath>
#include <string>
#include <functional>

class BSResourceNiBinaryStream;
class NiAVObject;
class TESRace;
class Actor;
class TESLevCharacter;
class TESNPC;
class NiNode;
struct ModInfo;

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

std::string bytes_to_string(std::size_t size);
TESRace * GetRaceByName(const std::string & name);
TESRace * GetActorRace(Actor * actor);
std::string GetFormIdentifier(TESForm * form);
TESForm * GetFormFromIdentifier(const std::string & formIdentifier);

template<int MaxBuf>
class BSResourceTextFile
{
public:
	BSResourceTextFile(BSResourceNiBinaryStream* file) : fin(file) { }

	bool ReadLine(std::string* str)
	{
		UInt32 ret = fin->ReadLine((char*)buf, MaxBuf, '\n');
		if (ret > 0) {
			*str = buf;
			return true;
		}
		return false;
	}

protected:
	BSResourceNiBinaryStream* fin;
	char buf[MaxBuf];
};

template<typename T>
static bool AreEqual(T f1, T f2) { 
	return (std::fabs(f1 - f2) <= std::numeric_limits<T>::epsilon() * (std::max)(fabs(f1), fabs(f2)));
}

void BSReadAll(BSResourceNiBinaryStream* fin, std::string* str);
bool VisitObjects(NiAVObject * parent, std::function<bool(NiAVObject*)> functor);
void VisitLeveledCharacter(TESLevCharacter * character, std::function<void(TESNPC*)> functor);

// Travels up a node tree to determine the right node
NiNode * GetRootNode(Actor * actor, NiAVObject * object);

void ForEachMod(std::function<void(const ModInfo*)> functor);