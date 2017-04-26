#include "Utilities.h"
#include "f4se/GameStreams.h"
#include "f4se/NiNodes.h"
#include "f4se/NiTypes.h"
#include "f4se/GameForms.h"
#include "f4se/GameData.h"

#include <cctype>

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

bool BSReadLine(BSResourceNiBinaryStream* fin, std::string* str)
{
	char buf[1024];
	UInt32 ret = 0;

	ret = fin->ReadLine((char*)buf, 1024, '\n');
	if (ret > 0) {
		buf[ret - 1] = '\0';
		*str = buf;
		return true;
	}
	return false;
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
	ModInfo* modInfo = (*g_dataHandler)->modList.loadedMods[modIndex];
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

	auto mod = (*g_dataHandler)->LookupLoadedModByName(modName.c_str());
	if(!mod) // No loaded mod by this name
		return nullptr;

	UInt32 formId = 0;
	sscanf_s(modForm.c_str(), "%X", &formId);
	formId |= ((UInt32)mod->modIndex) << 24;

	return LookupFormByID(formId);
}

TESRace * GetRaceByName(const std::string & raceName)
{
	BSAutoFixedString name(raceName.c_str());
	for(UInt64 i = 0; i < (*g_dataHandler)->arrRACE.count; i++)
	{
		TESRace * race;
		(*g_dataHandler)->arrRACE.GetNthItem(i, race);

		if(race->editorId == name)
		{
			return race;
		}
	}

	return nullptr;
}

std::string &std::ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

// trim from end
std::string &std::rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

// trim from both ends
std::string &std::trim(std::string &s) {
	return std::ltrim(std::rtrim(s));
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