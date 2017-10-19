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

		if (evn->state != 0)
		{
			g_hudExtension.AddNameplate(evn->source);
		}
		else
		{
			g_hudExtension.RemoveNameplate(evn->source);
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
			}
		}
		break;
	case F4SEMessagingInterface::kMessage_PostLoadGame:
		{
			GetEventDispatcher<TESCombatEvent>()->AddEventSink(&g_combatSink);
		}
		break;
	}
}

typedef void (* _RegisterFloatingQuestMarkerBase)(HUDMenu * menu, const char * clipName);
RelocAddr <_RegisterFloatingQuestMarkerBase> RegisterFloatingQuestMarkerBase(0x0127D670);
_RegisterFloatingQuestMarkerBase RegisterFloatingQuestMarkerBase_Original = nullptr;

typedef HUDContextArray<BSFixedString> * (* _GetHUDContexts)();
RelocAddr<_GetHUDContexts> GetHUDContexts(0x00A4F710);

typedef void (* _HUDMenu_Update_Internal)(HUDMenu * menu);
RelocAddr<_HUDMenu_Update_Internal> HUDMenu_Update_Internal(0x01277060);
_HUDMenu_Update_Internal HUDMenu_Update_Original = nullptr;

typedef void (* _HUDMenu_Destroy_Internal)(HUDMenu * menu);
RelocAddr<_HUDMenu_Destroy_Internal> HUDMenu_Destroy_Internal(0x01276D50);
_HUDMenu_Destroy_Internal HUDMenu_Destroy_Original = nullptr;

HUDExtensionBase * pHUDExtension = nullptr;
ICriticalSection g_HUDLock;

void HUDMenu_Update_Hook(HUDMenu * menu)
{
	HUDMenu_Update_Original(menu);

	/*BSGFxDisplayObject * pComponent = menu->subcomponents.entries[menu->subcomponents.count-1];
	if(typeid(*pComponent) == typeid(HUDExtensionBase))
	{
		HUDExtensionBase * pHUDExtension = (HUDExtensionBase *)pComponent;
		pHUDExtension->UpdateComponent();
	}*/
	g_HUDLock.Enter();
	if(pHUDExtension) {
		pHUDExtension->UpdateComponent();
	}
	g_HUDLock.Leave();
}

void HUDMenu_Destroy_Hook(HUDMenu * menu)
{
	/*BSGFxDisplayObject * pComponent = menu->subcomponents.entries[menu->subcomponents.count-1];
	if(typeid(*pComponent) == typeid(HUDExtensionBase))
	{
		HUDExtensionBase * pHUDExtension = (HUDExtensionBase *)pComponent;
		menu->subcomponents.Remove(menu->subcomponents.count-1);
		delete pHUDExtension;
	}*/
	g_HUDLock.Enter();
	if(pHUDExtension) {
		delete pHUDExtension;
		pHUDExtension = nullptr;
	}
	g_HUDLock.Leave();

	HUDMenu_Destroy_Original(menu);
}

/*
void HUDMenu_Unk13_Hook(HUDMenu * menu)
{
	HUDMenu_Unk13_Internal(menu);


}
*/
void RegisterShaderTarget_Hook(HUDMenu * menu, const char * clipName)
{
	RegisterFloatingQuestMarkerBase_Original(menu, clipName);

	/*HUDContextArray<BSFixedString> * contexts = GetHUDContexts();
	HUDExtensionBase * pHUDExtension = new HUDExtensionBase(menu->shaderTarget, "HUDExtensionBase", contexts);
	menu->subcomponents.Push(pHUDExtension);*/

	HUDContextArray<BSFixedString> * contexts = GetHUDContexts();
	g_HUDLock.Enter();
	pHUDExtension = new HUDExtensionBase(menu->shaderTarget, "HUDExtensionBase", contexts);
	g_HUDLock.Leave();
}

