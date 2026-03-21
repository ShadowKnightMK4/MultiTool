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
	const char* description;
};
const char* feature_disabled = "This Feature is Disabled or not supported.";

bool Disabled(int* result, const char** message_result, const char* argv[], int argc)
{
	*result = -1;
	*message_result = feature_disabled;
	return true;
}

bool NoSupport(int* result, const char** message_result, const char* argv[], int argc)
{
	*result = -1;
	*message_result = feature_disabled;
	return true;
}

bool ShowHelp_(int* result, const char** message_result, const char* argv[], int argc);

ToolEntry Entries[19] =
{
	{ "-EmptyRecyling", EmptyBin, "Empty the Recylling bin"},
	{ "-OsVer", ReportVersionStdout, "Report what version of Windows is running to stdout."},
	{ "-osMajor", ReportVersionMajorViaExit , "Report the major version of Windows as exit code. "},
	{ "-osMinor", ReportVersionMinorViaExit , "Report the minor version of Windows as exit code. "},
	{ "-osBuild", ReportVersionBuildViaExit , "Report the build verison Windows as exit code. "},
	{ "-osPlatform", ReportVersionPlatformIDViaExit , "Report if Windows ix NT based or older."},
	{ "-killprocess", KillProcess , "Terminate the target process. Tries playing nice 1st"},
	{ "-upTime", ReportUpTimeToStdout , "How long has this system been up for."},
	{ "-upTimeExitCode", ReportUpTimeAsExitCode , "Report length of time system running as exit code cap to 32-bit"},
	{ "-whoami", WhoAmI_WriteStdout,  "Short for decoding current token of this application (and user)"},
	{ "-whoami_priv", WhoAmi_WriteStdout_Priv, "What privlidges does this application have?"},
	{ "-?", ShowHelp_, "Show a quick help of all available commands"},
	{ "-elevated", WhoAmi_Writestdout_TokenElevatedQuestion, "Check if this app is running full admin or not."},
#ifdef EXPERIMENT
	{ "-whoami_priv_system", WhoAmi_WriteStdout_PrivSystemToken ,  "Open hard coded winlogon.exe target and use as target of whoami."},
	{ "-whoami_user_group", WhoAmi_WriteStdout_UserGroups ,  "Output Group info of the user."},
	{ "-which", SearchPath_EntryPoint, "When given a file/folder, asks windows where it's at."},
#else
	{ "-whoami_priv_system", Disabled , feature_disabled },
	{ "-whoami_user_group", WhoAmi_WriteStdout_UserGroups ,  "Output Group info of the user."},
	{ "-which", SearchPath_EntryPoint, "Find out where in the path a series on names is"},
#endif
	{ "-processprofile", ProcessProfileEntryPoint, "Run Process Profile on self"},
	{ "-checkSafeLoadPath", CheckSafeLoadPath_PipeStdout, "Check if SafeDllSearchMode is active or available and report to stdout."},
	{0, 0, feature_disabled}
};


bool ShowHelp_(int* result, const char** message_result, const char* argv[], int argc)
{
	LWAnsiString* output = LWAnsiString_CreateFromString("Available Commands For more info, consult help file:\r\n");
	if (output == nullptr)
	{
		if (message_result != nullptr)
		{
			*message_result = "Failed to create output string";
		}
		if (result != nullptr)
		{
			*result = -1;
		}
		return false;
	}
	for (int i = 0; ; i++)
	{
		if (Entries[i].flag_name != 0)
		{
			
			if ((Entries[i].description != 0) && ((Entries[i].FunctionPointer != 0) && (Entries[i].FunctionPointer != Disabled) && (Entries[i].FunctionPointer != NoSupport)))
			{
				LWAnsiString_PadNewLine(output, '=', 20); // pad the flag name to 20 chars
				LWAnsiString_AppendWithNewLine(output, Entries[i].flag_name);
				LWAnsiString_PadNewLine(output, '=', 20); // pad the flag name to 20 chars
				LWAnsiString_AppendWithNewLine(output, Entries[i].description);
				LWAnsiString_PadNewLine(output, '=', 20); // pad the flag name to 20 chars
				LWAnsiString_AppendNewLine(output);
			}
			LWAnsiString_Append(output, "\r\n");
		}
		else
		{
			break;
		}
	}
	WriteStdout(LWAnsiString_ToCStr(output));
	if (output != 0) LWAnsiString_FreeString(output);
	return true;
}

/// <summary>
/// lookup the function pointer for the given flag name
/// </summary>
/// <param name="flag_name"></param>
/// <returns></returns>
/// <remarks> ai generated but seems to work</remarks>
ToolFunction GetFunctionPointer(const char* flag_name)
{

	for (int i = 0; ; i++)
	{
		if (Entries[i].flag_name != 0)
		{
			if (lstrcmpiA(flag_name, Entries[i].flag_name) == 0)
			{
				return Entries[i].FunctionPointer;
			}
		}
		else
		{
			break;
		}
		
	}
	return nullptr;
}

const char* GetFunctionHelp(const char* flag_name)
{

	for (int i = 0; i < sizeof(Entries) / sizeof(Entries[0]); i++)
	{
		if (lstrcmpiA(flag_name, Entries[i].flag_name) == 0)
		{
			return Entries[i].description;
		}
	}
	return nullptr;
}