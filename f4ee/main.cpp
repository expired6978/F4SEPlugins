#include "f4se/PluginAPI.h"
#include "f4se_common/f4se_version.h"

#include "f4se/GameMenus.h"
#include "f4se/GameData.h"
#include "f4se/GameReferences.h"
#include "f4se/GameRTTI.h"

#include "f4se/ScaleformCallbacks.h"
#include "f4se/BSGraphics.h"

#include "CharGenInterface.h"
#include "BodyMorphInterface.h"
#include "BodyGenInterface.h"
#include "OverlayInterface.h"
#include "Utilities.h"

#include "PapyrusBodyGen.h"
#include "PapyrusOverlays.h"
#include "ScaleformNatives.h"

#include <shlobj.h>

#include "f4se_common/Relocation.h"
#include "f4se_common/BranchTrampoline.h"
#include "f4se_common/SafeWrite.h"
#include "xbyak/xbyak.h"

#include "common/ICriticalSection.h"

#include "f4se/NiMaterials.h"

CharGenInterface g_charGenInterface;
BodyGenInterface g_bodyGenInterface;
BodyMorphInterface g_bodyMorphInterface;
OverlayInterface g_overlayInterface;
StringTable g_stringTable;

IDebugLog	gLog;

PluginHandle	g_pluginHandle = kPluginHandle_Invalid;

F4SEScaleformInterface		* g_scaleform = nullptr;
F4SEMessagingInterface		* g_messaging = nullptr;
F4SEPapyrusInterface		* g_papyrus = nullptr;
F4SESerializationInterface	* g_serialization = nullptr;
F4SETaskInterface			* g_task = nullptr;
ICriticalSection	s_loadLock;

UInt32 g_f4seVersion;

bool g_bExportRace = false;
bool g_bEnableBodygen = false;
bool g_bEnableBodyMorphs = false;
bool g_bEnableOverlays = false;
bool g_bParallelShapes = false;
bool g_bEnableTintExtensions = true;
bool g_bIgnoreTintPalettes = false;
bool g_bIgnoreTintTextures = false;
bool g_bIgnoreTintMasks = false;

bool g_bEnableModelPreprocessor = true;

bool g_bUnlockHeadParts = false;
bool g_bUnlockTints = false;
bool g_bExtendedLUTs = true;

std::string g_strExportRace = "HumanRace";

UInt32 g_uExportIdMin = 0;
UInt32 g_uExportIdMax = 0xFFFF;

UInt32 g_tintMaskHeight = 1024;
UInt32 g_tintMaskWidth = 1024;

const std::string & F4EEGetRuntimeDirectory(void)
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

const std::string & F4EEGetConfigPath(bool custom = false)
{
	static std::string s_configPath;
	static std::string s_configPathCustom;

	if(s_configPath.empty())
	{
		std::string	runtimePath = F4EEGetRuntimeDirectory();
		if(!runtimePath.empty())
		{
			s_configPath = runtimePath + "Data\\F4SE\\Plugins\\f4ee.ini";

			_MESSAGE("default config path = %s", s_configPath.c_str());
		}
	}
	if(s_configPathCustom.empty())
	{
		std::string	runtimePath = F4EEGetRuntimeDirectory();
		if(!runtimePath.empty())
		{
			s_configPathCustom = runtimePath + "Data\\F4SE\\Plugins\\f4ee_custom.ini";

			_MESSAGE("custom config path = %s", s_configPathCustom.c_str());
		}
	}

	return custom ? s_configPathCustom : s_configPath;
}

std::string F4EEGetConfigOption(const char * section, const char * key)
{
	std::string	result;

	const std::string & configPath = F4EEGetConfigPath();
	const std::string & configPathCustom = F4EEGetConfigPath(true);

	char	resultBuf[256];
	resultBuf[0] = 0;

	if(!configPath.empty())
	{
		UInt32	resultLen = GetPrivateProfileString(section, key, NULL, resultBuf, sizeof(resultBuf), configPath.c_str());
		result = resultBuf;
	}
	if(!configPathCustom.empty())
	{
		UInt32	resultLen = GetPrivateProfileString(section, key, NULL, resultBuf, sizeof(resultBuf), configPathCustom.c_str());
		if(resultLen > 0) // Only take custom if we have it
			result = resultBuf;
	}

	return result;
}

template<typename T>
const char * F4EEGetTypeFormatting(T * dataOut)
{
	return false;
}

