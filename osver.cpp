#include "common.h"
#include "osver.h"
#include <LWAnsiString.h>
#include "IAT_VERSIONINFO.H"
#define WINDOWS_8_MAJOR 6
#define WINDOWS_8_MINOR 2


/// <summary>
/// FetchVerisonInfo sets this if RtlGetVersion was used to get the version info since that stores in unicode
/// </summary>
bool VERISON_INFO_IS_UNICODE;
/// <summary>
/// FetchVerisonInfo sets this if FetchVersionInfo was called at least once.  The rest of the software can just OSVERSIONINFO dependeing on what union
/// </summary>
bool VERSION_INFO_WAS_GOTTON;

/// <summary>
/// Storage for versioned routined to actually get the version info.
/// </summary>
MyOSVERSIONINFO GlobalVersionInfo = { 0, 0, 0, 0 };

/*typedef int (WINAPI* GetVersionInfOExA_PTR)(
	LPOSVERSIONINFOEXA lpVersionInformation
	);
typedef NTSTATUS(WINAPI* RtlGetVersion_PTR)(LPOSVERSIONINFOEXW);*/

void SetVersionInfoToZeroAndSetSize(bool Unicode, MyOSVERSIONINFO* GlobalVersionInfo)
{
	GlobalVersionInfo->A.dwBuildNumber = GlobalVersionInfo->A.dwMajorVersion = GlobalVersionInfo->A.dwMinorVersion = GlobalVersionInfo->A.dwPlatformId = 0;
	GlobalVersionInfo->A.wSuiteMask = GlobalVersionInfo->A.wServicePackMajor = GlobalVersionInfo->A.wServicePackMinor = 0;
	if (Unicode)
	{
		GlobalVersionInfo->W.szCSDVersion[0] = 0;
		for (int i = 0; i < 128; i++)
		{
			GlobalVersionInfo->W.szCSDVersion[i] = 0;
		}
	}
	else
	{
		GlobalVersionInfo->A.szCSDVersion[0] = 0;
	}
	GlobalVersionInfo->A.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
}

void SetVersionInfoFromAnother(MyOSVERSIONINFO* Source, bool Unicode)
{
	if (Unicode)
	{
		GlobalVersionInfo.W.dwBuildNumber = Source->W.dwBuildNumber;
		GlobalVersionInfo.W.dwMajorVersion = Source->W.dwMajorVersion;
		GlobalVersionInfo.W.dwMinorVersion = Source->W.dwMinorVersion;
		GlobalVersionInfo.W.dwPlatformId = Source->W.dwPlatformId;
		for (int i = 0; i < 128; i++)
		{
			GlobalVersionInfo.A.szCSDVersion[i] = Source->A.szCSDVersion[i];
		}
	}
	else
	{
		GlobalVersionInfo.A.dwBuildNumber = Source->A.dwBuildNumber;
		GlobalVersionInfo.A.dwMajorVersion = Source->A.dwMajorVersion;
		GlobalVersionInfo.A.dwMinorVersion = Source->A.dwMinorVersion;
		GlobalVersionInfo.A.dwPlatformId = Source->A.dwPlatformId;
		for (int i = 0; i < 128; i++)
		{
			GlobalVersionInfo.A.szCSDVersion[i] = Source->A.szCSDVersion[i];
		}
	}
}


int FetchVersionInfo(MyOSVERSIONINFO* Output, bool* UseUnicode)
{
	bool ProbeNtVerison = false;
	if (Output == nullptr)
		return 0;
	if (UseUnicode == nullptr)
		return 0;
	// why this:: no libc means memset.  Why not MemorySet or Zeromemory? That resolves to RtlXXXX which on windows resolves to well memset
	SetVersionInfoToZeroAndSetSize(false, Output);

	/*
	* Our plan is this:
	* try loading kernel32 for GetVersionExA.
	*	if it fails (how???) we give it up.
	* 
	*	Next we try to GetProcAddress the GetVersionExA routine. If it works, we call it, if not we set our check ntdll flag (ProbeNtVerison) to true.
	*	Additionally, GetVersionExA is known to return 6.2 for Windows 8 if no manifest is set.  We check for that and set the ProbeNtVerison flag to true which triggers the RtlGetVerison call if possible
	*/

	if ( 
		(
			((IAT_DynamicLinkVersionInfo(IAT_VERSIONINFO_GETVERSIONEXA | IAT_VERSIONINFO_NTDLL_RTLVERSIONINFO)) && (IAT_VERSIONINFO_GETVERSIONEXA)))  == (IAT_VERSIONINFO_GETVERSIONEXA)
		)
		
	{
		Output->A.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);

		if (IAT_GetVersionInfoExA(&Output->A) == 0)
		{
			ProbeNtVerison = true; // worth a shot
		}
		else
		{
			if (Output->A.dwMajorVersion == WINDOWS_8_MAJOR && Output->A.dwMinorVersion == WINDOWS_8_MINOR)
			{
				ProbeNtVerison = true;
			}
			else
			{
				VERSION_INFO_WAS_GOTTON = true;
				*UseUnicode = false;
				return 1;
			}
		}
	}
	else
	{
		ProbeNtVerison = true;
	}


	if (ProbeNtVerison)
	{
		{
			if (IAT_RtlGetVersion == 0)
			{
				return 0;
			}
			else
			{
				IAT_RtlGetVersion(&Output->W);
				VERSION_INFO_WAS_GOTTON = true;
				*UseUnicode = true;
				if (Output != &GlobalVersionInfo)
				{
					// copy our version data to the global version info on call if not the same
					SetVersionInfoFromAnother(Output, true);
				}
				return 1;
			}
		}
	}
	return 0;
}




