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
#include "TransformInterface.h"
#include "ActorUpdateManager.h"
#include "SkinInterface.h"
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
SkinInterface g_skinInterface;
#ifdef _TRANSFORMS
NiTransformInterface g_transformInterface;
#endif
ActorUpdateManager g_actorUpdateManager;

IDebugLog	gLog;

PluginHandle	g_pluginHandle = kPluginHandle_Invalid;

F4SEScaleformInterface		* g_scaleform = nullptr;
F4SEMessagingInterface		* g_messaging = nullptr;
F4SEPapyrusInterface		* g_papyrus = nullptr;
F4SESerializationInterface	* g_serialization = nullptr;
F4SETaskInterface			* g_task = nullptr;
F4SETrampolineInterface		* g_trampoline = nullptr;
ICriticalSection	s_loadLock;

UInt32 g_f4seVersion;

bool g_bExportRace = false;
bool g_bEnableBodygen = true;
bool g_bEnableBodyMorphs = true;
bool g_bEnableOverlays = true;
bool g_bEnableSkinOverrides = true;
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
			g_actorUpdateManager.SetLoading(true);
		}
		break;
	case F4SEMessagingInterface::kMessage_GameLoaded:
		{
			if(g_bEnableModelPreprocessor)
				g_bodyMorphInterface.SetModelProcessor();

#if _TRANSFORMS
			g_transformInterface.SetModelProcessor();
#endif

			GetEventDispatcher<TESInitScriptEvent>()->AddEventSink(&g_actorUpdateManager);
			GetEventDispatcher<TESLoadGameEvent>()->AddEventSink(&g_actorUpdateManager);
			GetEventDispatcher<TESObjectLoadedEvent>()->AddEventSink(&g_actorUpdateManager);
		}
		break;
	case F4SEMessagingInterface::kMessage_GameDataReady:
		{
			s_loadLock.Enter();

			bool isReady = static_cast<bool>(msg->data);
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
				if(g_bEnableSkinOverrides) {
					g_skinInterface.LoadSkinMods();
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
#ifdef _TRANSFORMS
				g_transformInterface.LoadAllSkeletons();
#endif
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
				if(g_bEnableSkinOverrides) {
					g_skinInterface.ClearMods();
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
	g_skinInterface.Revert();
	g_stringTable.Revert();
	g_actorUpdateManager.Revert();
}


void F4EESerialization_Save(const F4SESerializationInterface * intfc)
{
	g_stringTable.Save(intfc, StringTable::kSerializationVersion);
	g_bodyMorphInterface.Save(intfc, BodyMorphInterface::kSerializationVersion);
	g_overlayInterface.Save(intfc, OverlayInterface::kSerializationVersion);
	g_skinInterface.Save(intfc, SkinInterface::kSerializationVersion);
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
			case 'SOVR':	g_skinInterface.Load(intfc, version, stringTable);				break;
			default:
				_ERROR("unhandled type %08X (%.4s)", type, &type);
				error = true;
				break;
		}
	}

	g_actorUpdateManager.ResolvePendingBodyGen();
	g_actorUpdateManager.Flush(); // In case the load game came first for whatever reason
}