template<> const char * F4EEGetTypeFormatting(float * dataOut) { return "%f"; }
template<> const char * F4EEGetTypeFormatting(bool * dataOut) { return "%d"; }
template<> const char * F4EEGetTypeFormatting(SInt32 * dataOut) { return "%d"; }
template<> const char * F4EEGetTypeFormatting(UInt32 * dataOut) { return "%d"; }
template<> const char * F4EEGetTypeFormatting(UInt64 * dataOut) { return "%I64u"; }

template<typename T>
bool F4EEGetConfigValue(const char * section, const char * key, T * dataOut)
{
	std::string	data = F4EEGetConfigOption(section, key);
	if (data.empty())
		return false;

	T tmp;
	bool res = (sscanf_s(data.c_str(), F4EEGetTypeFormatting(dataOut), &tmp) == 1);
	if(res) {
		*dataOut = tmp;
	}
	return res;
}

template<>
bool F4EEGetConfigValue(const char * section, const char * key, bool * dataOut)
{
	std::string	data = F4EEGetConfigOption(section, key);
	if (data.empty())
		return false;

	UInt32 tmp;
	bool res = (sscanf_s(data.c_str(), F4EEGetTypeFormatting(&tmp), &tmp) == 1);
	if(res) {
		*dataOut = (tmp > 0);
	}
	return res;
}

void F4SEMessageHandler(F4SEMessagingInterface::Message* msg)
{
	switch(msg->type)
	{
	case F4SEMessagingInterface::kMessage_PreLoadGame:
		{
			if(g_bEnableBodyMorphs)
				g_bodyMorphInterface.SetLoading(true);
			if(g_bEnableOverlays)
				g_overlayInterface.SetLoading(true);
		}
		break;
	case F4SEMessagingInterface::kMessage_GameLoaded:
		{
			if(g_bEnableModelPreprocessor)
				g_bodyMorphInterface.SetModelProcessor();

			if(g_bEnableBodygen) {
				GetEventDispatcher<TESInitScriptEvent>()->AddEventSink(&g_bodyMorphInterface);
			}

			if(g_bEnableBodyMorphs) {
				GetEventDispatcher<TESLoadGameEvent>()->AddEventSink(&g_bodyMorphInterface);
				GetEventDispatcher<TESObjectLoadedEvent>()->AddEventSink(&g_bodyMorphInterface);
			}

			if(g_bEnableOverlays) {
				GetEventDispatcher<TESLoadGameEvent>()->AddEventSink(&g_overlayInterface);
				GetEventDispatcher<TESObjectLoadedEvent>()->AddEventSink(&g_overlayInterface);
			}
		}
		break;
	case F4SEMessagingInterface::kMessage_GameDataReady:
		{
			s_loadLock.Enter();

			bool isReady = reinterpret_cast<bool>(msg->data);
			if(isReady)
			{
				if(g_bEnableTintExtensions) {
					g_charGenInterface.LoadTintTemplateMods();
				}
				if(g_bEnableBodyMorphs) {
					g_bodyMorphInterface.LoadBodyGenSliderMods();
				}
				if(g_bEnableBodygen) {
					g_bodyGenInterface.LoadBodyGenMods();
				}
				if(g_bEnableOverlays) {
					g_overlayInterface.LoadOverlayMods();
				}
				if(g_bExtendedLUTs) {
					g_charGenInterface.LoadHairColorMods();
				}
				if(g_bUnlockHeadParts) {
					g_charGenInterface.UnlockHeadParts();
				}
				if(g_bUnlockTints) {
					g_charGenInterface.UnlockTints();
				}
			}
			else
			{
				if(g_bEnableBodyMorphs) {
					g_bodyMorphInterface.ClearBodyGenSliders();
				}
				if(g_bEnableBodygen) {
					g_bodyGenInterface.ClearBodyGenMods();
				}
				if(g_bEnableOverlays) {
					g_overlayInterface.ClearMods();
				}
				if(g_bExtendedLUTs) {
					g_charGenInterface.ClearHairColorMods();
				}
			}

			s_loadLock.Leave();
		}
		break;
	}
}

void F4EESerialization_Revert(const F4SESerializationInterface * intfc)
{
	g_bodyMorphInterface.Revert();
	g_overlayInterface.Revert();
	g_stringTable.Revert();
}


void F4EESerialization_Save(const F4SESerializationInterface * intfc)
{
	g_stringTable.Save(intfc, StringTable::kSerializationVersion);
	g_bodyMorphInterface.Save(intfc, BodyMorphInterface::kSerializationVersion);
	g_overlayInterface.Save(intfc, OverlayInterface::kSerializationVersion);
}

