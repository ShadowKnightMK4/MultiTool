#include "common.h"
#include "osver.h"
#include "Support/LWAnsiString/LWAnsiString.h"
#include <TlHelp32.h>

extern "C" {
	HMODULE kernel32 = 0;
	typedef DWORD (WINAPI *GetFinalPathNameByHandleA_callback)(
		 HANDLE hFile,
		 LPSTR  lpszFilePath,
		  DWORD  cchFilePath,
		  DWORD  dwFlags
	);
	GetFinalPathNameByHandleA_callback vista_help = 0;
	struct args
	{
		const char* target_dll;
		LWAnsiString* FinalName;
	};

	LWAnsiString* ToolKitDllFind(DEBUG_EVENT* EV)
	{
		bool Found = 0;
		auto ret =LWAnsiString_CreateString(255);
		HANDLE SnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, EV->dwProcessId);

		MODULEENTRY32 Walking;

		if (Module32First(SnapShot, &Walking))
		{

			while (Module32Next(SnapShot, &Walking))
			{
				if (EV->u.LoadDll.lpBaseOfDll == Walking.modBaseAddr)
				{
					// BINGO!
					Found = TRUE;
					LWAnsiString_AppendA(ret, Walking.szModule);
					break;
				}
			}

		}

		if (!Found)
		{
			CloseHandle(SnapShot);
			HANDLE SnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE32, EV->dwProcessId);

			if (Module32First(SnapShot, &Walking))
			{

				while (Module32Next(SnapShot, &Walking))
				{
					Found = TRUE;
					LWAnsiString_AppendA(ret, Walking.szModule);
					break;
				}
			}

		}

		if (SnapShot != 0) CloseHandle(SnapShot);
		return ret;
	}

	LWAnsiString* read_remote_var_string(HANDLE Process, LPVOID Target, BOOL IsUnicode)
	{
		SIZE_T Size = 2;
		LWAnsiString* ret = 0;

		{
			DWORD ReadBytes = 0;
	
			while (true)
			{
				ret  = LWAnsiString_CreateString(255);
				SetLastError(0);
				if (ReadProcessMemory(Process, Target, ret->Data, ret->AllocatedSize, &ReadBytes))
				{
					LWAnsiString_ProbeLength(ret);
					if (ReadBytes == 255)
					{

						// we quite frankly don't know how big it is.
						Size *= 2;
						LWAnsiString_ZeroString(ret);
						LWAnsiString_Reserve(ret, Size);
					}
					else
					{
						return ret;
					}
				}
				else
				{
					return nullptr;
				}
			}
		}
		return nullptr;
	}

	LWAnsiString* ParanoidReadDllNameFallback(DEBUG_EVENT* EV)
	{
		union u
		{
			CHAR* ansi;
			wchar_t* uni;
		} string_ptr;
		DWORD StringSize = 0;
		string_ptr.uni = 0;
		LWAnsiString* ret = 0;
		if (EV->dwDebugEventCode == LOAD_DLL_DEBUG_EVENT)
		{
			if (EV->u.LoadDll.lpImageName != 0)
			{
				DWORD size_read = 0;
				HANDLE Target = OpenProcess(PROCESS_VM_READ, FALSE, EV->dwProcessId);
				if (Target != 0)
				{
					if (ReadProcessMemory(Target, EV->u.LoadDll.lpImageName, &string_ptr.uni, sizeof(VOID*), &size_read))
					{
						if (string_ptr.uni == 0)
						{
							ret = ToolKitDllFind(EV);
						}
						else
						{
								ret = read_remote_var_string(Target, EV->u.LoadDll.lpImageName, EV->u.LoadDll.fUnicode);
						
						}
					}
					else
					{
						ret = ToolKitDllFind(EV);
					}
					CloseHandle(Target);
				}
			}
		}
		// we try lpImageName here and if null, toolsnapshot attempt
		return ret;
	}
	LWAnsiString* ParanoidReadDllName(DEBUG_EVENT* ev)
	{
		LWAnsiString* ret = LWAnsiString_CreateString(1);
		if (ev == 0)
			return 0;
		if (ev->dwDebugEventCode != LOAD_DLL_DEBUG_EVENT)
			return 0;
		else
		{
			
			if ( (ev->u.LoadDll.hFile != 0) && (ev->u.LoadDll.hFile != INVALID_HANDLE_VALUE))
			{
				if (vista_help != 0)
				{
					DWORD size_needed = (vista_help)(ev->u.LoadDll.hFile, 0, 0, VOLUME_NAME_DOS);
					if (size_needed != 0)
					{
						LWAnsiString_Reserve(ret, size_needed + 1);
						(vista_help)(ev->u.LoadDll.hFile, ret->AnsiData, size_needed + 1, VOLUME_NAME_DOS);
						LWAnsiString_ProbeLength(ret);
						CloseHandle(ev->u.LoadDll.hFile);
						return ret;
					}
				}
				else
				{
					if (kernel32 == 0)
					{
						kernel32 = LoadLibraryA("kernel32.dll");
					}
					vista_help = (GetFinalPathNameByHandleA_callback)GetProcAddress(kernel32, "GetFinalPathNameByHandleA");
					if (vista_help != 0)
					{
						DWORD size_needed = (vista_help)(ev->u.LoadDll.hFile, 0, 0, VOLUME_NAME_DOS);
						if (size_needed != 0)
						{
							LWAnsiString_Reserve(ret, size_needed + 1);
							(vista_help)(ev->u.LoadDll.hFile, ret->AnsiData, size_needed + 1, VOLUME_NAME_DOS);
							LWAnsiString_ProbeLength(ret);
							CloseHandle(ev->u.LoadDll.hFile);
							return ret;
						}
					}
				}
				auto tmp = ParanoidReadDllNameFallback(ev);
				LWAnsiString_AppendA(ret, LWAnsiString_ToCStr(tmp));
				// if we make it here, it didn't work.

				// also close it. Set to zero.
				CloseHandle(ev->u.LoadDll.hFile);
				ev->u.LoadDll.hFile = INVALID_HANDLE_VALUE;
				return ret;
			}
			else
			{
				;
			}
		}
	}

	// we do terminate on first breakpoint or if we find target.
	BOOL WINAPI terminate_on_find_target(DEBUG_EVENT* event, VOID* User, DWORD* ReplyName)
	{
		*ReplyName = DBG_CONTINUE;
		bool KeepDebugThingAlive = true;
		switch (event->dwDebugEventCode)
		{
		case EXCEPTION_DEBUG_EVENT:
		{
			if (event->u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
			{
				KeepDebugThingAlive = true;
				*ReplyName = DBG_CONTINUE;
				//ContinueDebugEvent(event->dwProcessId, event->dwThreadId, DBG_CONTINUE);
			}
			else
			{
				*ReplyName = DBG_EXCEPTION_NOT_HANDLED;
				//ContinueDebugEvent(event->dwProcessId, event->dwThreadId, DBG_EXCEPTION_NOT_HANDLED);
			}
			break;
		}
		case EXIT_PROCESS_DEBUG_EVENT:
		{
			KeepDebugThingAlive = false;
			return DefaultMiniBugHandler(event, User, ReplyName);
		}
		case LOAD_DLL_DEBUG_EVENT:
		{
			((args*)User)->FinalName = ParanoidReadDllName(event);
			
			if ( (((args*)User)->FinalName != 0) && (((args*)User)->FinalName->A[0] != 0))
			{
				if (LWAnsiString_EndsWith(((args*)User)->FinalName, ((args*)User)->target_dll, false))
				{

					KeepDebugThingAlive = false;
				}
				else
				{
					LWAnsiString_FreeString(((args*)User)->FinalName);
					((args*)User)->FinalName = 0;
					KeepDebugThingAlive = true;
				}
			}
			
			break;
		}
			default: return DefaultMiniBugHandler(event, User, ReplyName);
		}

		return  KeepDebugThingAlive;
	}
	bool whichdll_entrypoint(int* result, const char** message_result, const char* argv[], int argc)
	{
		const char* test_dll;
		if ((result == 0) && (message_result == 0))
		{
			return false;
		}
		if (argc < 3)
		{
			return false;
		}
		test_dll = argv[2];

		args Args;
		Args.target_dll = test_dll;
		Args.FinalName = 0; // the thread will allocate it.
		HANDLE WorkerThread = 0;
		if (StartMiniBugServer("C:\\Users\\Thoma\\source\\repos\\MultiTool\\Debug\\MiniBugClient.exe", test_dll, &Args, terminate_on_find_target, &WorkerThread))
		{
			WaitForSingleObject(WorkerThread, INFINITE);
			CloseHandle(WorkerThread);
		}

		if (Args.FinalName == 0)
		{
			WriteStdout("DLL: ");
			WriteStdout(Args.target_dll);
			WriteStdout(" was not found on attempt to load.\r\n");
		}
		else
		{
			WriteStdout("DLL: ");
			WriteStdout(Args.target_dll);
			WriteStdout(" Found by loader ----> [");
			WriteStdout(LWAnsiString_ToCStr(Args.FinalName));
			WriteStdout("]\r\n");
		}
	}

}