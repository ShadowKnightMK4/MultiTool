#include "common.h"	
#include "osver.h"

bool ReportUpTimeToStdout(int* result, const char** message_result, const char* argv[], int argc)
{
	bool Probe64 = false;
	ULONGLONG uptime = 0;
	if (!VERSION_INFO_WAS_GOTTON)
		FetchVersionInfo(&GlobalVersionInfo, &VERISON_INFO_IS_UNICODE);
	if (!VERSION_INFO_WAS_GOTTON)
	{
		return false;
	}
	if ((GlobalVersionInfo.A.dwMajorVersion >= 6) && (GlobalVersionInfo.A.dwMinorVersion >= 0))
	{
		// Vista+. Dynamticly Preference is GetTickCount64.
	}
	else
	{
		// Vista minus.  Use GetTickCount.  Do see if we can try GetTickCount64 and if it fails, use GetTickCount
		Probe64 = true;
	}
	HMODULE kernel32 = LoadLibraryA("kernel32.dll");
	
	// check kernel failure.

	// define the typedef to call gettickcount64 and gettickcount.
	// try 64 first if probe64 is true, otherwise already do the gettickcount
}