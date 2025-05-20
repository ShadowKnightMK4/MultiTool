#include "common.h"

bool DebugSnipe(int PIDLIST, int* result, const char** message_result)
{
	// al we care about is attaching as debugger and if we works. Windows will kill said process when we exit
	if (DebugActiveProcess(PIDLIST))
	{
		*message_result = "Attached as Debugger to Target. It should exit when we do so";
		*result = 0;
		return true;
	}
	else
	{
		*message_result = "Failed to Attach as debugger to attampt termination. It won't exit when we do so";
		*result = GetLastError();
		return false;
	}
}
bool SnipeViaPID(int PIDList, int*result, const char** message_result)
{
	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, PIDList);
	if (hProcess == NULL)
	{
		*message_result = "Either we don't have permssion to open the process or the PID is wrong";
		return false;
	}
	else
	{
		if (TerminateProcess(hProcess, 0) )
		{
			*message_result = "Process Terminated";
			*result = 0;
			CloseHandle(hProcess);
			return true;
		}
		else
		{
			return DebugSnipe(PIDList, result, message_result);
		}
	}
	
}
bool KillProcess(int* result, const char** message_result, const char* argv[], int argc)
{
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
		for (int i = 0; i < collection_size; i++)
		{
			int PID = 0;
			if (StringToNumber(collection[i], &PID))
			{
				if (PID != 0)
				{
					if (!SnipeViaPID(PID, result, message_result))
					{
						WriteStderr(*message_result);
						some_fail = true;
					}
					else
					{
						WriteStdout(*message_result);
					}
				}
			}
			else
			{
			}
		}
	}
}