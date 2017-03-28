#include "f4se/PluginAPI.h"
#include "f4se_common/f4se_version.h"
#include "f4se_common/SafeWrite.h"

#include "f4se/GameData.h"

#include <shlobj.h>
#include <string>

#include "Pattern.h"

IDebugLog	gLog;
PluginHandle	g_pluginHandle = kPluginHandle_Invalid;

const std::string & F4SUGetRuntimeDirectory(void)
{
	static std::string s_runtimeDirectory;

	if(s_runtimeDirectory.empty())
	{
		// can't determine how many bytes we'll need, hope it's not more than MAX_PATH
		char	runtimePathBuf[MAX_PATH];
		UInt32	runtimePathLength = GetModuleFileName(GetModuleHandle(NULL), runtimePathBuf, sizeof(runtimePathBuf));

		if(runtimePathLength && (runtimePathLength < sizeof(runtimePathBuf)))
		{
			std::string	runtimePath(runtimePathBuf, runtimePathLength);

			// truncate at last slash
			std::string::size_type	lastSlash = runtimePath.rfind('\\');
			if(lastSlash != std::string::npos)	// if we don't find a slash something is VERY WRONG
			{
				s_runtimeDirectory = runtimePath.substr(0, lastSlash + 1);

				_DMESSAGE("runtime root = %s", s_runtimeDirectory.c_str());
			}
			else
			{
				_WARNING("no slash in runtime path? (%s)", runtimePath.c_str());
			}
		}
		else
		{
			_WARNING("couldn't find runtime path (len = %d, err = %08X)", runtimePathLength, GetLastError());
		}
	}

	return s_runtimeDirectory;
}

const std::string & F4SUGetConfigPath(void)
{
	static std::string s_configPath;

	if(s_configPath.empty())
	{
		std::string	runtimePath = F4SUGetRuntimeDirectory();
		if(!runtimePath.empty())
		{
			s_configPath = runtimePath + "Data\\F4SE\\Plugins\\survivalunlock.ini";

			_MESSAGE("config path = %s", s_configPath.c_str());
		}
	}

	return s_configPath;
}

std::string F4SUGetConfigOption(const char * section, const char * key)
{
	std::string	result;

	const std::string & configPath = F4SUGetConfigPath();
	if(!configPath.empty())
	{
		char	resultBuf[256];
		resultBuf[0] = 0;

		UInt32	resultLen = GetPrivateProfileString(section, key, NULL, resultBuf, sizeof(resultBuf), configPath.c_str());

		result = resultBuf;
	}

	return result;
}

template<typename T>
const char * F4SUGetTypeFormatting(T * dataOut)
{
	return false;
}



template<> const char * F4SUGetTypeFormatting(float * dataOut) { return "%f"; }
template<> const char * F4SUGetTypeFormatting(bool * dataOut) { return "%d"; }
template<> const char * F4SUGetTypeFormatting(SInt32 * dataOut) { return "%d"; }
template<> const char * F4SUGetTypeFormatting(UInt32 * dataOut) { return "%d"; }
template<> const char * F4SUGetTypeFormatting(UInt64 * dataOut) { return "%I64u"; }

template<typename T>
bool F4SUGetConfigValue(const char * section, const char * key, T * dataOut)
{
	std::string	data = F4SUGetConfigOption(section, key);
	if (data.empty())
		return false;

	T tmp;
	bool res = (sscanf_s(data.c_str(), F4SUGetTypeFormatting(dataOut), &tmp) == 1);
	if(res) {
		*dataOut = tmp;
	}
	return res;
}

template<>
bool F4SUGetConfigValue(const char * section, const char * key, bool * dataOut)
{
	std::string	data = F4SUGetConfigOption(section, key);
	if (data.empty())
		return false;

	UInt32 tmp;
	bool res = (sscanf_s(data.c_str(), F4SUGetTypeFormatting(&tmp), &tmp) == 1);
	if(res) {
		*dataOut = (tmp > 0);
	}
	return res;
}

