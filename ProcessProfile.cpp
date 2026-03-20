#include "common.h"
#include "osver.h"
#include "LWAnsiString.h"

#include "TlHelp32.h"

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



HMODULE ADVAPI32 = 0;
typedef LONG (WINAPI* RegEnumKeyA_Ptr)(
	HKEY   hKey,
	DWORD  dwIndex,
	LPSTR  lpName,
	DWORD  cchName
);
typedef LSTATUS(WINAPI* RegOpenKeyA_Ptr)(
	HKEY hKey,
	LPCSTR lpSubKey,
	HKEY* phkResult
	);
typedef LSTATUS (WINAPI* RegCloseKey_Ptr)(
	HKEY hKey
);
typedef LSTATUS (WINAPI* RegEnumValueA_Ptr)(
     HKEY    hKey,
	DWORD   dwIndex,
	  LPSTR   lpValueName,
	LPDWORD lpcchValueName,
	LPDWORD lpReserved,
     LPDWORD lpType,
	    LPBYTE  lpData,
	LPDWORD lpcbData
);
typedef LSTATUS (WINAPI* RegQueryValueExA_Ptr)(
       HKEY    hKey,
                    LPCSTR  lpValueName,
					LPDWORD lpReserved,
                    LPDWORD lpType,
                    LPBYTE  lpData,
                    LPDWORD lpcbData
);  
RegOpenKeyA_Ptr RegOpenKeyA_Calling = nullptr;
RegEnumKeyA_Ptr RegEnumKeyA_Calling = nullptr;
RegCloseKey_Ptr RegCloseKey_Calling = nullptr;
RegEnumValueA_Ptr RegEnumValueA_Calling = nullptr;
RegQueryValueExA_Ptr RegQueryValueExA_Calling = nullptr;

