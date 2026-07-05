#pragma once


typedef BOOL (WINAPI* AdjustTokenPrivilegesPtr)(
	           HANDLE            TokenHandle,
	            BOOL              DisableAllPrivileges,
	 PTOKEN_PRIVILEGES NewState,
	            DWORD             BufferLength,
	PTOKEN_PRIVILEGES PreviousState,
	 PDWORD            ReturnLength
);

typedef BOOL (WINAPI* LookupPrivilegeValueAPtr)(
	 LPCSTR lpSystemName,
	          LPCSTR lpName,
	        PLUID  lpLuid
);

typedef BOOL(WINAPI* LookupPrivilegeValueWPtr)(
	LPWSTR lpSystemName,
	LPWSTR lpName,
	PLUID  lpLuid
	);

typedef BOOL (WINAPI *OpenProcessTokenPtr)(
	 HANDLE  ProcessHandle,
	 DWORD   DesiredAccess,
	 PHANDLE TokenHandle
);
#ifndef IAT_PRIV_LINKING
#define IAT_PRIV_LINKING
#define IAT_PRIV_LINKING_ADJUSTTOKENPRIV (1)
#define IAT_PRIV_LINKING_LOOKUPPRIVA (2)
#define IAT_PRIV_LINKING_LOOKUPPRIVW (4)
#define IAT_PRIV_LINKING_OPENPROCESSTOKEN (8)
#endif
extern "C"
{
	extern AdjustTokenPrivilegesPtr IAT_AdjustTokenPriv;
	extern LookupPrivilegeValueAPtr IAT_LookupPrivilegeValueA;
	extern LookupPrivilegeValueWPtr IAT_LookupPrivilegeValueW;
	extern OpenProcessTokenPtr IAT_OpenProcessToken;
}

DWORD IAT_DynamicLink_TokenPriv_Advapi32(DWORD IAT_settings);
DWORD IAT_DynamicLink_TokenPriv_Advapi32_cleanup();
