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

typedef BOOL(WINAPI* MoveFileExA_Ptr)(
	LPCSTR lpExistingFileName,
	LPCSTR lpNewFileName,
	DWORD  dwFlags
	);

typedef DWORD (WINAPI* SetFilePointer_Ptr)(
	                    HANDLE hFile,
	                    LONG   lDistanceToMove,
	                    PLONG  lpDistanceToMoveHigh,
	                    DWORD  dwMoveMethod
);
extern "C"
{
	extern MoveFileExA_Ptr IAT_MoveFileExA;
	extern CreateFileA_Ptr IAT_CreateFileA;
	extern GetFileSize_Ptr IAT_GetFileSize;
	extern SetFilePointer_Ptr IAT_SetFilePointer;

	extern DWORD IAT_DynamicLink_FileApi_Cleanup();
	extern DWORD IAT_DynamicLink_FileApi(DWORD IAT_settings);
}

#ifndef IAT_FILE_LINKING
#define IAT_FILE_LINKING
#define IAT_FILE_LINKING_CREATEFILEA (1)
#define IAT_FILE_LINKING_CREATEFILEW (2)
#define IAT_FILE_LINKING_GETFILESIZE (4)
#define IAT_FILE_LINKING_MOVEFILEEXA (8)
#define IAT_FILE_LINKING_MOVEFILEEXW (16)
#define IAT_FILE_LINKING_SETFILEPOINTER (32)

#define IAT_FILE_LINKING_IMPORTS_ANIS (IAT_FILE_LINKING_CREATEFILEA | IAT_FILE_LINKING_GETFILESIZE |IAT_FILE_LINKING_MOVEFILEEXA | IAT_FILE_LINKING_SETFILEPOINTER )
#define IAT_FILE_LINKING_IMPORTS_UNICODE  (IAT_FILE_LINKING_CREATEFILEW | IAT_FILE_LINKING_GETFILESIZE | IAT_FILE_LINKING_MOVEFILEEXW | IAT_FILE_LINKING_SETFILEPOINTER)
#endif 