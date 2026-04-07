#pragma once
#include "common.h"

typedef HMODULE (WINAPI* LoadLibraryAPtr)(
	 LPCSTR lpLibFileName
);

typedef HMODULE (WINAPI *LoadLibraryWPtr)(
	LPCWSTR lpLibFileName
);

typedef FARPROC (WINAPI *GetProcAddressPtr)(
	HMODULE hModule,
	 LPCSTR  lpProcName
);


extern "C" {
	extern LoadLibraryAPtr IAT_LoadLibraryA;
	extern LoadLibraryWPtr IAT_LoadLibraryW;
	extern GetProcAddressPtr IAT_GetProcAddress;



	extern	DWORD IAT_DynamicLink_BootStrapCleanup();
	extern DWORD IAT_DynamicLink_BootStrapLoader(DWORD IAT_settings);
}

#ifndef IAT_BOOTSTRAP_LOADER
#define IAT_BOOTSTRAP_LOADER
#define IAT_BOOTSTRAP_LOADER_GETPROCADDRESS (1)
#define IAT_BOOTSTRAP_LOADER_ANSI (2)
#define IAT_BOOTSTRAP_LOADER_UNICODE (4)
#define IAT_BOOTSTRAP_LOADER_GETPROCADDRESS_ANSI (8)
#define IAT_BOOTSTRAP_LOADER_GETPROCADDRESS_UNICODE (IAT_BOOTSTRAP_LOADER_GETPROCADDRESS | IAT_BOOTSTRAP_LOADER_UNICODE)

#endif