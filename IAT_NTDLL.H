#pragma once

#include "IAT_DLLS.H"
#include "common.h"
#include "winternl.h"

#ifndef IAT_NTDLL_API
#define IAT_INTDLL_API
#define IAT_NTDLL_LINKING_NTQUERYINFOPROCESS (1)
#define IAT_NTDLL_LINKING_NTTERMINATEPROCESS (2)
#endif

typedef NTSTATUS(WINAPI* NtQueryInformationProcessPtr)(
                 HANDLE           ProcessHandle,
	                PROCESSINFOCLASS ProcessInformationClass,
	                PVOID            ProcessInformation,
	           ULONG            ProcessInformationLength,
	                PULONG           ReturnLength
	);

typedef NTSTATUS(WINAPI* NtTerminateProcessPtr)(
	 HANDLE   ProcessHandle,
	 NTSTATUS ExitStatus
);

extern "C" {
	extern NtQueryInformationProcessPtr IAT_NtQueryInformationProcess;
	extern NtTerminateProcessPtr IAT_NtTerminateProcess;
}

extern DWORD IAT_DynamicLink_NTDLL(DWORD IAT_settings);
extern DWORD IAT_DynamicLink_NTDLL_Cleanup();
