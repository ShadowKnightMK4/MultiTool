#include "common.h"

/*
	This file is reached via main.cpp.		

	Essentually the plan is loop thru the command line until we get one we know, then branch to the right one.   
	All commands follow the same protocol typedef bool (*ToolFunction)(int* result, const char** message_result, const char* argv[]); localed in tool_dipatch.h



	They should return true on sucess and false on fail.
	They shouldn't assume result is not null and play defensive BUT if it's not null, main.cpp uses that as the exit code passed to ExitProcess().
	They will get a pointer to a char* that they can set for a message to send.  Note if the tool returns false, stderr.  If the tool returns true, stdout.
	argv[] is yep that argv[] from main.cpp.  It's free to inspect and determine how it will read and be effected by it.  Note it's const and and should be treated as such..


*/
struct ToolEntry
{
	const char* flag_name;
	ToolFunction FunctionPointer;
};

ToolEntry Entries[100] =
{
	{ "-EmptyRecyling", EmptyBin },
	{ "-osver", ReportVersionStdout },
	{ "-osMajor", ReportVersionMajorViaExit },
	{ "-osMinor", ReportVersionMinorViaExit },
	{ "-osBuild", ReportVersionBuildViaExit },
	{ "-osPlatform", ReportVersionPlatformIDViaExit },
	{ "-killprocess", KillProcess },
	{ "-upTime", ReportUpTimeToStdout },
	{ "-upTimeExitCode", ReportUpTimeAsExitCode },
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