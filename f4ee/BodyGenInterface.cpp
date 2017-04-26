#include "BodyGenInterface.h"
#include "Utilities.h"

#include <stdlib.h>
#include <random>

#include "f4se/GameRTTI.h"
#include "f4se/GameData.h"
#include "f4se/GameObjects.h"
#include "f4se/GameReferences.h"
#include "f4se/GameStreams.h"

#include "BodyMorphInterface.h"

extern BodyMorphInterface g_bodyMorphInterface;

bool BodyGenInterface::ReadBodyMorphTemplates(const BSFixedString & filePath)
{
	BSResourceNiBinaryStream file(filePath.c_str());
	if (!file.IsValid()) {
		return false;
	}

	UInt32 lineCount = 0;
	std::string str = "";

	while (BSReadLine(&file, &str))
	{
		lineCount++;
		str = std::trim(str);
		if (str.length() == 0)
			continue;
		if (str.at(0) == '#')
			continue;

		std::vector<std::string> side = std::explode(str, '=');
		if (side.size() < 2) {
			_ERROR("%s - Error - Line (%d) loading a morph from %s has no left-hand side.", __FUNCTION__, lineCount, filePath.c_str());
			continue;
		}

		std::string lSide = std::trim(side[0]);
		std::string rSide = std::trim(side[1]);

		BSFixedString templateName = lSide.c_str();

		BodyGenTemplatePtr bodyGenSets = std::make_shared<BodyGenTemplate>();

		std::string error = "";
		std::vector<std::string> sets = std::explode(rSide, '/');
		for (UInt32 i = 0; i < sets.size(); i++) {
			sets[i] = std::trim(sets[i]);

			BodyGenMorphs bodyMorphs;

			std::vector<std::string> morphs = std::explode(sets[i], ',');
			for (UInt32 j = 0; j < morphs.size(); j++) {
				morphs[j] = std::trim(morphs[j]);

				std::vector<std::string> selectors = std::explode(morphs[j], '|');

				BodyGenMorphSelector selector;

				for (UInt32 k = 0; k < selectors.size(); k++) {
					selectors[k] = std::trim(selectors[k]);

					std::vector<std::string> pairs = std::explode(selectors[k], '@');
					if (pairs.size() < 2) {
						error = "Must have value pair with @";
						break;
					}

					std::string morphName = std::trim(pairs[0]);
					if (morphName.length() == 0) {
						error = "Empty morph name";
						break;
					}

					std::string morphValues = std::trim(pairs[1]);
					if (morphValues.length() == 0) {
						error = "Empty values";
						break;
					}

					float lowerValue = 0;
					float upperValue = 0;

					std::vector<std::string> range = std::explode(morphValues, ':');
					if (range.size() > 1) {
						std::string lowerRange = std::trim(range[0]);
						if (lowerRange.length() == 0) {
							error = "Empty lower range";
							break;
						}

						lowerValue = atof(lowerRange.c_str());

						std::string upperRange = std::trim(range[1]);
						if (upperRange.length() == 0) {
							error = "Empty upper range";
							break;
						}

						upperValue = atof(upperRange.c_str());
					}
					else {
						lowerValue = atof(morphValues.c_str());
						upperValue = lowerValue;
					}

					BodyGenMorphData morphData;
					morphData.name = morphName.c_str();
					morphData.lower = lowerValue;
					morphData.upper = upperValue;
					selector.push_back(morphData);
				}

				if (error.length() > 0)
					break;

				bodyMorphs.push_back(selector);
			}

			if (error.length() > 0)
				break;

			bodyGenSets->push_back(bodyMorphs);
		}

		if (error.length() > 0) {
			_ERROR("%s - Error - Line (%d) could not parse morphs from %s. (%s)", __FUNCTION__, lineCount, filePath.c_str(), error.c_str());
			continue;
		}

		if (bodyGenSets->size() > 0)
			bodyGenTemplates[templateName] = bodyGenSets;
	}

	return true;
}

