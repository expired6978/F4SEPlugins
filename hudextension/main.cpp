#include "f4se/PluginAPI.h"
#include "f4se_common/f4se_version.h"

#include <shlobj.h>

#include "f4se/ScaleformCallbacks.h"
#include "f4se/ScaleformValue.h"

#include "f4se_common/Relocation.h"
#include "f4se_common/BranchTrampoline.h"
#include "f4se_common/SafeWrite.h"
#include "xbyak/xbyak.h"

#include "common/ICriticalSection.h"

#include "f4se/GameData.h"
#include "f4se/GameReferences.h"
#include "HUDExtension.h"

IDebugLog	gLog;

PluginHandle	g_pluginHandle = kPluginHandle_Invalid;

F4SEScaleformInterface		* g_scaleform = nullptr;
F4SEMessagingInterface		* g_messaging = nullptr;

UInt32 g_f4seVersion;

const std::string & F4HEGetRuntimeDirectory(void)
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

const std::string & F4HEGetConfigPath(void)
{
	static std::string s_configPath;

	if(s_configPath.empty())
	{
		std::string	runtimePath = F4HEGetRuntimeDirectory();
		if(!runtimePath.empty())
		{
			s_configPath = runtimePath + "Data\\F4SE\\Plugins\\hudextension.ini";

			_MESSAGE("config path = %s", s_configPath.c_str());
		}
	}

	return s_configPath;
}

