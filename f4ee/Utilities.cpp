#include "Utilities.h"
#include "f4se/GameStreams.h"
#include "f4se/NiNodes.h"
#include "f4se/NiTypes.h"
#include "f4se/GameForms.h"
#include "f4se/GameData.h"
#include "f4se/GameReferences.h"
#include "f4se/GameObjects.h"
#include "f4se/GameRTTI.h"

#include <iomanip>
#include <sstream>
#include <cctype>
#include <unordered_set>
#include <queue>
#include <algorithm>
#include <functional>

template <>
bool Serialization::WriteData<F4EEFixedString>(const F4SESerializationInterface * intfc, const F4EEFixedString * str)
{
	UInt16 len = strlen(str->c_str());
	if (len > SHRT_MAX)
		return false;
	if (! intfc->WriteRecordData(&len, sizeof(len)))
		return false;
	if (len == 0)
		return true;
	if (! intfc->WriteRecordData(str->c_str(), len))
		return false;
	return true;
}

template <>
bool Serialization::ReadData<F4EEFixedString>(const F4SESerializationInterface * intfc, F4EEFixedString * str)
{
	UInt16 len = 0;

	if (! intfc->ReadRecordData(&len, sizeof(len)))
		return false;
	if(len == 0)
		return true;
	if (len > SHRT_MAX)
		return false;

	char * buf = new char[len + 1];
	buf[0] = 0;

	if (! intfc->ReadRecordData(buf, len)) {
		delete [] buf;
		return false;
	}
	buf[len] = 0;

	*str = F4EEFixedString(buf);
	delete [] buf;
	return true;
}

std::string bytes_to_string(std::size_t size) {               
	static const char *SIZES[] = { "B", "KB", "MB", "GB" };

	int div = 0;
	size_t rem = 0;
	while (size >= 1024 && div < (sizeof SIZES / sizeof *SIZES)) {
		rem = (size % 1024);
		div++;
		size /= 1024;
	}

	double size_d = (float)size + (float)rem / 1024.0;

	std::stringstream ss;
	ss << std::fixed << std::setprecision(2) << size_d << SIZES[div];
	return ss.str();
}

void BSReadAll(BSResourceNiBinaryStream* fin, std::string* str)
{
	char ch;
	UInt32 ret = fin->Read(&ch, 1);
	while (ret > 0) {
		str->push_back(ch);
		ret = fin->Read(&ch, 1);
	}
}

bool VisitObjects(NiAVObject * parent, std::function<bool(NiAVObject*)> functor)
{
	if (functor(parent))
		return true;

	NiPointer<NiNode> node(parent->GetAsNiNode());
	if(node) {
		for(UInt32 i = 0; i < node->m_children.m_emptyRunStart; i++) {
			NiPointer<NiAVObject> object(node->m_children.m_data[i]);
			if(object) {
				if (VisitObjects(object, functor))
					return true;
			}
		}
	}

	return false;
}


std::string GetFormIdentifier(TESForm * form)
{
	char formName[MAX_PATH];
	UInt8 modIndex = form->formID >> 24;
	UInt32 modForm = form->formID & 0xFFFFFF;

	ModInfo* modInfo = nullptr;
	if(modIndex == 0xFE)
	{
		UInt16 lightIndex = (form->formID >> 12) & 0xFFF;
		if(lightIndex < (*g_dataHandler)->modList.lightMods.count)
			modInfo = (*g_dataHandler)->modList.lightMods[lightIndex];
	}
	else
	{
		modInfo = (*g_dataHandler)->modList.loadedMods[modIndex];
	}
	
	if (modInfo) {
		sprintf_s(formName, "%s|%06X", modInfo->name, modForm);
	}

	return formName;
}

