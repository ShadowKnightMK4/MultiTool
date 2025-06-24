
#include <LWAnsiString.h>
#include <Windows.h>
#include "whoami.h"
#include "common.h"


extern "C" {

bool helper_lookup_priv(LookUPPrivnameA lookup, PSID Target, HANDLE Unused, const char* PrefixName, const char* SuffixName)
{
	DWORD PrivNameSize = 256;
	HLOCAL PrivLocal = LocalAlloc(LMEM_ZEROINIT, PrivNameSize + 1);
	if (PrivLocal == nullptr)
	{
		return FALSE;
	}
	char* PrivNameContainer = (char*)LocalLock(PrivLocal);
	int res = lookup(0, (PLUID)Target, PrivNameContainer, &PrivNameSize);
	if ((res != 0) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
	{
		LocalUnlock(PrivLocal);
		LocalFree(PrivLocal);
		PrivLocal = LocalAlloc(LMEM_ZEROINIT, PrivNameSize + 1);
		if (PrivLocal == nullptr)
		{
			return FALSE;
		}
		else
		{
			PrivNameContainer = (char*)LocalLock(PrivLocal);
			res = lookup(0, (PLUID)Target, PrivNameContainer, &PrivNameSize);
		}
	}

	if (res != 0)
	{
		// output to std
		WriteStdout(PrefixName);
		WriteStdout(PrivNameContainer);
		WriteStdout(SuffixName);

	}
	if (PrivLocal != 0)
	{
		LocalUnlock(PrivLocal);
		LocalFree(PrivLocal);
	}
	return true;
}



//const char* SOCRiskNoticeString = "Notice: A process or User possessing a privilege does not inherently indicate malicious behavior; context is essential.For example if calc.exe has the SeDebugPrivilege, something is likely wrong, but seeing SeDebugPriv on the WinDbg.exe is probably fine. ";
const char* SOCRiskNoticeString = "Notice: A priv in a process or user token is not cause for alarm by itself. Ensure the Windows object (user/process) the token is from holds enabled privs as part of normal operation.  \r\n\r\n For example calc.exe having that SeDebugPrivilege could be compromise. While WinDbg.exe is probably fine having it if needed and actively in use in its role.";
PrivText PrivDecrip[] = 
{
   {"SeAssignPrimaryTokenPrivilege", "Allows assigning a primary token to another process.",
	"Can be used to hijack or escalate another process context if token manipulation is possible.", FLAG_YELLOW},

   {"SeAuditPrivilege", "Allows writing to the security audit log.",
	"Used by attackers to forge or suppress audit trail entries. Abused in stealth scenarios.", RED_RISK},

   {"SeBackupPrivilege", "Allows backup operations with bypass of ACL checks.",
	"Bypasses read ACLs. Can be used to dump sensitive files regardless of NTFS protection.", FLAG_YELLOW},

   {"SeChangeNotifyPrivilege", "Bypasses directory traversal checks and enables file change notifications.",
	nullptr, FLAG_INFORMAL},

   {"SeCreateGlobalPrivilege", "Allows creation of named global objects.",
	"Sometimes abused in terminal service or sandbox escape contexts, but usually benign.", FLAG_YELLOW},

   {"SeCreatePagefilePrivilege", "Allows creation of a system paging file.",
	nullptr, FLAG_INFORMAL},

   {"SeCreatePermanentPrivilege", "Allows creation of permanent named kernel objects.",
	"Rarely assigned. Potential persistence vector via object namespace abuse.", FLAG_YELLOW},

   {"SeCreateSymbolicLinkPrivilege", "Allows creation of symlinks.",
	"Can be abused for file or path spoofing, especially in developer or compromised contexts.", FLAG_YELLOW},

   {"SeCreateTokenPrivilege", "Allows creation of a primary security token.",
	"Very dangerous. Enables forging arbitrary user tokens. Should never be active in user mode.", RED_RISK},

   {"SeDebugPrivilege", "Allows debugging and code injection into any process.",
	"Effectively a free pass to access or inject into any process, including SYSTEM services.", RED_RISK},

   {"SeEnableDelegationPrivilege", "Allows marking accounts as trusted for delegation.",
	"Manipulation of this can escalate privilege over network resources via impersonation chaining.", FLAG_YELLOW},

   {"SeTcbPrivilege", "Allows acting as part of the operating system.",
	"This should *never* appear outside of select SYSTEM processes. Grants total authority, bypasses all security checks.", RED_RISK},

   {"SeImpersonatePrivilege", "Allows impersonation of authenticated clients.",
	"Commonly abused by malware and lateral tools (e.g., potato family) to escalate.", RED_RISK},

   {"SeIncreaseBasePriorityPrivilege", "Allows raising base scheduling priority.",
	"Normally benign, but could be used to starve other processes or hog CPU.", FLAG_INFORMAL},

   {"SeIncreaseQuotaPrivilege", "Allows increasing process memory quota.",
	"Rarely useful alone. Possible abuse to affect system stability or inject into memory-starved services.", FLAG_YELLOW},

   {"SeIncreaseWorkingSetPrivilege", "Allows increasing RAM commitment for a process.",
	nullptr, FLAG_INFORMAL},

   {"SeLoadDriverPrivilege", "Allows loading and unloading kernel drivers.",
	"Kernel drivers = ring 0. If a userland process has this, it can go full kernel-mode. Red alert.", RED_RISK},

   {"SeLockMemoryPrivilege", "Allows locking pages into RAM (non-swappable).",
	"Can be used to hide memory-resident payloads or prevent AV scanning.", FLAG_YELLOW},

   {"SeMachineAccountPrivilege", "Allows creating computer accounts in a domain.",
	"Potential abuse for machine spoofing or AD pivoting.", FLAG_YELLOW},

   {"SeManageVolumePrivilege", "Allows raw volume access operations.",
	"Access to raw volumes can bypass filesystem-level protections. Useful for exfil or tamper.", FLAG_YELLOW},

   {"SeProfileSingleProcessPrivilege", "Allows profiling a single process.",
	"Mild leakage vector. Could enable timing or memory mapping info, but rarely abused.", FLAG_INFORMAL},

   {"SeRelabelPrivilege", "Allows modifying an object’s integrity level.",
	"Can lower integrity checks and escape low box/sandbox environments.", FLAG_YELLOW},

   {"SeRemoteShutdownPrivilege", "Allows shutting down the system remotely.",
	"Could be used in destructive or DoS scenarios.", FLAG_YELLOW},

   {"SeRestorePrivilege", "Allows writing to files regardless of ACLs and changing file ownership.",
	"Can be used to overwrite protected files, inject system binaries, or hijack ownership.", RED_RISK},

   {"SeSecurityPrivilege", "Allows managing the security log and viewing SACLs.",
	"Access to SACLs and log control = stealth modification or audit deletion.", RED_RISK},

   {"SeShutdownPrivilege", "Allows shutting down the system locally.",
	"Only risky if present in unusual user-mode apps. Can be part of DoS or ransom logic.", FLAG_YELLOW},

   {"SeSyncAgentPrivilege", "Allows domain sync reads of all directory objects.",
	"High-risk in AD environments. Can dump sensitive AD schema and objects.", RED_RISK},

   {"SeSystemEnvironmentPrivilege", "Allows modification of firmware environment variables (NVRAM).",
	"Used for bootkit installs and secure boot tampering.", RED_RISK},

   {"SeSystemProfilePrivilege", "Allows system-wide performance profiling.",
	"Mildly useful to attackers doing timing or leakage, but rarely abused alone.", FLAG_INFORMAL},

   {"SeSystemtimePrivilege", "Allows changing the system time.",
	"Used to tamper with logs or misalign forensic timelines.", FLAG_YELLOW},

   {"SeTakeOwnershipPrivilege", "Allows taking ownership of any object.",
	"Bypasses ACLs for file takeover. Enables persistent hijack of system files.", RED_RISK },

   {"SeTimeZonePrivilege", "Allows setting the system timezone.",
	"Not usually risky, but may affect log correlation.", FLAG_INFORMAL},

   {"SeTrustedCredManAccessPrivilege", "Allows access to the Credential Manager as trusted caller.",
	"Credential stealing vector. Useful for lateral movement or recon.", RED_RISK },

   {"SeUndockPrivilege", "Allows undocking from a docking station.",
	nullptr, FLAG_INFORMAL},
	// additioanl privlages go between these comments

	// if the microsoft adds more priv, always put this one at the bottom.

		{nullptr, nullptr, nullptr, FLAG_INFORMAL}
};


#define YELLOW_RISK_STR " [!] "
#define RED_RISK_STR " [!!!!] "
#define INFORMAL_RISK_STR " [ ] "
void OutputPrivDescription(PLUID_AND_ATTRIBUTES x, LWAnsiString* output, const char* name)
{
	const char* risk_rate = 0;
	int len = 0;
	if (x->Attributes != 0)
	{
		for (int i = 0; ; i++)
		{
			if (PrivDecrip[i].Name == nullptr)
				break; // that's the hard stop
			if (lstrcmpA(PrivDecrip[i].Name, name) == 0)
			{
				// its a hit.

				len += lstrlenA(PrivDecrip[i].Name);

				if (PrivDecrip[i].Flags != FLAG_INFORMAL)
				{
					if (PrivDecrip[i].Flags == FLAG_YELLOW)
					{
						risk_rate = YELLOW_RISK_STR;

					}
					else if (PrivDecrip[i].Flags == RED_RISK)
					{
						risk_rate = RED_RISK_STR;

					}
				}
				else
				{
					risk_rate = INFORMAL_RISK_STR;
				}
				len += lstrlenA(risk_rate);
				LWAnsiString_Append(output, PrivDecrip[i].Name);
				LWAnsiString_Append(output, risk_rate);

				LWAnsiString_Append(output, " -  ");
				LWAnsiString_Append(output, PrivDecrip[i].Description);
				LWAnsiString_AppendNewLine(output);
				if (PrivDecrip[i].Risk != nullptr)
				{
					LWAnsiString_Append(output, "  Risk: ");
					LWAnsiString_Append(output, PrivDecrip[i].Risk);
					LWAnsiString_AppendNewLine(output);
				}
				break;
			}
		}
	}
}


/// <summary>
/// Common route for the whoami_priv flavors
/// </summary>
/// <param name="result">normal tool arg</param>
/// <param name="message_result">normal tool</param>
/// <param name="argv">normal tool</param>
/// <param name="argc">normal tool</param>
/// <param name="Output">writes output to this string</param>
/// <param name="source_from">says the source token is this</param>
/// <param name="IncludeSOCDesc">do we include the priv description and risk notices</param>
/// <param name="IncludeSOCRiskSetNotice">risk notice</param>
/// <param name="TargetToken">if 0 or invalid_handle_value - will be the current token of this running process</param>
/// <returns></returns>
bool WhoAmi_Write_Priv_common_string(int* result, const char** message_result, const char* argv[], int argc, LWAnsiString* Output, const char* source_from, bool IncludeSOCDesc, bool IncludeSOCRiskSetNotice, HANDLE TargetToken)
{
	HMODULE ADVAPI32 = 0;
	HMODULE SECUR32 = 0;
	GetTokenInfoPtr GetTokenInfoAPI = 0;
	LookupAccountSidA_PTR LookupSIDAPI = 0;
	LookUPPrivnameA LookUpPriv = 0;
	TOKEN_PRIVILEGES* Privs = nullptr;
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

	LWAnsiString* PrivName = LWAnsiString_CreateString(0);
	if (PrivName == nullptr)
	{
		if (message_result != nullptr)
		{
			*message_result = "Failed to allocate memory for privilege name";
		}
		if (result != nullptr)
		{
			*result = GetLastError();
		}
		FreeLibrary(ADVAPI32);
		FreeLibrary(SECUR32);
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
			LWAnsiString_FreeString(PrivName);
			FreeLibrary(ADVAPI32);
			FreeLibrary(SECUR32);
			return false; // we're dead in the water without that
		}
	}
	else
	{
		selfToken = TargetToken;
	}


	if (!GetTokenInfoAPI(selfToken, TokenPrivileges, nullptr, 0, &SizeNeeded))
	{
		if (SizeNeeded != 0)
		{
			Privs = (TOKEN_PRIVILEGES*)LocalAlloc(LMEM_ZEROINIT, SizeNeeded);
			if (Privs == nullptr)
			{
				if (message_result != nullptr)
				{
					*message_result = "Failed to allocate memory for token privileges";
				}
				if (result != nullptr)
				{
					*result = GetLastError();
				}
				LWAnsiString_FreeString(PrivName);
				FreeLibrary(ADVAPI32);
				FreeLibrary(SECUR32);
				if (TargetToken != selfToken) CloseHandle(selfToken);
				return false;
			}
			if (!GetTokenInfoAPI(selfToken, TokenPrivileges, Privs, SizeNeeded, &SizeNeeded))
			{
				if (message_result != nullptr)
				{
					*message_result = "Failed to get token privileges";
				}
				if (result != nullptr)
				{
					*result = GetLastError();
				}
				if (Privs != 0) LocalFree(Privs);
				LWAnsiString_FreeString(PrivName);
				FreeLibrary(ADVAPI32);
				FreeLibrary(SECUR32);
				if (TargetToken != selfToken) CloseHandle(selfToken);
				return false;
			}
		}
		else
		{
			if (message_result != nullptr)
			{
				*message_result = "No privileges found in token";
			}
			if (Privs != 0) LocalFree(Privs);
			LWAnsiString_FreeString(PrivName);
			if (TargetToken != selfToken) CloseHandle(selfToken);
			return true; // no privileges to display
		}
	}
	for (int i = 0; i < Privs->PrivilegeCount; i++)
	{
		SizeNeeded = 0;
		SetLastError(0);
		LWAnsiString_ZeroString(PrivName);
		LookUpPriv(nullptr, &Privs->Privileges[i].Luid, PrivName->Data, &SizeNeeded);
		{
			if ((GetLastError() == ERROR_INSUFFICIENT_BUFFER) || (GetLastError() == ERROR_MORE_DATA))
			{
				LWAnsiString_Reserve(PrivName, SizeNeeded);
				LookUpPriv(LWAnsiString_ToCStr(PrivName), &Privs->Privileges[i].Luid, PrivName->Data, &SizeNeeded);
			}
			LWAnsiString_Append(Output, "Privilege NAME: ");
			if (SizeNeeded == 0)
			{
				LWAnsiString_Append(Output, "\"Unknown Privilege\" ");
			}
			else
			{
				LWAnsiString_Append(Output, "\"");
				LWAnsiString_Append(Output, LWAnsiString_ToCStr(PrivName));
				LWAnsiString_Append(Output, "\" ");
			}
			if (Privs->Privileges[i].Attributes != 0)
			{
				bool hit = false;
				bool last_entry = false;
				LWAnsiString_Append(Output, "Attributes: [");
				if (Privs->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED)
				{
					LWAnsiString_Append(Output, "ENABLED");
					last_entry = true;
					hit = true;
				}
				if (Privs->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT)
				{
					if (last_entry)
					{
						LWAnsiString_Append(Output, ", ");
					}
					LWAnsiString_Append(Output, "ENABLED BY DEFAULT");
					last_entry = true;
					hit = true;
				}
				if (Privs->Privileges[i].Attributes & SE_PRIVILEGE_USED_FOR_ACCESS)
				{
					if (last_entry)
					{
						LWAnsiString_Append(Output, ", ");
					}
					LWAnsiString_Append(Output, "USED FOR ACCESS");
					last_entry = true;
					hit = true;
				}
				if (!hit)
				{
					LWAnsiString_Append(Output, "Unknown state");
				}
				LWAnsiString_Append(Output, "] ");
			}
			else
			{
				LWAnsiString_Append(Output, "Attributes: [ DISABLED ]");
			}
			//	LWAnsiString_Append(Output, "Number: <not supported yet/tdb");
			LWAnsiString_Append(Output, "\r\n");
		}
	}

	if (IncludeSOCDesc)
	{
		LWAnsiString_Append(Output, "\r\n");
		LWAnsiString_Append(Output, "\r\n\r\n*****\r\n");
		LWAnsiString_Append(Output, "Description of Privileges enabled\r\n*****\r\n\r\n");
		for (int i = 0; i < Privs->PrivilegeCount; i++)
		{
			if (Privs->Privileges[i].Attributes != 0)
			{
				SizeNeeded = 0;
				SetLastError(0);
				LWAnsiString_ZeroString(PrivName);
				LookUpPriv(nullptr, &Privs->Privileges[i].Luid, PrivName->Data, &SizeNeeded);
				{
					if ((GetLastError() == ERROR_INSUFFICIENT_BUFFER) || (GetLastError() == ERROR_MORE_DATA))
					{
						LWAnsiString_Reserve(PrivName, SizeNeeded);
						LookUpPriv(LWAnsiString_ToCStr(PrivName), &Privs->Privileges[i].Luid, PrivName->Data, &SizeNeeded);
					}
				}
				OutputPrivDescription(&Privs->Privileges[i], Output, PrivName->Data);
			}
		}
	}
	LWAnsiString_Append(Output, "\r\n");
	if (IncludeSOCRiskSetNotice)
	{
		LWAnsiString_Append(Output, "\r\n*****\r\n");
		LWAnsiString_Append(Output, SOCRiskNoticeString);
		LWAnsiString_Append(Output, "\r\n*****\r\n");
	}
	if (Privs != 0) LocalFree(Privs);
	LWAnsiString_FreeString(PrivName);
	FreeLibrary(ADVAPI32);
	FreeLibrary(SECUR32);
	if (TargetToken != selfToken) CloseHandle(selfToken);
	return true;
}


bool helper_WhoAmi_PrivString(int* result, const char** message_result, const char* argv[], int argc, const char* TokenName, HANDLE Token, LWAnsiString* Output)
{
	return WhoAmi_Write_Priv_common_string(result, message_result, argv, argc,  Output, TokenName, true, false, Token);
}


bool WhoAmi_WriteStdout_Priv(int* result, const char** message_result, const char* argv[], int argc)
{
	LWAnsiString* OutputString = LWAnsiString_CreateFromString("WhoAmI Privs: \r\n");
	if (OutputString != nullptr)
	{
		// 0 as the last are is shortcut for says look at the current process token
		auto res = WhoAmi_Write_Priv_common_string(result, message_result, argv, argc, OutputString, "User Token Source\r\n", true, true, 0);
		WriteStdout(LWAnsiString_ToCStr(OutputString));
		LWAnsiString_FreeString(OutputString);
		return res;
	}
	return false;
}


}