std::string F4HEGetConfigOption(const char * section, const char * key)
{
	std::string	result;

	const std::string & configPath = F4HEGetConfigPath();
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
const char * F4HEGetTypeFormatting(T * dataOut)
{
	return false;
}



template<> const char * F4HEGetTypeFormatting(float * dataOut) { return "%f"; }
template<> const char * F4HEGetTypeFormatting(bool * dataOut) { return "%d"; }
template<> const char * F4HEGetTypeFormatting(SInt32 * dataOut) { return "%d"; }
template<> const char * F4HEGetTypeFormatting(UInt32 * dataOut) { return "%d"; }
template<> const char * F4HEGetTypeFormatting(UInt64 * dataOut) { return "%I64u"; }

template<typename T>
bool F4HEGetConfigValue(const char * section, const char * key, T * dataOut)
{
	std::string	data = F4HEGetConfigOption(section, key);
	if (data.empty())
		return false;

	T tmp;
	bool res = (sscanf_s(data.c_str(), F4HEGetTypeFormatting(dataOut), &tmp) == 1);
	if(res) {
		*dataOut = tmp;
	}
	return res;
}

template<>
bool F4HEGetConfigValue(const char * section, const char * key, bool * dataOut)
{
	std::string	data = F4HEGetConfigOption(section, key);
	if (data.empty())
		return false;

	UInt32 tmp;
	bool res = (sscanf_s(data.c_str(), F4HEGetTypeFormatting(&tmp), &tmp) == 1);
	if(res) {
		*dataOut = (tmp > 0);
	}
	return res;
}

class CombatEventSink : public BSTEventSink<TESCombatEvent>
{
public:
	virtual	EventResult	ReceiveEvent(TESCombatEvent * evn, void * dispatcher)
	{
		if(evn->source == *g_player)
			return kEvent_Continue;

		BSFixedString menuName(HUDExtensionMenu::sMenuName);
		if((*g_ui)->IsMenuOpen(menuName))
		{
			HUDExtensionMenu * menu = static_cast<HUDExtensionMenu*>((*g_ui)->GetMenu(menuName));
			if(menu)
			{
				if (evn->state != 0)
					menu->AddNameplate(evn->source);
				else
					menu->RemoveNameplate(evn->source);
			}
		}

		
		return kEvent_Continue;
	};
};

CombatEventSink g_combatSink;

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

static IMenu * CreateHUDExtensionMenu()
{
	return new HUDExtensionMenu();
}

class HUDExtensionOpenCloseHandler : public BSTEventSink<MenuOpenCloseEvent>
{
public:
	virtual ~HUDExtensionOpenCloseHandler() { };
	virtual	EventResult	ReceiveEvent(MenuOpenCloseEvent * evn, void * dispatcher) override
	{
		static BSFixedString HUDMenu("HUDMenu");
		static BSFixedString faderMenu("FaderMenu");
		static BSFixedString menuName(HUDExtensionMenu::sMenuName);
		if (evn->menuName == HUDMenu)
		{
			if (evn->isOpen)
				HUDExtensionMenu::OpenMenu();
			else
				HUDExtensionMenu::CloseMenu();
		}
		if (!evn->isOpen && evn->menuName == faderMenu && (*g_ui) != nullptr && (*g_ui)->IsMenuOpen(HUDMenu) && !(*g_ui)->IsMenuOpen(menuName))
		{
			HUDExtensionMenu::OpenMenu();
		}

		return kEvent_Continue;
	};
};

HUDExtensionOpenCloseHandler g_openCloseHandler;

void F4SEMessageHandler(F4SEMessagingInterface::Message* msg)
{
	switch(msg->type)
	{
	case F4SEMessagingInterface::kMessage_GameDataReady:
		{
			if(msg->data)
			{
				if(!g_hudSettings.avHealth)
				{
					TESForm * form = LookupFormByID(0x2D4);
					if(form && form->formType == ActorValueInfo::kTypeID)
						g_hudSettings.avHealth = (ActorValueInfo *)form; // You'd have to be a dumbass to change the ID of this
				}
				if(!g_hudSettings.avRomanced)
				{
					TESForm * form = LookupFormByID(0x148DF6);
					if(form && form->formType == ActorValueInfo::kTypeID)
						g_hudSettings.avRomanced = (ActorValueInfo *)form;
				}
				if(!g_hudSettings.avInvisibility)
				{
					TESForm * form = LookupFormByID(0x2F3);
					if(form && form->formType == ActorValueInfo::kTypeID)
						g_hudSettings.avInvisibility = (ActorValueInfo *)form;
				}

				std::string	data = F4HEGetConfigOption("Colors", "avUserDefined");
				if(data.length() > 0) {
					TESForm * form = GetFormFromIdentifier(data);
					if(form && form->formType == ActorValueInfo::kTypeID)
						g_hudSettings.avUserDefined = (ActorValueInfo *)form;
				}

				std::string	flags = F4HEGetConfigOption("General", "avFlags");
				if(flags.length() > 0) {
					TESForm * form = GetFormFromIdentifier(flags);
					if(form && form->formType == ActorValueInfo::kTypeID)
						g_hudSettings.avFlags = (ActorValueInfo *)form;
				}

				BSFixedString menuName(HUDExtensionMenu::sMenuName);
				if ((*g_ui) != nullptr && !(*g_ui)->menuTable.Find(&menuName))
				{
					(*g_ui)->Register("HUDExtensionMenu", CreateHUDExtensionMenu);
				}

				GetEventDispatcher<TESCombatEvent>()->AddEventSink(&g_combatSink);
				(*g_ui)->menuOpenCloseEventSource.AddEventSink(&g_openCloseHandler);
			}
		}
		break;
	case F4SEMessagingInterface::kMessage_PostLoadGame:
		{
			
		}
		break;
	}
}

extern "C"
{

bool F4SEPlugin_Query(const F4SEInterface * f4se, PluginInfo * info)
{
	SInt32	logLevel = IDebugLog::kLevel_DebugMessage;
	if (F4HEGetConfigValue("Debug", "iLogLevel", &logLevel))
		gLog.SetLogLevel((IDebugLog::LogLevel)logLevel);

	gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Fallout4\\F4SE\\hudextension.log");

	g_f4seVersion = f4se->f4seVersion;

	// populate info structure
	info->infoVersion =	PluginInfo::kInfoVersion;
	info->name =		"F4HE";
	info->version =		1;

	// store plugin handle so we can identify ourselves later
	g_pluginHandle = f4se->GetPluginHandle();

	if(f4se->isEditor)
	{
		_FATALERROR("loaded in editor, marking as incompatible");
		return false;
	}
	else if(f4se->runtimeVersion != RUNTIME_VERSION_1_10_82)
	{
		UInt32 runtimeVersion = RUNTIME_VERSION_1_10_82;
		char buf[512];
		sprintf_s(buf, "HUD Extension Version Error:\nexpected game version %d.%d.%d.%d\nyour game version is %d.%d.%d.%d\nsome features may not work correctly.", 
			GET_EXE_VERSION_MAJOR(runtimeVersion), 
			GET_EXE_VERSION_MINOR(runtimeVersion), 
			GET_EXE_VERSION_BUILD(runtimeVersion), 
			GET_EXE_VERSION_SUB(runtimeVersion), 
			GET_EXE_VERSION_MAJOR(f4se->runtimeVersion), 
			GET_EXE_VERSION_MINOR(f4se->runtimeVersion), 
			GET_EXE_VERSION_BUILD(f4se->runtimeVersion), 
			GET_EXE_VERSION_SUB(f4se->runtimeVersion));
		MessageBox(NULL, buf, "Game Version Error", MB_OK | MB_ICONEXCLAMATION);
		_FATALERROR("unsupported runtime version %08X", f4se->runtimeVersion);
		return false;
	}

	// get the papyrus interface and query its version
	g_scaleform = (F4SEScaleformInterface *)f4se->QueryInterface(kInterface_Scaleform);
	if (!g_scaleform)
	{
		_FATALERROR("couldn't get scaleform interface");
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

bool F4SEPlugin_Load(const F4SEInterface * skse)
{
	F4HEGetConfigValue("General", "uFlags", &g_hudSettings.barFlags);
	F4HEGetConfigValue("General", "fScaleFactor", &g_hudSettings.fScaleFactor);
	F4HEGetConfigValue("General", "fMaxPercent", &g_hudSettings.fMaxPercent);
	F4HEGetConfigValue("Colors", "clrLover", &g_hudSettings.colorLover);
	F4HEGetConfigValue("Colors", "clrTeammate", &g_hudSettings.colorTeammate);
	F4HEGetConfigValue("Colors", "clrNonHostile", &g_hudSettings.colorNonHostile);
	F4HEGetConfigValue("Colors", "clrEnemy", &g_hudSettings.colorEnemy);

	if(g_messaging)
		g_messaging->RegisterListener(g_pluginHandle, "F4SE", F4SEMessageHandler);

	return true;
}

};
