#include "common.h"
#include "Support\\LWAnsiString\\LWAnsiString.h">
#include "osver.h"

#include "IAT_ENV.H"
#include "IAT_FILE.H"
#include "IAT_REGISTRY.H"

bool Legacy_ShowPendingDeletes(int* result, const char** message_result, const char* argv[], int argc);
bool MoveEX_ShowPendingDeletes(int* result, const char** message_result, const char* argv[], int argc);


void AddMapKey(LWAnsiString* verbal, BOOL NTMap)
{
	if (NTMap)
	{
		LWAnsiString_AppendWithNewLineA(verbal, "[KEY]");
		LWAnsiString_AppendWithNewLineA(verbal, "*1 entries represent pending delete operations scheduled at next reboot\r\n");
		LWAnsiString_AppendWithNewLineA(verbal, "[DATA]");
	}
	else
	{
		LWAnsiString_AppendWithNewLineA(verbal, "This is a raw dump of the contents of wininit.ini in the system windows drive.");
		LWAnsiString_AppendWithNewLineA(verbal, "Entries are destination=source");
		LWAnsiString_AppendWithNewLineA(verbal, "Entries marked NUL=source are slated for deleting");
		LWAnsiString_AppendWithNewLineA(verbal, "These pending delete operations scheduled at next reboot\r\n");
	}
}

