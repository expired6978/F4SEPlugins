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