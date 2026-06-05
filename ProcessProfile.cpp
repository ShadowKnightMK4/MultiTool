#include "common.h"
#include "osver.h"
#include "Support\LWAnsiString\LWAnsiString.h"

#include "TlHelp32.h"
#include "IAT_REGISTRY.H"

/*
* -processprofile.cpp   -target {PID|ProcessName} {PID|ProcessName} {PID|ProcessName} ... 
* 
* OR
* -processprofile.cpp	-self
* 
* 
*OR 
* -processprofile.cpp	-self 
*/

HMODULE PSAPI = nullptr;
typedef DWORD (WINAPI* GetModuleFileNameEXA_PTR)(HANDLE hProcess, HMODULE hModule, LPSTR lpFileName, DWORD nSize);


GetModuleFileNameEXA_PTR NT_FallBack = nullptr;
bool NTOK = false;


bool ResolveNTFallback()
{
	if (NTOK)
	{
		return true; // already resolved fallback
	}
	if (PSAPI == nullptr)
	{
		PSAPI = LoadLibraryA("psapi.dll");
		if (PSAPI == nullptr)
		{
			return false; // failed to load kernel32
		}
	}
	if (NT_FallBack == nullptr)
	{
		NT_FallBack = (GetModuleFileNameEXA_PTR)GetProcAddress(PSAPI, "GetModuleFileNameExA");
		if (NT_FallBack == nullptr)
		{
			return false; // failed to get GetModuleFileNameExA
		}
	}
	NTOK = true; // we resolved the fallback
	return true;
}