extern "C" {

	bool ReportVersionByExitCodeCommon(int* result, const char** message_result, const char* argv[], DWORD offset)
	{
		bool use_unicode = false;
		MyOSVERSIONINFO osvi;
		int ret = FetchVersionInfo(&osvi, &use_unicode);
		if (ret != 1)
		{
			if (message_result != nullptr)
			{
				*message_result = "Failed to get version info";
			}
			if (result != nullptr)
			{
				*result = ret;
			}
			return false;
		}
		*result = (int)*((unsigned char*)(&osvi.A) + offset);
		*message_result = "Success";
		return true;
	}

	bool ReportVersionPlatformIDViaExit(int* result, const char** message_result, const char* argv[], int argc)
	{
		return ReportVersionByExitCodeCommon(result, message_result, argv, offsetof(OSVERSIONINFOA, dwPlatformId));
	}

	bool ReportVersionBuildViaExit(int* result, const char** message_result, const char* argv[], int argc)
	{
		return ReportVersionByExitCodeCommon(result, message_result, argv, offsetof(OSVERSIONINFOA, dwBuildNumber));
	}

	bool ReportVersionMinorViaExit(int* result, const char** message_result, const char* argv[], int argc)
	{
		return ReportVersionByExitCodeCommon(result, message_result, argv, offsetof(OSVERSIONINFOA, dwMinorVersion));
	}
	bool ReportVersionMajorViaExit(int* result, const char** message_result, const char* argv[], int argc)
	{
		return ReportVersionByExitCodeCommon(result, message_result, argv, offsetof(OSVERSIONINFOA, dwMajorVersion));
	}

	bool ReportVersionStdout(int* result, const char** message_result, const char* argv[], int argc)
	{
		bool use_unicode = false;
		MyOSVERSIONINFO osvi;
		LWAnsiString* OutputString = LWAnsiString_CreateString(0); // it'll grow
		int ret = FetchVersionInfo(&osvi, &use_unicode);
		if (ret != 1)
		{
			if (message_result != nullptr)
			{
				*message_result = "Failed to get version info";
			}
			if (result != nullptr)
			{
				*result = ret;
			}
			return false;
		}
		else
		{



			{
				LWAnsiString_Append(OutputString, "Version: ");
				LWAnsiString_AppendNumber(osvi.A.dwMajorVersion, OutputString, 0);
				LWAnsiString_Append(OutputString, ".");


				LWAnsiString_AppendNumber(osvi.A.dwMinorVersion, OutputString, 0);

				LWAnsiString_Append(OutputString, "\r\nBuild Version: ");




				LWAnsiString_AppendNumber(osvi.A.dwBuildNumber, OutputString, 0);

				LWAnsiString_Append(OutputString, "\r\n");
				if (osvi.A.dwPlatformId == VER_PLATFORM_WIN32_NT)
				{
					LWAnsiString_Append(OutputString, "Platform: NT\r\n");
				}
				else if (osvi.A.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
				{
					LWAnsiString_Append(OutputString, "Platform: Windows 95/98/ME\r\n");
				}
				else
				{
					LWAnsiString_Append(OutputString, "Platform: Unknown\r\n");
				}
				LWAnsiString_Append(OutputString, "Service Pack: ");

				if ((osvi.A.szCSDVersion[0] != 0) || (osvi.W.szCSDVersion[0] != 0))
				{
					if (use_unicode)
					{
						WriteStdout(LWAnsiString_ToCStr(OutputString));
						LWAnsiString_ZeroString(OutputString);
					}
				}
			}
			LWAnsiString_FreeString(OutputString);
			return true;
		}


	}

}