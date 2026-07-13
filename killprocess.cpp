#include "common.h"
#include "TlHelp32.h"
#include "osver.h"
#include "IAT_PRIV.h"
#include "IAT_NTDLL.H"
#include "PEHeaderHandling.h"
#include <ntstatus.h>
bool _cdecl StringToNumber_ForKillProcess(const char* input, int* output);




bool RemoteExitProcess(int PID, LWAnsiString* output)
{
	SetLastError(0);
	HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, PID);

	if (hProcess != 0)
	{
		if (output != 0)
		{
			LWAnsiString_AppendA(output, "Process opened ok with the needed permssions\r\n");
			LWAnsiString_AppendA(output, "Notice; CUrrently this mode is not implemented yet\r\n");
		}
		CloseHandle(hProcess);
		return false;
		
	}
	else
	{
		if (output != 0)
		{
			LWAnsiString_AppendA(output, "Process couldn't be opened with the needed permssions\r\n");
		}
		return false;
	}

}

bool DebugSnipe(int PIDLIST, int* result, const char** message_result, LWAnsiString* output)
{
	if (output != 0)
	{
		LWAnsiString_AppendA(output, "Attempting to attach as a debugger (which on exit of will have Windows itself end the process):  ");
	}

	SetLastError(0);
	if (DebugActiveProcess((DWORD)PIDLIST) && (GetLastError() == 0))
	{
		if (output)
		{
			LWAnsiString_AppendA(output, "=> SUCESS\r\n");
		}
		if (DebugSetProcessKillOnExit(TRUE) && (GetLastError() == 0))
		{
			LWAnsiString_AppendA(output, "=> Requested Windows to end the target once midas exits\r\n");
			// we're done
			return true;
		}
	}

	if (output != 0)
	{
		LWAnsiString_AppendA(output, "Could not attach as debugger. Could be ACLs/permission or target is not same bitness as midas.\r\n");
	}
	return false;
}
bool SnipeViaPID(int PIDList, int*result, const char** message_result, LWAnsiString* output)
{
	bool ret = false;
	if (output != 0)
	{
		LWAnsiString_AppendA(output, "Attempting to open handle to process: ");
	}
	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, PIDList);
	if (hProcess == NULL)
	{
		if (output != 0)
		{
			LWAnsiString_AppendA(output, "=>   FAILD!\r\n");
		}
		*message_result = "Either we don't have permssion to open the process or the PID is wrong";
		return ret;
	}
	else
	{
		if (output != 0)
		{
			LWAnsiString_AppendA(output, "=>   SUCCESS!\r\n");
		}
		
		if (TerminateProcess(hProcess, 0) )
		{
			if (output != 0)
			{
				LWAnsiString_AppendA(output, "Attempting call to directly terminate process worked. Confirming process is DOA\r\n");
			}

			DWORD await = WaitForSingleObject(hProcess, 250);
			
			if (await != WAIT_OBJECT_0)
			{
				if (output != 0)
				{
					LWAnsiString_AppendA(output, "Waiting for the process to exit reveals it's still alive.\r\n");
				}
				ret = false;
			}
			else
			{
				if (output != 0)
				{
					LWAnsiString_AppendA(output, "Waiting for the process to exit reveals it's still doa.\r\n");
				}

				ret = true;
			}
		}
		else
		{
			if (output != 0)
			{
				LWAnsiString_AppendA(output, "Attempting call to directly terminate process failed.\r\n");
			}
		}
		CloseHandle(hProcess);
		return ret;
	}
}

bool AskForDebugPriv()
{
	bool ret = false;
	if ((IAT_DynamicLink_TokenPriv_Advapi32(IAT_PRIV_LINKING_ADJUSTTOKENPRIV | IAT_PRIV_LINKING_LOOKUPPRIVA | IAT_PRIV_LINKING_OPENPROCESSTOKEN)) == (IAT_PRIV_LINKING_LOOKUPPRIVA| IAT_PRIV_LINKING_ADJUSTTOKENPRIV | IAT_PRIV_LINKING_OPENPROCESSTOKEN))
	{
		TOKEN_PRIVILEGES Privs;
		LUID luid;
		HANDLE self = 0;
		SetLastError(0);
		if (IAT_OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &self))
		{
			if (IAT_LookupPrivilegeValueA(0, SE_DEBUG_NAME, &luid))
			{
				Privs.PrivilegeCount = 1;
				Privs.Privileges[0].Luid = luid;
				Privs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

				if (IAT_AdjustTokenPriv(self, false, &Privs, 0, 0, 0))
				{
					if (GetLastError() != ERROR_NOT_ALL_ASSIGNED)
					{
						if (GetLastError() == 0)
							ret = true;
						else
							ret = false;
					}
					else
						ret = false;
				}
			}

			CloseHandle(self);
		}

	}
	return ret;
}