bool IsFolderWritable(const char* target)
{
	auto folder = LWAnsiString_CreateString(MAX_PATH);
	if (folder == nullptr)
	{
		return false; // failed to allocate memory for folder
	}
again:
	LWAnsiString_AppendA(folder, target);
	GetTempFileNameA(folder->AnsiData, "LWT", 0, folder->AnsiData);
	HANDLE tmp = CreateFileA(folder->AnsiData, GENERIC_WRITE , FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
	if (tmp != INVALID_HANDLE_VALUE)
	{
		CloseHandle(tmp);
		return true;
	}
	return false;
}
bool LoadAdvapi32Needs()
{
	return (IAT_DynamicLinkRegistry(IAT_REGISTRY_ANSI) & IAT_REGISTRY_ANSI) == IAT_REGISTRY_ANSI;
}


/// <summary>
/// Essentually module walks via toolhelp to just get main exe spawn point
/// </summary>
/// <param name="ID"></param>
/// <param name="output"></param>
/// <returns></returns>
bool ProcessProfileHelper_MainExe(DWORD ID, LWAnsiString** output)
{
	DWORD flag = TH32CS_SNAPMODULE32;
retry:
	HANDLE Snap = CreateToolhelp32Snapshot(flag, ID);
	if (Snap == INVALID_HANDLE_VALUE)
	{
		if (flag == TH32CS_SNAPMODULE32)
		{
			goto retry;
		}
		if (output != nullptr)
		{
			*output = nullptr;
		}
		return false;
	}
	MODULEENTRY32 MainEntry;
	MainEntry.dwSize = sizeof(MODULEENTRY32);
	if (Module32First(Snap, &MainEntry) == ERROR_SUCCESS)
	{
		*output = LWAnsiString_CreateFromString(MainEntry.szExePath);
	}
	if (Snap != 0)
		if (Snap != INVALID_HANDLE_VALUE)
			CloseHandle(Snap);

	return true;

}

bool ProcessProfileHelper_VerifySafeDllSearchFlag(bool *PostXpSpo2, bool *IsActive)
{
	/* are we post  XP-spo2?*/
	FetchVersionInfo(&GlobalVersionInfo, &VERISON_INFO_IS_UNICODE);
	if (PostXpSpo2 == nullptr || IsActive == nullptr)
	{
		return false; // invalid arguments
	}
	*PostXpSpo2 = false; // default to false
	*IsActive = false; // default to false

	if (GlobalVersionInfo.A.dwPlatformId != VER_PLATFORM_WIN32_NT)
	{
		// not NT based, so we can't be post XP-spo2
		return true;
	}
	if ((GlobalVersionInfo.A.dwMajorVersion == 5) && (GlobalVersionInfo.A.dwMinorVersion == 1))
	{
		if ((GlobalVersionInfo.A.wServicePackMajor <  2))
		{
			// xp but not sp2
			return true;
		}
	}
	if ((GlobalVersionInfo.A.dwMajorVersion < 5))
	{
		// older than xp or not NT based
		return true;
	}
	if ( 
		(GlobalVersionInfo.A.dwMajorVersion >= 5) && (GlobalVersionInfo.A.dwMinorVersion >= 0) ||
		(GlobalVersionInfo.A.dwMajorVersion = 5) && (GlobalVersionInfo.A.dwMinorVersion >= 2)
		)
	{
		// Vista+, so we are post XP and need to test

		if (!LoadAdvapi32Needs())
		{
			if (PostXpSpo2 != nullptr)
			{
				*PostXpSpo2 = false;
			}
			if (IsActive != nullptr)
			{
				*IsActive = false;
			}
			return false;
		}

		*PostXpSpo2 = true; // we are post XP-spo2
		*IsActive = false;
		// go for tryting
		HKEY hKey = 0;
		if (IAT_RegOpenKeyA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\", &hKey) == ERROR_SUCCESS)
		{
			DWORD Size = sizeof(DWORD);
			DWORD Results = 0;
			if (IAT_RegQueryValueExA(hKey, "SafeDllSearchMode", 0, 0, (LPBYTE)&Results, &Size) == ERROR_FILE_NOT_FOUND)
			{
				// somehow the key is not there, so we assume it's not active
				*IsActive = false;
			}
			else
			{
				if (Results != 0)
				{
					*IsActive = true;
				}
			}
			IAT_RegCloseKey(hKey);
			return true;
		}
	}
	return false;

}
bool ProcessProcefileHelper_IsKnownDLL(HANDLE Snapshot, DWORD ID, LWAnsiString* Output, MODULEENTRY32* CheckMe, PROCESSENTRY32* FromProcess)
{

	if (!LoadAdvapi32Needs())
	{
		if (Output != nullptr)
		{
			LWAnsiString_AppendA(Output, "Failed to load advapi32.dll or get required functions.");
		}
		return false; // failed to load advapi32.dll or get required functions
	}

	BOOL HitSomething = false;

	LWAnsiString* KeyName = LWAnsiString_CreateString(255);

	if (KeyName == nullptr)
	{
		if (Output != nullptr)
		{
			LWAnsiString_AppendA(Output, "Failed to allocate memory for KeyName");
		}
		return false; // failed to allocate memory for KeyName
	}


	{
		HKEY hKey = 0;
		if (IAT_RegOpenKeyA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\KnownDLLs", &hKey) == ERROR_SUCCESS)
		{
			DWORD index = 0;
			BOOL YAY = true;
			while (YAY)
			{
				DWORD AllotSize = KeyName->AllocatedSize;
				auto result = IAT_RegEnumValueA (hKey, index, KeyName->AnsiData, &AllotSize, nullptr, nullptr, nullptr, nullptr);
				YAY = (result == ERROR_SUCCESS);
				/*
				* the probe length updates the stored length if its zero and we aint an empty string
				*/
				auto _ForceRecalc = LWAnsiString_ProbeLength(KeyName);
				int test_slice = LWAnsiString_EndsAtA(KeyName, ".DLL", false);
				size_t size = lstrlenA(CheckMe->szModule);
				size_t i = size;
				bool HitDot = false;
				{
				
				
					
					for (i = i; i >= 0; i = i-1)
					{
						if (CheckMe->szModule[i] == '\\' || CheckMe->szModule[i] == '/')
						{
							break; // we hit a dot or slash
						}
						if (CheckMe->szModule[i] == '.')
						{
							HitDot = true; // we hit a dot
							break;
						}
					}

					if (HitDot)
					{
						CheckMe->szModule[i] = '\0';
					}
				}
				
			
				if (LWAnsiString_CompareA(KeyName, CheckMe->szModule, false) == 0)
				{
					if (Output != nullptr)
					{
						HitSomething = true;
					}
					if (HitDot)
					{
						CheckMe->szModule[i] = '.'; // restore the dot
					}
					
					LWAnsiString_FreeString(KeyName);
					KeyName = 0;
					HitSomething = true;
					break;
				}
				index++;
			}
			IAT_RegCloseKey(hKey);
		}
	}
	if (KeyName) LWAnsiString_FreeString(KeyName);
	return HitSomething;
}
#pragma region VerifySeachPathStuff
void WalkDependancyFolder()
{

}
#pragma endregion
void ProcessProfileHelper_VerifySearchPath(HANDLE Snapshot, DWORD ID, LWAnsiString* Output, MODULEENTRY32* CheckMe, const char* MainExe)
{
	if (GlobalVersionInfo.A.dwPlatformId != VER_PLATFORM_WIN32_NT)
	{
		return;
	}
	LWAnsiString* ExePath = nullptr;
	if (!ProcessProfileHelper_MainExe(ID, &ExePath))
	{
		if (Output != nullptr)
		{
			LWAnsiString_AppendA(Output, "Failed to get main executable path to verify search path of exe.");
		}
		return; // failed to get main executable path
	}



	if (ExePath != 0) LWAnsiString_FreeString(ExePath);

}

void ProfileProcessHelper_EnumModel(HANDLE SnapShot, DWORD id, LWAnsiString* output, PROCESSENTRY32* FromProcess)
{
	MODULEENTRY32 ModuleEntry;
	DWORD count = 0;
	ModuleEntry.dwSize = sizeof(MODULEENTRY32);
	if (Module32First(SnapShot, &ModuleEntry))
	{
		
		do
		{
			count++;
			if (ModuleEntry.th32ProcessID == id)
			{
				if (output != nullptr)
				{
					LWAnsiString_AppendA(output, "Module: ");
					LWAnsiString_AppendNumberA(count, output, nullptr);
					LWAnsiString_AppendA(output, " (");
					LWAnsiString_AppendA(output, ModuleEntry.szModule);
					LWAnsiString_AppendA(output, ")");

					LWAnsiString_AppendA(output, " from: \"");
					LWAnsiString_AppendA(output, ModuleEntry.szExePath);
					LWAnsiString_AppendA(output, "\"");
					LWAnsiString_AppendA(output, "\r\n");

			
					if (count != 0)
					{
						LWAnsiString_AppendA(output, "Verification Checks: ");
						if (GlobalVersionInfo.A.dwPlatformId == VER_PLATFORM_WIN32_NT)
						{
							if (!ProcessProcefileHelper_IsKnownDLL(SnapShot, id, output, &ModuleEntry, FromProcess))
							{
								LWAnsiString_AppendA(output, "Module is not a known DLL\r\n");
							}
							else
							{
								LWAnsiString_AppendA(output, "Module is a known DLL\r\n");
							}
						}
					}

					LWAnsiString_AppendA(output, "\r\n");
				}
			}
		} while (Module32Next(SnapShot, &ModuleEntry));
	}
}

void ProfileProcessHelper_EnumHeap(HANDLE SnapShot, DWORD id, LWAnsiString* output)
{
	HEAPLIST32 HeapList = { 0 };
	HeapList.dwSize = sizeof(HEAPLIST32);
	if (Heap32ListFirst(SnapShot, &HeapList))
	{
		do
		{
			if (output != nullptr)
			{
				LWAnsiString_AppendA(output, "Heap: ");
				LWAnsiString_AppendNumberA(HeapList.th32HeapID, output, nullptr);
				LWAnsiString_AppendNewLine(output);
			}
			HEAPENTRY32 HeapEntry = { 0 };
			HeapEntry.dwSize = sizeof(HEAPENTRY32);
			if (Heap32First(&HeapEntry, id, HeapList.th32ProcessID))
			{
				do
				{
					if (output != nullptr)
					{
						LWAnsiString_AppendA(output, "  Block: ");
						LWAnsiString_AppendNumberA(HeapEntry.dwAddress, output, nullptr);
						LWAnsiString_AppendNewLine(output);
					}
				} while (Heap32Next(&HeapEntry));
			}
		} while (Heap32ListNext(SnapShot, &HeapList));
	}
}
bool ProfileProcess(DWORD id, LWAnsiString* output)
{
	if (id == 0)
	{
		id = GetCurrentProcessId(); // default to self
	}
	HANDLE hProcess = 0;
	if (GlobalVersionInfo.A.dwMajorVersion >= 10)
	{
		hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, id);
	}

	if (hProcess == 0)
	{
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, id);
	}
	
;
		HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, id);

		if (hProcessSnap == INVALID_HANDLE_VALUE)
		{
			hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE32, 0);
		}


		if (hProcessSnap == INVALID_HANDLE_VALUE)
		{
			if (output != nullptr)
			{
				LWAnsiString_AppendA(output, "Failed to create snapshot of process modules.");
			}
			return false;
		}

		bool PostXp = false;
		bool SafeSearchDll = false;
	
		if (!ProcessProfileHelper_VerifySafeDllSearchFlag(&PostXp, &SafeSearchDll))
		{
			LWAnsiString_AppendA(output, "Failed to verify SafeDllSearchMode flag. ");
			if (PostXp == false) LWAnsiString_AppendA(output, "Primary reason is system OS reports being older than WinXP SP2\r\n");
			if (PostXp == true) LWAnsiString_AppendA(output, "For Unknown Reason\r\n");
		}
		else
		{
			if (PostXp)
			{
				LWAnsiString_AppendA(output, "System indicates being post XP SP2 Version (SafeDllSearch flag is option ere)\r\n");
			}
			else
			{
				LWAnsiString_AppendA(output, "System indicates being PRE XP SP2 Version (SafeDllSearch doesn't exist here)\r\n");
			}
			if (SafeSearchDll)
			{
				LWAnsiString_AppendA(output, "SafeDllSearchMode is active\r\n");
			}
			else
			{
				LWAnsiString_AppendA(output, "SafeDllSearchMode [!!!!] is not active\r\n");
			}
		}
		if (!SafeSearchDll)
		{
			LWAnsiString* MainExe;
			
			LWAnsiString_AppendA(output, "With SafeDLLSearchMode not active/non existent, this can allow an malicious actor to drop a a decoy DLL in this app's folder to gain access.\r\n");
			if (ProcessProfileHelper_MainExe(id, &MainExe))
			{
				LWAnsiString_TrimEndsWithA(MainExe, "\\", false);


				int dot_loc = LWAnsiString_FindLastExA(MainExe, '.', 0);
				if (dot_loc != -1)
				{
					MainExe->AnsiData[dot_loc] = '\0'; // temp trim the ext if any
				}
				int slash_loc = LWAnsiString_FindLastExA(MainExe, '\\', 0);
				if (slash_loc != -1)
				{
					MainExe->AnsiData[slash_loc] = '\0'; // temp trim the slash if any
				}
				else
				{
					slash_loc = LWAnsiString_FindLastExA(MainExe, '/', 0);
					if (slash_loc != -1)
					{
						MainExe->AnsiData[slash_loc] = '\0'; // temp trim the slash if any
					}
				}

				
				if (IsFolderWritable(MainExe->AnsiData))
				{
					LWAnsiString_AppendA(output, "Warning [!!!!]: Main Executable Path is writable by current user.\r\n");
				}
			}
			
		}
		ProfileProcessHelper_EnumModel(hProcessSnap, id, output, 0);


		CloseHandle(hProcessSnap);
	




	if (hProcess != 0)
	{
		CloseHandle(hProcess);
	}
}

