

#include "common.h"
#include <LWAnsiString.h>
#include "osver.h"
HMODULE kernel32 = 0;
typedef BOOL (WINAPI* MoveFileExA_ptr)(
	           LPCSTR lpExistingFileName,
	 LPCSTR lpNewFileName,
	           DWORD  dwFlags
);
typedef UINT (WINAPI* GetWindowsDirectoryAPTR)(
	 LPSTR lpBuffer,
	  UINT  uSize
);

typedef BOOL (WINAPI* WritePrivateProfileStringAPTR)(
	 LPCSTR lpAppName,
	 LPCSTR lpKeyName,
	 LPCSTR lpString,
	 LPCSTR lpFileName
);


typedef HANDLE(WINAPI*  CreateFileAPTR)(
                LPCSTR                lpFileName,
                DWORD                 dwDesiredAccess,
	               DWORD                 dwShareMode,
	               LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	               DWORD                 dwCreationDisposition,
	               DWORD                 dwFlagsAndAttributes,
	               HANDLE                hTemplateFile
);
typedef DWORD(WINAPI* SetFilePointerPTR)(
	                    HANDLE hFile,
	                    LONG   lDistanceToMove,
	                    PLONG  lpDistanceToMoveHigh,
	                    DWORD  dwMoveMethod
);
typedef DWORD (WINAPI *GetShortPathNameAPTR)(
	      LPCSTR lpszLongPath,
	      LPSTR  lpszShortPath,
	      DWORD   cchBuffer
);
MoveFileExA_ptr ptrMoveFileExA = 0;
 
GetWindowsDirectoryAPTR ptrGetWindowsDirectoryA = 0;
SetFilePointerPTR ptrSetFile = 0;
CreateFileAPTR ptrCreateFile = 0;
GetShortPathNameAPTR  ptrShortPath = 0;


