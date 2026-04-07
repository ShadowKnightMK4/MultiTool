#pragma once
#include "common.h"
#include "IAT_DLLS.H"


typedef UINT(WINAPI* GetWindowsDirectoryAPtr)(
	LPSTR lpBuffer,
	UINT  uSize
	);

typedef DWORD(WINAPI* GetShortPathNameAPTR)(
	LPCSTR lpszLongPath,
	LPSTR  lpszShortPath,
	DWORD   cchBuffer
	);


extern "C" {
	extern GetWindowsDirectoryAPtr IAT_GetWindowsDirectoryA;
	extern GetShortPathNameAPTR  IAT_GetShortPathA;
	extern DWORD IAT_DynamicLink_Environment(DWORD iat_settings);
	extern DWORD IAT_DynamicLink_Environment_Cleanup();
}
#ifndef IAT_ENV_API
#define IAT_ENV_API
#define IAT_ENV_API_GETWINDOWSDIRA (1)
#define IAT_ENV_API_GETSHORTPATHA (2)
#define IAT_ENV_API_ALL_A (IAT_ENV_API_GETWINDOWSDIRA | IAT_ENV_API_GETSHORTPATHA )


#endif