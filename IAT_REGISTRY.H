#pragma once
#include "IAT_DLLS.H"
#include "common.h"


typedef LONG(WINAPI* RegEnumKeyA_Ptr)(
	HKEY   hKey,
	DWORD  dwIndex,
	LPSTR  lpName,
	DWORD  cchName
	);
typedef LSTATUS(WINAPI* RegOpenKeyA_Ptr)(
	HKEY hKey,
	LPCSTR lpSubKey,
	HKEY* phkResult
	);
typedef LSTATUS(WINAPI* RegCloseKey_Ptr)(
	HKEY hKey
	);
typedef LSTATUS(WINAPI* RegEnumValueA_Ptr)(
	HKEY    hKey,
	DWORD   dwIndex,
	LPSTR   lpValueName,
	LPDWORD lpcchValueName,
	LPDWORD lpReserved,
	LPDWORD lpType,
	LPBYTE  lpData,
	LPDWORD lpcbData
	);
typedef LSTATUS(WINAPI* RegQueryValueExA_Ptr)(
	HKEY    hKey,
	LPCSTR  lpValueName,
	LPDWORD lpReserved,
	LPDWORD lpType,
	LPBYTE  lpData,
	LPDWORD lpcbData
	);


extern "C"
{
	extern RegOpenKeyA_Ptr IAT_RegOpenKeyA;
	extern RegEnumKeyA_Ptr IAT_RegEnumKeyA;
	extern RegCloseKey_Ptr IAT_RegCloseKey;
	extern RegEnumValueA_Ptr IAT_RegEnumValueA;
	extern RegQueryValueExA_Ptr IAT_RegQueryValueExA;

	extern	DWORD IAT_DynamicLinkRegistry_Cleanup();
	extern DWORD IAT_DynamicLinkRegistry(DWORD IAT_settings);
}

#ifndef IAT_REGISTRY_KEY_API
#define IAT_REGISTRY_KEY_API
#define IAT_REGISTRY_OPENKEYA (1)
#define IAT_REGISTRY_OPENKEYW (2)
#define IAT_REGISTRY_ENUMKEYA (4)
#define IAT_REGISTRY_ENUMKEYW (8)
#define IAT_REGISTRY_QUERYVALUEEXA (16)
#define IAT_REGISTRY_QUERYVALUEEXW (32)
#define IAT_REGISTRY_CLOSEKEY (64)


#define IAT_REGISTRY_ANSI ( IAT_REGISTRY_OPENKEYA +IAT_REGISTRY_ENUMKEYA + IAT_REGISTRY_QUERYVALUEEXA + IAT_REGISTRY_CLOSEKEY)
#define IAT_REGISTRY_UNICODE (IAT_REGISTRY_OPENKEYW +IAT_REGISTRY_ENUMKEYW + IAT_REGISTRY_QUERYVALUEEXW + IAT_REGISTRY_CLOSEKEY)

#define IAT_REGISTORY_BOTH (IAT_REGISTRY_ANSI | IAT_REGISTRY_UNICODE)
#endif