void F4EESerialization_Load(const F4SESerializationInterface * intfc)
{
	UInt32 type, length, version;
	bool error = false;

	std::unordered_map<UInt32, StringTableItem> stringTable;
	while (intfc->GetNextRecordInfo(&type, &version, &length))
	{
		switch (type)
		{
			case 'STTB':	g_stringTable.Load(intfc, version, stringTable);		break;
			case 'MRPH':	g_bodyMorphInterface.Load(intfc, true, version, stringTable);		break;	// Female Morphs
			case 'MRPM':	g_bodyMorphInterface.Load(intfc, false, version, stringTable);		break;	// Male Morphs
			case 'OVRL':	g_overlayInterface.Load(intfc, version, stringTable);			break;	// Female Overlays
			default:
				_MESSAGE("unhandled type %08X (%.4s)", type, &type);
				error = true;
				break;
		}
	}

	g_bodyMorphInterface.ResolvePendingMorphs();
}

extern "C"
{

bool F4SEPlugin_Query(const F4SEInterface * f4se, PluginInfo * info)
{
	SInt32	logLevel = IDebugLog::kLevel_DebugMessage;
	if (F4EEGetConfigValue("Debug", "iLogLevel", &logLevel))
		gLog.SetLogLevel((IDebugLog::LogLevel)logLevel);

	if (logLevel >= 0)
		gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Fallout4\\F4SE\\f4ee.log");
	_DMESSAGE("f4ee");

	g_f4seVersion = f4se->f4seVersion;

	// populate info structure
	info->infoVersion =	PluginInfo::kInfoVersion;
	info->name =		"F4EE";
	info->version =		1;

	// store plugin handle so we can identify ourselves later
	g_pluginHandle = f4se->GetPluginHandle();

	if(f4se->isEditor)
	{
		_FATALERROR("loaded in editor, marking as incompatible");
		return false;
	}
	else if(f4se->runtimeVersion != RUNTIME_VERSION_1_9_4)
	{
		UInt32 runtimeVersion = RUNTIME_VERSION_1_9_4;
		char buf[512];
		sprintf_s(buf, "LooksMenu Version Error:\nexpected game version %d.%d.%d.%d\nyour game version is %d.%d.%d.%d\nsome features may not work correctly.", 
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

	g_papyrus = (F4SEPapyrusInterface *)f4se->QueryInterface(kInterface_Papyrus);
	if(!g_papyrus)
	{
		_WARNING("couldn't get papyrus interface");
	}

	g_serialization = (F4SESerializationInterface *)f4se->QueryInterface(kInterface_Serialization);
	if(!g_serialization)
	{
		_FATALERROR("couldn't get serialization interface");
		return false;
	}

	g_task = (F4SETaskInterface *)f4se->QueryInterface(kInterface_Task);
	if(!g_task)
	{
		_WARNING("couldn't get task interface");
	}

	// supported runtime version
	return true;
}

bool ScaleformCallback(GFxMovieView * view, GFxValue * value)
{
	RegisterFunction<F4EEScaleform_LoadPreset>(value, view->movieRoot, "LoadPreset");
	RegisterFunction<F4EEScaleform_ReadPreset>(value, view->movieRoot, "ReadPreset");
	RegisterFunction<F4EEScaleform_SavePreset>(value, view->movieRoot, "SavePreset");
	RegisterFunction<F4EEScaleform_SetCurrentBoneRegionID>(value, view->movieRoot, "SetCurrentBoneRegionID");
	RegisterFunction<F4EEScaleform_AllowTextInput>(value, view->movieRoot, "AllowTextInput");
	RegisterFunction<F4EEScaleform_GetExternalFiles>(value, view->movieRoot, "GetExternalFiles");
	RegisterFunction<F4EEScaleform_GetBodySliders>(value, view->movieRoot, "GetBodySliders");
	RegisterFunction<F4EEScaleform_SetBodyMorph>(value, view->movieRoot, "SetBodyMorph");
	RegisterFunction<F4EEScaleform_UpdateBodyMorphs>(value, view->movieRoot, "UpdateBodyMorphs");
	RegisterFunction<F4EEScaleform_CloneBodyMorphs>(value, view->movieRoot, "CloneBodyMorphs");

	RegisterFunction<F4EEScaleform_GetOverlays>(value, view->movieRoot, "GetOverlays");
	RegisterFunction<F4EEScaleform_GetOverlayTemplates>(value, view->movieRoot, "GetOverlayTemplates");
	RegisterFunction<F4EEScaleform_CreateOverlay>(value, view->movieRoot, "CreateOverlay");
	RegisterFunction<F4EEScaleform_DeleteOverlay>(value, view->movieRoot, "DeleteOverlay");
	RegisterFunction<F4EEScaleform_SetOverlayData>(value, view->movieRoot, "SetOverlayData");
	RegisterFunction<F4EEScaleform_ReorderOverlay>(value, view->movieRoot, "ReorderOverlay");
	RegisterFunction<F4EEScaleform_UpdateOverlays>(value, view->movieRoot, "UpdateOverlays");
	RegisterFunction<F4EEScaleform_CloneOverlays>(value, view->movieRoot, "CloneOverlays");

	GFxValue dispatchEvent;
	GFxValue eventArgs[3];
	view->movieRoot->CreateString(&eventArgs[0], "F4EE::Initialized");
	eventArgs[1] = GFxValue(true);
	eventArgs[2] = GFxValue(false);
	view->movieRoot->CreateObject(&dispatchEvent, "flash.events.Event", eventArgs, 3);
	view->movieRoot->Invoke("root.dispatchEvent", nullptr, &dispatchEvent, 1);

	return true;
}

typedef UInt32 (* _InstallArmorAddon)(void * unk1, UInt32 unk2, TESForm * form);
RelocAddr <_InstallArmorAddon> InstallArmorAddon_Original(0x001C23A0);
RelocAddr <uintptr_t> InstallArmorAddon_Start(0x001BCF00 + 0xC27);

typedef void (* _ApplyMaterialProperties)(NiAVObject * object);
RelocAddr <_ApplyMaterialProperties> ApplyMaterialProperties(0x027D1B40);
RelocAddr <uintptr_t> HairColorModify_Start(0x00689D30 + 0x24A);

RelocAddr <uintptr_t> GetHairTexturePath_Start(0x00687920 + 0xDED);
//RelocAddr <uintptr_t> GetActorBaseHeadData_Start(0x00687920 + 0xCF);

RelocPtr<UInt32> g_faceGenTextureWidth(0x037DFAD0);
RelocPtr<UInt32> g_faceGenTextureHeight(0x037DFAD4);

typedef void (* _InitializeSharedTarget)(BSRenderTargetManager * targetManager, UInt32 type, BSRenderTargetManager::SharedTargetInfo * targetInfo, UInt8 unk1);
RelocAddr <_InitializeSharedTarget> InitializeSharedTarget(0x01D08B90);
_InitializeSharedTarget InitializeSharedTarget_Original = nullptr;

void ApplyMaterialProperties_Hook(NiAVObject * node, BGSColorForm * colorForm, BSLightingShaderMaterialBase * shaderMaterial)
{
	if(shaderMaterial)
		shaderMaterial->fLookupScale = colorForm->color.remappingIndex;

	g_charGenInterface.ProcessHairColor(node, colorForm, shaderMaterial);
	
	ApplyMaterialProperties(node);
}

const char * GetHairTexturePath_Hook(TESNPC * npc)
{
	return g_charGenInterface.ProcessEyebrowPath(npc);
}

void InstallArmorAddon(TESForm * form, NiAVObject * object, UInt32 slotIndex)
{
	Actor * actor = DYNAMIC_CAST(form, TESForm, Actor);
	if(actor) {
		if(g_bEnableBodyMorphs)
			g_bodyMorphInterface.ApplyMorphsToShapes(actor, object);

		if(g_bEnableOverlays) {
			NiNode * rootNode = GetRootNode(actor, object);
			if(rootNode)
				g_overlayInterface.UpdateOverlays(actor, rootNode, object, slotIndex);
		}
	}
}

/*
UInt32 InstallArmorAddon_Hook(void * unk1, UInt32 unk2, TESForm * form, NiAVObject * object)
{
	UInt32 ret = InstallArmorAddon_Original(unk1, unk2, form);

	Actor * actor = DYNAMIC_CAST(form, TESForm, Actor);
	if(actor) {
		g_bodyMorphInterface.ApplyMorphsToShapes(actor, object);
	}

	return ret;
}
*/

void InitializeSharedTarget_Hook(BSRenderTargetManager * targetManager, UInt32 type, BSRenderTargetManager::SharedTargetInfo * targetInfo, UInt8 unk1)
{
	switch(type)
	{
	case 16:
	case 17:
	case 18:
		targetInfo->width = g_tintMaskWidth;
		targetInfo->height = g_tintMaskHeight;
		break;
	}
	InitializeSharedTarget_Original(targetManager, type, targetInfo, unk1);	
}

bool RegisterFuncs(VirtualMachine * vm)
{	
	papyrusBodyGen::RegisterFuncs(vm);
	papyrusOverlays::RegisterFuncs(vm);
	return true;
}

bool F4SEPlugin_Load(const F4SEInterface * skse)
{
	F4EEGetConfigValue("Debug", "bExportRace", &g_bExportRace);
	std::string strExportRace = F4EEGetConfigOption("Debug", "strExportRace");
	if(!strExportRace.empty())
	{
		g_strExportRace = strExportRace;
	}
	F4EEGetConfigValue("Debug", "uExportIdMin", &g_uExportIdMin);
	F4EEGetConfigValue("Debug", "uExportIdMax", &g_uExportIdMax);

	F4EEGetConfigValue("Global", "bEnableModelPreprocessor", &g_bEnableModelPreprocessor);

	F4EEGetConfigValue("Overlays", "bEnableOverlays", &g_bEnableOverlays);

	F4EEGetConfigValue("BodyMorph", "bEnable", &g_bEnableBodyMorphs);
	F4EEGetConfigValue("BodyMorph", "bEnableBodyGen", &g_bEnableBodygen);

	UInt64 uMaxCache;
	if(F4EEGetConfigValue("BodyMorph", "uMaxCache", &uMaxCache))
	{
		g_bodyMorphInterface.SetCacheLimit(uMaxCache);
	}
	F4EEGetConfigValue("BodyMorph", "bParallelShapes", &g_bParallelShapes);

	F4EEGetConfigValue("CharGen", "bEnableTintExtensions", &g_bEnableTintExtensions);
	F4EEGetConfigValue("CharGen", "bUnlockHeadParts", &g_bUnlockHeadParts);
	F4EEGetConfigValue("CharGen", "bUnlockTints", &g_bUnlockTints);
	F4EEGetConfigValue("CharGen", "bExtendedLUTs", &g_bExtendedLUTs);
	F4EEGetConfigValue("CharGen", "uTintWidth", &g_tintMaskWidth);
	F4EEGetConfigValue("CharGen", "uTintHeight", &g_tintMaskHeight);

	F4EEGetConfigValue("Presets", "bIgnoreTintPalettes", &g_bIgnoreTintPalettes);
	F4EEGetConfigValue("Presets", "bIgnoreTintTextures", &g_bIgnoreTintTextures);
	F4EEGetConfigValue("Presets", "bIgnoreTintPalettes", &g_bIgnoreTintMasks);

	*g_faceGenTextureWidth = g_tintMaskWidth;
	*g_faceGenTextureHeight = g_tintMaskHeight;

	if (g_scaleform)
		g_scaleform->Register("F4EE", ScaleformCallback);
	if(g_messaging)
		g_messaging->RegisterListener(g_pluginHandle, "F4SE", F4SEMessageHandler);
	if (g_papyrus)
		g_papyrus->Register(RegisterFuncs);
	if (g_serialization) {
		g_serialization->SetUniqueID(g_pluginHandle, 'F4EE');
		g_serialization->SetRevertCallback(g_pluginHandle, F4EESerialization_Revert);
		g_serialization->SetSaveCallback(g_pluginHandle, F4EESerialization_Save);
		g_serialization->SetLoadCallback(g_pluginHandle, F4EESerialization_Load);
		g_serialization->SetFormDeleteCallback(g_pluginHandle, nullptr);
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
	
	// hook Armor Attachment
	if(g_bEnableBodyMorphs || g_bEnableOverlays)
	{
		struct InstallArmorAddon_Code : Xbyak::CodeGenerator {
			InstallArmorAddon_Code(void * buf, UInt64 funcAddr) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label funcLabel;
				Xbyak::Label retnLabel;
				Xbyak::Label originLabel;

				call(ptr [rip + originLabel]);

				mov(r8d, ptr[rbp+0x1C0-0x208]); // Stack var to current index of equip item loop
				mov(rdx, r12);					// ObjectNode
				mov(rcx, ptr[rsp+0x2A8-0x268]); // Actor - rbx - ptr[rsp+0x2A8-0x268]
				call(ptr [rip + funcLabel]);

				jmp(ptr [rip + retnLabel]);

				L(funcLabel);
				dq(funcAddr);

				L(originLabel);
				dq(InstallArmorAddon_Original.GetUIntPtr());

				L(retnLabel);
				dq(InstallArmorAddon_Start.GetUIntPtr() + 5);
			}
		};
		/*struct InstallArmorAddon_Code : Xbyak::CodeGenerator {
			InstallArmorAddon_Code(void * buf, UInt64 funcAddr) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label funcLabel;
				Xbyak::Label retnLabel;

				mov(r9, r12);
				call(ptr [rip + funcLabel]);
				jmp(ptr [rip + retnLabel]);

				L(funcLabel);
				dq(funcAddr);

				L(retnLabel);
				dq(InstallArmorAddon_Start.GetUIntPtr() + 5);
			}
		};*/

		void * codeBuf = g_localTrampoline.StartAlloc();
		InstallArmorAddon_Code code(codeBuf, (uintptr_t)InstallArmorAddon);
		//InstallArmorAddon_Code code(codeBuf, (uintptr_t)InstallArmorAddon_Hook);
		g_localTrampoline.EndAlloc(code.getCurr());

		g_branchTrampoline.Write5Branch(InstallArmorAddon_Start.GetUIntPtr(), uintptr_t(code.getCode()));
	}

	// SetHairColor Palette Index
	if(g_bExtendedLUTs)
	{
		struct SetHairColorPalette_Code : Xbyak::CodeGenerator {
			SetHairColorPalette_Code(void * buf, UInt64 funcAddr) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label funcLabel;
				Xbyak::Label retnLabel;

				mov(r8, r13);
				mov(rdx, rax);
				mov(rcx, r12);
				call(ptr [rip + funcLabel]);
				jmp(ptr [rip + retnLabel]);

				L(funcLabel);
				dq(funcAddr);

				L(retnLabel);
				dq(HairColorModify_Start.GetUIntPtr() + 0x14);
			}
		};

		void * codeBuf = g_localTrampoline.StartAlloc();
		SetHairColorPalette_Code code(codeBuf, (uintptr_t)ApplyMaterialProperties_Hook);
		g_localTrampoline.EndAlloc(code.getCurr());

		g_branchTrampoline.Write6Branch(HairColorModify_Start.GetUIntPtr(), uintptr_t(code.getCode()));
	}

	// SetEyebrowLUTPath
	if(g_bExtendedLUTs)
	{
		struct GetHairTexturePath_Code : Xbyak::CodeGenerator {
			GetHairTexturePath_Code(void * buf, UInt64 funcAddr) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label funcLabel;
				Xbyak::Label retnLabel;

				// NPC pointer is already in rcx
				// mov     rcx, [rbp+1DE0h+npc2]
				call(ptr [rip + funcLabel]);
				jmp(ptr [rip + retnLabel]);

				L(funcLabel);
				dq(funcAddr);

				L(retnLabel);
				/* Skip this chunk, will be done in our own code, looking up of the hair texture and returning the C-string
				mov     rcx, [rbp+1DE0h+npc2]
				mov     rcx, [rcx+1B8h]
				add     rcx, 6C0h
				call    BSFixedString__GetCString
				*/
				dq(GetHairTexturePath_Start.GetUIntPtr() + 0x13);
			}
		};

		void * codeBuf = g_localTrampoline.StartAlloc();
		GetHairTexturePath_Code code(codeBuf, (uintptr_t)GetHairTexturePath_Hook);
		g_localTrampoline.EndAlloc(code.getCurr());

		g_branchTrampoline.Write6Branch(GetHairTexturePath_Start.GetUIntPtr(), uintptr_t(code.getCode()));
	}

	// Resizes tint mask target
	{
		struct InitializeSharedTarget_Code : Xbyak::CodeGenerator {
			InitializeSharedTarget_Code(void * buf) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label retnLabel;

				push(rsi);
				push(rdi);
				push(r14);

				jmp(ptr [rip + retnLabel]);

				L(retnLabel);
				dq(InitializeSharedTarget.GetUIntPtr() + 5);
			}
		};

		void * codeBuf = g_localTrampoline.StartAlloc();
		InitializeSharedTarget_Code code(codeBuf);
		g_localTrampoline.EndAlloc(code.getCurr());

		InitializeSharedTarget_Original = (_InitializeSharedTarget)codeBuf;

		g_branchTrampoline.Write5Branch(InitializeSharedTarget.GetUIntPtr(), (uintptr_t)InitializeSharedTarget_Hook);
	}
	
	return true;
}

};
