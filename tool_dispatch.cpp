#include "common.h"

struct ToolEntry
{
	const char* flag_name;
	ToolFunction FunctionPointer;
};

ToolEntry Entries[25] =
{
	{ "-EmptyRecyling", EmptyBin },
	{ "-osver", ReportVersionStdout },
	{ "-osMajor", ReportVersionMajorViaExit },
	{ "-osMinor", ReportVersionMinorViaExit },
	{ "-osBuild", ReportVersionBuildViaExit },
	{ "-osPlatform", ReportVersionPlatformIDViaExit },
	{ "-help", 0 },
	{ "--help", 0 },
	{0, 0}
};

/// <summary>
/// lookup the function pointer for the given flag name
/// </summary>
/// <param name="flag_name"></param>
/// <returns></returns>
/// <remarks> ai generated but seems to work</remarks>
ToolFunction GetFunctionPointer(const char* flag_name)
{
	for (int i = 0; i < sizeof(Entries) / sizeof(Entries[0]); i++)
	{
		if (lstrcmpiA(flag_name, Entries[i].flag_name) == 0)
		{
			return Entries[i].FunctionPointer;
		}
	}
	return nullptr;
}