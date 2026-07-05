#include "common.h"

#include "IAT_PRIV.h"
#include "common.h"
#include "osver.h"
#include "Support\\LWAnsiString\\LWAnsiString.h">


#include "IAT_DLLS.H"
#include "IAT_JOBS.H"
#include "IAT_BOOSTRAP_LOADER.H"

AdjustTokenPrivilegesPtr IAT_AdjustTokenPriv = 0;
LookupPrivilegeValueAPtr IAT_LookupPrivilegeValueA = 0;
LookupPrivilegeValueWPtr IAT_LookupPrivilegeValueW = 0;
OpenProcessTokenPtr IAT_OpenProcessToken = 0;
DWORD IAT_DynamicLink_TokenPriv_Advapi32_cleanup()
{
	IAT_AdjustTokenPriv = 0;
	IAT_LookupPrivilegeValueA = 0;
	IAT_LookupPrivilegeValueW = 0;
	IAT_OpenProcessToken = 0;
	return 0;
}
DWORD IAT_DynamicLink_TokenPriv_Advapi32(DWORD IAT_settings)
{
	DWORD ret = 0;
	if (iatAdvapi32 == 0)
	{
		if (IAT_LoadLibraryA != 0)
		{
			iatAdvapi32 = IAT_LoadLibraryA("advapi32.dll");
		}
		else
		{
			if (IAT_LoadLibraryW != 0)
			{
				iatAdvapi32 = IAT_LoadLibraryW(L"advapi32.dll");
			}
		}
		if (iatAdvapi32 == 0)
		{
			return 0;
		}
	}

	if ((IAT_settings & IAT_PRIV_LINKING_ADJUSTTOKENPRIV) == (IAT_PRIV_LINKING_ADJUSTTOKENPRIV))
	{
		IAT_AdjustTokenPriv = (AdjustTokenPrivilegesPtr)IAT_GetProcAddress(iatAdvapi32, "AdjustTokenPrivileges");
		if (IAT_AdjustTokenPriv != 0)
		{
			ret |= IAT_PRIV_LINKING_ADJUSTTOKENPRIV;
		}
	}

	if ((IAT_settings & IAT_PRIV_LINKING_OPENPROCESSTOKEN) == (IAT_PRIV_LINKING_OPENPROCESSTOKEN))
	{
		IAT_OpenProcessToken = (OpenProcessTokenPtr)IAT_GetProcAddress(iatAdvapi32, "OpenProcessToken");
		if (IAT_AdjustTokenPriv != 0)
		{
			ret |= IAT_PRIV_LINKING_OPENPROCESSTOKEN;
		}
	}

	if ((IAT_settings & IAT_PRIV_LINKING_LOOKUPPRIVA) == (IAT_PRIV_LINKING_LOOKUPPRIVA))
	{
		IAT_LookupPrivilegeValueA = (LookupPrivilegeValueAPtr)IAT_GetProcAddress(iatAdvapi32, "LookupPrivilegeValueA");
		if (IAT_AdjustTokenPriv != 0)
		{
			ret |= IAT_PRIV_LINKING_LOOKUPPRIVA;
		}
	}


	if ((IAT_settings & IAT_PRIV_LINKING_LOOKUPPRIVW) == (IAT_PRIV_LINKING_LOOKUPPRIVW))
	{
		IAT_LookupPrivilegeValueW = (LookupPrivilegeValueWPtr)IAT_GetProcAddress(iatAdvapi32, "LookupPrivilegeValueW");
		if (IAT_LookupPrivilegeValueW != 0)
		{
			ret |= IAT_PRIV_LINKING_LOOKUPPRIVW;
		}
	}

	return ret;
	
	
}


