
#include "common.h"
#include "osver.h"
#include <LWAnsiString.h>


#include "whoami.h"











bool helper_WhoAmi_UserTokenEnumAllParts_Dump(int* result, const char** message_result, const char* argv[], int argc, LWAnsiString* Output)
{
	HANDLE selfToken = 0;
	HMODULE Advapi32 = 0;
	GetTokenInfoPtr GetTokenInfoAPI = 0;
	LookupAccountSidA_PTR LookupSIDAPI = 0;
	LookUPPrivnameA LookUpPriv = 0;
	BYTE Buffer[256];
	if (!ResolveTokenDlls(&Advapi32, &GetTokenInfoAPI, &LookupSIDAPI, &LookUpPriv, message_result))
		return false;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_QUERY_SOURCE, &selfToken))
	{
		return false; // we're dead in the water without that
	}
	else
	{
		TokenInformationUnion tokenInfo;
		tokenInfo.TheVoid = nullptr;
		for (TOKEN_INFORMATION_CLASS c = TokenUser; c != MaxTokenInfoClass; c = (TOKEN_INFORMATION_CLASS)((int)c + 1))
		{
			DWORD size = 0;
			if (!GetTokenInfoAPI(selfToken, c, nullptr, 0, &size) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
			{
				if (tokenInfo.TheVoid != nullptr)
				{
					LocalUnlock(tokenInfo.TheVoid);
					LocalFree(tokenInfo.TheVoid);
					tokenInfo.TheVoid = nullptr;
				}
				HLOCAL tokenMem = LocalAlloc(LMEM_ZEROINIT, size);
				if (tokenMem == nullptr)
				{
					if (message_result != nullptr)
					{
						*message_result = "Failed to allocate memory for token information";
					}
					if (result != nullptr)
					{
						*result = GetLastError();
					}
					return false;
				}
				else
				{
					tokenInfo.TheVoid = LocalLock(tokenMem);
					if (tokenInfo.TheVoid == nullptr)
					{
						if (message_result != nullptr)
						{
							*message_result = "Failed to lock memory for token information";
						}
						if (result != nullptr)
						{
							*result = GetLastError();
						}
						LocalFree(tokenMem);
						return false;
					}

					if (GetTokenInfoAPI(selfToken, c, tokenInfo.TheVoid, size, &size))
					{
						switch (c)
						{
						case TokenUser:
						{
							helper_WhoAmi_UserAccountName(result, message_result, argv, argc, Output);
							break;
						}
						case TokenGroups:
						{
							helper_who_ami_usertoken_token_groups_string(result, message_result, argv, argc, tokenInfo.TokenGroups, LookupSIDAPI, Output);
							break;
						}
						case TokenPrivileges:
						{
							helper_WhoAmi_PrivString(result, message_result, argv, argc, "Process Token", selfToken, Output);
							break;
						}
						default:
							break;
						}
					}
					else
					{
						if (message_result != nullptr)
						{
							*message_result = "Failed to get token information";
						}
						if (result != nullptr)
						{
							*result = GetLastError();
						}
					}
				}
			}

		}
	}
}





#ifdef NDEBUG
#ifdef EXPERIMENT
#error atttempting to build expirement and release version.  Maybe you meant to build debug? Drop the EXPERIMENT flag in settings to build release. 
#endif
#endif

#ifdef EXPERIMENT
bool EnableSeDebugPrivilege(HANDLE tokenHandle) {
	TOKEN_PRIVILEGES tp;
	LUID luid;

	if (!LookupPrivilegeValueA(NULL, "SeDebugPrivilege", &luid)) {
		// Failed to get LUID
		return false;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(tokenHandle, FALSE, &tp, sizeof(tp), NULL, NULL)) {
		// Failed to adjust token privileges
		return false;
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
		// Privilege not held by token
		return false;
	}

	return true;
}


HANDLE GetSystemToken()
{
	HANDLE self = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
	HANDLE selfToken;
	OpenProcessToken(self, TOKEN_ALL_ACCESS, &selfToken);
	CloseHandle(self);
	EnableSeDebugPrivilege(selfToken);
	CloseHandle(selfToken);
	// currently i'm just hardcoding 1044 winlogon. 
	HANDLE Process, Token;
	Process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, 1044);
	if (Process == nullptr)
	{
		return INVALID_HANDLE_VALUE;
	}
	else
	{
		if (OpenProcessToken(Process, TOKEN_QUERY | TOKEN_QUERY_SOURCE, &Token))
		{
			CloseHandle(Process);
			return Token;
		}
		else
		{
			CloseHandle(Process);
			return INVALID_HANDLE_VALUE;
		}
	}
}


bool WhoAmi_WriteStdout_PrivSystemToken(int* result, const char** message_result, const char* argv[], int argc)
{
	LWAnsiString* OutputString = LWAnsiString_CreateFromString("WhoAmI Privs: \r\n");
	if (OutputString != nullptr)
	{
		
		auto res = WhoAmi_Write_Priv_common_string(result, message_result, argv, argc, OutputString, "System Token Source\r\n", true, true, GetSystemToken() );
		WriteStdout(LWAnsiString_ToCStr(OutputString));
		LWAnsiString_FreeString(OutputString);
		return res;
	}
	return false;
}
#endif


/// <summary>
/// Functionally the entrypoint for midas.exe -whoami
/// </summary>
/// <param name="result">what gets returned to the system</param>
/// <param name="message_result">last value assigned here gets send to stdout</param>
/// <param name="argv">main</param>
/// <param name="argc">main</param>
/// <returns>true if it worked and false if it failed</returns>
bool WhoAmI_WriteStdout(int* result, const char** message_result, const char* argv[], int argc)
{
	LWAnsiString* OutputString = LWAnsiString_CreateFromString("WhoAmI: \r\n");
	// some of the code is version dependend. Let's go fetch the version first
	FetchVersionInfo(&GlobalVersionInfo, &VERISON_INFO_IS_UNICODE);
	
	// start with outputing the account SID/ ids
	//if (!helper_WhoAmi_UserAccountName(result, message_result, argv, argc, OutputString))
	//{
		//LWAnsiString_FreeString(OutputString);
		//return false;
	//}


	if (!helper_WhoAmi_UserTokenEnumAllParts_Dump(result, message_result, argv, argc, OutputString))
	{
		LWAnsiString_FreeString(OutputString);
		return false;
	}
	WriteStdout(LWAnsiString_ToCStr(OutputString));
	LWAnsiString_FreeString(OutputString);
	return true;

}