#pragma once
#include "common.h"
#include "IAT_DLLS.H"


typedef UINT(WINAPI* GetWindowsDirectoryAPtr)(
	LPSTR lpBuffer,
	UINT  uSize
	);
extern "C" {
	extern GetWindowsDirectoryAPtr IAT_GetWindowsDirectoryA;

	extern DWORD IAT_DynamicLink_Environment(DWORD iat_settings);
	extern DWORD IAT_DynamicLink_Environment_Cleanup();
}
#ifndef IAT_ENV_API
#define IAT_ENV_API
#define IAT_ENV_API_GETWINDOWSDIRA (1)


#endif