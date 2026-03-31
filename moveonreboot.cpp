

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

MoveFileExA_ptr ptrMoveFileExA = 0;
WritePrivateProfileStringAPTR ptrWritePrivateProfileStringA = 0;
GetWindowsDirectoryAPTR ptrGetWindowsDirectoryA = 0;

void LegacyMarkForDelete(const char* name, LWAnsiString* vebal)
{
	bool WeLoadedIt = false;
	if (kernel32 == 0)
	{
		kernel32 = LoadLibraryA("kernel32.dll");
		WeLoadedIt = true;
	}
	ptrWritePrivateProfileStringA = (WritePrivateProfileStringAPTR)GetProcAddress(kernel32, "WritePrivateProfileStringA");
	ptrGetWindowsDirectoryA = (GetWindowsDirectoryAPTR)GetProcAddress(kernel32, "GetWindowsDirectoryA");

	if ((ptrWritePrivateProfileStringA == 0) || (ptrGetWindowsDirectoryA == 0))
	{
		LWAnsiString_Append(vebal, name);
		LWAnsiString_AppendWithNewLine(vebal, "---> Failed to sucessfully register this for legacy delete routine. ");
		return;
	}
	else
	{
		LWAnsiString* WinDir = LWAnsiString_CreateString(MAX_PATH);
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
		// now the windir
		LWAnsiString_Append(WinDir, "wininit.ini");


		ptrWritePrivateProfileStringA("rename", "NUL", name, WinDir->Data);

		// free
		LWAnsiString_FreeString(WinDir);
		if (WeLoadedIt)
		{
			FreeLibrary(kernel32);
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
							LWAnsiString_AppendWithNewLine(vebal, " Registered for deletion. ");
						}
					}
				}
				FreeLibrary(kernel32);
			}
			WriteStdout(vebal->Data);
			LWAnsiString_FreeString(vebal);
		}
		SetLastError(0);
	}
	


}