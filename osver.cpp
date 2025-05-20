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

bool NtVersionProbe(int* result, const char** message_result, const char* argv[])
{
	OSVERSIONINFOW NtVersionData;
	NtVersionData.dwBuildNumber = NtVersionData.dwMajorVersion = NtVersionData.dwMinorVersion = NtVersionData.dwPlatformId = 0;
	NtVersionData.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
	// load ntdll and handle failure stuff
	HMODULE hModule = LoadLibraryA("ntdll.dll");
	if (hModule == NULL)
	{
		if (message_result != nullptr)
		{
			*message_result = "Failed to load ntdll.dll";
		}
		if (result != nullptr)
		{
			*result = -1;
		}
		return false;
	}
	typedef NTSTATUS(WINAPI* RtlGetVersion_PTR)(OSVERSIONINFOW*);
	RtlGetVersion_PTR RtlGetVersion = (RtlGetVersion_PTR)GetProcAddress(hModule, "RtlGetVersion");
	if (RtlGetVersion == nullptr)
	{
		if (message_result != nullptr)
		{
			*message_result = "Failed to locate RtlGetVersion";
		}
		if (result != nullptr)
		{
			*result = -2;
		}
		return false;
	}
	RtlGetVersion(&NtVersionData);

	int size = 0;
	char* local;


	NumberToString(NtVersionData.dwMajorVersion, &local, &size);
	if (local != 0)
	{
		WriteStdout("Version: ");
		WriteStdout(local);
		LocalFree(local);
	}

	WriteStdout(".");

	NumberToString(NtVersionData.dwMinorVersion, &local, &size);
	if (local != 0)
	{
		if (local != 0)
		{
			WriteStdout(local);
			WriteStdout("\r\n");
			LocalFree(local);
		}
	}

	NumberToString(NtVersionData.dwBuildNumber, &local, &size);
	if (local != 0)
	{
		WriteStdout("Build Version: ");
		WriteStdout(local);
		WriteStdout("\r\n");
		LocalFree(local);
	}

	if (NtVersionData.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		WriteStdout("Platform: NT\r\n");
	}
	else if (NtVersionData.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
	{
		WriteStdout("Platform: Windows\r\n");
	}
	else
	{
		WriteStdout("Platform: Unknown\r\n");
	}

	if (NtVersionData.szCSDVersion[0] != 0)
	{
		WriteStdout("Service Pack: ");
		WriteStdout(NtVersionData.szCSDVersion);
	}
	return true;

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

	bool ReportVersionPlatformIDViaExit(int* result, const char** message_result, const char* argv[])
	{
		return ReportVersionByExitCodeCommon(result, message_result, argv, offsetof(OSVERSIONINFOA, dwPlatformId));
	}

	bool ReportVersionBuildViaExit(int* result, const char** message_result, const char* argv[])
	{
		return ReportVersionByExitCodeCommon(result, message_result, argv, offsetof(OSVERSIONINFOA, dwBuildNumber));
	}

	bool ReportVersionMinorViaExit(int* result, const char** message_result, const char* argv[])
	{
		return ReportVersionByExitCodeCommon(result, message_result, argv, offsetof(OSVERSIONINFOA, dwMinorVersion));
	}
	bool ReportVersionMajorViaExit(int* result, const char** message_result, const char* argv[])
	{
		return ReportVersionByExitCodeCommon(result, message_result, argv, offsetof(OSVERSIONINFOA, dwMajorVersion));
	}

	bool ReportVersionStdout(int* result, const char** message_result, const char* argv[])
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

	bool ReportVersionStdoutOld(int* result, const char** message_result, const char* argv[])
	{
		GetVersionInfOExA_PTR GetVersionExA = nullptr;
		OSVERSIONINFOA osvi;
		osvi.dwBuildNumber = osvi.dwMajorVersion = osvi.dwMinorVersion = osvi.dwPlatformId = osvi.szCSDVersion[0] = 0;
		osvi.dwOSVersionInfoSize = sizeof(osvi);
		HMODULE hModule = LoadLibraryA("kernel32.dll");
		if (hModule == NULL)
		{
			if (message_result != nullptr)
			{
				*message_result = "Failed to load kernel32.dll";
			}
			if (result != nullptr)
			{
				*result = -1;
			}
			return false;
		}
		GetVersionExA = (GetVersionInfOExA_PTR)GetProcAddress(hModule, "GetVersionExA");
		if (GetVersionExA == nullptr)
		{
			if (message_result != nullptr)
			{
				*message_result = "Failed to locate GetVersionExA";
			}
			if (result != nullptr)
			{
				*result = -2;
			}
			return false;
		}
		else
		{
			if (GetVersionExA(&osvi) == 0)
			{
				if (message_result != nullptr)
				{
					*message_result = "Failed to get version info";
				}
				if (result != nullptr)
				{
					*result = -3;
				}
				return false;
			}
			else
			{
				// https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getversionexa
				/* we're checking for this, assuming the app (us) doesn't have a manifest
				* and probing deapper with the RTLGetVersion if needed.
				*/
				if (((osvi.dwMajorVersion == 6)) && (osvi.dwMinorVersion == 2))
				{
					return NtVersionProbe(result, message_result, argv);
				}
				else
				{
					int size = 0;
					char* local;

					NumberToString(osvi.dwMajorVersion, &local, &size);
					if (local != 0)
					{
						WriteStdout("Version: ");
						WriteStdout(local);

						LocalFree(local);
					}

					WriteStdout(".");

					NumberToString(osvi.dwMinorVersion, &local, &size);
					if (local != 0)
					{
						if (local != 0)
						{
							WriteStdout(local);
							WriteStdout("\r\n");
							LocalFree(local);
						}
					}

					NumberToString(osvi.dwBuildNumber, &local, &size);
					if (local != 0)
					{
						WriteStdout("Build Version: ");
						WriteStdout(local);
						WriteStdout("\r\n");
						LocalFree(local);
					}

					if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
					{
						WriteStdout("Platform: NT\n");
					}
					else if (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
					{
						WriteStdout("Platform: Windows\r\n");
					}
					else
					{
						WriteStdout("Platform: Unknown\r\n");
					}

					if (osvi.szCSDVersion[0] != 0)
					{
						WriteStdout("Service Pack: ");
						WriteStdout(osvi.szCSDVersion);
						WriteStdout("\r\n");
					}
					return true;
				}
			}
		}
		return false;

	}
}