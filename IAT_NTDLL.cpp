#include "common.h"
#include "IAT_DLLS.H"
#include "IAT_NTDLL.H"
#include "IAT_BOOSTRAP_LOADER.H"
#include <winternl.h>



NtQueryInformationProcessPtr IAT_NtQueryInformationProcess = 0;
NtTerminateProcessPtr IAT_NtTerminateProcess = 0;
DWORD IAT_DynamicLink_NTDLL(DWORD IAT_settings)
{
	DWORD ret = 0;
	if (iatNTDLL == 0)
	{
		if (IAT_LoadLibraryA != 0)
		{
			iatNTDLL = IAT_LoadLibraryA("ntdll.dll");
		}
		else
		{
			iatNTDLL = IAT_LoadLibraryW(L"ntdll.dll");
		}
	}
	if (iatNTDLL == 0)
	{
		return 0;
	}
	else
	{
		if ((IAT_settings & IAT_NTDLL_LINKING_NTQUERYINFOPROCESS) == (IAT_NTDLL_LINKING_NTQUERYINFOPROCESS))
		{
			IAT_NtQueryInformationProcess = (NtQueryInformationProcessPtr) IAT_GetProcAddress(iatNTDLL, "NtQueryInformationProcess");
			if (IAT_NtQueryInformationProcess != 0)
			{
				ret |= IAT_NTDLL_LINKING_NTQUERYINFOPROCESS;
			}
		}

		if ((IAT_settings & IAT_NTDLL_LINKING_NTTERMINATEPROCESS) == (IAT_NTDLL_LINKING_NTTERMINATEPROCESS))
		{
			IAT_NtTerminateProcess = (NtTerminateProcessPtr)IAT_GetProcAddress(iatNTDLL, "NtTerminateProcess");
			if (IAT_NtTerminateProcess != 0)
			{
				ret |= IAT_NTDLL_LINKING_NTTERMINATEPROCESS;
			}
		}

		
		return ret;
	}
}

DWORD IAT_DynamicLink_NTDLL_Cleanup()
{
	if (iatNTDLL != 0)
	{
		iatNTDLL = 0;
		IAT_NtQueryInformationProcess = 0;
	}
	return 0;
}