TESForm * GetFormFromIdentifier(const std::string & formIdentifier)
{
	std::size_t pos = formIdentifier.find_first_of('|');
	std::string modName = formIdentifier.substr(0, pos);
	std::string modForm = formIdentifier.substr(pos+1);

	UInt32 formId = 0;
	sscanf_s(modForm.c_str(), "%X", &formId);

	UInt8 modIndex = (*g_dataHandler)->GetLoadedModIndex(modName.c_str());
	if(modIndex != 0xFF) {
		formId |= ((UInt32)modIndex) << 24;
	}
	else
	{
		UInt16 lightModIndex = (*g_dataHandler)->GetLoadedLightModIndex(modName.c_str());
		if(lightModIndex != 0xFFFF) {
			formId |= 0xFE000000 | (UInt32(lightModIndex) << 12);
		}
	}

	return LookupFormByID(formId);
}

TESRace * GetActorRace(Actor * actor)
{
	TESRace * race = actor->race;
	if(!race) {
		TESNPC * npc = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
		if(npc)
			race = npc->race.race;
	}

	return race;
}

TESRace * GetRaceByName(const std::string & raceName)
{
	F4EEFixedString lower(raceName.c_str());
	for(UInt64 i = 0; i < (*g_dataHandler)->arrRACE.count; i++)
	{
		TESRace * race;
		(*g_dataHandler)->arrRACE.GetNthItem(i, race);

		F4EEFixedString raceName(race->editorId.c_str());
		if(raceName == lower)
		{
			return race;
		}
	}

	return nullptr;
}

namespace std {
std::string& ltrim(std::string& s) {
	s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), isspace));
	return s;
}

// trim from end
std::string& rtrim(std::string& s) {
	s.erase((std::find_if_not(s.rbegin(), s.rend(), isspace)).base(), s.end());
	return s;
}

// trim from both ends
std::string &std::trim(std::string &s) {
	return std::ltrim(std::rtrim(s));
}
}

std::vector<std::string> std::explode(const std::string& str, const char& ch) {
	std::string next;
	std::vector<std::string> result;

	for (std::string::const_iterator it = str.begin(); it != str.end(); it++) {
		if (*it == ch) {
			if (!next.empty()) {
				result.push_back(next);
				next.clear();
			}
		}
		else {
			next += *it;
		}
	}
	if (!next.empty())
		result.push_back(next);
	return result;
}

void VisitLeveledCharacter(TESLevCharacter * character, std::function<void(TESNPC*)> functor)
{
	std::unordered_set<TESLevCharacter*> visited;
	std::queue<TESLevCharacter*> visit;

	visit.push(character);

	while(!visit.empty())
	{
		character = visit.front();
		visit.pop();

		if(character)
		{
			for(UInt32 i = 0; i < character->leveledList.length; i++)
			{
				TESForm * form = character->leveledList.entries[i].form;
				if(form) {
					TESLevCharacter * levCharacter = DYNAMIC_CAST(form, TESForm, TESLevCharacter);
					if(levCharacter && visited.find(levCharacter) == visited.end())
						visit.push(levCharacter);

					TESNPC * npc = DYNAMIC_CAST(form, TESForm, TESNPC);
					if(npc)
						functor(npc);
				}
			}

			visited.insert(character);
		}
	}
}

NiNode * GetRootNode(Actor * actor, NiAVObject * object)
{
	NiNode * rootNode = actor->GetActorRootNode(false);

	bool isFirstPerson = false;

	// Only the player will have a first person skeleton
	if(actor == (*g_player)) {
		NiNode * node1P = actor->GetActorRootNode(true);

		// Go up to the root and see if it is the first person one
		NiNode * foundNode = nullptr;
		NiNode * parent = object->m_parent;
		while(parent)
		{
			if (parent == node1P) {
				foundNode = node1P;
				break;
			}
			parent = parent->m_parent;
		}

		isFirstPerson = (foundNode == node1P);
		if(isFirstPerson)
			rootNode = node1P;
	}

	return rootNode;
}

void ForEachMod(std::function<void(const ModInfo*)> functor)
{
	for(int i = 0; i < (*g_dataHandler)->modList.loadedMods.count; i++)
	{
		functor((*g_dataHandler)->modList.loadedMods[i]);
	}

	for(int i = 0; i < (*g_dataHandler)->modList.lightMods.count; i++)
	{
		functor((*g_dataHandler)->modList.lightMods[i]);
	}
}