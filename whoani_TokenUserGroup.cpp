#include "whoami.h"
#include <LWAnsiString.h>

#define GFTO_ENABLE "'ENABLED SID'"
#define GFTO_OFFLINE "'INACTIVE SID'"
#define GTFO_DEFAULT_ACTIVE "'DEFAULT ENABLE'"
#define GTFO_INTEGRITY_SID "'REQUIRED SID'"
#define GTOF_INTEGRITY_SID_ACTIVE "'ACTIVE REQUIRED SID'"
#define GTFO_INTEGRITY_SID_OFFLINE "'INACTIVE REQUIRED SID'"
#define GTFO_LOGON_SID "'LOGON SID'"
#define GTFO_NOADJUST_SID "'UNADJUSTABLE SID'"
#define GTFO_GROUP_OWNER "'GROUP OWNER'"
#define GTFO_GROUP_RESOURCE "'GROUP RESOURCE'"

extern "C" {

	void GroupFlagToOutput(DWORD Flag, LWAnsiString* Out)
	{
		{
			if ((Flag && SE_GROUP_ENABLED) != 0)
			{
				LWAnsiString_Append(Out, GFTO_ENABLE);
				Flag &= ~SE_GROUP_ENABLED;
				if (Flag != 0) LWAnsiString_Append(Out, ", ");
			}
			else
			{
				LWAnsiString_Append(Out, GFTO_OFFLINE);
				if (Flag != 0) LWAnsiString_Append(Out, ", ");
			}


			if ((Flag && SE_GROUP_ENABLED_BY_DEFAULT) != 0)
			{
				LWAnsiString_Append(Out, GTFO_DEFAULT_ACTIVE);
				Flag &= ~SE_GROUP_ENABLED_BY_DEFAULT;
				if (Flag != 0) LWAnsiString_Append(Out, ", ");
			}


			if ((Flag && SE_GROUP_INTEGRITY) != 0)
			{
				LWAnsiString_Append(Out, GTFO_INTEGRITY_SID);
				Flag &= ~SE_GROUP_INTEGRITY;
				if (Flag != 0) LWAnsiString_Append(Out, ", ");
			}

			if ((Flag && SE_GROUP_INTEGRITY_ENABLED) != 0)
			{
				LWAnsiString_Append(Out, GTOF_INTEGRITY_SID_ACTIVE);
				Flag &= ~SE_GROUP_INTEGRITY_ENABLED;
				if (Flag != 0) LWAnsiString_Append(Out, ", ");
			}
			else
			{
				LWAnsiString_Append(Out, GTFO_INTEGRITY_SID_OFFLINE);
				if (Flag != 0) LWAnsiString_Append(Out, ", ");
			}


			if ((Flag && SE_GROUP_LOGON_ID) != 0)
			{
				LWAnsiString_Append(Out, GTFO_LOGON_SID);
				Flag &= ~SE_GROUP_LOGON_ID;
				if (Flag != 0) LWAnsiString_Append(Out, ", ");
			}


			if ((Flag && SE_GROUP_MANDATORY) != 0)
			{
				LWAnsiString_Append(Out, GTFO_NOADJUST_SID);
				Flag &= ~SE_GROUP_MANDATORY;
				if (Flag != 0) LWAnsiString_Append(Out, ", ");
			}

			if ((Flag && SE_GROUP_OWNER) != 0)
			{
				LWAnsiString_Append(Out, GTFO_GROUP_OWNER);
				Flag &= ~SE_GROUP_OWNER;
				if (Flag != 0) LWAnsiString_Append(Out, ", ");
			}

			if ((Flag && SE_GROUP_RESOURCE) != 0)
			{
				LWAnsiString_Append(Out, "GroupResource");
				Flag &= ~SE_GROUP_RESOURCE;
				//if (Flag != 0) LWAnsiString_Append(Out, ", ");
			}

		}
	}

	void helper_who_ami_usertoken_token_groups_string(int* result, const char** message_result, const char* argv[], int argc, TOKEN_GROUPS* target, LookupAccountSidA_PTR LookupSid, LWAnsiString* Output)
	{
		LWAnsiString* NameOfGroup = LWAnsiString_CreateString(0);
		LWAnsiString_Append(Output, "Groups in this Token:: ");
		for (int i = 0; i < target->GroupCount; i++)
		{
			LWAnsiString_ZeroString(NameOfGroup);


			LWAnsiString_AppendNewLine(Output);
			LWAnsiString_Pad(Output, '-', 20);
			LWAnsiString_AppendNewLine(Output);
			LWAnsiString_Append(Output, "Group: ");
			helper_lookup_sid(LookupSid, target->Groups[i].Sid, 0, 0, 0, "Domain *", "*********", true, NameOfGroup);
			LWAnsiString_Pad(Output, '*', 2);
			LWAnsiString_Append(Output, LWAnsiString_ToCStr(NameOfGroup));
			LWAnsiString_Pad(Output, '*', 2);
			LWAnsiString_AppendNewLine(Output);
			LWAnsiString_Pad(Output, '-', 20);
			LWAnsiString_AppendNewLine(Output);

			LWAnsiString_Append(Output, "Attrib: [");
			GroupFlagToOutput(target->Groups[i].Attributes, Output);
			LWAnsiString_Append(Output, " ]");
			LWAnsiString_AppendNewLine(Output);

		}


		return;
	}


	bool WhoAmi_WriteStdout_UserGroup_common(int* result, const char** message_result, const char* argv[], int argc, LWAnsiString* Output, const char* source_from, HANDLE TargetToken)
	{
		HMODULE ADVAPI32 = 0;
		HMODULE SECUR32 = 0;
		GetTokenInfoPtr GetTokenInfoAPI = 0;
		LookupAccountSidA_PTR LookupSIDAPI = 0;
		LookUPPrivnameA LookUpPriv = 0;

		DWORD SizeNeeded = 0;
		if (result == nullptr || message_result == nullptr || Output == nullptr)
		{
			// because there's no meninafly way to set a return falue
			return false;
		}
		if (!ResolveTokenDlls(&ADVAPI32, &GetTokenInfoAPI, &LookupSIDAPI, &LookUpPriv, message_result))
		{
			return false;
		}

		if (source_from != nullptr)
		{
			LWAnsiString_Append(Output, source_from);
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
		TokenInformationUnion data;
		DWORD Size = 0;
		data.TheVoid = 0;

		if (!GetTokenInfoAPI(selfToken, TokenGroups, 0, 0, &Size))
		{
			if ((GetLastError() == ERROR_INSUFFICIENT_BUFFER) || ((GetLastError() == ERROR_MORE_DATA)))
			{
				data.TheVoid = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, Size);
				if (data.TheVoid == 0)
				{
					*result = -1;
					*message_result = "Out of memory!";
					FreeLibrary(ADVAPI32);
					FreeLibrary(SECUR32);
					return false;
				}
				else
				{

					if (!GetTokenInfoAPI(selfToken, TokenGroups, data.TheVoid, Size, &Size))
					{
						*result = -1;
						*message_result = "Failure fetching Groups.";
						FreeLibrary(ADVAPI32);
						FreeLibrary(SECUR32);
						return false;
					}
				}
			}
		}



		if (data.TheVoid == 0)
		{
			*result = -1;
			*message_result = "Failure fetching Groups.";
			FreeLibrary(ADVAPI32);
			FreeLibrary(SECUR32);
			return false;
		}
		else
		{
			helper_who_ami_usertoken_token_groups_string(result, message_result, argv, argc, data.TokenGroups, LookupSIDAPI, Output);
		}

		if (data.TheVoid != 0) {
			HeapFree(GetProcessHeap(), 0, data.TheVoid); data.TheVoid = 0;
		}

		return TRUE;
	}

	bool WhoAmi_WriteStdout_UserGroups(int* result, const char** message_result, const char* argv[], int argc)
	{

		LWAnsiString* out;
		out = LWAnsiString_CreateString(0);





		if (out != nullptr)
		{

			auto res = WhoAmi_WriteStdout_UserGroup_common(result, message_result, argv, argc, out, "Self", 0);
			WriteStdout(LWAnsiString_ToCStr(out));
			LWAnsiString_FreeString(out);
			return TRUE;
		}
		else
			return FALSE;
	}

}
