#include "common.h"
#include <LWAnsiString.h>

typedef LONG(WINAPI* RegEnumKeyA_Ptr2)(
	HKEY   hKey,
	DWORD  dwIndex,
	LPSTR  lpName,
	DWORD  cchName
	);
typedef LSTATUS(WINAPI* RegOpenKeyA_Ptr2)(
	HKEY hKey,
	LPCSTR lpSubKey,
	HKEY* phkResult
	);
typedef LSTATUS(WINAPI* RegCloseKey_Ptr2)(
	HKEY hKey
	);
typedef LSTATUS(WINAPI* RegEnumValueA_Ptr2)(
	HKEY    hKey,
	DWORD   dwIndex,
	LPSTR   lpValueName,
	LPDWORD lpcchValueName,
	LPDWORD lpReserved,
	LPDWORD lpType,
	LPBYTE  lpData,
	LPDWORD lpcbData
	);
typedef LSTATUS(WINAPI* RegQueryValueExA_Ptr2)(
	HKEY    hKey,
	LPCSTR  lpValueName,
	LPDWORD lpReserved,
	LPDWORD lpType,
	LPBYTE  lpData,
	LPDWORD lpcbData
	);
RegOpenKeyA_Ptr2 RegOpenKeyA_Calling2 = nullptr;
RegEnumKeyA_Ptr2 RegEnumKeyA_Calling2 = nullptr;
RegCloseKey_Ptr2 RegCloseKey_Calling2 = nullptr;
RegEnumValueA_Ptr2 RegEnumValueA_Calling2 = nullptr;
RegQueryValueExA_Ptr2 RegQueryValueExA_Calling2 = nullptr;


HMODULE advapi32_2 = 0;
bool LoadStuff()
{
	advapi32_2 = LoadLibraryA("advapi32.dll");
	if (advapi32_2 != 0)
	{
		RegEnumKeyA_Calling2 = (RegEnumKeyA_Ptr2)GetProcAddress(advapi32_2, "RegEnumKeyA");
		RegOpenKeyA_Calling2 = (RegOpenKeyA_Ptr2)GetProcAddress(advapi32_2, "RegOpenKeyA");
		RegCloseKey_Calling2 = (RegCloseKey_Ptr2)GetProcAddress(advapi32_2, "RegCloseKey");
		RegEnumValueA_Calling2 = (RegEnumValueA_Ptr2)GetProcAddress(advapi32_2, "RegEnumValueA");
		RegQueryValueExA_Calling2 = (RegQueryValueExA_Ptr2)GetProcAddress(advapi32_2, "RegQueryValueExA");
		return  (advapi32_2 != nullptr && RegEnumKeyA_Calling2 != nullptr && RegOpenKeyA_Calling2 != nullptr && RegCloseKey_Calling2 != nullptr && RegEnumValueA_Calling2 != nullptr && RegQueryValueExA_Calling2 != nullptr);
	}
	return false;
}

void AddMapKey(LWAnsiString* verbal)
{
	LWAnsiString_AppendWithNewLine(verbal, "[KEY]");
	LWAnsiString_AppendWithNewLine(verbal,"*1 entries represent pending delete operations scheduled at next reboot\r\n");
	LWAnsiString_AppendWithNewLine(verbal, "[DATA]");
}
bool ShowPendingDeletes(int* result, const char** message_result, const char* argv[], int argc)
{
	if ((result == 0) && (message_result == 0))
	{
		return false;;
	}
	if (!LoadStuff())
	{
		return false;
	}
	LWAnsiString* Verbal = LWAnsiString_CreateString(1);
	HKEY NT_OpsHandle;
	LSTATUS IsOk = RegOpenKeyA_Calling2(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\", &NT_OpsHandle);
	AddMapKey(Verbal);
	if (IsOk == ERROR_SUCCESS)
	{

		LWAnsiString* KeyName = LWAnsiString_CreateString(MAX_PATH);
		DWORD KeyNameSize = MAX_PATH;
		DWORD Index = -1;
		BOOL Again = false;
		BOOL Quit = false;

			
			if (RegQueryValueExA_Calling2(NT_OpsHandle, "PendingFileRenameOperations", 0, 0, 0, &KeyNameSize) == 0)
			{
				LWAnsiString_MarkLenDirty(KeyName);
				LWAnsiString_AddReserve(KeyName, KeyNameSize);

				if (RegQueryValueExA_Calling2(NT_OpsHandle, "PendingFileRenameOperations", 0, 0, (LPBYTE) KeyName->Data, &KeyNameSize) == 0)
				{
					// now fix of the null stuff with a few things.
					for (int i = 0; i < KeyNameSize - 1; i++)
					{
						if (i > 0)
						{
							if (KeyName->Data[i] == 0)
							{
								KeyName->Data[i] = '\r';
								if (KeyName->Data[i + 1] == 0)
								{
									KeyName->Data[i + 1] = '\n';
								}
							}
						}
					}
					KeyName->Data[KeyNameSize - 1] = '\0';
					LWAnsiString_AppendWithNewLine(Verbal, KeyName->Data);
					Quit = true;
				}
			}

			
		


			LWAnsiString_FreeString(KeyName);
			RegCloseKey_Calling2(NT_OpsHandle);
	}


	
	WriteStdout(Verbal->Data);
	LWAnsiString_FreeString(Verbal);
	FreeLibrary(advapi32_2);
	return true;
}