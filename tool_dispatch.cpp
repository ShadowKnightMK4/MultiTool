#include "common.h"
#include "whoami.h"
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

bool Disabled(int* result, const char** message_result, const char* argv[], int argc)
{
	*result = -1;
	*message_result = "This Feature is Disabled.";
	return true;
}

bool NoSupport(int* result, const char** message_result, const char* argv[], int argc)
{
	*result = -1;
	*message_result = "This Feature is unsupported.";
	return true;
}


ToolEntry Entries[100] =
{
	{ "-EmptyRecyling", EmptyBin },
	{ "-OsVer", ReportVersionStdout },
	{ "-osMajor", ReportVersionMajorViaExit },
	{ "-osMinor", ReportVersionMinorViaExit },
	{ "-osBuild", ReportVersionBuildViaExit },
	{ "-osPlatform", ReportVersionPlatformIDViaExit },
	{ "-killprocess", KillProcess },
	{ "-upTime", ReportUpTimeToStdout },
	{ "-upTimeExitCode", ReportUpTimeAsExitCode },
	{ "-whoami", WhoAmI_WriteStdout},
	{ "-whoami_priv", WhoAmi_WriteStdout_Priv},
	{ "-help", NoSupport },
	{ "-elevated", WhoAmi_Writestdout_TokenElevatedQuestion},
#ifdef EXPERIMENT
	{ "-whoami_priv_system", WhoAmi_WriteStdout_PrivSystemToken },
	{ "-whoami_user_group", WhoAmi_WriteStdout_UserGroups },
#else
	{ "-whoami_priv_system", Disabled },
	{ "-whoami_user_group", Disabled },
#endif
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