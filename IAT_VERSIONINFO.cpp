#include "IAT_DLLS.H"
#include "common.h"
#include "IAT_VERSIONINFO.H"

extern "C"
{
	GetVersionInfOExA_Ptr IAT_GetVersionInfoExA = 0;
	RtlGetVersion_Ptr IAT_RtlGetVersion = 0;

	DWORD IAT_DynamicLinkVersionInfo_Cleanup()
	{
		IAT_GetVersionInfoExA = 0;
		IAT_RtlGetVersion = 0;
		return 1;
	}
	 DWORD IAT_DynamicLinkVersionInfo(DWORD IAT_settings)
	{
		 DWORD ret = 0;
		 if (iatKernel32 == 0)
		 {
			 iatKernel32 = LoadLibraryA("kernel32.dll");
		 }
		 if ((IAT_settings & (IAT_VERSIONINFO_NTDLL_RTLVERSIONINFO)) == (IAT_VERSIONINFO_NTDLL_RTLVERSIONINFO))
		 {
			 if (iatNTDLL == 0)
			 {
				 iatNTDLL = LoadLibraryA("ntdll.dll");
			 }
		 }

		 if (iatKernel32 == 0)
		 {
			 return 0;
		 }
		 if ((IAT_settings & (IAT_VERSIONINFO_GETVERSIONEXA)) == (IAT_VERSIONINFO_GETVERSIONEXA))
		 {
			 IAT_GetVersionInfoExA = (GetVersionInfOExA_Ptr)GetProcAddress(iatKernel32, "GetVersionExA");
			 if (IAT_GetVersionInfoExA != 0)
			 {
				 ret |= IAT_VERSIONINFO_GETVERSIONEXA;
			 }
		 }
		 if ((IAT_settings & (IAT_VERSIONINFO_NTDLL_RTLVERSIONINFO)) == (IAT_VERSIONINFO_NTDLL_RTLVERSIONINFO))
		 {
			 IAT_RtlGetVersion = (RtlGetVersion_Ptr)GetProcAddress(iatNTDLL, "RtlGetVersion");
			 if (IAT_RtlGetVersion != 0)
			 {
				 ret |= IAT_VERSIONINFO_NTDLL_RTLVERSIONINFO;
			 }
		 }
		 else
		 {
			 IAT_RtlGetVersion = 0;
		 }

		 return ret;
	}
}