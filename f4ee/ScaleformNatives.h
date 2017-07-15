#pragma once

#include "f4se/ScaleformAPI.h"
#include "f4se/ScaleformCallbacks.h"

class F4EEScaleform_LoadPreset : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_SavePreset : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_ReadPreset : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_SetCurrentBoneRegionID : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_AllowTextInput : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_GetExternalFiles : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_GetBodySliders : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_SetBodyMorph : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_CloneBodyMorphs : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_UpdateBodyMorphs : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_GetOverlays : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_GetOverlayTemplates : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_CreateOverlay : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_DeleteOverlay : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_SetOverlayData : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_ReorderOverlay : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_UpdateOverlays : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_CloneOverlays : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_GetEquippedItems : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_UnequipItems : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_EquipItems : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_GetSkinOverrides : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_GetSkinOverride : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_SetSkinOverride : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_UpdateSkinOverride : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_CloneSkinOverride : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_GetSkinColor : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_SetSkinColor : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_GetExtraColor : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class F4EEScaleform_SetExtraColor : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};