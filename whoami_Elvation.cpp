#include "common.h"
#include <LWAnsiString.h>
#include "whoami.h"

extern "C" {

	const char* ElevatedMessage = "Notice: [!!!!] A Process running as full admin has more abilities. \r\nAdmin shouldn't be used without a very good reason.\r\n";

	bool WhoAmi_Write_TokenElevatedQuestion_string(int* result, const char** message_result, const char* argv[], int argc, LWAnsiString* Output, const char* Prefix, HANDLE TargetToken)
	{
		HMODULE ADVAPI32 = 0;
		HMODULE SECUR32 = 0;
		GetTokenInfoPtr GetTokenInfoAPI = 0;
		LookupAccountSidA_PTR LookupSIDAPI = 0;
		LookUPPrivnameA LookUpPriv = 0;
		DWORD SizeNeeded = 0;
		BOOL IsElevated = false;
		TokenInformationUnion Token;
		//Token.TheVoid = 0;
		Token.TheVoid = (void*)1;
		if (result == nullptr || message_result == nullptr || Output == nullptr)
		{
			// because there's no meninafly way to set a return falue
			return false;
		}
		if (!ResolveTokenDlls(&ADVAPI32, &GetTokenInfoAPI, &LookupSIDAPI, &LookUpPriv, message_result))
		{
			return false;
		}

		HANDLE selfToken;
		if ((TargetToken == 0) || (TargetToken == INVALID_HANDLE_VALUE))
		{
			selfToken = 0;
			if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_QUERY_SOURCE, &selfToken))
			{
				if (message_result != nullptr)
				{
					*message_result = "Failed to open process token";
				}
				if (result != nullptr)
				{
					*result = GetLastError();
				}
				FreeLibrary(ADVAPI32);
				FreeLibrary(SECUR32);
				return false; // we're dead in the water without that
			}
		}
		else
		{
			selfToken = TargetToken;
		}



		if (Prefix != nullptr)
		{
			LWAnsiString_Append(Output, Prefix);
		}

		// first are we eleveanted or not?
		if (!GetTokenInfoAPI(selfToken, TokenElevation, 0, 0, &SizeNeeded))
		{
			if ((GetLastError() == ERROR_INSUFFICIENT_BUFFER) || (GetLastError() == ERROR_MORE_DATA) || (GetLastError() == ERROR_BAD_LENGTH))
			{
				Token.TheVoid = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, SizeNeeded);
			}
			else
			{
				if (SizeNeeded != 0)
				{
					Token.TheVoid = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, SizeNeeded);
				}
				else
				{
					Token.TheVoid = 0;
				}
			}
		}
		if (Token.TheVoid != 0)
		{
			if (GetTokenInfoAPI(selfToken, TokenElevation, Token.TheVoid, SizeNeeded, &SizeNeeded))
			{
				IsElevated = (*Token.DwordValue) != 0;

			}
			if (Token.TheVoid != 0) HeapFree(GetProcessHeap(), 0, Token.TheVoid); Token.TheVoid = 0;
		}

		// next elevation type
		if (!GetTokenInfoAPI(selfToken, TokenElevationType, 0, 0, &SizeNeeded))
		{
			if ((GetLastError() == ERROR_INSUFFICIENT_BUFFER) || (GetLastError() == ERROR_MORE_DATA) || (GetLastError() == ERROR_BAD_LENGTH))
			{
				Token.TheVoid = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, SizeNeeded);
			}
		}

		if (IsElevated)
			LWAnsiString_Append(Output, "Currently Elevated with ");
		else
			LWAnsiString_Append(Output, "NOT Elevated with ");

		if (Token.TheVoid != 0)
		{
			/* Note: IsElevated bool switchs to be a flag if running as admin*/

			if (GetTokenInfoAPI(selfToken, TokenElevationType, Token.TheVoid, SizeNeeded, &SizeNeeded))
			{
				IsElevated = false;
				switch (*Token.DwordValue)
				{
				case TokenElevationTypeDefault:
					LWAnsiString_Append(Output, "and has No Linked Secondary Token");
					break;
				case TokenElevationTypeFull:
					LWAnsiString_Append(Output, "and has Full Priv (admin)");
					IsElevated = true;
					break;
				case TokenElevationTypeLimited:
					LWAnsiString_Append(Output, "and has limited Priv (possible restricted)");
					break;
				default:
					LWAnsiString_Append(Output, "Unknown Ability");
					break;
				}
				LWAnsiString_Append(Output, ".\r\n");
			}

			if (Token.TheVoid != 0) HeapFree(GetProcessHeap(), 0, Token.TheVoid); Token.TheVoid = 0;
		}
		else
		{
			LWAnsiString_Append(Output, "Unknown Ability");
		}

		if (IsElevated)
		{
			LWAnsiString_Append(Output, ElevatedMessage);
		}



		if (Token.TheVoid != 0) HeapFree(GetProcessHeap(), 0, Token.TheVoid);
		if (selfToken != TargetToken) CloseHandle(selfToken);
		if (ADVAPI32 != 0) FreeLibrary(ADVAPI32);
		if (SECUR32 != 0) FreeLibrary(SECUR32);
		return true;
	}

	bool WhoAmi_Writestdout_TokenElevatedQuestion(int* result, const char** message_result, const char* argv[], int argc)
	{
		LWAnsiString* OutputString = LWAnsiString_CreateFromString("WhoAmI Token Elevation: \r\n");
		if (OutputString != nullptr)
		{
			// 0 as the last are is shortcut for says look at the current process token
			auto res = WhoAmi_Write_TokenElevatedQuestion_string(result, message_result, argv, argc, OutputString, "User Token Source\r\n", 0);
			WriteStdout(LWAnsiString_ToCStr(OutputString));
			LWAnsiString_FreeString(OutputString);
			return res;
		}
		return false;
	}
}