#include "f4se/PluginAPI.h"
#include "f4se_common/f4se_version.h"
#include "f4se_common/SafeWrite.h"

#include "f4se/GameData.h"

#include <shlobj.h>

#include "Pattern.h"

IDebugLog	gLog;
PluginHandle	g_pluginHandle = kPluginHandle_Invalid;

extern "C"
{

bool F4SEPlugin_Query(const F4SEInterface * f4se, PluginInfo * info)
{
	gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Fallout4\\F4SE\\achievements.log");
	_DMESSAGE("achievements");

	// populate info structure
	info->infoVersion =	PluginInfo::kInfoVersion;
	info->name =		"ACNT";
	info->version =		1;

	// store plugin handle so we can identify ourselves later
	g_pluginHandle = f4se->GetPluginHandle();

	if(f4se->isEditor)
	{
		_FATALERROR("loaded in editor, marking as incompatible");
		return false;
	}

	// supported runtime version
	return true;
}


bool F4SEPlugin_Load(const F4SEInterface * f4se)
{

	uintptr_t * isModded = nullptr;

	if(f4se->runtimeVersion >= RUNTIME_VERSION_1_10_20)
	{
		isModded = Utility::pattern( "48 83 EC 28 C6 44 24 ? ? 84 D2").count( 1 ).get( 0 ).get<uintptr_t>();
	}
	else if(f4se->runtimeVersion >= RUNTIME_VERSION_1_5_412)
	{
		isModded = Utility::pattern( "40 57 48 83 EC 30 40 32 FF").count( 1 ).get( 0 ).get<uintptr_t>();
	}
	else if(f4se->runtimeVersion >= RUNTIME_VERSION_1_5_205)
	{
		isModded = Utility::pattern( "40 53 48 83 EC 30 32 DB 84 D2" ).count( 1 ).get( 0 ).get<uintptr_t>();
	}
	else if(f4se->runtimeVersion == RUNTIME_VERSION_1_5_157)
	{
		isModded = Utility::pattern( "48 89 5C 24 ? 55 57 41 57 48 83 EC 20 8B A9 ? ? ? ?" ).count( 1 ).get( 0 ).get<uintptr_t>();
	}
	
	if(isModded)
	{
		// xor al, al
		// retn
		SafeWrite32((uintptr_t)isModded, 0x00C3C030); //0x30C0C300
		_MESSAGE("Applied achievements patch to runtime");
		return true;
	}

	_FATALERROR("Failed to find pattern for IsModded function");
	return false;
}

};