bool BodyGenInterface::ReadBodyMorphs(const BSFixedString & filePath)
{
	BSResourceNiBinaryStream file(filePath.c_str());
	if (!file.IsValid()) {
		return false;
	}

	UInt32 lineCount = 0;
	std::string str = "";
	UInt32 totalTargets = 0;

	while (BSReadLine(&file, &str))
	{
		lineCount++;
		str = std::trim(str);
		if (str.length() == 0)
			continue;
		if (str.at(0) == '#')
			continue;

		std::vector<std::string> side = std::explode(str, '=');
		if (side.size() < 2) {
			_ERROR("%s - Error - %s (%d) loading a morph from %s has no left-hand side.", __FUNCTION__, filePath.c_str(), lineCount);
			continue;
		}

		std::string lSide = std::trim(side[0]);
		std::string rSide = std::trim(side[1]);

		std::vector<std::string> form = std::explode(lSide, '|');
		if (form.size() < 2) {
			_ERROR("%s - Error - %s (%d) morph left side from %s missing mod name or formID.", __FUNCTION__, filePath.c_str(), lineCount);
			continue;
		}

		std::vector<TESNPC*> activeNPCs;
		std::string modNameText = std::trim(form[0]);
		if (_strnicmp(modNameText.c_str(), "All", 3) == 0)
		{
			std::string genderText = std::trim(form[1]);
			UInt8 gender = 0;
			if (_strnicmp(genderText.c_str(), "Male", 4) == 0)
				gender = 0;
			else if (_strnicmp(genderText.c_str(), "Female", 6) == 0)
				gender = 1;
			else {
				_ERROR("%s - Error - %s (%d) invalid gender specified.", __FUNCTION__, filePath.c_str(), lineCount);
				continue;
			}

			bool raceFilter = false;
			TESRace * foundRace = nullptr;
			if (form.size() >= 3)
			{
				std::string raceText = std::trim(form[2]);
				for (UInt32 i = 0; i < (*g_dataHandler)->arrRACE.count; i++)
				{
					TESRace * race = nullptr;
					if ((*g_dataHandler)->arrRACE.GetNthItem(i, race)) {
						if (race && _strnicmp(raceText.c_str(), race->editorId.c_str(), raceText.size()) == 0) {
							foundRace = race;
							break;
						}
					}
				}
				raceFilter = true;

				if (foundRace == nullptr)
				{
					_ERROR("%s - Error - %s (%d) invalid race %s specified.", __FUNCTION__, filePath.c_str(), lineCount, raceText.c_str());
					continue;
				}
			}

			for (UInt32 i = 0; i < (*g_dataHandler)->arrNPC_.count; i++)
			{
				TESNPC * npc = nullptr;
				if ((*g_dataHandler)->arrNPC_.GetNthItem(i, npc)) {
					if (npc && npc->templateNPC == nullptr && CALL_MEMBER_FN(npc, GetSex)() == gender && (raceFilter == false || npc->race.race == foundRace))
						activeNPCs.push_back(npc);
				}
			}
		}
		else
		{
			BSFixedString modText(modNameText.c_str());
			UInt8 modIndex = (*g_dataHandler)->GetModIndex(modText.c_str());
			if (modIndex == -1) {
				_WARNING("%s - Warning - %s (%d) Mod %s not a loaded mod.", __FUNCTION__, filePath.c_str(), lineCount, modText.c_str());
				continue;
			}

			std::string formIdText = std::trim(form[1]);
			UInt32 formLower = strtoul(formIdText.c_str(), NULL, 16);

			if (formLower == 0) {
				_ERROR("%s - Error - %s (%d) invalid formID.", __FUNCTION__, filePath.c_str(), lineCount);
				continue;
			}

			UInt32 formId = modIndex << 24 | formLower & 0xFFFFFF;
			TESForm * foundForm = LookupFormByID(formId);
			if (!foundForm) {
				_ERROR("%s - Error - %s (%d) invalid form %08X.", __FUNCTION__, filePath.c_str(), lineCount, formId);
				continue;
			}

			// Dont apply randomization to the player
			if (formId == 7)
				continue;

			TESNPC * npc = DYNAMIC_CAST(foundForm, TESForm, TESNPC);
			if (!npc) {
				_ERROR("%s - Error - %s (%d) invalid form %08X not an ActorBase.", __FUNCTION__, filePath.c_str(), lineCount, formId);
				continue;
			}

			activeNPCs.push_back(npc);
		}

		BodyGenDataTemplatesPtr dataTemplates = std::make_shared<BodyGenDataTemplates>();
		std::string error = "";
		std::vector<std::string> sets = std::explode(rSide, ',');
		for (UInt32 i = 0; i < sets.size(); i++) {
			sets[i] = std::trim(sets[i]);
			std::vector<std::string> selectors = std::explode(sets[i], '|');
			BodyTemplateList templateList;
			for (UInt32 k = 0; k < selectors.size(); k++) {
				selectors[k] = std::trim(selectors[k]);
				BSFixedString templateName(selectors[k].c_str());
				auto & temp = bodyGenTemplates.find(templateName);
				if (temp != bodyGenTemplates.end())
					templateList.push_back(temp->second);
				else
					_WARNING("%s - Warning - %s (%d) template %s not found.", __FUNCTION__, filePath.c_str(), lineCount, templateName.c_str());
			}

			dataTemplates->push_back(templateList);
		}

		for (auto & npc : activeNPCs)
		{
			bodyGenData[npc] = dataTemplates;
#ifdef _DEBUG
			_DMESSAGE("%s - Read target %s", __FUNCTION__, npc->fullName.name.c_str());
#endif
		}

		totalTargets += activeNPCs.size();
	}

	_DMESSAGE("%s - Read %d target(s) from %s", __FUNCTION__, totalTargets, filePath.c_str());
	return true;
}

