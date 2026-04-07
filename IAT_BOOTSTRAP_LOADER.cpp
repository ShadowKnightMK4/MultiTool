#include "common.h"
#include "IAT_DLLS.H"
#include "IAT_BOOSTRAP_LOADER.H"

/*
* WHAT IS BOOTSTRAP_LOADER
* 
* We hard link LoadLibraryA for kernel32.dll and getprocedureaddress
* We late link LoadLibraryA/W, and GetProcaddress
* 
* The rest of the IAT stuff loads off pointers and the late link of LoadLibraryA/W and GetProcAddress.
* 
* Why? Seemed like an idea at the time.
* And it lets us swap pointers say for unit testing.
* 
*/
extern "C"
{
	LoadLibraryAPtr IAT_LoadLibraryA = 0;
	LoadLibraryWPtr IAT_LoadLibraryW = 0;
	GetProcAddressPtr IAT_GetProcAddress = 0;

	DWORD IAT_DynamicLink_BootStrapCleanup()
	{
		IAT_LoadLibraryA = 0;
		IAT_LoadLibraryW = 0;
		IAT_GetProcAddress = 0;
		return 1;
	}

	DWORD IAT_DynamicLink_BootStrapLoader(DWORD IAT_settings)
	{
		DWORD ret = 0;
		if (iatKernel32 == 0)
		{
			iatKernel32 = LoadLibraryA("kernel32.dll");
		}
		if (iatKernel32 == 0)
		{
			return 0;
		}
		DWORD dummy = (IAT_settings & IAT_BOOTSTRAP_LOADER_ANSI);
		if ((IAT_settings & IAT_BOOTSTRAP_LOADER_ANSI) != 0)
		{
			IAT_LoadLibraryA = (LoadLibraryAPtr)GetProcAddress(iatKernel32, "LoadLibraryA");
			if (IAT_LoadLibraryA != 0)
			{
				ret |= IAT_BOOTSTRAP_LOADER_ANSI;
			}
		}

		if ((IAT_settings & IAT_BOOTSTRAP_LOADER_UNICODE))
		{
			IAT_LoadLibraryW = (LoadLibraryWPtr)GetProcAddress(iatKernel32, "LoadLibraryW");
			if (IAT_LoadLibraryW != 0)
			{
				ret |= IAT_BOOTSTRAP_LOADER_UNICODE;
			}
		}

		if ((IAT_settings & IAT_BOOTSTRAP_LOADER_GETPROCADDRESS) )
		{
			IAT_GetProcAddress = (GetProcAddressPtr)GetProcAddress(iatKernel32, "GetProcAddress");
			if (IAT_GetProcAddress != 0)
			{
				ret |= IAT_BOOTSTRAP_LOADER_GETPROCADDRESS;
			}
		}

		return ret;
	}

}