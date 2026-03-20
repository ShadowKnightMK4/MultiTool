
#include <windows.h>
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
HANDLE stdin;
HANDLE stdout;
HANDLE stderr;

HANDLE TargetProcessHandle;

bool LastHandleIsID = false;
HANDLE StringToHandle(char* target)
{
	LONGLONG x = 0;
	if (target[0] == 'i')
	{
		target++;
		LastHandleIsID = true;
		if (sizeof(void*) == 8)
		{
			StrToInt64ExA(target, STIF_SUPPORT_HEX, &x);
		}
		else
		{
			StrToInt64ExA(target, STIF_SUPPORT_HEX, &x);
			return (HANDLE)x;
		}
	}
	else
	{
		if (sizeof(void*) == 8)
		{
			StrToInt64ExA(target, STIF_SUPPORT_HEX, &x);
		}
		else
		{
			StrToInt64ExA(target, STIF_SUPPORT_HEX, &x);
			return (HANDLE)x;
		}
	}
	return (HANDLE)x;
	
}

/* from https://learn.microsoft.com/en-us/windows/win32/secauthz/enabling-and-disabling-privileges-in-c-- */
BOOL SetPrivilege(
	HANDLE hToken,          // access token handle
	LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
	BOOL bEnablePrivilege   // to enable or disable privilege
)
{
	TOKEN_PRIVILEGES tp;
	LUID luid;

	if (!LookupPrivilegeValue(
		NULL,            // lookup privilege on local system
		lpszPrivilege,   // privilege to lookup 
		&luid))        // receives LUID of privilege
	{
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	if (bEnablePrivilege)
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	else
		tp.Privileges[0].Attributes = 0;

	// Enable the privilege or disable all privileges.

	if (!AdjustTokenPrivileges(
		hToken,
		FALSE,
		&tp,
		0,
		(PTOKEN_PRIVILEGES)NULL,
		(PDWORD)NULL))
	{
		return FALSE;
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)

	{
		return FALSE;
	}

	return TRUE;
}


bool IsHandleValid(HANDLE x)
{
	DWORD _discard = 0;
	return GetHandleInformation(x, &_discard) == true;
}

bool SanityCheck(DWORD id)
{
	return id != 0;
}

bool Grapple(HANDLE Target)
{
	if (!IsHandleValid(Target))
	{
		return false;
	}
	DWORD id = GetProcessId(Target);
	if (!SanityCheck(id))
	{
		return false;
	}
	else
	{
		bool SetTheKilLFlag = false;
		SetLastError(0);
		if (DebugActiveProcess(id) == 0)
		{
			if (!SetPrivilege(GetCurrentProcessToken(), SE_DEBUG_NAME, true))
			{
				return false;
			}
			else
			{
				if (DebugActiveProcess(id) != 0)
				{

				}
			}

		}
		else
		{
			SetTheKilLFlag = true;
		}

		if (SetTheKilLFlag)
		{
			DebugSetProcessKillOnExit(true);
			return true;
		}
		return false;
	}
}




#define BYTE_SIZE 256
BYTE HandleBuffer[BYTE_SIZE];
/// <summary>
/// We receive the process to freeze via a int value from stdin
/// </summary>
/// <returns></returns>
bool GetTargetProcessHandle()
{
	if (!IsHandleValid(stdin))
	{
		return false;
	}
	else
	{
		DWORD BytesRed = 0;
		if (ReadFile(stdin, HandleBuffer, 256, &BytesRed, 0))
		{
			TargetProcessHandle = StringToHandle((char*)HandleBuffer);
			if (LastHandleIsID == false)
			{

				if (!IsHandleValid(TargetProcessHandle))
				{
					return false;
				}
			}
			else
			{
				HANDLE THandle = 0;
				THandle = OpenProcess(PROCESS_ALL_ACCESS, false, (DWORD) TargetProcessHandle);		
				if (THandle != 0)
				{
					TargetProcessHandle = THandle;
					return true;
				}
				return false;
			}
		}
	}

}
int main()
{
	for (int i = 0; i < BYTE_SIZE; i++)
	{
		HandleBuffer[i] = 0;
	}
	stdin = GetStdHandle(STD_INPUT_HANDLE);
	stdout = GetStdHandle(STD_OUTPUT_HANDLE);
	stderr = GetStdHandle(STD_ERROR_HANDLE);

	if (!GetTargetProcessHandle())
	{
		ExitProcess(255);
	}
	else
	{
		Grapple(TargetProcessHandle);
	}
	Sleep(2000);
	ExitProcess(0);
}