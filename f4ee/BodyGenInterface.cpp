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

bool BodyGenInterface::ReadBodyMorphTemplates(const std::string & filePath)
{
	BSResourceNiBinaryStream file(filePath.c_str());
	if (!file.IsValid()) {
		return false;
	}

	BSResourceTextFile<0x7FFF> textFile(&file);

	UInt32 lineCount = 0;
	std::string str = "";
	UInt32 loadedTemplates = 0;

	while (textFile.ReadLine(&str))
	{
		lineCount++;
		str = std::trim(str);
		if (str.length() == 0)
			continue;
		if (str.at(0) == '#')
			continue;

		std::vector<std::string> side = std::explode(str, '=');
		if (side.size() < 2) {
			_ERROR("%s - Error - Template has no left-hand side.\tLine (%d) [%s]", __FUNCTION__, lineCount, filePath);
			continue;
		}

		std::string lSide = std::trim(side[0]);
		std::string rSide = std::trim(side[1]);

		F4EEFixedString templateName = lSide.c_str();

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
						error = "Must have value pair with @ (";
						error += selectors[k];
						error += ")";
						break;
					}

					std::string morphName = std::trim(pairs[0]);
					if (morphName.length() == 0) {
						error = "Empty morph name";
						break;
					}

					std::string morphValues = std::trim(pairs[1]);
					if (morphValues.length() == 0) {
						error = "Empty values for (";
						error += morphName;
						error += ")";
						break;
					}

					float lowerValue = 0;
					float upperValue = 0;

					std::vector<std::string> range = std::explode(morphValues, ':');
					if (range.size() > 1) {
						std::string lowerRange = std::trim(range[0]);
						if (lowerRange.length() == 0) {
							error = "Empty lower range for (";
							error += morphName;
							error += ")";
							break;
						}

						lowerValue = atof(lowerRange.c_str());

						std::string upperRange = std::trim(range[1]);
						if (upperRange.length() == 0) {
							error = "Empty upper range for (";
							error += morphName;
							error += ")";
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
			_ERROR("%s - Error - Could not parse morphs %s.\tLine (%d) [%s]", __FUNCTION__, error.c_str(), lineCount, filePath);
			continue;
		}

		if (bodyGenSets->size() > 0) {
			bodyGenTemplates[templateName] = bodyGenSets;
			loadedTemplates++;
		}
	}

	_DMESSAGE("%s - Info - Loaded %d template(s).\t[%s]", __FUNCTION__, loadedTemplates, filePath);
	return true;
}

void BodyGenInterface::GetFilteredNPCList(std::vector<TESNPC*> activeNPCs[], SInt32 modIndex, UInt32 gender, TESRace * raceFilter)
{
	for (UInt32 i = 0; i < (*g_dataHandler)->arrNPC_.count; i++)
	{
		TESNPC * npc = nullptr;
		if ((*g_dataHandler)->arrNPC_.GetNthItem(i, npc))
		{
			bool matchMod = (modIndex == -1 || (npc->formID >> 24) == modIndex);
			bool matchRace = (npc->race.race == nullptr || npc->race.race == raceFilter);
			if (npc && npc->templateNPC == nullptr && matchMod && matchRace)
			{
				if(gender == 0xFF)
				{
					activeNPCs[0].push_back(npc);
					activeNPCs[1].push_back(npc);
				}
				else
					activeNPCs[gender].push_back(npc);
			}
		}
	}
}

