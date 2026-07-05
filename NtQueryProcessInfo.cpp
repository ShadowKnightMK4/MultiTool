#include "common.h"
#include "IAT_NTDLL.H"
#include "osver.h"
#include "ntstatus.h"

typedef struct _PROCESS_BASIC_INFORMATION_NT_MSDN_PAGE {
	NTSTATUS ExitStatus;
	PPEB PebBaseAddress;
	ULONG_PTR AffinityMask;
	KPRIORITY BasePriority;
	ULONG_PTR UniqueProcessId;
	ULONG_PTR InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION_MSDN;


void subProcessBasicInfoPriority(KPRIORITY Pri, LWAnsiString* output)
{
	LWAnsiString_AppendNumberA(Pri, output, 0);
	LWAnsiString_AppendNewLineA(output);
}
void subProcessBasicInfo(HANDLE PID, LWAnsiString* output)
{
	PROCESS_BASIC_INFORMATION_MSDN Info = { 0 };
	ULONG len = 0;
	if (IAT_NtQueryInformationProcess(PID, ProcessBasicInformation, (VOID*)&Info, sizeof(Info), &len) == STATUS_SUCCESS)
	{
		LWAnsiString_AppendA(output, "Process Basic Info:\r\n");
		LWAnsiString_AppendA(output, "\tExitCode => ");
		LWAnsiString_AppendNumberA(Info.ExitStatus, output, 0);
		LWAnsiString_AppendNewLineA(output);

		LWAnsiString_AppendA(output, "\PED => Skipped for now:\r\n");

		LWAnsiString_AppendA(output, "\tAffinity (MSDN GetProcessAffinityMask)=> ");
		LWAnsiString_AppendNumberA(Info.AffinityMask, output, 0);
		LWAnsiString_AppendNewLineA(output);

		LWAnsiString_AppendA(output, "\tPriority => (MSDN Scheduling Priorities)=>");
		subProcessBasicInfoPriority(Info.BasePriority, output);

		LWAnsiString_AppendA(output, "\tPID => Skipped For now ");
		LWAnsiString_AppendNewLineA(output);


		LWAnsiString_AppendA(output, "\tParent PID =>  ");
		LWAnsiString_AppendNumberA(Info.InheritedFromUniqueProcessId, output, 0);
		LWAnsiString_AppendNewLineA(output);


	}
}
bool RunNTQueryProcessInfo(int* result, const char** message_result, const char* argv[], int argc)
{
	if (argc < 3)
	{
		return false;
	}
	FetchVersionInfo(&GlobalVersionInfo, &VERISON_INFO_IS_UNICODE);
	if (GlobalVersionInfo.A.dwPlatformId != VER_PLATFORM_WIN32_NT)
	{
		WriteStderr("Sorry: NtQueryProcessInfo is for NT based systems only (most of modern Windows)");
	}
	else
	{
		if (IAT_DynamicLink_NTDLL(IAT_NTDLL_LINKING_NTQUERYINFOPROCESS) != IAT_NTDLL_LINKING_NTQUERYINFOPROCESS)
		{
			WriteStderr("Unable to link and find the required ntdll routine.... How!?");
		}
		LWAnsiString* output = LWAnsiString_CreateString(255);


		if (output != 0)
		{
			LWAnsiString_AppendA(output, "Getting Information about PID: ");
			LWAnsiString_AppendA(output, argv[3]);
			LWAnsiString_AppendNewLine(output);
			/*
			* arg0 = midas.exe
			* arg1 = -ntqueryprocss
			* arg2 = { all, or one off}
			* arg3 = PID
			*/
			DWORD PID = 0;
			if (!StringToNumber(argv[3],(int*) & PID))
			{
				LWAnsiString_AppendA(output, "Error converting to a string. \r\n");
				goto clean;
			}
			{
				HANDLE x = OpenProcess(GENERIC_READ, FALSE, PID);
				if (x == 0)
				{
					LWAnsiString_AppendA(output, "Access was denied or it's a system process.\r\n");
				}
				else
				{
					if (lstrcmpiA(argv[2], "-all"))
					{
						LWAnsiString_AppendA(output, "All Available Info\r\n");
					}

					subProcessBasicInfo(x, output);
					CloseHandle(x);
					WriteStdout(output);
				}
			}


			clean:
			LWAnsiString_FreeString(output);
		}
	}
}