bool CheckIsKnownDllValues_PipeStdout(int* result, const char** message_result, const char* argv[], int argc)
{
	LWAnsiString* output = LWAnsiString_CreateString(0);
	if (output == nullptr)
	{
		if (message_result != nullptr)
		{
			*message_result = "Failed to allocate memory for output string.";
		}
		if (result != nullptr)
		{
			*result = -1;
		}
		return false; // failed to allocate memory for output string
	}

//#error "CheckIsKnownDllValues_PipeStdout is not implemented yet."
	if (output != 0) LWAnsiString_FreeString(output);	
}

bool CheckSafeLoadPath_PipeStdout(int* result, const char** message_result, const char* argv[], int argc)
{
	LWAnsiString* output = LWAnsiString_CreateString(0);
	bool IsPostXpSpo2 = false;
	bool IsSafeDllSearchActive = false;
	if (!ProcessProfileHelper_VerifySafeDllSearchFlag(&IsPostXpSpo2, &IsSafeDllSearchActive))
	{
		if (message_result != nullptr)
		{
			*message_result = "Failed to verify SafeDllSearchMode flag.";
		}
		if (result != nullptr)
		{
			*result = -1;
		}
		if (output != 0) LWAnsiString_FreeString(output);
		return false; // failed to verify SafeDllSearchMode flag
	}
	else
	{
		LWAnsiString_AppendA(output, "SafeDllSearchMode Flag Check:\r\n");
		if (!IsPostXpSpo2)
		{
			LWAnsiString_AppendA(output, "System indicates being pre XP SP2 Version (SafeDllSearch doesn't exist here)\r\n");
		}
		else
		{
			LWAnsiString_AppendA(output, "System indicates being  XP SP2 or higher Version (SafeDllSearch flag is possible here)\r\n");
		}
		if (IsSafeDllSearchActive)
		{
			LWAnsiString_AppendA(output, "SafeDllSearchMode is active. System is gonna check protected system folders first.\r\n");
		}
		else
		{
			LWAnsiString_AppendA(output, "SafeDllSearchMode [!!!!] is **not** active");
			if (!IsPostXpSpo2)
			{
				LWAnsiString_AppendA(output, " likely because System may be older than Win XP SP2\n");
			}
			else
			{
				LWAnsiString_AppendA(output, " for an unknown reason on this Post XP SP2 OS. Check if that's valid\r\n");
			}
			LWAnsiString_AppendA(output, "This can allow an malicious actor to drop a a decoy DLL in this any app folder to gain accessm, providing writing to it is allowed.\r\n");
		}

		WriteStdout(LWAnsiString_ToCStr(output));
	}
	if (output != 0) LWAnsiString_FreeString(output);
	return true;
}

	bool ProcessProfileEntryPoint(int* result, const char** message_result, const char* argv[], int argc)
	{
		if (result == nullptr || message_result == nullptr || argv == nullptr)
		{
			if (message_result != nullptr)
			{
				*message_result = "Invalid arguments passed to ProcessProfileEntryPoint";
			}
			if (result != nullptr)
			{
				*result = -1;
			}
			return false;
		}
		FetchVersionInfo(&GlobalVersionInfo, &VERISON_INFO_IS_UNICODE);
		if (GlobalVersionInfo.A.dwPlatformId == VER_PLATFORM_WIN32_NT)
		{
			// resolve the simple version of the process walker
		}
		bool IsSelf = false;
		bool ISALL = false;
		for (int i = 0; i < argc; i++)
		{
			if (lstrcmpiA(argv[i], "-self") == 0)
			{
				IsSelf = true;
				break;
			}
			else if (lstrcmpiA(argv[i], "-all") == 0)
			{
				ISALL = true;
				break;
			}
		}
		LWAnsiString* Output = LWAnsiString_CreateString(0);

		auto res = ProfileProcess(0, Output);

			WriteStdout(LWAnsiString_ToCStr(Output));
			return res;
	}