bool ScaleformCallback(GFxMovieView * view, GFxValue * value)
{
	GFxMovieRoot * root = view->movieRoot;
	if(root)
	{
		GFxValue result;
		GFxValue stage;
		root->GetVariable(&stage, "stage");
		GFxValue firstChild;
		stage.Invoke("getChildAt", &firstChild, &GFxValue((SInt32)0), 1);
		root->Invoke("flash.utils.getQualifiedClassName", &result, &firstChild, 1);

		// Find the HUDMenu by class name
		if(result.IsString())
		{
			const char * clipName = result.GetString();
			if(strcmp("HUDMenu", clipName) == 0)
			{
				GFxValue loadArgs[2];
				GFxValue loader;
				root->CreateObject(&loader, "flash.display.Loader");

				// Create a container that will be later casted to a HUDComponent
				GFxValue gHUDExtensionBase;
				root->CreateObject(&gHUDExtensionBase, "flash.display.MovieClip");
				GFxValue gHUDExtensionInstanceName;
				root->CreateString(&gHUDExtensionInstanceName, "HUDExtensionBase");
				gHUDExtensionBase.SetMember("name", &gHUDExtensionInstanceName);

				// Add HUDExtensionBase to HUDMenu
				firstChild.Invoke("addChild", nullptr, &gHUDExtensionBase, 1);

				// Add the loader so when it finishes it will add the child object
				gHUDExtensionBase.Invoke("addChild", nullptr, &loader, 1);

				// Request loading of the extension
				root->CreateObject(&loadArgs[0], "flash.net.URLRequest", &GFxValue("HUDExtension.swf"), 1);
				loadArgs[1].SetNull();
				loader.Invoke("load", nullptr, loadArgs, 2);
			}
		}
	}

	return true;
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
	else if(f4se->runtimeVersion != RUNTIME_VERSION_1_10_26)
	{
		UInt32 runtimeVersion = RUNTIME_VERSION_1_10_26;
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


	if (g_scaleform)
		g_scaleform->Register("F4HE", ScaleformCallback);
	if(g_messaging)
		g_messaging->RegisterListener(g_pluginHandle, "F4SE", F4SEMessageHandler);

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

	//g_branchTrampoline.Write5Call(RegisterFloatingQuestMarkerBase_Start.GetUIntPtr(), (uintptr_t)RegisterShaderTarget_Hook);

	{
		struct RegisterFloatingQuestMarkerBase_Code : Xbyak::CodeGenerator {
			RegisterFloatingQuestMarkerBase_Code(void * buf) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label retnLabel;

				mov(ptr[rsp+0x08], rbx);

				jmp(ptr [rip + retnLabel]);

				L(retnLabel);
				dq(RegisterFloatingQuestMarkerBase.GetUIntPtr() + 5);
			}
		};

		void * codeBuf = g_localTrampoline.StartAlloc();
		RegisterFloatingQuestMarkerBase_Code code(codeBuf);
		g_localTrampoline.EndAlloc(code.getCurr());

		RegisterFloatingQuestMarkerBase_Original = (_RegisterFloatingQuestMarkerBase)codeBuf;

		g_branchTrampoline.Write5Branch(RegisterFloatingQuestMarkerBase.GetUIntPtr(), (uintptr_t)RegisterShaderTarget_Hook);
	}

	{
		struct HUDMenu_Update_Code : Xbyak::CodeGenerator {
			HUDMenu_Update_Code(void * buf) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label retnLabel;

				mov(ptr[rsp+0x08], rbx);

				jmp(ptr [rip + retnLabel]);

				L(retnLabel);
				dq(HUDMenu_Update_Internal.GetUIntPtr() + 5);
			}
		};

		void * codeBuf = g_localTrampoline.StartAlloc();
		HUDMenu_Update_Code code(codeBuf);
		g_localTrampoline.EndAlloc(code.getCurr());

		HUDMenu_Update_Original = (_HUDMenu_Update_Internal)codeBuf;

		g_branchTrampoline.Write5Branch(HUDMenu_Update_Internal.GetUIntPtr(), (uintptr_t)HUDMenu_Update_Hook);
	}

	{
		struct HUDMenu_Destroy_Code : Xbyak::CodeGenerator {
			HUDMenu_Destroy_Code(void * buf) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label retnLabel;

				mov(ptr[rsp+0x10], rbx);

				jmp(ptr [rip + retnLabel]);

				L(retnLabel);
				dq(HUDMenu_Destroy_Internal.GetUIntPtr() + 5);
			}
		};

		void * codeBuf = g_localTrampoline.StartAlloc();
		HUDMenu_Destroy_Code code(codeBuf);
		g_localTrampoline.EndAlloc(code.getCurr());

		HUDMenu_Destroy_Original = (_HUDMenu_Destroy_Internal)codeBuf;

		g_branchTrampoline.Write5Branch(HUDMenu_Destroy_Internal.GetUIntPtr(), (uintptr_t)HUDMenu_Destroy_Hook);
	}

	//g_branchTrampoline.Write5Call(HUDMenu_Update_Start.GetUIntPtr(), (uintptr_t)HUDMenu_Update_Hook);

	//SafeWrite64(HUDMenu_Unk13_Start.GetUIntPtr(), (uintptr_t)HUDMenu_Unk13_Hook);
	
	return true;
}

};
