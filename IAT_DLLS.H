#pragma once
#include "common.h"
#include <Windows.h>

extern HMODULE iatKernel32;
extern HMODULE iatAdvapi32;
extern HMODULE iatSecurity;
extern HMODULE iatNTDLL;

// if (IAT_DynamicLinkJob(IAT_JOB_CREATEJOBA) != IAT_JOB_CREATEJOBA)
#define IAT_OK(IAT_settings, FunctionCall, LoadThese)  ((FunctionCall(IAT_settings) & LoadThese)) == (LoadThese)))