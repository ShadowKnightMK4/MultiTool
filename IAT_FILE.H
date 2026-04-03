#pragma once
#include "common.h"
#include "IAT_DLLS.H"

typedef HANDLE(WINAPI* CreateFileA_Ptr)(
	LPCSTR                lpFileName,
	DWORD                 dwDesiredAccess,
	DWORD                 dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD                 dwCreationDisposition,
	DWORD                 dwFlagsAndAttributes,
	HANDLE                hTemplateFile
	);
typedef DWORD(WINAPI* GetFileSize_Ptr)(
	HANDLE  hFile,
	LPDWORD lpFileSizeHigh
	);

extern "C"
{

	extern CreateFileA_Ptr IAT_CreateFileA;
	extern GetFileSize_Ptr IAT_GetFileSize;

	extern DWORD IAT_DynamicLink_FileApi_Cleanup();
	extern DWORD IAT_DynamicLink_FileApi(DWORD IAT_settings);
}

#ifndef IAT_FILE_LINKING
#define IAT_FILE_LINKING
#define IAT_FILE_LINKING_CREATEFILEA (1)
#define IAT_FILE_LINKING_CREATEFILEW (2)
#define IAT_FILE_LINKING_GETFILESIZE (4)

#define IAT_FILE_LINKING_IMPORTS_ANIS (IAT_FILE_LINKING_CREATEFILEA | IAT_FILE_LINKING_GETFILESIZE)
#define IAT_FILE_LINKING_IMPORTS_ANIS  (IAT_FILE_LINKING_CREATEFILEW | IAT_FILE_LINKING_GETFILESIZE)
#endif 