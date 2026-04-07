#include "IAT_DLLS.H"
#include "IAT_ENV.H"

extern "C"
{
	GetWindowsDirectoryAPtr IAT_GetWindowsDirectoryA = 0;
	GetShortPathNameAPTR  IAT_GetShortPathA = 0;


	DWORD IAT_DynamicLink_Environment(DWORD iat_settings)
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
		if ((iat_settings & IAT_ENV_API_GETWINDOWSDIRA) == (IAT_ENV_API_GETWINDOWSDIRA))
		{

			IAT_GetWindowsDirectoryA = (GetWindowsDirectoryAPtr)GetProcAddress(iatKernel32, "GetWindowsDirectory");
			if (IAT_GetWindowsDirectoryA != 0)
			{
				ret |= IAT_ENV_API_GETWINDOWSDIRA;
			}
		}

		if ((iat_settings & IAT_ENV_API_GETSHORTPATHA) == (IAT_ENV_API_GETSHORTPATHA))
		{

			IAT_GetShortPathA = (GetShortPathNameAPTR)GetProcAddress(iatKernel32, "GetShortPathA");
			if (IAT_GetShortPathA != 0)
			{
				ret |= IAT_ENV_API_GETSHORTPATHA;
			}
		}
		return ret;

	}
	DWORD IAT_DynamicLink_Environment_Cleanup()
	{
		IAT_GetWindowsDirectoryA = 0;
		return 1;
	}
}