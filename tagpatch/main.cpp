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
	gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Fallout4\\F4SE\\tagpatch.log");
	_DMESSAGE("achievements");

	// populate info structure
	info->infoVersion =	PluginInfo::kInfoVersion;
	info->name =		"TGPH";
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

	uintptr_t * componentCheck = nullptr;

	if(f4se->runtimeVersion >= RUNTIME_VERSION_1_8_7)
	{
		componentCheck = Utility::pattern( "4C 8D 0D ? ? ? ? 4C 8D 05 ? ? ? ? 33 D2 48 8B C8 44 89 6C 24 ? E8 ? ? ? ? 41 B9 ? ? ? ? 48 8D 94 24 ? ? ? ? 45 8B C1 49 8B CE").count( 1 ).get( 0 ).get<uintptr_t>();
	}
	
	if(componentCheck)
	{
		// jmp +1B
		uint8_t buff[] = { 0xEB, 0x1B };
		SafeWriteBuf((uintptr_t)componentCheck, &buff, sizeof(buff));
		_MESSAGE("Applied tag patch");
		return true;
	}

	_FATALERROR("Failed to find pattern for tag patch");
	return false;
}

};
