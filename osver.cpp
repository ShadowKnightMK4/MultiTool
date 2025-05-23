#include "common.h"

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

typedef int (WINAPI* GetVersionInfOExA_PTR)(
	LPOSVERSIONINFOA lpVersionInformation
	);
typedef NTSTATUS(WINAPI* RtlGetVersion_PTR)(OSVERSIONINFOW*);

void SetVersionInfoToZeroAndSetSize(bool Unicode)
{
	GlobalVersionInfo.A.dwBuildNumber = GlobalVersionInfo.A.dwMajorVersion = GlobalVersionInfo.A.dwMinorVersion = GlobalVersionInfo.A.dwPlatformId = 0;
	if (Unicode)
	{
		GlobalVersionInfo.W.szCSDVersion[0] = 0;
		for (int i = 0; i < 128; i++)
		{
			GlobalVersionInfo.W.szCSDVersion[i] = 0;
		}
	}
	else
	{
		GlobalVersionInfo.A.szCSDVersion[0] = 0;
	}
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
	if (Output == nullptr)
		return -1;
	if (VERSION_INFO_WAS_GOTTON)
	{
		// we already got the version info, no need to do it again. Not like the Windows version can change while running right?
		if (VERISON_INFO_IS_UNICODE)
			Output->W = GlobalVersionInfo.W;
		else
			Output->A = GlobalVersionInfo.A;

		
		return 0;
	}


	// why this:: no libc means memset.  Why not MemorySet or Zeromemory? That resolves to RtlXXXX which on windows resolves to well memset
	SetVersionInfoToZeroAndSetSize(false);
	*UseUnicode = false;


	// load and bail if we can't
	HMODULE kernel32 = LoadLibraryA("kernel32.dll");
	if (kernel32 == NULL)
	{
		return -2;
	}
	// get the function pointer and bail if we can't
	GetVersionInfOExA_PTR GetVersionExA = (GetVersionInfOExA_PTR)GetProcAddress(kernel32, "GetVersionExA");
	if (GetVersionExA == nullptr)
	{
		return -3;
	}
	else
	{
		// we want to mantain ANSI strings for compatibility when talking to OS.
		Output->A.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
		if (GetVersionExA(&Output->A) == 0)
		{
			return -4;
		}
		else
		{
			// https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getversionexa
			/* we're checking for this 6.2, assuming the app (us) doesn't have a manifest
				* and probing deapper with the RTLGetVersion if needed.
				* 
				* Why this? cause the GetVersionEx has meen modified to always return 6.2 aka windows 8 if no manifest and return the manifiest if set.
				* We're looking for the true version. Anything below this 6.2 is likely legit.
			*/
			if (((Output->A.dwMajorVersion == WINDOWS_8_MAJOR)) && (Output->A.dwMinorVersion == WINDOWS_8_MINOR))
			{
				HMODULE ntdll = LoadLibraryA("ntdll.dll");
				if (ntdll == NULL)
				{
					// reset output to null and free kernel32 for good practice
					SetVersionInfoToZeroAndSetSize(false);
					// adds bytes but reasonable practice
					FreeLibrary(kernel32);
					return -5;
				}
				else
				{
					// define our pointer and try to get the RtlVersion.     If nay, we're done, free kernenl32 and ntdll.
					RtlGetVersion_PTR RtlGetVersion = (RtlGetVersion_PTR)GetProcAddress(ntdll, "RtlGetVersion");
					if (RtlGetVersion == nullptr)
					{
						FreeLibrary(kernel32);
						FreeLibrary(ntdll);
						return -6;
					}
					else
					{
						// call the routine and set to output
						*UseUnicode = true;
						RtlGetVersion(&Output->W);
						if (Output != &GlobalVersionInfo)
						{
							// copy our version data to the global version info on call if not the same
							SetVersionInfoFromAnother(Output, true);
						}
						FreeLibrary(kernel32);
						FreeLibrary(ntdll);
						return 1;
					}
				}
			}
			else
			{
				*UseUnicode = false;
				FreeLibrary(kernel32);

			}
		}

		return 0;
	}
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
		*result = (int)  *((unsigned char*)(&osvi.A)+offset);
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
			int size = 0;
			char* local;

			NumberToString(osvi.A.dwMajorVersion, &local, &size);
			if (local != 0)
			{
				WriteStdout("Version: ");
				WriteStdout(local);

				LocalFree(local);
			}

			WriteStdout(".");

			NumberToString(osvi.A.dwMinorVersion, &local, &size);
			if (local != 0)
			{
				if (local != 0)
				{
					WriteStdout(local);
					WriteStdout("\r\n");
					LocalFree(local);
				}
			}

			NumberToString(osvi.A.dwBuildNumber, &local, &size);
			if (local != 0)
			{
				WriteStdout("Build Version: ");
				WriteStdout(local);
				WriteStdout("\r\n");
				LocalFree(local);
			}

			if (osvi.A.dwPlatformId == VER_PLATFORM_WIN32_NT)
			{
				WriteStdout("Platform: NT\n");
			}
			else if (osvi.A.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
			{
				WriteStdout("Platform: Windows 32bit\r\n");
			}
			else
			{
				WriteStdout("Platform: Unknown\r\n");
			}

			WriteStdout("Service Pack: ");

			if ((osvi.A.szCSDVersion[0] != 0) || (osvi.W.szCSDVersion[0] != 0))
			{
				if (use_unicode)
				{
					WriteStdout(osvi.W.szCSDVersion);
				}
				else
				{
					WriteStdout(osvi.A.szCSDVersion);
				}
				WriteStdout("\r\n");
			}

			if (result != nullptr)
			{
				*result = 0;
			}
			if (message_result != nullptr)
			{
				*message_result = "Success";
			}
			return true;
		}
	}


}