bool IsFolderWritable(const char* target)
{
	auto folder = LWAnsiString_CreateString(MAX_PATH);
	if (folder == nullptr)
	{
		return false; // failed to allocate memory for folder
	}
again:
	LWAnsiString_Append(folder, target);
	GetTempFileNameA(folder->Data, "LWT", 0, folder->Data);
	HANDLE tmp = CreateFileA(folder->Data, GENERIC_WRITE , FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
	if (tmp != INVALID_HANDLE_VALUE)
	{
		CloseHandle(tmp);
		return true;
	}
	return false;
}
bool LoadAdvapi32Needs()
{
	if (ADVAPI32 == 0)
	{
		ADVAPI32 = LoadLibraryA("advapi32.dll");
		if (ADVAPI32 != 0)
		{
			RegEnumKeyA_Calling = (RegEnumKeyA_Ptr)GetProcAddress(ADVAPI32, "RegEnumKeyA");
			RegOpenKeyA_Calling = (RegOpenKeyA_Ptr)GetProcAddress(ADVAPI32, "RegOpenKeyA");
			RegCloseKey_Calling = (RegCloseKey_Ptr)GetProcAddress(ADVAPI32, "RegCloseKey");
			RegEnumValueA_Calling = (RegEnumValueA_Ptr)GetProcAddress(ADVAPI32, "RegEnumValueA");
			RegQueryValueExA_Calling = (RegQueryValueExA_Ptr)GetProcAddress(ADVAPI32, "RegQueryValueExA");
			return (ADVAPI32 != nullptr && RegEnumKeyA_Calling != nullptr && RegOpenKeyA_Calling != nullptr && RegCloseKey_Calling != nullptr && RegEnumValueA_Calling != nullptr && RegQueryValueExA_Calling != nullptr);
		}
		else
		{
			return false;
		}
	}
	else
	{
		return true;
	}

	return false;
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
		if (RegOpenKeyA_Calling(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\", &hKey) == ERROR_SUCCESS)
		{
			DWORD Size = sizeof(DWORD);
			DWORD Results = 0;
			if (RegQueryValueExA_Calling(hKey, "SafeDllSearchMode", 0, 0, (LPBYTE)&Results, &Size) == ERROR_FILE_NOT_FOUND)
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
			RegCloseKey_Calling(hKey);
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
			LWAnsiString_Append(Output, "Failed to load advapi32.dll or get required functions.");
		}
		return false; // failed to load advapi32.dll or get required functions
	}

	BOOL HitSomething = false;

	LWAnsiString* KeyName = LWAnsiString_CreateString(255);

	if (KeyName == nullptr)
	{
		if (Output != nullptr)
		{
			LWAnsiString_Append(Output, "Failed to allocate memory for KeyName");
		}
		return false; // failed to allocate memory for KeyName
	}


	{
		HKEY hKey = 0;
		if (RegOpenKeyA_Calling(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\KnownDLLs", &hKey) == ERROR_SUCCESS)
		{
			DWORD index = 0;
			BOOL YAY = true;
			while (YAY)
			{
				DWORD AllotSize = KeyName->AllocatedSize;
				auto result = RegEnumValueA_Calling(hKey, index, KeyName->Data, &AllotSize, nullptr, nullptr, nullptr, nullptr);
				YAY = (result == ERROR_SUCCESS);
				/*
				* the probe length updates the stored length if its zero and we aint an empty string
				*/
				auto _ForceRecalc = LWAnsiString_ProbeLength(KeyName);
				int test_slice = LWAnsiString_EndsAt(KeyName, ".DLL", false);
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
				
			
				if (LWAnsiString_Compare(KeyName, CheckMe->szModule, false) == 0)
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
			RegCloseKey(hKey);
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
			LWAnsiString_Append(Output, "Failed to get main executable path to verify search path of exe.");
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
					LWAnsiString_Append(output, "Module: ");
					LWAnsiString_AppendNumber(count, output, nullptr);
					LWAnsiString_Append(output, " (");
					LWAnsiString_Append(output, ModuleEntry.szModule);
					LWAnsiString_Append(output, ")");

					LWAnsiString_Append(output, " from: \"");
					LWAnsiString_Append(output, ModuleEntry.szExePath);
					LWAnsiString_Append(output, "\"");
					LWAnsiString_Append(output, "\r\n");

			
					if (count != 0)
					{
						LWAnsiString_Append(output, "Verification Checks: ");
						if (GlobalVersionInfo.A.dwPlatformId == VER_PLATFORM_WIN32_NT)
						{
							if (!ProcessProcefileHelper_IsKnownDLL(SnapShot, id, output, &ModuleEntry, FromProcess))
							{
								LWAnsiString_Append(output, "Module is not a known DLL\r\n");
							}
							else
							{
								LWAnsiString_Append(output, "Module is a known DLL\r\n");
							}
						}
					}

					LWAnsiString_Append(output, "\r\n");
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
				LWAnsiString_Append(output, "Heap: ");
				LWAnsiString_AppendNumber(HeapList.th32HeapID, output, nullptr);
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
						LWAnsiString_Append(output, "  Block: ");
						LWAnsiString_AppendNumber(HeapEntry.dwAddress, output, nullptr);
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
				LWAnsiString_Append(output, "Failed to create snapshot of process modules.");
			}
			return false;
		}

		bool PostXp = false;
		bool SafeSearchDll = false;
	
		if (!ProcessProfileHelper_VerifySafeDllSearchFlag(&PostXp, &SafeSearchDll))
		{
			LWAnsiString_Append(output, "Failed to verify SafeDllSearchMode flag. ");
			if (PostXp == false) LWAnsiString_Append(output, "Primary reason is system OS reports being older than WinXP SP2\r\n");
			if (PostXp == true) LWAnsiString_Append(output, "For Unknown Reason\r\n");
		}
		else
		{
			if (PostXp)
			{
				LWAnsiString_Append(output, "System indicates being post XP SP2 Version (SafeDllSearch flag is option ere)\r\n");
			}
			else
			{
				LWAnsiString_Append(output, "System indicates being PRE XP SP2 Version (SafeDllSearch doesn't exist here)\r\n");
			}
			if (SafeSearchDll)
			{
				LWAnsiString_Append(output, "SafeDllSearchMode is active\r\n");
			}
			else
			{
				LWAnsiString_Append(output, "SafeDllSearchMode [!!!!] is not active\r\n");
			}
		}
		if (!SafeSearchDll)
		{
			LWAnsiString* MainExe;
			
			LWAnsiString_Append(output, "With SafeDLLSearchMode not active/non existent, this can allow an malicious actor to drop a a decoy DLL in this app's folder to gain access.\r\n");
			if (ProcessProfileHelper_MainExe(id, &MainExe))
			{
				LWAnsiString_TrimEndsWith(MainExe, "\\", false);


				int dot_loc = LWAnsiString_FindLastEx(MainExe, '.', 0);
				if (dot_loc != -1)
				{
					MainExe->Data[dot_loc] = '\0'; // temp trim the ext if any
				}
				int slash_loc = LWAnsiString_FindLastEx(MainExe, '\\', 0);
				if (slash_loc != -1)
				{
					MainExe->Data[slash_loc] = '\0'; // temp trim the slash if any
				}
				else
				{
					slash_loc = LWAnsiString_FindLastEx(MainExe, '/', 0);
					if (slash_loc != -1)
					{
						MainExe->Data[slash_loc] = '\0'; // temp trim the slash if any
					}
				}

				
				if (IsFolderWritable(MainExe->Data))
				{
					LWAnsiString_Append(output, "Warning [!!!!]: Main Executable Path is writable by current user.\r\n");
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
		LWAnsiString_Append(output, "SafeDllSearchMode Flag Check:\r\n");
		if (!IsPostXpSpo2)
		{
			LWAnsiString_Append(output, "System indicates being pre XP SP2 Version (SafeDllSearch doesn't exist here)\r\n");
		}
		else
		{
			LWAnsiString_Append(output, "System indicates being  XP SP2 or higher Version (SafeDllSearch flag is possible here)\r\n");
		}
		if (IsSafeDllSearchActive)
		{
			LWAnsiString_Append(output, "SafeDllSearchMode is active. System is gonna check protected system folders first.\r\n");
		}
		else
		{
			LWAnsiString_Append(output, "SafeDllSearchMode [!!!!] is **not** active");
			if (!IsPostXpSpo2)
			{
				LWAnsiString_Append(output, " likely because System may be older than Win XP SP2\n");
			}
			else
			{
				LWAnsiString_Append(output, " for an unknown reason on this Post XP SP2 OS. Check if that's valid\r\n");
			}
			LWAnsiString_Append(output, "This can allow an malicious actor to drop a a decoy DLL in this any app folder to gain accessm, providing writing to it is allowed.\r\n");
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