UInt32 BodyGenMorphSelector::Evaluate(std::function<void(const F4EEFixedString &, float)> eval)
{
	if (size() > 0) {
		std::random_device rd;
		std::default_random_engine gen(rd());
		std::uniform_int_distribution<> rndMorph(0, size() - 1);

		auto & bodyMorph = at(rndMorph(gen));
		std::uniform_real_distribution<> rndValue(bodyMorph.lower, bodyMorph.upper);
		float val = rndValue(gen);
		if (val != 0) {
			eval(bodyMorph.name, val);
			return 1;
		}
	}

	return 0;
}

UInt32 BodyGenMorphs::Evaluate(std::function<void(const F4EEFixedString &, float)> eval)
{
	UInt32 total = 0;
	for (auto value : *this) {
		if (value.size() < 1)
			continue;

		total += value.Evaluate(eval);
	}

	return total;
}

UInt32 BodyGenTemplate::Evaluate(std::function<void(const F4EEFixedString &, float)> eval)
{
	if (size() > 0) {
		std::random_device rd;
		std::default_random_engine gen(rd());
		std::uniform_int_distribution<> rnd(0, size() - 1);

		auto & morphs = at(rnd(gen));
		return morphs.Evaluate(eval);
	}

	return 0;
}

UInt32 BodyTemplateList::Evaluate(std::function<void(const F4EEFixedString &, float)> eval)
{
	if (size() > 0) {
		std::random_device rd;
		std::default_random_engine gen(rd());
		std::uniform_int_distribution<> rnd(0, size() - 1);

		auto & bodyTemplate = at(rnd(gen));
		return bodyTemplate->Evaluate(eval);
	}

	return 0;
}

UInt32 BodyGenDataTemplates::Evaluate(std::function<void(const F4EEFixedString &, float)> eval)
{
	UInt32 total = 0;
	for (auto & tempList : *this)
	{
		total += tempList.Evaluate(eval);
	}

	return total;
}

UInt32 BodyGenInterface::EvaluateBodyMorphs(Actor * actor, bool isFemale)
{
	TESNPC * actorBase = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
	if (actorBase) {
		BodyGenData::iterator morphSet = bodyGenData.end();
		do {
			morphSet = bodyGenData.find(actorBase);
			actorBase = actorBase->templateNPC;
		} while (actorBase && morphSet == bodyGenData.end());

		// Found a matching template
		if (morphSet != bodyGenData.end()) {
			std::random_device rd;
			std::default_random_engine gen(rd());

			auto & templates = morphSet->second;
			UInt32 ret = templates->Evaluate([&](const F4EEFixedString & morphName, float value)
			{
				g_bodyMorphInterface.SetMorph(actor, isFemale, morphName, nullptr, value);
			});
			return ret;
		}
	}

	return 0;
}

void BodyGenInterface::LoadBodyGenMods()
{
	// Load templates
	for(int i = 0; i < (*g_dataHandler)->modList.loadedModCount; i++)
	{
		ModInfo * modInfo = (*g_dataHandler)->modList.loadedMods[i];
		std::string templatesPath = std::string("Data\\F4SE\\Plugins\\F4EE\\BodyGen\\") + std::string(modInfo->name) + "\\templates.ini";
		ReadBodyMorphTemplates(templatesPath.c_str());
	}

	for(int i = 0; i < (*g_dataHandler)->modList.loadedModCount; i++)
	{
		ModInfo * modInfo = (*g_dataHandler)->modList.loadedMods[i];
		std::string templatesPath = std::string("Data\\F4SE\\Plugins\\F4EE\\BodyGen\\") + std::string(modInfo->name) + "\\morphs.ini";
		ReadBodyMorphs(templatesPath.c_str());
	}
}
