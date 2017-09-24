#pragma once

#include "f4se/GameTypes.h"
#include "StringTable.h"

class TESNPC;
class Actor;
class TESRace;

#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

class BodyGenMorphData
{
public:
	F4EEFixedString	name;
	float			lower;
	float			upper;
};

class BodyGenMorphSelector : public std::vector<BodyGenMorphData>
{
public:
	UInt32 Evaluate(std::function<void(const F4EEFixedString &, float)> eval);
};

class BodyGenMorphs : public std::vector<BodyGenMorphSelector>
{
public:
	UInt32 Evaluate(std::function<void(const F4EEFixedString &, float)> eval);
};

class BodyGenTemplate : public std::vector<BodyGenMorphs>
{
public:
	UInt32 Evaluate(std::function<void(const F4EEFixedString &, float)> eval);
};
typedef std::shared_ptr<BodyGenTemplate> BodyGenTemplatePtr;


typedef std::unordered_map<F4EEFixedString, BodyGenTemplatePtr> BodyGenTemplates;

class BodyTemplateList : public std::vector<BodyGenTemplatePtr>
{
public:
	UInt32 Evaluate(std::function<void(const F4EEFixedString &, float)> eval);
};

class BodyGenDataTemplates : public std::vector<BodyTemplateList>
{
public:
	UInt32 Evaluate(std::function<void(const F4EEFixedString &, float)> eval);
};
typedef std::shared_ptr<BodyGenDataTemplates> BodyGenDataTemplatesPtr;

typedef std::unordered_map<TESNPC*, BodyGenDataTemplatesPtr> BodyGenData;

class BodyGenInterface
{
public:
	void LoadBodyGenMods();

	virtual bool ReadBodyMorphs(const std::string & filePath);
	virtual bool ReadBodyMorphTemplates(const std::string & filePath);
	virtual UInt32 EvaluateBodyMorphs(Actor * actor, bool isFemale);
	virtual void ClearBodyGenMods()
	{
		bodyGenTemplates.clear();
		bodyGenData[0].clear();
		bodyGenData[1].clear();
	}

	void GetFilteredNPCList(std::vector<TESNPC*> activeNPCs[], UInt8 modIndex, UInt16 lightIndex, UInt32 gender, TESRace * raceFilter);


protected:
	BodyGenTemplates bodyGenTemplates;
	BodyGenData	bodyGenData[2];
};