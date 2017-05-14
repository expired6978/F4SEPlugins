#include "f4se/PluginAPI.h"
#include "f4se_common/f4se_version.h"
#include "f4se_common/SafeWrite.h"

#include "f4se/GameData.h"
#include "f4se/GameSettings.h"
#include "f4se/GameReferences.h"

#include "f4se/PapyrusEvents.h"

#include "f4se_common/Relocation.h"
#include "f4se_common/BranchTrampoline.h"
#include "f4se_common/SafeWrite.h"
#include "xbyak/xbyak.h"

#include <shlobj.h>
#include <ctime>
#include <string>

#include "Pattern.h"

IDebugLog	gLog;
PluginHandle	g_pluginHandle = kPluginHandle_Invalid;
F4SEMessagingInterface		* g_messaging = nullptr;
UInt32 g_runtimeVersion = 0;

bool g_aprilFirst = false;

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

void F4SEMessageHandler(F4SEMessagingInterface::Message* msg)
{
	switch(msg->type)
	{
	case F4SEMessagingInterface::kMessage_GameDataReady:
		{
			bool isReady = reinterpret_cast<bool>(msg->data);
			if(isReady)
			{
				if(g_runtimeVersion == RUNTIME_VERSION_1_9_4 && g_aprilFirst)
				{
					// Extend the reload time
					Setting * setting = GetGameSetting("fPlayerDeathReloadTime");
					if(setting) {
						setting->data.f32 = 1.5;
					}
				}
			}
		}
		break;
	}
}

typedef void (* _PlayerKilled)(Actor * pc, Actor * killer);
_PlayerKilled PlayerKilled_Original = nullptr;

void PlayerKilled_Hook(Actor * pc, Actor * killer)
{
	PlayerKilled_Original(pc, killer);

	if(g_runtimeVersion == RUNTIME_VERSION_1_9_4 && g_aprilFirst) // All the code following this is version dependent
	{
		BSFixedString fileName("survival.bik");
		bool bInterruptible = false;
		bool bMuteAudio = false;
		bool bMuteMusic = false;
		bool bLetterBox = false;
		bool bIsNewGameBink = false;
		CallGlobalFunctionNoWait6<BSFixedString, bool, bool, bool, bool, bool>("Game", "PlayBink", fileName, bInterruptible, bMuteAudio, bMuteMusic, bLetterBox, bIsNewGameBink);

		bool abFadingOut = true;
		bool abBlackFade = true;
		float afSecsBeforeFade = 0;
		float afFadeDuration = 3.0;
		bool abStayFaded = true;
		CallGlobalFunctionNoWait5<bool, bool, float, float, bool>("Game", "FadeOutGame", abFadingOut, abBlackFade, afSecsBeforeFade, afFadeDuration, abStayFaded);
	}
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

	g_runtimeVersion = f4se->runtimeVersion;

	if(f4se->isEditor)
	{
		_FATALERROR("loaded in editor, marking as incompatible");
		return false;
	}

	g_messaging = (F4SEMessagingInterface *)f4se->QueryInterface(kInterface_Messaging);
	if(!g_messaging)
	{
		_FATALERROR("couldn't get messaging interface");
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
	uintptr_t * playerDeath = nullptr;
	uintptr_t * fastTravel = nullptr;

	bool allowSaving = true;
	bool returnSurvival = true;
	bool allowGodMode = true;
	bool allowConsole = true;
	bool allowFastTravel = true;

	bool ignoreAprilFirst = false;
	bool aprilFirst = false;
	F4SUGetConfigValue("Features", "bAllowFastTravel", &allowFastTravel);
	F4SUGetConfigValue("Features", "bAllowSaving", &allowSaving);
	F4SUGetConfigValue("Features", "bAllowReturnToSurvival", &returnSurvival);
	F4SUGetConfigValue("Features", "bAllowGodMode", &allowGodMode);
	F4SUGetConfigValue("Features", "bAllowConsole", &allowConsole);
	F4SUGetConfigValue("Features", "bApril1st", &aprilFirst);
	F4SUGetConfigValue("Features", "bIgnoreApril1st", &ignoreAprilFirst);

	if(!ignoreAprilFirst)
	{
		time_t rawtime = time(NULL);
		struct tm timeinfo;
		localtime_s(&timeinfo, &rawtime);
		if((timeinfo.tm_mon == 3 && timeinfo.tm_mday == 1) || aprilFirst)
		{
			g_aprilFirst = true;
		}
	}

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
		if(allowFastTravel) {
			fastTravel = Utility::pattern( "83 F8 06 0F 85 ? ? ? ? 83 FB FF").count( 1 ).get( 0 ).get<uintptr_t>();
			if(!fastTravel)
				_MESSAGE("Failed to find pattern for fast travel");
		}
		
		playerDeath = Utility::pattern( "48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 50 48 8B F2 33 D2" ).count( 1 ).get( 0 ).get<uintptr_t>();
		if(!playerDeath)
			_MESSAGE("Failed to find pattern for player death");

		
	}
	
	if(pauseSave && quickSave)
	{
		SafeWrite8(((uintptr_t)pauseSave) + 2, 0x07);	// Changes cmp 6 to cmp 7
		SafeWrite8(((uintptr_t)quickSave) + 2, 0x07);	// Changes cmp 6 to cmp 7

		_MESSAGE("Enabled saving in Survival difficulty");
	}
	if(fastTravel)
	{
		SafeWrite8(((uintptr_t)fastTravel) + 2, 0x07);	// Changes cmp 6 to cmp 7

		_MESSAGE("Enabled fast travel in Survival difficulty");
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

	
	if(!g_branchTrampoline.Create(1024 * 64))
	{
		_ERROR("couldn't create branch trampoline. this is fatal. skipping remainder of init process.");
		return false;
	}

	if(!g_localTrampoline.Create(1024 * 64, nullptr))
	{
		_ERROR("couldn't create codegen buffer. this is fatal. skipping remainder of init process.");
		return false;
	}

	if(g_aprilFirst) // Hook player death code to play bik
	{
		struct PlayerKilled_Code : Xbyak::CodeGenerator {
			PlayerKilled_Code(void * buf, uintptr_t playerDeath) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label retnLabel;

				mov(ptr[rsp+0x18], rbp);

				jmp(ptr [rip + retnLabel]);

				L(retnLabel);
				dq(playerDeath + 5);
			}
		};

		void * codeBuf = g_localTrampoline.StartAlloc();
		PlayerKilled_Code code(codeBuf, (uintptr_t)playerDeath);
		g_localTrampoline.EndAlloc(code.getCurr());

		PlayerKilled_Original = (_PlayerKilled)codeBuf;

		g_branchTrampoline.Write5Branch((uintptr_t)playerDeath, (uintptr_t)PlayerKilled_Hook);
	}

	if(g_messaging)
		g_messaging->RegisterListener(g_pluginHandle, "F4SE", F4SEMessageHandler);

	return true;
}

};
