#include "common.h"	
#include "osver.h"
typedef ULONGLONG(WINAPI* GetTickCount64_PTR)();
typedef DWORD(WINAPI* GetTickCount_PTR)();

#define WINDOWS_VISTA_MAJOR 6
#define WINDOWS_VISTA_MINOR 0


/// <summary>
/// GetTickCount or GetTickCount64() and return it in out. 
/// </summary>
/// <param name="out">result of the call. IF GetTickCount, upper ULONG is 0</param>
/// <param name="message_result">message to display if needed</param>
/// <param name="result"></param>
/// <returns></returns>
int GetUptime(ULONGLONG* out, const char** message_result, int* result)
{
	bool Probe64 = false;
	ULONGLONG uptime = 0;
	// arg validate

	if (result == nullptr)
	{
		return -3;
	}

	if (out == nullptr)
	{
		return -1;
	}

	if (message_result == nullptr)
	{
		return -2;
	}

	*message_result = nullptr;
	*result = 0;

	// check if we have the version info. If not, get it.

	if (!VERSION_INFO_WAS_GOTTON)
		FetchVersionInfo(&GlobalVersionInfo, &VERISON_INFO_IS_UNICODE); // note this is the same routien osver tool uses.
	if (!VERSION_INFO_WAS_GOTTON)
	{
		return -4;
	}
	if ((GlobalVersionInfo.A.dwMajorVersion >= WINDOWS_VISTA_MAJOR) && (GlobalVersionInfo.A.dwMinorVersion >= WINDOWS_VISTA_MINOR))
	{
		// Vista+. Dynamticly Preference is GetTickCount64.
		Probe64 = true;
	}
	HMODULE kernel32 = LoadLibraryA("kernel32.dll");

	// check kernel failure.
	if (!kernel32)
	{
		if (message_result != nullptr)
		{
			*message_result = Message_CantLoadKernel32;
		}
		if (result != nullptr)
		{
			*result = -5;
		}
		return -6;
	}
	else
	{
		GetTickCount64_PTR Tick64 = nullptr;
		if (Probe64)
		{
			Tick64 = (GetTickCount64_PTR)GetProcAddress(kernel32, "GetTickCount64");
		}
		if (Tick64 != nullptr)
		{
			*out = Tick64();
		}
		else
		{
			*out = GetTickCount();
		}
		FreeLibrary(kernel32);
	}
	return -99;
}

bool ReportUpTimeAsExitCode(int* result, const char** message_result, const char* argv[], int argc)
{
	ULONGLONG update;
	GetUptime(&update, nullptr, nullptr);
	*result = (int)update;
	return true;
}

bool ReportUpTimeToStdout(int* result, const char** message_result, const char* argv[], int argc)
{
	ULONGLONG update=0;
	GetUptime(&update, nullptr, nullptr);
	if (update == 0)
	{
		if (message_result != nullptr)
		{
			*message_result = "Failed to get uptime";
		}
		if (result != nullptr)
		{
			*result = -1;
		}
		return false;
	}
	else
	{
		char* local = nullptr;
		int size = 0;
		bool res = NumberToString(update, &local, &size);
		if (res)
		{
			WriteStdout("Uptime (millseconds): ");
			WriteStdout(local);
			LocalFree(local);
			return true;
		}
		return false;
	}
}