bool ScaleformCallback(GFxMovieView* view, GFxValue* value)
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

	RegisterFunction<F4EEScaleform_GetEquippedItems>(value, view->movieRoot, "GetEquippedItems");
	RegisterFunction<F4EEScaleform_UnequipItems>(value, view->movieRoot, "UnequipItems");
	RegisterFunction<F4EEScaleform_EquipItems>(value, view->movieRoot, "EquipItems");

	RegisterFunction<F4EEScaleform_GetSkinOverrides>(value, view->movieRoot, "GetSkinOverrides");
	RegisterFunction<F4EEScaleform_GetSkinOverride>(value, view->movieRoot, "GetSkinOverride");
	RegisterFunction<F4EEScaleform_SetSkinOverride>(value, view->movieRoot, "SetSkinOverride");
	RegisterFunction<F4EEScaleform_UpdateSkinOverride>(value, view->movieRoot, "UpdateSkinOverride");
	RegisterFunction<F4EEScaleform_CloneSkinOverride>(value, view->movieRoot, "CloneSkinOverride");

	RegisterFunction<F4EEScaleform_GetSkinColor>(value, view->movieRoot, "GetSkinColor");
	RegisterFunction<F4EEScaleform_SetSkinColor>(value, view->movieRoot, "SetSkinColor");
	RegisterFunction<F4EEScaleform_GetExtraColor>(value, view->movieRoot, "GetExtraColor");
	RegisterFunction<F4EEScaleform_SetExtraColor>(value, view->movieRoot, "SetExtraColor");


	GFxValue dispatchEvent;
	GFxValue eventArgs[3];
	view->movieRoot->CreateString(&eventArgs[0], "F4EE::Initialized");
	eventArgs[1] = GFxValue(true);
	eventArgs[2] = GFxValue(false);
	view->movieRoot->CreateObject(&dispatchEvent, "flash.events.Event", eventArgs, 3);
	view->movieRoot->Invoke("root.dispatchEvent", nullptr, &dispatchEvent, 1);

	return true;
}

using _AttachSkinnedObject = NiAVObject* (*)(BipedAnim* bipedInfo, NiNode* objectRoot, NiNode* parent, UInt32 bipedIndex, bool abFirstPerson);
RelocAddr< _AttachSkinnedObject> AttachSkinnedObject(BipedAnim::AttachSkinnedObject_Address);
_AttachSkinnedObject AttachSkinnedObject_Original = nullptr;

RelocPtr<UInt32> g_faceGenTextureWidth(0x02F42910); // "FaceCustomization"
RelocPtr<UInt32> g_faceGenTextureHeight(0x02F42914);

namespace BSGraphics
{
	using _CreateRenderTarget = void (*)(RenderTargetManager* targetManager, UInt32 type, RenderTargetManager::RenderTargetProperties& properties, UInt8 persistent);
	RelocAddr<_CreateRenderTarget> CreateRenderTarget(RenderTargetManager::CreateRenderTarget_Address);
	_CreateRenderTarget CreateRenderTarget_Original = nullptr;
}

void HairColorModify_Hook(NiAVObject* node, BGSColorForm* colorForm, BSLightingShaderMaterialBase* shaderMaterial)
{
	if (shaderMaterial)
		shaderMaterial->fLookupScale = colorForm->color.remappingIndex;

	g_charGenInterface.ProcessHairColor(node, colorForm, shaderMaterial);

	BSShaderutil::ClearRenderPasses(node);
}

const char* GetHairTexturePath_Hook(TESNPC* npc)
{
	return g_charGenInterface.ProcessEyebrowPath(npc);
}

NiAVObject* AttachSkinnedObject_Hooked(BipedAnim* bipedInfo, NiNode* objectRoot, NiNode* parentNode, UInt32 bipedIndex, bool abFirstPerson)
{
	NiAVObject* retVal = AttachSkinnedObject_Original(bipedInfo, objectRoot, parentNode, bipedIndex, abFirstPerson);

	NiPointer<TESObjectREFR> reference;
	BSPointerHandleManagerInterface<TESObjectREFR>::GetSmartPointer(reference, bipedInfo->actorRef);
	if (reference)
	{
		Actor* actor = DYNAMIC_CAST(reference, TESForm, Actor);
		if (actor)
		{
			if (retVal)
			{
				if (g_bEnableBodyMorphs)
					g_bodyMorphInterface.ApplyMorphsToShapes(actor, retVal);

				if (g_bEnableOverlays) {
					if (parentNode)
						g_overlayInterface.UpdateOverlays(actor, parentNode, retVal, bipedIndex);
				}
			}
		}
	}

	return retVal;
}

