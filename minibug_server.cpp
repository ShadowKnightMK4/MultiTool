#include "common.h"
#include "osver.h"
#include "IAT_JOBS.H"
#include "Support/LWAnsiString/LWAnsiString.h"
extern "C" {

	struct Args
	{
		char* source;
		char* argument;
		VOID* mode;
		MinibugCallBack callback;
	};




	/*
	typedef HANDLE(WINAPI* CreateJobAnsi)(VOID*, VOID*);
	typedef HANDLE(WINAPI* AssignToJob)(HANDLE, HANDLE);
	typedef BOOL(WINAPI* SetJobLimit)(HANDLE, JOBOBJECTINFOCLASS, VOID*, DWORD);
	*/

	void AssignBasicLimits(HANDLE Job, SetJobLimit* AssignLimiter, MyOSVERSIONINFO* VersionInfo)
	{
		_JOBOBJECT_EXTENDED_LIMIT_INFORMATION  Limits;

		volatile  unsigned char* l = (unsigned char*)&Limits;
		for (size_t x = 0; x < sizeof(Limits); x++)
		{
			l[x] = 0;
		}

		Limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
		Limits.BasicLimitInformation.ActiveProcessLimit = 1;



		Limits.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
		(*AssignLimiter)(Job, JobObjectBasicLimitInformation, &Limits, sizeof(Limits));
	}

	void AssignUiRestrict(HANDLE Job, SetJobLimit* AssignLimiter, MyOSVERSIONINFO* VersionInfo)
	{
		JOBOBJECT_BASIC_UI_RESTRICTIONS Limits;
		Limits.UIRestrictionsClass |= JOB_OBJECT_UILIMIT_DESKTOP | JOB_OBJECT_UILIMIT_DISPLAYSETTINGS | JOB_OBJECT_UILIMIT_EXITWINDOWS | JOB_OBJECT_UILIMIT_GLOBALATOMS | JOB_OBJECT_UILIMIT_HANDLES | JOB_OBJECT_UILIMIT_READCLIPBOARD | JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS;
		(*AssignLimiter)(Job, JobObjectBasicUIRestrictions, &Limits, sizeof(Limits));
	}

	void AsignMemoryRestrict(HANDLE Job, SetJobLimit* AssignLimiter, MyOSVERSIONINFO* VersionInfo)
	{
	}

	void AssignNetworkRestrict(HANDLE Job, SetJobLimit* AssignLimiter, MyOSVERSIONINFO* VersionInfo)
	{
		bool ok = false;
		if (VersionInfo->W.dwMajorVersion > 6)
		{
			ok = true;
		}
		else if (VersionInfo->W.dwMajorVersion == 6)
		{
			if (VersionInfo->W.dwMinorVersion > 3)
			{
				ok = true; // strickly speaking, msdn doc suggests 6.4++ shouldn't exist. probably should add a check
			}
		}
		else
		{
			ok = false;
		}

		if (ok)
		{
			// try it
			JOBOBJECT_NET_RATE_CONTROL_INFORMATION Limits;
			for (size_t x = 0; x < sizeof(JOBOBJECT_NET_RATE_CONTROL_INFORMATION); x++)
			{
				((char*)&Limits)[x] = 0;
			}
			Limits.MaxBandwidth = Limits.DscpTag = Limits.ControlFlags = (JOB_OBJECT_NET_RATE_CONTROL_FLAGS)0;

			Limits.MaxBandwidth = 0;//
			Limits.ControlFlags = JOB_OBJECT_NET_RATE_CONTROL_ENABLE | JOB_OBJECT_NET_RATE_CONTROL_MAX_BANDWIDTH;
			(*AssignLimiter)(Job, JobObjectNetRateControlInformation, &Limits, sizeof(Limits));
		}
	}

	VOID AssignMinJobRules(HANDLE Job, SetJobLimit* AssignLimiter, MyOSVERSIONINFO* VersionInfo)
	{
		AssignBasicLimits(Job, AssignLimiter, VersionInfo);
		AssignUiRestrict(Job, AssignLimiter, VersionInfo);
		AsignMemoryRestrict(Job, AssignLimiter, VersionInfo);
		AssignNetworkRestrict(Job, AssignLimiter, VersionInfo);
	}
	/// <summary>
	/// This role is to dynamically assign most restricted job abiity to the debugged app based on osver data47
	/// </summary>
	/// <param name="Process"></param>
	/// <returns></returns>
	/// <remarks>call IAT_DynamicLinkJob first</remarks>
	HANDLE AssignAsJob(HANDLE Process, MyOSVERSIONINFO* VersionInfo)
	{
		HANDLE JobObject = INVALID_HANDLE_VALUE;


		IAT_DynamicLinkJob(IAT_JOB_ALL_W | IAT_JOB_ALL_A);
	

			JobObject = (*IAT_CreateJobObjectA)(NULL, NULL);

			if (JobObject != 0)
			{
				if ((*IAT_AssignProcessToJobObject)(JobObject, Process))
				{
					AssignMinJobRules(JobObject, &IAT_SetInformationJobObject, VersionInfo);
				}
			}



			return JobObject;
		
	}


	ULONG _stdcall WorkerThread(LPVOID ptr_ptr)
	{
		Args ptr;
		ptr.argument = ((Args*)ptr_ptr)->argument;
		ptr.callback = ((Args*)ptr_ptr)->callback;
		ptr.mode = ((Args*)ptr_ptr)->mode;
		ptr.source = ((Args*)ptr_ptr)->source;
		if (ptr_ptr != 0) HeapFree(GetProcessHeap(), 0, ptr_ptr);

		if (IAT_DynamicLinkJob(IAT_JOB_CREATEJOBA) != IAT_JOB_CREATEJOBA)
		{
			return -1;
		}
		
		MyOSVERSIONINFO ThreadInfo;
		if (!VERSION_INFO_WAS_GOTTON)
			FetchVersionInfo(&ThreadInfo, &VERISON_INFO_IS_UNICODE); // note this is the same routien osver tool uses.

		STARTUPINFOA StartInfo;
		PROCESS_INFORMATION PInfo;
		StartInfo.cb = sizeof(STARTUPINFO);
		StartInfo.hStdError = StartInfo.hStdInput = StartInfo.hStdOutput = INVALID_HANDLE_VALUE;
		StartInfo.lpReserved = 0;
		StartInfo.lpDesktop = 0;
		StartInfo.lpTitle = 0;
		StartInfo.dwX = 0;
		StartInfo.dwY = 0;
		StartInfo.dwXSize = StartInfo.dwYSize = 0;
		StartInfo.dwXCountChars = StartInfo.dwYCountChars = 0;
		StartInfo.dwFlags = 0;
		StartInfo.wShowWindow = SW_NORMAL;
		StartInfo.cbReserved2 = 0;
		StartInfo.lpReserved2 = 0;
		LWAnsiString* args = LWAnsiString_CreateString(1);
		if (args == 0)
		{
			ExitThread(254);
		}
		LWAnsiString_AppendA(args, "\"");
		LWAnsiString_AppendA(args, ptr.source);
		LWAnsiString_AppendA(args, "\" ");
		LWAnsiString_AppendA(args, ptr.argument);

		BOOL Res = CreateProcessA(ptr.source, args->AnsiData, nullptr, nullptr, FALSE, DEBUG_PROCESS | CREATE_SUSPENDED, 0, 0, &StartInfo, &PInfo);

		if (!Res)
		{
			ExitThread(255);
		}
		else
		{
			HANDLE JobObject = INVALID_HANDLE_VALUE;
			JobObject = AssignAsJob(PInfo.hProcess, &ThreadInfo);
			BOOL ReceivedExitEvent = FALSE;
			ResumeThread(PInfo.hThread);
			CloseHandle(PInfo.hThread);

			DEBUG_EVENT Event;

			/* our general plan is callback returns true and also is reponsible for conitnue debug event.
			* we just go if false, kill the process and quit.
			*/
			DWORD Reply = 0;
			while (true)
			{
				
				SetLastError(0);
				if (WaitForDebugEvent(&Event, 200))
				{
					if (ptr.callback(&Event, ptr.mode, &Reply) == 0)
					{
						// caller tertined falc
						TerminateProcess(PInfo.hProcess, 0);
						break;
					}
					else
					{
						ContinueDebugEvent(Event.dwProcessId, Event.dwThreadId, Reply);
					}
					if (Event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
					{
						ReceivedExitEvent = true;
					}
				}
				else
				{
					if (ReceivedExitEvent)
					{
						break;
					}
				}
			}
			if (JobObject != INVALID_HANDLE_VALUE)
			{
				CloseHandle(JobObject);
			}
		}
		if (args != 0)
		{
			LWAnsiString_FreeString(args);
		}
		return 0;
	}


	BOOL DefaultMiniBugHandler(DEBUG_EVENT* event, VOID*, DWORD* SendBack)
	{
		if (event->dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
		{
			*SendBack = DBG_EXCEPTION_NOT_HANDLED;
		}
		else
		{
			*SendBack = DBG_CONTINUE;
		}

		return TRUE;
	}

	BOOL StartMiniBugServer(const char* source, const char* argument, VOID* Mode, MinibugCallBack DebugEventHandler, HANDLE* WorkerThreadHandler)
	{
		Args* work_args = (Args*)HeapAlloc(GetProcessHeap(), 0, sizeof(Args));
		if (work_args == 0)
		{
			return FALSE;
		}
		work_args->argument = (char*)argument;
		work_args->callback = DebugEventHandler;
		work_args->mode = Mode;
		work_args->source = (char*)source;

		HANDLE ret = CreateThread(nullptr, 0, WorkerThread, work_args, 0, 0);
		if (WorkerThreadHandler != 0)
			*WorkerThreadHandler = ret;


		if (ret != 0)
		{
			return TRUE;
		}
		return FALSE;
	}




}