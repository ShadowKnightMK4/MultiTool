#include "whoami.h"
#include <LWAnsiString.h>

void GroupFlagToOutput(DWORD Flag, LWAnsiString* Out)
{
	bool SkipIncludeSpace = false;
	//while (Flag != 0)
	{
		if ((Flag && SE_GROUP_ENABLED) != 0)
		{
			LWAnsiString_Append(Out, "Enabled");
			SkipIncludeSpace = true;
			Flag &= ~SE_GROUP_ENABLED;
		}
		else
		{
			LWAnsiString_Append(Out, "Disable");
			SkipIncludeSpace = true;
		}
		
			LWAnsiString_Append(Out, " ");

		if ((Flag && SE_GROUP_ENABLED_BY_DEFAULT) != 0)
		{
			LWAnsiString_Append(Out, "DefaultEnable");
			SkipIncludeSpace = true;
			Flag &= ~SE_GROUP_ENABLED_BY_DEFAULT;
		
				LWAnsiString_Append(Out, " ");
		}


		if ((Flag && SE_GROUP_INTEGRITY) != 0)
		{
			LWAnsiString_Append(Out, "ManIntegritySID");
			//IncludeSpace = true;
			Flag &= ~SE_GROUP_INTEGRITY;
		//	if (!IncludeSpace)
				LWAnsiString_Append(Out, " ");
		}

		if ((Flag && SE_GROUP_INTEGRITY_ENABLED) != 0)
		{
			LWAnsiString_Append(Out, "Active-ManIntegritySid");
		//	IncludeSpace = true;
			Flag &= ~SE_GROUP_INTEGRITY_ENABLED;
			//if (!IncludeSpace)
				LWAnsiString_Append(Out, " ");
		}
		else
		{
			LWAnsiString_Append(Out, "Disabled-ManIntegritySid");
			//IncludeSpace = true;
			//if (!IncludeSpace)
				LWAnsiString_Append(Out, " ");
		}


		if ((Flag && SE_GROUP_LOGON_ID) != 0)
		{
			LWAnsiString_Append(Out, "LogonSid");
			//IncludeSpace = true;
			Flag &= ~SE_GROUP_LOGON_ID;
		//	if (!IncludeSpace)
				LWAnsiString_Append(Out, " ");
		}


		if ((Flag && SE_GROUP_MANDATORY) != 0)
		{
			LWAnsiString_Append(Out, "RequiredSID-CannotAdjust");
			//IncludeSpace = true;
			Flag &= ~SE_GROUP_MANDATORY;
			//if (!IncludeSpace)
				LWAnsiString_Append(Out, " ");
		}

		if ((Flag && SE_GROUP_OWNER) != 0)
		{
			LWAnsiString_Append(Out, "GroupOwner");
			//IncludeSpace = true;
			Flag &= ~SE_GROUP_OWNER;
			//if (!IncludeSpace)
				LWAnsiString_Append(Out, " ");
		}

		if ((Flag && SE_GROUP_RESOURCE) != 0)
		{
			LWAnsiString_Append(Out, "GroupResource");
			//IncludeSpace = true;
			Flag &= ~SE_GROUP_RESOURCE;
		}

	}
}
void helper_who_ami_usertoken_token_groups_string(int* result, const char** message_result, const char* argv[], int argc, TOKEN_GROUPS* target, LookupAccountSidA_PTR LookupSid, LWAnsiString* Output)
{
	LWAnsiString* NameOfGroup = LWAnsiString_CreateString(0);
	LWAnsiString_Append(Output, "Groups in this Token:: ");
	for (int i = 0; i < target->GroupCount; i++)
	{
		LWAnsiString_Append(Output, "Group: ");
		helper_lookup_sid(LookupSid, target->Groups[i].Sid, 0, 0, "\r\n", 0, "\r\n", true, NameOfGroup);
		LWAnsiString_Append(Output, LWAnsiString_ToCStr(NameOfGroup));
		LWAnsiString_Pad(Output, ' ', 2);
		LWAnsiString_Append(Output, ": [");
		GroupFlagToOutput(target->Groups[i].Attributes, Output);
		LWAnsiString_Append(Output, " ]");
		LWAnsiString_AppendNewLine(Output);
	}

	return;
	for (int i = 0; i < target->GroupCount; i++)
	{
		// emit the display name for the sid
		helper_lookup_sid(LookupSid, target->Groups[i].Sid, 0, "SID: ", "\r\n", "Domain: ", "\r\n", true, Output);
		if (target->Groups[i].Attributes & SE_GROUP_ENABLED)
		{
			LWAnsiString_Append(Output, "  Attributes: Enabled");
			//WriteStdout("  Attributes: Enabled");
		}
		else if (target->Groups[i].Attributes & SE_GROUP_ENABLED_BY_DEFAULT)
		{
			LWAnsiString_Append(Output, "  Attributes: Enabled by Default");
			//WriteStdout("  Attributes: Enabled by Default");
		}
		else if (target->Groups[i].Attributes & SE_GROUP_OWNER)
		{
			LWAnsiString_Append(Output, "  Attributes: Owner");
		}
		else if (target->Groups[i].Attributes & SE_GROUP_LOGON_ID)
		{
			LWAnsiString_Append(Output, "  Attributes: Logon ID");
		}
		else if (target->Groups[i].Attributes & SE_GROUP_RESOURCE)
		{
			LWAnsiString_Append(Output, "  Attributes: Resource");
		}
		else if (target->Groups[i].Attributes & SE_GROUP_MANDATORY)
		{
			LWAnsiString_Append(Output, "  Attributes: Mandatory");
		}
		else if (target->Groups[i].Attributes & SE_GROUP_ENABLED_BY_DEFAULT)
		{
			LWAnsiString_Append(Output, "  Attributes: Enabled by Default");
		}
		else if (target->Groups[i].Attributes & SE_GROUP_USE_FOR_DENY_ONLY)
		{
			LWAnsiString_Append(Output, "  Attributes: Use for Deny Only");
		}
		else
		{
			LWAnsiString_Append(Output, "  Attributes: Not Enabled");
		}
		LWAnsiString_Append(Output, "\r\n");
	}
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

	LWAnsiString_FreeString(Output);
	return TRUE;
}

bool WhoAmi_WriteStdout_UserGroups(int* result, const char** message_result, const char* argv[], int argc)
{
	return FALSE;
}