void CreateRenderTarget_Hook(BSGraphics::RenderTargetManager* targetManager, UInt32 type, BSGraphics::RenderTargetManager::RenderTargetProperties& properties, UInt8 persistent)
{
	switch (type)
	{
	case 16:
	case 17:
	case 18:
		properties.uiWidth = g_tintMaskWidth;
		properties.uiHeight = g_tintMaskHeight;
		break;
	}
	BSGraphics::CreateRenderTarget_Original(targetManager, type, properties, persistent);
}

bool RegisterFuncs(VirtualMachine* vm)
{
	papyrusBodyGen::RegisterFuncs(vm);
	papyrusOverlays::RegisterFuncs(vm);
	return true;
}

extern "C"
{
__declspec(dllexport) F4SEPluginVersionData F4SEPlugin_Version =
{
	F4SEPluginVersionData::kVersion,
	2,
	"Fallout 4 Engine Extender",
	"Expired6978",
	0,	// not version independent
	0,	// not version independent (extended field)
	{ RUNTIME_VERSION_1_11_191, 0 },	// compatible with 1.11.191
	0,	// works with any version of the script extender. you probably do not need to put anything here
};

bool F4SEPlugin_Preload(const F4SEInterface * f4se)
{
	SInt32	logLevel = IDebugLog::kLevel_DebugMessage;
	if (F4EEGetConfigValue("Debug", "iLogLevel", &logLevel))
		gLog.SetLogLevel((IDebugLog::LogLevel)logLevel);

	if (logLevel >= 0)
		gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Fallout4\\F4SE\\f4ee.log");
	_DMESSAGE("f4ee - log level %d", logLevel);

	g_f4seVersion = f4se->f4seVersion;

	// store plugin handle so we can identify ourselves later
	g_pluginHandle = f4se->GetPluginHandle();

	if(f4se->isEditor)
	{
		_FATALERROR("loaded in editor, marking as incompatible");
		return false;
	}
	else if(f4se->runtimeVersion != RUNTIME_VERSION_1_11_191)
	{
		UInt32 runtimeVersion = RUNTIME_VERSION_1_11_191;
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

	g_trampoline = (F4SETrampolineInterface*)f4se->QueryInterface(kInterface_Trampoline);
	if (!g_trampoline) {
		_ERROR("couldn't get trampoline interface");
	}

	// supported runtime version
	return true;
}

__declspec(dllexport) bool F4SEPlugin_Load(const F4SEInterface * f4se)
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

	F4EEGetConfigValue("Skin", "bEnableSkinOverrides", &g_bEnableSkinOverrides);

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

	if (g_trampoline) {
		void* branch = g_trampoline->AllocateFromBranchPool(g_pluginHandle, 512);
		if (!branch) {
			_ERROR("couldn't create branch trampoline. this is fatal. skipping remainder of init process.");
			return false;
		}

		g_branchTrampoline.SetBase(512, branch);

		void* local = g_trampoline->AllocateFromLocalPool(g_pluginHandle, 512);
		if (!local) {
			_ERROR("couldn't create codegen buffer. this is fatal. skipping remainder of init process.");
			return false;
		}

		g_localTrampoline.SetBase(512, local);
	}
	else {
		if (!g_branchTrampoline.Create(512)) {
			_ERROR("couldn't create branch trampoline. this is fatal. skipping remainder of init process.");
			return false;
		}
		if (!g_localTrampoline.Create(512, nullptr))
		{
			_ERROR("couldn't create codegen buffer. this is fatal. skipping remainder of init process.");
			return false;
		}
	}


	
	// hook Armor Attachment
	if(g_bEnableBodyMorphs || g_bEnableOverlays)
	{
		struct AttachSkinnedObjectHook_Entry_Code : Xbyak::CodeGenerator {
			AttachSkinnedObjectHook_Entry_Code(void * buf, uintptr_t address) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label funcLabel;
				Xbyak::Label retnLabel;

				mov(ptr[rsp + 0x20], r9d);
				jmp(ptr[rip + retnLabel]);

				L(retnLabel);
				dq(address + 5);
			}
		};

		void * codeBuf = g_localTrampoline.StartAlloc();
		AttachSkinnedObjectHook_Entry_Code code(codeBuf, (uintptr_t)AttachSkinnedObject.GetUIntPtr());
		g_localTrampoline.EndAlloc(code.getCurr());
		AttachSkinnedObject_Original = (_AttachSkinnedObject)codeBuf;
		g_branchTrampoline.Write5Branch(AttachSkinnedObject.GetUIntPtr(), uintptr_t(AttachSkinnedObject_Hooked));
	}

	// SetHairColor Palette Index
	if(g_bExtendedLUTs)
	{
		struct SetHairColorPalette_Code : Xbyak::CodeGenerator {
			SetHairColorPalette_Code(void * buf, uintptr_t funcAddr, uintptr_t hookTarget) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label funcLabel;
				Xbyak::Label retnLabel;

				mov(r8, r12);
				mov(rdx, rax);
				mov(rcx, r15);
				call(ptr [rip + funcLabel]);
				jmp(ptr [rip + retnLabel]);

				L(funcLabel);
				dq(funcAddr);

				L(retnLabel);
				dq(hookTarget + 0x15);
			}
		};

		uintptr_t targetPoint = BSFaceGenUtils::PrepareHeadPartForShaders.GetUIntPtr() + 0x262;
		void * codeBuf = g_localTrampoline.StartAlloc();
		SetHairColorPalette_Code code(codeBuf, uintptr_t(HairColorModify_Hook), targetPoint);
		g_localTrampoline.EndAlloc(code.getCurr());
		g_branchTrampoline.Write6Branch(targetPoint, uintptr_t(code.getCode()));
	}

	// SetEyebrowLUTPath
	if(g_bExtendedLUTs)
	{
		struct GetHairTexturePath_Code : Xbyak::CodeGenerator {
			GetHairTexturePath_Code(void * buf, uintptr_t funcAddr, uintptr_t targetAddr) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label funcLabel;
				Xbyak::Label retnLabel;

				// NPC pointer is already in rcx
				// mov     rcx, [rbp+1DD0h+arg_0]
				call(ptr [rip + funcLabel]);
				jmp(ptr [rip + retnLabel]);

				L(funcLabel);
				dq(funcAddr);

				L(retnLabel);
				// Skip this chunk, will be done in our own code, looking up of the hair texture and returning the C-string
				// mov     rcx, [rcx+1B8h]
				// add     rcx, 6C0h
				// call    BSFixedString__GetCString
				dq(targetAddr + 0x13);
			}
		};

		uintptr_t targetAddr = BSFaceGenUtils::StartFaceCustomizationGenerationForNPC.GetUIntPtr() + 0x10D4;
		void * codeBuf = g_localTrampoline.StartAlloc();
		GetHairTexturePath_Code code(codeBuf, (uintptr_t)GetHairTexturePath_Hook, targetAddr);
		g_localTrampoline.EndAlloc(code.getCurr());
		g_branchTrampoline.Write6Branch(targetAddr, uintptr_t(code.getCode()));
	}

	// Resizes tint mask target
	{
		struct CreateRenderTargetHook_Code : Xbyak::CodeGenerator {
			CreateRenderTargetHook_Code(void * buf, uintptr_t targetAddr) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label retnLabel;

				push(rdi);
				push(r14);
				push(r15);

				jmp(ptr [rip + retnLabel]);

				L(retnLabel);
				dq(targetAddr + 6);
			}
		};

		uintptr_t targetAddr = BSGraphics::CreateRenderTarget.GetUIntPtr();
		void * codeBuf = g_localTrampoline.StartAlloc();
		CreateRenderTargetHook_Code code(codeBuf, targetAddr);
		g_localTrampoline.EndAlloc(code.getCurr());

		BSGraphics::CreateRenderTarget_Original = (BSGraphics::_CreateRenderTarget)codeBuf;

		g_branchTrampoline.Write6Branch(targetAddr, (uintptr_t)CreateRenderTarget_Hook);
	}
	
	return true;
}

};