bool BodyGenInterface::ReadBodyMorphs(const std::string & filePath)
{
	BSResourceNiBinaryStream file(filePath.c_str());
	if (!file.IsValid()) {
		return false;
	}

	BSResourceTextFile<0x7FFF> textFile(&file);

	UInt32 lineCount = 0;
	std::string str = "";
	UInt32 maleTargets = 0;
	UInt32 femaleTargets = 0;
	UInt32 maleOverwrite = 0;
	UInt32 femaleOverwrite = 0;

	while (textFile.ReadLine(&str))
	{
		lineCount++;
		str = std::trim(str);
		if (str.length() == 0)
			continue;
		if (str.at(0) == '#')
			continue;

		std::vector<std::string> side = std::explode(str, '=');
		if (side.size() < 2) {
			_ERROR("%s - Error - Morph has no left-hand side.\tLine (%d) [%s]", __FUNCTION__, lineCount, filePath);
			continue;
		}

		std::string lSide = std::trim(side[0]);
		std::string rSide = std::trim(side[1]);

		std::vector<std::string> form = std::explode(lSide, '|');
		if (form.size() < 2) {
			_ERROR("%s - Error - Morph left side missing mod name or formID.\tLine (%d) [%s]", __FUNCTION__, lineCount, filePath);
			continue;
		}

		int paramIndex = 0;

		std::vector<TESNPC*> activeNPCs[2];
		std::string modNameText = std::trim(form[paramIndex]);
		paramIndex++;

		// All|Gender[|Race]
		if (_strnicmp(modNameText.c_str(), "all", 3) == 0)
		{
			UInt8 gender = 0xFF;
			if (form.size() > paramIndex)
			{
				std::string genderText = std::trim(form[paramIndex]);
				std::transform(genderText.begin(), genderText.end(), genderText.begin(), ::tolower);
				if (genderText.compare("male") == 0) {
					gender = 0;
					paramIndex++;
				}
				else if (genderText.compare("female") == 0) {
					gender = 1;
					paramIndex++;
				}
			}

			TESRace * foundRace = nullptr;
			if (form.size() > paramIndex)
			{
				std::string raceText = std::trim(form[paramIndex]);
				foundRace = GetRaceByName(raceText);
				if (foundRace == nullptr)
				{
					_ERROR("%s - Error - Invalid race %s specified.\tLine (%d) [%s]", __FUNCTION__, raceText.c_str(), lineCount, filePath);
					continue;
				}
				paramIndex++;
			}

			GetFilteredNPCList(activeNPCs, -1, gender, foundRace);
		}
		else
		{
			UInt8 modIndex = (*g_dataHandler)->GetLoadedModIndex(modNameText.c_str());
			if (modIndex == -1) {
				_WARNING("%s - Warning - Mod '%s' not a loaded mod.\tLine (%d) [%s]", __FUNCTION__, modNameText.c_str(), lineCount, filePath);
				continue;
			}

			TESForm * foundForm = nullptr;
			std::string formIdText = std::trim(form[paramIndex]);
			paramIndex++;

			UInt8 gender = 0xFF;
			if (form.size() > paramIndex) {
				std::string genderText = std::trim(form[paramIndex]);
				std::transform(genderText.begin(), genderText.end(), genderText.begin(), ::tolower);
				if (genderText.compare("male") == 0) {
					gender = 0;
					paramIndex++;
				}
				else if (genderText.compare("female") == 0) {
					gender = 1;
					paramIndex++;
				}
			}

			// Fallout4.esm|All[|Gender][|Race]
			if (_strnicmp(formIdText.c_str(), "all", 3) == 0)
			{
				TESRace * foundRace = nullptr;
				if (form.size() > paramIndex)
				{
					std::string raceText = std::trim(form[paramIndex]);
					foundRace = GetRaceByName(raceText);
					if (foundRace == nullptr)
					{
						_ERROR("%s - Error - Invalid race '%s' specified.\tLine (%d) [%s]", __FUNCTION__, raceText.c_str(), lineCount, filePath);
						continue;
					}
					paramIndex++;
				}

				GetFilteredNPCList(activeNPCs, modIndex, gender, foundRace);
			}
			else // Fallout4.esm|XXXX[|Gender]
			{
				UInt32 formLower = strtoul(formIdText.c_str(), NULL, 16);
				if (formLower == 0) {
					_ERROR("%s - Error - Invalid formID.\tLine (%d) [%s]", __FUNCTION__, lineCount, filePath);
					continue;
				}

				UInt32 formId = modIndex << 24 | formLower & 0xFFFFFF;
				foundForm = LookupFormByID(formId);
				if (!foundForm) {
					_ERROR("%s - Error - Invalid form %08X.\tLine (%d) [%s]", __FUNCTION__, formId, lineCount, filePath);
					continue;
				}

				// Dont apply randomization to the player
				if (formId == 7)
					continue;
			}


			if(foundForm)
			{
				TESLevCharacter * levCharacter = DYNAMIC_CAST(foundForm, TESForm, TESLevCharacter);
				if(levCharacter) {
					VisitLeveledCharacter(levCharacter, [&](TESNPC * npc)
					{
						if(gender == 0xFF) {
							activeNPCs[0].push_back(npc);
							activeNPCs[1].push_back(npc);
						}
						else
							activeNPCs[gender].push_back(npc);
					});
				}

				TESNPC * npc = DYNAMIC_CAST(foundForm, TESForm, TESNPC);
				if(npc) {
					if(gender == 0xFF) {
						activeNPCs[0].push_back(npc);
						activeNPCs[1].push_back(npc);
					}
					else
						activeNPCs[gender].push_back(npc);
				}

				if (!npc && !levCharacter) {
					_ERROR("%s - Error - Invalid form %08X not an ActorBase or LeveledActor.\tLine (%d) [%s]", __FUNCTION__, foundForm->formID, lineCount, filePath);
					continue;
				}
			}
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
				F4EEFixedString templateName(selectors[k].c_str());
				auto & temp = bodyGenTemplates.find(templateName);
				if (temp != bodyGenTemplates.end())
					templateList.push_back(temp->second);
				else
					_WARNING("%s - Warning - template %s not found.\tLine (%d) [%s]", __FUNCTION__, templateName.c_str(), lineCount, filePath);
			}

			dataTemplates->push_back(templateList);
		}

		for (auto & npc : activeNPCs[0])
		{
			if(bodyGenData[0].find(npc) == bodyGenData[0].end()) {
#ifdef _DEBUG
				_DMESSAGE("%s - Read male target %s (%08X)", __FUNCTION__, npc->fullName.name.c_str(), npc->formID);
#endif
				maleTargets++;
			} else {
				maleOverwrite++;
			}

			bodyGenData[0][npc] = dataTemplates;


		}

		for (auto & npc : activeNPCs[1])
		{
			if(bodyGenData[1].find(npc) == bodyGenData[1].end()) {
#ifdef _DEBUG
				_DMESSAGE("%s - Read female target %s (%08X)", __FUNCTION__, npc->fullName.name.c_str(), npc->formID);
#endif
				femaleTargets++;
			} else {
				femaleOverwrite++;
			}

			bodyGenData[1][npc] = dataTemplates;
		}

		if(maleOverwrite)
			_DMESSAGE("%s - Info - %d male NPC targets(s) overwritten.\tLine (%d) [%s]", __FUNCTION__, maleOverwrite, lineCount, filePath);
		if(femaleOverwrite)
			_DMESSAGE("%s - Info - %d female NPC targets(s) overwritten.\tLine (%d) [%s]", __FUNCTION__, femaleOverwrite, lineCount, filePath);

		maleOverwrite = 0;
		femaleOverwrite = 0;
	}

	_DMESSAGE("%s - Info - Acquired %d male NPC target(s).\t[%s]", __FUNCTION__, maleTargets, filePath);
	_DMESSAGE("%s - Info - Acquired %d female NPC target(s).\t[%s]", __FUNCTION__, femaleTargets, filePath);
	return true;
}

UInt32 BodyGenMorphSelector::Evaluate(std::function<void(const F4EEFixedString &, float)> eval)
{
	if (size() > 0) {
		std::random_device rd;
		std::default_random_engine gen(rd());
		std::uniform_int_distribution<> rndMorph(0, size() - 1);

		auto & bodyMorph = at(rndMorph(gen));
		float a = bodyMorph.lower;
		float b = bodyMorph.upper;
		std::uniform_real_distribution<> rndValue(a <= b ? a : b, a <= b ? b : a);
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
		UInt8 gender = isFemale ? 1 : 0;
		BodyGenData::iterator morphSet = bodyGenData[gender].end();
		do {
			morphSet = bodyGenData[gender].find(actorBase);
			actorBase = actorBase->templateNPC;
		} while (actorBase && morphSet == bodyGenData[gender].end());

		// Found a matching template
		if (morphSet != bodyGenData[gender].end()) {
			_DMESSAGE("%s - Generating BodyMorphs for %s (%08X)", __FUNCTION__, CALL_MEMBER_FN(actor, GetReferenceName)(), actor->formID);
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