extern "C"
{

bool F4SEPlugin_Query(const F4SEInterface * f4se, PluginInfo * info)
{
	gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Fallout4\\F4SE\\survivalunlock.log");
	_DMESSAGE("survivalunlock");

	// populate info structure
	info->infoVersion =	PluginInfo::kInfoVersion;
	info->name =		"SVUL";
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

	uintptr_t * pauseSave = nullptr;
	uintptr_t * quickSave = nullptr;
	uintptr_t * retSurvival = nullptr;
	uintptr_t * godMode = nullptr;
	uintptr_t * immortalMode = nullptr;
	uintptr_t * console = nullptr;

	bool allowSaving = true;
	bool returnSurvival = true;
	bool allowGodMode = true;
	bool allowConsole = true;
	F4SUGetConfigValue("Features", "bAllowSaving", &allowSaving);
	F4SUGetConfigValue("Features", "bAllowReturnToSurvival", &returnSurvival);
	F4SUGetConfigValue("Features", "bAllowGodMode", &allowGodMode);
	F4SUGetConfigValue("Features", "bAllowConsole", &allowConsole);

	if(f4se->runtimeVersion >= RUNTIME_VERSION_1_8_7)
	{
		if(allowSaving) {
			pauseSave = Utility::pattern( "83 F8 06 74 04 32 DB").count( 1 ).get( 0 ).get<uintptr_t>();
			if(!pauseSave)
				_MESSAGE("Failed to find pattern for allowing save from the pause menu");

			quickSave = Utility::pattern( "83 F8 06 74 18").count( 1 ).get( 0 ).get<uintptr_t>();
			if(!quickSave)
				_MESSAGE("Failed to find pattern for allowing quicksave");
		}
		if(returnSurvival) {
			retSurvival = Utility::pattern( "E8 ? ? ? ? 44 39 75 67").count( 1 ).get( 0 ).get<uintptr_t>();
			if(!retSurvival)
				_MESSAGE("Failed to find pattern for returning to Survival");
		}
		if(allowGodMode) {
			auto pattern = Utility::pattern( "74 27 8B 05 ? ? ? ? 83 F8 06 7F 09 85").count( 2 );
			godMode = pattern.get( 0 ).get<uintptr_t>();
			immortalMode = pattern.get( 1 ).get<uintptr_t>();

			if(!godMode || !immortalMode)
				_MESSAGE("Failed to find pattern for God Mode");
		}
		if(allowConsole) {
			console = Utility::pattern( "83 F8 06 75 0D 80 3D ? ? ? ? ?").count( 1 ).get( 0 ).get<uintptr_t>();
			if(!console)
				_MESSAGE("Failed to find pattern for console");
		}
	}
	
	if(pauseSave && quickSave)
	{
		SafeWrite8(((uintptr_t)pauseSave) + 2, 0x07);	// Changes cmp 6 to cmp 7
		SafeWrite8(((uintptr_t)quickSave) + 2, 0x07);	// Changes cmp 6 to cmp 7

		_MESSAGE("Enabled saving in Survival difficulty");
	}
	if(retSurvival)
	{
		UInt8 nop[] = {0x90, 0x90, 0x90, 0x90, 0x90};
		SafeWriteBuf((uintptr_t)retSurvival, nop, sizeof(nop)); // Overwrite call to get pre-existing survival
		_MESSAGE("Enabled returning to Survival difficulty");
	}
	if(godMode && immortalMode)
	{
		UInt8 jmp1c[] = {0xEB, 0x27};
		SafeWriteBuf((uintptr_t)godMode, jmp1c, sizeof(jmp1c));
		SafeWriteBuf((uintptr_t)immortalMode, jmp1c, sizeof(jmp1c));
		_MESSAGE("Enabled god mode and immortal mode for Survival difficulty");
	}
	if(allowConsole)
	{
		SafeWrite8(((uintptr_t)console) + 2, 0x07);	// Changes cmp 6 to cmp 7
		_MESSAGE("Enabled console in Survival difficulty");
	}

	return true;
}

};
