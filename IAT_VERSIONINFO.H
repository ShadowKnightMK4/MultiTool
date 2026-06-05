#pragma once
#include "IAT_DLLS.H"
#include "common.h"


typedef int (WINAPI* GetVersionInfOExA_Ptr)(
	LPOSVERSIONINFOEXA lpVersionInformation
	);


typedef NTSTATUS(WINAPI* RtlGetVersion_Ptr)(LPOSVERSIONINFOEXW);



	extern "C"	GetVersionInfOExA_Ptr IAT_GetVersionInfoExA;
	extern "C"	RtlGetVersion_Ptr IAT_RtlGetVersion;

	extern "C"	DWORD IAT_DynamicLinkVersionInfo_Cleanup();
	extern "C"	DWORD IAT_DynamicLinkVersionInfo(DWORD IAT_settings);

#ifndef IAT_VERSIONINFO_API
#define IAT_VERSIONINFO_API
#define IAT_VERSIONINFO_GETVERSIONEXA (1)
#define IAT_VERSIONINFO_NTDLL_RTLVERSIONINFO (2)

#endif