#include "common.h"
#include "IAT_FILE.H"
#include "IAT_DLLS.H"


extern "C"
{
	CreateFileA_Ptr IAT_CreateFileA = nullptr;
	GetFileSize_Ptr IAT_GetFileSize = nullptr;

	DWORD IAT_DynamicLink_FileApi_Cleanup()
	{
		IAT_CreateFileA =  0; IAT_GetFileSize = 0;
		return 1;
	}
	DWORD IAT_DynamicLink_FileApi(DWORD IAT_settings)
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
		if ( (IAT_settings & IAT_FILE_LINKING_CREATEFILEA) == (IAT_FILE_LINKING_CREATEFILEA))
		{
			IAT_CreateFileA = (CreateFileA_Ptr)GetProcAddress(iatKernel32, "CreateFileA");
			if (IAT_CreateFileA != 0)
			{
				ret |= IAT_FILE_LINKING_CREATEFILEA;
			}
		}

		if ((IAT_settings & IAT_FILE_LINKING_GETFILESIZE) == (IAT_FILE_LINKING_GETFILESIZE))
		{
			IAT_GetFileSize = (GetFileSize_Ptr)GetProcAddress(iatKernel32, "GetFileSize");
			if (IAT_GetFileSize != 0)
			{
				ret |= IAT_FILE_LINKING_GETFILESIZE;
			}
		}
		return ret;
	}
}