void LegacyMarkForDelete(const char* name, LWAnsiString* vebal)
{
	bool WeLoadedIt = false;
	if (kernel32 == 0)
	{
		kernel32 = LoadLibraryA("kernel32.dll");
		WeLoadedIt = true;
	}
	
	if (kernel32 == 0)
	{
		LWAnsiString_Append(vebal, "Somehow, kerne32 not found. We can't use delete.");
	}

	ptrGetWindowsDirectoryA = (GetWindowsDirectoryAPTR)GetProcAddress(kernel32, "GetWindowsDirectoryA");
	ptrCreateFile = (CreateFileAPTR)GetProcAddress(kernel32, "CreateFileA");
	ptrSetFile = (SetFilePointerPTR)GetProcAddress(kernel32, "SetFilePointer");
	ptrShortPath = (GetShortPathNameAPTR)GetProcAddress(kernel32, "GetShortPathNameA");

	if ((ptrCreateFile == 0) || (ptrGetWindowsDirectoryA == 0) || (ptrSetFile == 0) || (ptrShortPath == 0))
	{
		LWAnsiString_Append(vebal, name);
		LWAnsiString_AppendWithNewLine(vebal, "---> Failed to sucessfully register this for legacy delete routine. ");
		if ((WeLoadedIt) && (kernel32 != 0))
		{
			FreeLibrary(kernel32);
		}
		return;
	}
	else
	{
		LWAnsiString* WinDir = LWAnsiString_CreateString(MAX_PATH);
		LWAnsiString_MarkLenDirty(WinDir);
		DWORD size = ptrGetWindowsDirectoryA(WinDir->Data, MAX_PATH);
		if (size > MAX_PATH)
		{
			LWAnsiString_Reserve(WinDir, size + 1);
			SetLastError(0);
			size = ptrGetWindowsDirectoryA(WinDir->Data, size + 1);
			if (!LWAnsiString_EndsWith(WinDir, "\\", false))
			{
				LWAnsiString_Append(WinDir, "\\");
			}
		}
		else
		{
			if (!LWAnsiString_EndsWith(WinDir, "\\", false))
			{
				LWAnsiString_Append(WinDir, "\\");
			}
		}

		LWAnsiString_ZeroString(WinDir);

		// now the windir
		LWAnsiString_Append(WinDir, "wininit.ini");

		{
			DWORD ShortPath = ptrShortPath(name, NULL, 0);
			if (ShortPath != 0)
			{
				LWAnsiString* EntryLine = LWAnsiString_CreateString(1);
				LWAnsiString_Append(EntryLine, "NUL=");
				LWAnsiString_AddReserve(EntryLine, ShortPath + 1);
				char* offset = (char*)LWAnsiString_EndingOffset(EntryLine);
				
				if (ptrShortPath(name, offset, ShortPath) != 0)
				{
					LWAnsiString_MarkLenDirty(EntryLine);
					LWAnsiString_AppendNewLine(EntryLine);



					HANDLE file = ptrCreateFile(LWAnsiString_ToCStr(WinDir), GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
					if (file != INVALID_HANDLE_VALUE)
					{
						DWORD wrote = 0;
						if (GetLastError() != ERROR_ALREADY_EXISTS)
						{
							WriteFile(file, "[rename]\r\n", 10, &wrote, 0);
						}
						SetLastError(0);
						if ((ptrSetFile(file, 0, 0, FILE_END) != INVALID_SET_FILE_POINTER) && (GetLastError() == NO_ERROR))
						{
							if (!WriteFile(file, LWAnsiString_ToCStr(EntryLine), EntryLine->Length * sizeof(char), &wrote, 0))
							{
								LWAnsiString_AppendWithNewLine(vebal, "Failed to write the new entry to delete on reboot");
							}
							else
							{
								LWAnsiString_Append(vebal, "");
								LWAnsiString_Append(vebal, LWAnsiString_ToCStr(EntryLine));
								LWAnsiString_Append(vebal, "----> registered for deletion in ");
								LWAnsiString_Append(vebal, LWAnsiString_ToCStr(WinDir));
								LWAnsiString_AppendWithNewLine(vebal, " OK ");
							}
						}
						else
						{
							LWAnsiString_AppendWithNewLine(vebal, "Failed to seek to end of wininit.ini to write the pending delete on reboot");
						}
						CloseHandle(file);
					}
					else
					{
						LWAnsiString_Append(vebal, "Failed to Create / Open wininit.ini in the expected location of  ");
						LWAnsiString_Append(vebal, LWAnsiString_ToCStr(WinDir));
						LWAnsiString_AppendNewLine(vebal);
						LWAnsiString_Append(vebal, "Error code in attempt is ");
						LWAnsiString_AppendNumber(GetLastError(), vebal, 0);
						LWAnsiString_AppendNewLine(vebal);
					}

				}
				else
				{
					LWAnsiString_AppendWithNewLine(vebal, "Failed to get the short (8.3 DOS naming) size for the pending delete.");
				}
				LWAnsiString_FreeString(EntryLine);
			}
			else
			{
				LWAnsiString_AppendWithNewLine(vebal, "Failed to get the short (8.3 DOS naming) size for the pending delete.");
			}
		}
		

		// free
		LWAnsiString_FreeString(WinDir);

		if ((WeLoadedIt && (kernel32 != 0)))
		{
			FreeLibrary(kernel32);
			kernel32 = 0;
		}
	}

}
bool DeleteOnReboot(int* result, const char** message_result, const char* argv[], int argc)
{
	MyOSVERSIONINFO osvi;
	bool UnicodeVer = FALSE;
	FetchVersionInfo(&osvi, &UnicodeVer);
	LWAnsiString* vebal = LWAnsiString_CreateString(1);
	BOOL LegacyOS = FALSE;
	if (argc < 2)
	{
		LWAnsiString_FreeString(vebal);
		return false;
	}
	else
	{
		if ((result == 0 || (message_result == 0)) )
		{
			LWAnsiString_FreeString(vebal);
			return false;
		}
		else
		{
			if ( (osvi.A.dwMajorVersion < 5) || (osvi.A.dwPlatformId != VER_PLATFORM_WIN32_NT))
			{
				LWAnsiString_AppendWithNewLine(vebal, "Warning: Windows verison reports OS possibly older than Windows 2000. Modern register for delete might not work ");
				LegacyOS = TRUE;
			}
			kernel32 = LoadLibraryA("kernel32.dll");
			if (kernel32 == 0)
			{
				*result = GetLastError();
				*message_result = "failure";
				return false;
			}
			else
			{

				ptrMoveFileExA = (MoveFileExA_ptr)GetProcAddress(kernel32, "MoveFileExA");
				if (ptrMoveFileExA == 0)
				{
					if (LegacyOS)
					{
						LWAnsiString_AppendWithNewLine(vebal, "Failed to get MoveFileExA routine. Falling back to legacy register.");
						for (int i = 2; i < argc; i++)
						{
							LegacyMarkForDelete(argv[i], vebal);
						}
					}
					else
					{
						LWAnsiString_AppendWithNewLine(vebal, "Failed to get MoveFileExA routine. No files registered for deletion. ");
					}
				}
				else
				{

					for (int i = 2; i < argc; i++)
					{
						SetLastError(0);
						if (ptrMoveFileExA(argv[i], NULL, MOVEFILE_DELAY_UNTIL_REBOOT))
						{
							LWAnsiString_Append(vebal, argv[i]);
							LWAnsiString_AppendWithNewLine(vebal, " Registered for deletion. ");
						}
						else
						{
							LWAnsiString_Append(vebal, argv[i]);
							LWAnsiString_AppendWithNewLine(vebal, " Failed to Register for Deletion.");
						}
					}
				}
				FreeLibrary(kernel32);
			}
			WriteStdout(vebal->Data);
			LWAnsiString_FreeString(vebal);
		}
		SetLastError(0);
		return true;
	}
	


}