bool SnipeViaPID_NtOnly(int PIDList, int* result, const char** message_result, LWAnsiString* output)
{
	if (output)
	{
		LWAnsiString_AppendA(output, "Attempting to use direct call to ntdll to terminate the process\r\n");
	}

	{
		bool ret = false;
		if (IAT_NtTerminateProcess == 0)
		{
			if (output)
			{
				LWAnsiString_AppendA(output, "Skipping the call, due to required routine not found in ntdll.dll \r\n");
			}
			return ret;
		}
		SetLastError(0);
		HANDLE hProcess = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, PIDList);
		if (hProcess != 0)
		{
			if (output)
			{
				LWAnsiString_AppendA(output, "Got Handle to Process ok\r\n");
			}
			auto ok = IAT_NtTerminateProcess(hProcess, 0);
			if (ok == STATUS_SUCCESS)
			{
				if (output)
				{
					LWAnsiString_AppendA(output, "Termiation request sent: ");
				}
				DWORD await = WaitForSingleObject(hProcess, 250);
				if (await == WAIT_OBJECT_0)
				{
					if (output)
					{
						LWAnsiString_AppendA(output, "Wait signled ok. Windows reports process is gone.\r\n");
					}
					ret = true;
				}
				else
				{
					if (await == WAIT_TIMEOUT)
					{
						if (output)
						{
							LWAnsiString_AppendA(output, "Wait Signal Timeout. Windows reports process is alive somehow!?\r\n");
						}
					}
					else
					{
						if (output)
						{
							LWAnsiString_AppendA(output, "Wait Signal Failure. Windows reports process is alive somehow.");
						}

					}
					ret = false;
				}
			}
			else
			{
				if (output)
				{
					LWAnsiString_AppendA(output, "NtTermiateProcess reports failure to send process exit request.");
				}
			}
			if (hProcess != 0) CloseHandle(hProcess);
		}
		else
		{
			if (output)
			{
				LWAnsiString_AppendA(output, "Failed to get handle to try exiting the process\r\n");
			}
		}
		return ret;
	}
}




bool KillProcess(int* result, const char** message_result, const char* argv[], int argc)
{
	LWAnsiString* output = LWAnsiString_CreateString(255);
	FetchVersionInfo(&GlobalVersionInfo, &VERISON_INFO_IS_UNICODE);

	bool some_fail = false;
	int collection_offset = 0;
	bool PID_MODE = false;
	const char** collection = nullptr;
	// what we're mostly doing is copying the command line args to a new array minus the first time and the /killprocess flag.
	// we loop thru checking for the -PID flag and if we find it, we set the PID_MODE flag to true.  We then copy the rest of the args to a new array.
	collection = (const char**)LocalAlloc(LMEM_ZEROINIT, sizeof(char*) * (argc));
	int collection_size = argc - 3;
	if (collection == nullptr)
	{
		return false;
	}
	else
	{
		for (int i = 2; i < argc; i++)
		{
			if (lstrcmpiA("-PID", argv[i]) == 0)
			{
				PID_MODE = true;
			}
			else
			{
				collection[collection_offset] = argv[i];
				collection_offset++;
			}
		}
	}

	if (PID_MODE)
	{
		bool DebugModeConfig = false;
		for (int i = 0; i < collection_size; i++)
		{
			int PID = 0;
			if (StringToNumber_ForKillProcess(collection[i], &PID))
			{
				LWAnsiString_AppendA(output, "Attempting to Stop Process with PID: ");
				LWAnsiString_AppendA(output, collection[i]);
				LWAnsiString_AppendNewLine(output);
				


				if (PID != 0)
				{
				debug_retry:
					LWAnsiString_AppendA(output, "Current Mode: DebugPriv ");
					if (DebugModeConfig == false)
					{
						LWAnsiString_AppendA(output, "[OFF]\r\n");
					}
					else
					{
						LWAnsiString_AppendA(output, "[ON]\r\n");
					}
					bool res = SnipeViaPID(PID, result, message_result, output);
					if (!res)
					{
						res = DebugSnipe(PID, result, message_result, output);
					}

					if (!res)
					{
						if (GlobalVersionInfo.A.dwPlatformId == VER_PLATFORM_WIN32_NT)
						{
							if (IAT_DynamicLink_NTDLL(IAT_NTDLL_LINKING_NTTERMINATEPROCESS) == IAT_NTDLL_LINKING_NTTERMINATEPROCESS)
							{
								// ok we're go for attmepting to terminate the process again
								res = SnipeViaPID_NtOnly(PID, result, message_result, output);
							}
						}
					}

					if (!res)
					{
			 			res = RemoteExitProcess(PID, output);
					}

					if (DebugModeConfig == false)
					{
						if (res == false)
						{
							if (!AskForDebugPriv())
							{
								LWAnsiString_AppendA(output, "Unable to enable SeDebugPriv. Try running as admin.\r\n");
								goto clean;
							}
							else
							{
								DebugModeConfig = true;
								goto debug_retry;
							}
						}
					}
				}
			}
			else
			{
			}
		}
	}
	clean:
	WriteStdout(output);
	LWAnsiString_FreeString(output);
	return true;
}