bool checkShowPendingDeletes_Branch(int* result, const char** message_result, const char* argv[], int argc)
{
	bool TriggerLegacy = false;
	bool use_unicode = false;
	MyOSVERSIONINFO osvi;
	int ret = FetchVersionInfo(&osvi, &use_unicode);
	if (use_unicode)
	{
		if (osvi.W.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		{
			return true;
		}
		else
		{
			if (osvi.W.dwPlatformId == VER_PLATFORM_WIN32_NT)
			{
				return false;
			}
			return false;
		}
	}
	else
	{
		if (osvi.A.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		{
			return true;
		}
		else
		{
			if (osvi.A.dwPlatformId == VER_PLATFORM_WIN32_NT)
			{
				return false;
			}
			return false;
		}
	}
}

bool ShowPendingDeletes(int* result, const char** message_result, const char* argv[], int argc)
{
	if (checkShowPendingDeletes_Branch(result, message_result, argv, argc))
	{
		return Legacy_ShowPendingDeletes(result, message_result, argv, argc);
	}
	else
	{
		return MoveEX_ShowPendingDeletes(result, message_result, argv, argc);
	}

}
bool Legacy_ShowPendingDeletes(int* result, const char** message_result, const char* argv[], int argc)
{
	if ((result == 0) && (message_result == 0))
	{
		return false;
	}

	if (IAT_DynamicLink_Environment(IAT_ENV_API_GETWINDOWSDIRA) != IAT_ENV_API_GETWINDOWSDIRA)
	{
		return false;
	}
	if (IAT_DynamicLink_FileApi(IAT_FILE_LINKING_IMPORTS_ANIS) != IAT_FILE_LINKING_IMPORTS_ANIS)
	{
		return false;
	}

	LWAnsiString* Verbal = LWAnsiString_CreateString(1);

	LWAnsiString* WinDir = LWAnsiString_CreateString(MAX_PATH);
	LWAnsiString_MarkLenDirty(WinDir);
	DWORD size = IAT_GetWindowsDirectoryA(WinDir->AnsiData, MAX_PATH);
	if (size > MAX_PATH)
	{
		LWAnsiString_Reserve(WinDir, size + 1);
		SetLastError(0);
		size = IAT_GetWindowsDirectoryA(WinDir->AnsiData, size + 1);
		if (!LWAnsiString_EndsWith(WinDir, "\\", false))
		{
			LWAnsiString_AppendA(WinDir, "\\");
		}
	}
	else
	{
		if (!LWAnsiString_EndsWith(WinDir, "\\", false))
		{
			LWAnsiString_AppendA(WinDir, "\\");
		}
	}



	// now the windir
	LWAnsiString_AppendA(WinDir, "wininit.ini");


	HANDLE hFile = IAT_CreateFileA(LWAnsiString_ToCStr(WinDir), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		LWAnsiString_AppendWithNewLineA(Verbal, "The pending file ops at next reboot is aparently non existant or midas was denied read access.");
		LWAnsiString_AppendWithNewLineA(Verbal, "If actually not existant, no pending file operations next reboot");
	}
	else
	{


		AddMapKey(Verbal, FALSE);
		
		{
			LWAnsiString* Contents = LWAnsiString_CreateString(1);
			if (Contents != nullptr)
			{
				bool fail = false;
				DWORD BytesRead = 0;
				LARGE_INTEGER FileSizeHigh;
				FileSizeHigh.QuadPart = 0;
				FileSizeHigh.LowPart = IAT_GetFileSize(hFile, (ULONG*) & FileSizeHigh.HighPart);
				FileSizeHigh ;

				if ( (FileSizeHigh.QuadPart >= UINT_MAX) && (GetLastError() == 0))
				{
					LWAnsiString_AppendWithNewLineA(Verbal, "Warning pending file ops appears to be HUGE (>= 4GB -1 size). That's not really normal. Limiting to 4GB view.");
					FileSizeHigh.QuadPart = UINT_MAX - 1;
				}
				fail = GetLastError() != 0;
				
				if (!fail)
				{
					LWAnsiString_Reserve(Contents, FileSizeHigh.LowPart);
					LWAnsiString_MarkLenDirty(Contents);

					if (ReadFile(hFile, Contents->Data, FileSizeHigh.LowPart, &BytesRead, 0))
					{
						LWAnsiString_AppendWithNewLineA(Verbal, "Contents of pending file ops");
						LWAnsiString_AppendWithNewLineA(Verbal, LWAnsiString_ToCStr(Contents));
					}
					else
					{
						LWAnsiString_AppendWithNewLineA(Verbal, "Failed to read pending file ops contents.");
					}
				}
				LWAnsiString_FreeString(Contents);
				
			}
		}

	}
	if (Verbal != 0)
	{
		WriteStdout(LWAnsiString_ToCStr(Verbal));
		LWAnsiString_FreeString(Verbal);
	}
	if (WinDir != 0)
	{
		LWAnsiString_FreeString(WinDir);
	}
	if ((hFile != INVALID_HANDLE_VALUE) && (hFile != 0))
	{
		CloseHandle(hFile);
	}
	

}

bool MoveEX_ShowPendingDeletes(int* result, const char** message_result, const char* argv[], int argc)
{
	if ((result == 0) && (message_result == 0))
	{
		return false;
	}

	if (!IAT_DynamicLinkRegistry(IAT_REGISTRY_ANSI) == IAT_REGISTRY_ANSI)
	{
		return false;
	}
	LWAnsiString* Verbal = LWAnsiString_CreateString(1);
	HKEY NT_OpsHandle;
	LSTATUS IsOk = IAT_RegOpenKeyA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\", &NT_OpsHandle);
	AddMapKey(Verbal, TRUE);
	if (IsOk == ERROR_SUCCESS)
	{

		LWAnsiString* KeyName = LWAnsiString_CreateString(MAX_PATH);
		DWORD KeyNameSize = MAX_PATH;
		DWORD Index = -1;
		BOOL Again = false;
		BOOL Quit = false;

			
			if (IAT_RegQueryValueExA(NT_OpsHandle, "PendingFileRenameOperations", 0, 0, 0, &KeyNameSize) == 0)
			{
		
				LWAnsiString_AddReserve(KeyName, KeyNameSize);
				LWAnsiString_MarkLenDirty(KeyName);
				if (IAT_RegQueryValueExA(NT_OpsHandle, "PendingFileRenameOperations", 0, 0, (LPBYTE) KeyName->Data, &KeyNameSize) == 0)
				{
					// now fix of the null stuff with a few things.
					for (int i = 0; i < KeyNameSize - 1; i++)
					{
						if (i > 0)
						{
							if (KeyName->AnsiData[i] == 0)
							{
								KeyName->AnsiData[i] = '\r';
								if (KeyName->AnsiData[i + 1] == 0)
								{
									KeyName->AnsiData[i + 1] = '\n';
								}
							}
						}
					}
					//KeyName->Data[KeyNameSize - 1] = '\0';
					LWAnsiString_AppendWithNewLineA(Verbal, KeyName->AnsiData);
					Quit = true;
				}
			}

			
		


			LWAnsiString_FreeString(KeyName);
			IAT_RegCloseKey(NT_OpsHandle);
	}


	
	WriteStdout(Verbal->AnsiData);
	LWAnsiString_FreeString(Verbal);
	return true;
}