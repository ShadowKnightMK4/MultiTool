#include "common.h"

typedef union MyOSVERSIONINFO
{
	OSVERSIONINFOA A;
	OSVERSIONINFOW W;
} ;



typedef int (WINAPI* GetVersionInfOExA_PTR)(
	LPOSVERSIONINFOA lpVersionInformation
	);
typedef NTSTATUS(WINAPI* RtlGetVersion_PTR)(OSVERSIONINFOW*);

int FetchVersionInfo(MyOSVERSIONINFO* Output, bool* UseUnicode)
{
	if (Output == nullptr)
		return -1;
	Output->W.dwBuildNumber = Output->W.dwMajorVersion = Output->W.dwMinorVersion = Output->W.dwPlatformId = 0;
	Output->W.szCSDVersion[0] = 0;
	*UseUnicode = false;
	HMODULE kernel32 = LoadLibraryA("kernel32.dll");
	if (kernel32 == NULL)
	{
		return -2;
	}
	GetVersionInfOExA_PTR GetVersionExA = (GetVersionInfOExA_PTR)GetProcAddress(kernel32, "GetVersionExA");
	if (GetVersionExA == nullptr)
	{
		return -3;
	}
	else
	{
		Output->A.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
		if (GetVersionExA(&Output->A) == 0)
		{
			return -4;
		}
		else
		{
			// https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getversionexa
			/* we're checking for this, assuming the app (us) doesn't have a manifest
				* and probing deapper with the RTLGetVersion if needed.
			*/
			if (((Output->A.dwMajorVersion == 6)) && (Output->A.dwMinorVersion == 2))
			{
				HMODULE ntdll = LoadLibraryA("ntdll.dll");
				if (ntdll == NULL)
				{
					Output->W.dwBuildNumber = Output->W.dwMajorVersion = Output->W.dwMinorVersion = Output->W.dwPlatformId = 0;
					// adds bytes but reasonable practice
					FreeLibrary(kernel32);
					return -5;
				}
				else
				{
					RtlGetVersion_PTR RtlGetVersion = (RtlGetVersion_PTR)GetProcAddress(ntdll, "RtlGetVersion");
					if (RtlGetVersion == nullptr)
					{
						FreeLibrary(kernel32);
						FreeLibrary(ntdll);
						return -6;
					}
					else
					{
						*UseUnicode = true;
						RtlGetVersion(&Output->W);
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

		return false;
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