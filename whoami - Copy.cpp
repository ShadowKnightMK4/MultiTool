
#include "common.h"
#include "osver.h"
#include <LWAnsiString.h>
#define SECURITY_WIN32 1
#include <security.h>
typedef BOOL(WINAPI* GetUserNameAPTR)(LPSTR, LPDWORD);
typedef BOOL(WINAPI* GetUserNameEX_PTR)(INT, LPSTR, PULONG);
typedef BOOL(WINAPI* LookupAccountSidA_PTR)(
	LPCSTR        lpSystemName,
	PSID          Sid,
	LPSTR         Name,
	LPDWORD       cchName,
	LPSTR         ReferencedDomainName,
	LPDWORD       cchReferencedDomainName,
	PSID_NAME_USE peUse
	);
typedef BOOL(WINAPI* GetTokenInfoPtr)(
	HANDLE TokenHandle,
	TOKEN_INFORMATION_CLASS TokenInformationClass,
	LPVOID TokenInformation,
	DWORD TokenInformationLength,
	LPDWORD ReturnLength
	);
typedef BOOL(WINAPI* LookUPPrivnameA)(
	LPCSTR lpSystemName,
	PLUID lpLuid,
	LPSTR lpName,
	LPDWORD cchName
	);
struct NameType
{
	EXTENDED_NAME_FORMAT Type;
	const char* DisplayType;
};



typedef int FlagType;

#define FLAG_INFORMAL 0
#define FLAG_YELLOW 1
#define REG_RISK 2
struct PrivText
{
	const char* Name;
	const char* Description;
	const char* Risk;
	FlagType Flags;  
};

//const char* SOCRiskNoticeString = "Notice: A process or User possessing a privilege does not inherently indicate malicious behavior; context is essential.For example if calc.exe has the SeDebugPrivilege, something is likely wrong, but seeing SeDebugPriv on the WinDbg.exe is probably fine. ";
const char* SOCRiskNoticeString = "Notice: A priv in a process or user token is not cause for alarm by itself. Ensure the Windows object (user/process) the token is from holds enabled privs as part of normal operation.  \r\n\r\n For example calc.exe having that SeDebugPrivilege could be compromise. While WinDbg.exe is probably fine having it if needed and actively in use in its role.";
PrivText PrivDecrip[] = /*
{
	{"SeAssignPrimaryTokenPrivilege", "Allows a process to assign a primary token to another process."},
	{"SeAuditPrivilege", "Allows a process to generate audit records."},
	{"SeBackupPrivilege", "Allows a process to back up files and directories.", "Skips ACL checks when active. Essentially makes them useless"},
	{"SeChangeNotifyPrivilege", "Allows a process to bypass traverse checking."},
	{"SeCreateGlobalPrivilege", "Allows a process to create global objects."},
	{"SeCreatePagefilePrivilege", "Allows a process to create page files."},
	{"SeCreatePermanentPrivilege", "Allows a process to create permanent objects."},
	{"SeCreateSymbolicLinkPrivilege", "Allows a process to create symbolic links."},
	{"SeDebugPrivilege", "Allows a process to debug other processes created by any user.", "Effectively a free pass to any process if active including core system processes"},
	{"SeEnableDelegationPrivilege", "Allows a process to enable delegation."},
	{"SeTcbPrivilege", "Act as part of the operating system.", "This should *never* appear outside of SYSTEM. Grants total authority to bypass security checks, inpersonate to be any user on it and  change Windows tokens. This is full SYSTEM access if active. Linux people: this is root."}
	*/


 {
	{"SeAssignPrimaryTokenPrivilege", "Allows assigning a primary token to another process.",
	 "Can be used to hijack or escalate another process context if token manipulation is possible.", FLAG_YELLOW},

	{"SeAuditPrivilege", "Allows writing to the security audit log.",
	 "Used by attackers to forge or suppress audit trail entries. Abused in stealth scenarios.", REG_RISK},

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
	 "Very dangerous. Enables forging arbitrary user tokens. Should never be active in user mode.", REG_RISK},

	{"SeDebugPrivilege", "Allows debugging and code injection into any process.",
	 "Effectively a free pass to access or inject into any process, including SYSTEM services.", REG_RISK},

	{"SeEnableDelegationPrivilege", "Allows marking accounts as trusted for delegation.",
	 "Manipulation of this can escalate privilege over network resources via impersonation chaining.", FLAG_YELLOW},

	{"SeTcbPrivilege", "Allows acting as part of the operating system.",
	 "This should *never* appear outside of select SYSTEM processes. Grants total authority, bypasses all security checks.", REG_RISK},

	{"SeImpersonatePrivilege", "Allows impersonation of authenticated clients.",
	 "Commonly abused by malware and lateral tools (e.g., potato family) to escalate.", REG_RISK},

	{"SeIncreaseBasePriorityPrivilege", "Allows raising base scheduling priority.",
	 "Normally benign, but could be used to starve other processes or hog CPU.", FLAG_INFORMAL},

	{"SeIncreaseQuotaPrivilege", "Allows increasing process memory quota.",
	 "Rarely useful alone. Possible abuse to affect system stability or inject into memory-starved services.", FLAG_YELLOW},

	{"SeIncreaseWorkingSetPrivilege", "Allows increasing RAM commitment for a process.",
	 nullptr, FLAG_INFORMAL},

	{"SeLoadDriverPrivilege", "Allows loading and unloading kernel drivers.",
	 "Kernel drivers = ring 0. If a userland process has this, it can go full kernel-mode. Red alert.", REG_RISK},

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
	 "Can be used to overwrite protected files, inject system binaries, or hijack ownership.", REG_RISK},

	{"SeSecurityPrivilege", "Allows managing the security log and viewing SACLs.",
	 "Access to SACLs and log control = stealth modification or audit deletion.", REG_RISK},

	{"SeShutdownPrivilege", "Allows shutting down the system locally.",
	 "Only risky if present in unusual user-mode apps. Can be part of DoS or ransom logic.", FLAG_YELLOW},

	{"SeSyncAgentPrivilege", "Allows domain sync reads of all directory objects.",
	 "High-risk in AD environments. Can dump sensitive AD schema and objects.", REG_RISK},

	{"SeSystemEnvironmentPrivilege", "Allows modification of firmware environment variables (NVRAM).",
	 "Used for bootkit installs and secure boot tampering.", REG_RISK},

	{"SeSystemProfilePrivilege", "Allows system-wide performance profiling.",
	 "Mildly useful to attackers doing timing or leakage, but rarely abused alone.", FLAG_INFORMAL},

	{"SeSystemtimePrivilege", "Allows changing the system time.",
	 "Used to tamper with logs or misalign forensic timelines.", FLAG_YELLOW},

	{"SeTakeOwnershipPrivilege", "Allows taking ownership of any object.",
	 "Bypasses ACLs for file takeover. Enables persistent hijack of system files.", REG_RISK},

	{"SeTimeZonePrivilege", "Allows setting the system timezone.",
	 "Not usually risky, but may affect log correlation.", FLAG_INFORMAL},

	{"SeTrustedCredManAccessPrivilege", "Allows access to the Credential Manager as trusted caller.",
	 "Credential stealing vector. Useful for lateral movement or recon.", REG_RISK},

	{"SeUndockPrivilege", "Allows undocking from a docking station.",
	 nullptr, FLAG_INFORMAL},
// additioanl privlages go between these comments

// if the microsoft adds more priv, always put this one at the bottom.

	{nullptr, nullptr, nullptr, FLAG_INFORMAL}
};


NameType NameTypes[] = {
  { NameUnknown, "NameUnknown" },
  { NameFullyQualifiedDN, "Fully Qualified Distinguished Name" },
  { NameSamCompatible, "SAM or Legacy Account Name" },
  { NameDisplay, "Display Name" },
  { NameUniqueId, "Unique Account GUID" },
  { NameCanonical, "Canonical Name" },
  { NameCanonicalEx, "Canonical Name (Extended)" }, // MISSING
  { NameUserPrincipal, "User Principal Name" },
  { NameServicePrincipal, "Service Principal Name" },
  { NameDnsDomain, "DNS Domain Name" },
  { NameGivenName, "Given Name" },
  { NameSurname, "Surname" },
  { NameUnknown, 0 } // sentinel
}; // also missing user perincibple


#include <windows.h>
#include <ntsecapi.h> // For TOKEN_MANDATORY_LABEL
#include <winnt.h>    // Core token structures
#include <securitybaseapi.h> // TOKEN_GROUPS_AND_PRIVILEGES, etc.

// Union that can hold any GetTokenInformation return type (by value)
typedef union TokenInformationUnion {
	TOKEN_USER*                      TokenUser;
	TOKEN_GROUPS*                    TokenGroups;
	TOKEN_PRIVILEGES*                TokenPrivileges;
	TOKEN_OWNER *                    TokenOwner;
	TOKEN_PRIMARY_GROUP *            TokenPrimaryGroup;
	TOKEN_DEFAULT_DACL   *           TokenDefaultDacl;
	TOKEN_SOURCE          *          TokenSource;
	TOKEN_TYPE             *         TokenType;
	SECURITY_IMPERSONATION_LEVEL*   ImpersonationLevel;
	TOKEN_STATISTICS             *   TokenStatistics;
	TOKEN_GROUPS_AND_PRIVILEGES  *  TokenGroupsAndPrivileges;
	TOKEN_ORIGIN                 *   TokenOrigin;
	TOKEN_ELEVATION_TYPE         *  ElevationType;
	TOKEN_LINKED_TOKEN           *   LinkedToken;
	TOKEN_ELEVATION              *   Elevation;
	TOKEN_ACCESS_INFORMATION     *   AccessInfo;
	TOKEN_MANDATORY_LABEL        *   IntegrityLevel;
	TOKEN_MANDATORY_POLICY       *   MandatoryPolicy;
	TOKEN_APPCONTAINER_INFORMATION * AppContainer;
	CLAIM_SECURITY_ATTRIBUTES_INFORMATION* UserClaims;
	//TOKEN_PROCESS_TRUST_LEVEL *      TrustLevel;

	DWORD                       *    DwordValue;
	VOID* TheVoid;
	//BYTE                            RawBuffer[4096]; // For unknown/undocumented types or overflows
} TokenInformationUnion;

#define YELLOW_RISK_STR " [!] "
#define RED_RISK_STR " [!!!!] "
#define INFORMAL_RISK_STR " [ ] "
void OutputPrivDescription(PLUID_AND_ATTRIBUTES x, LWAnsiString* output, const char* name)
{
	const char* risk_rate;
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
					else if (PrivDecrip[i].Flags == REG_RISK)
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

bool helper_lookup_sid_domain_name(LookupAccountSidA_PTR lookup, PSID Target, LWAnsiString* Output)
{
	DWORD TokenNameSize = 0;
	DWORD DomainNameSize = 0;
	SID_NAME_USE SidUse;

	HLOCAL DomainLocal = 0;
	if ((!lookup(0, Target, nullptr, &TokenNameSize, nullptr, &DomainNameSize, &SidUse) && (GetLastError() != ERROR_INSUFFICIENT_BUFFER)))
	{
		return FALSE;
	}
	else
	{
		DomainLocal = LocalAlloc(LMEM_ZEROINIT, DomainNameSize + 1);
		if (DomainLocal == nullptr)
		{
			return FALSE;
		}

		char* DomainNameContainer = (char*)LocalLock(DomainLocal);



		int res = lookup(0, Target, 0, &TokenNameSize, DomainNameContainer, &DomainNameSize, &SidUse);
		if (res != 0)
		{
			LWAnsiString_Append(Output, DomainNameContainer);
			LocalFree(DomainLocal);
			return true;
		}
		else
		{
			LocalFree(DomainLocal);
			return false;
		}
	}

}


bool helper_lookup_sid_display_name(LookupAccountSidA_PTR lookup, PSID Target, LWAnsiString* Output)
{
	DWORD TokenNameSize = 0;
	DWORD DomainNameSize = 0;
	SID_NAME_USE SidUse;
	
		HLOCAL TokenLocal = 0;
		if ((!lookup(0, Target, nullptr, &TokenNameSize, nullptr, &DomainNameSize, &SidUse) && (GetLastError() != ERROR_INSUFFICIENT_BUFFER)))
		{
			return FALSE;
		}
		else
		{
			TokenLocal = LocalAlloc(LMEM_ZEROINIT, TokenNameSize + 1);
			if (TokenLocal == nullptr)
			{
				return FALSE;
			}

			char* TokenNameContainer = (char*)LocalLock(TokenLocal);



			int res = lookup(0, Target, TokenNameContainer, &TokenNameSize, 0, &DomainNameSize, &SidUse);
			if (res != 0)
			{
				LWAnsiString_Append(Output, TokenNameContainer);
				LocalFree(TokenLocal);
				return true;
			}
			else
			{
				LocalFree(TokenLocal);
				return false;
			}
		}

}
bool helper_lookup_sid(LookupAccountSidA_PTR lookup, PSID Target, HANDLE Unused, const char* PrefixName, const char* SuffixName, const char* PrefixDomain, const char* SuffixDomain, bool IncludeDomain, LWAnsiString* Output)
{
	LWAnsiString* SIDNAME = LWAnsiString_CreateString(0);
	LWAnsiString* DomainName = LWAnsiString_CreateString(0);

	helper_lookup_sid_display_name(lookup, Target, (SIDNAME));
	if (PrefixName != nullptr)
	{
		LWAnsiString_Append(Output, PrefixName);
	}
	LWAnsiString_Append(Output, LWAnsiString_ToCStr(SIDNAME));
	if (SuffixName != nullptr)
	{
		LWAnsiString_Append(Output, SuffixName);
	}

	if (IncludeDomain)
	{
		helper_lookup_sid_domain_name(lookup, Target, DomainName);
		if (PrefixDomain != nullptr)
		{
			LWAnsiString_Append(Output, PrefixName);
		}
		LWAnsiString_Append(Output, LWAnsiString_ToCStr(DomainName));
		if (SuffixDomain != nullptr)
		{
			LWAnsiString_Append(Output, SuffixName);
		}
	}



	LWAnsiString_FreeString(SIDNAME);
	LWAnsiString_FreeString(DomainName);
	return true;
}



void helper_who_ami_priv_display_stdout(int* result, const char** message_result, const char* argv[], int argc, TOKEN_PRIVILEGES* target, LookUPPrivnameA LookupPriv)
{
	if (target == nullptr || LookupPriv == nullptr)
	{
		if (message_result != nullptr)
		{
			*message_result = "Invalid parameters passed to helper_who_ami_priv_display_stdout";
		}
		if (result != nullptr)
		{
			*result = -1;
		}
		return;
	}
	for (DWORD i = 0; i < target->PrivilegeCount;i++)
	{
		// emit the display name for the privilege
		if (!helper_lookup_priv(LookupPriv, &target->Privileges[i].Luid, 0, "Privilege: ", "\r\n"))
		{
			if (message_result != nullptr)
			{
				*message_result = "Failed to lookup privilege";
			}
			if (result != nullptr)
			{
				*result = GetLastError();
			}
			return;
		}
		if (target->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED)
		{
			WriteStdout("  Attributes: Enabled");
		}
		else if (target->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT)
		{
			WriteStdout("  Attributes: Enabled by Default");
		}
		else if (target->Privileges[i].Attributes & SE_PRIVILEGE_USED_FOR_ACCESS)
		{
			WriteStdout("  Attributes: Used for Access");
		}
		else
		{
			WriteStdout("  Attributes: Not Enabled");
		}
		WriteStdout("\r\n");
	}
}
void helper_who_ami_usertoken_token_groups_stdout(int* result, const char** message_result, const char* argv[], int argc, TOKEN_GROUPS* target, LookupAccountSidA_PTR LookupSid, LWAnsiString* Output)
{
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
			WriteStdout("  Attributes: Enabled by Default");
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

void helper_who_ami_token_stdout(int* result, const char** message_result, const char* argv[], int argc, TOKEN_USER* target, LookupAccountSidA_PTR LookupSID)
{
	if (target == nullptr || LookupSID == nullptr)
	{
		if (message_result != nullptr)
		{
			*message_result = "Invalid parameters passed to helper_who_ami_usertoken_stdout";
		}
		if (result != nullptr)
		{
			*result = -1;
		}
		return;
	}
}


void helper_who_ami_usertoken_stdout(int* result, const char** message_result, const char* argv[], int argc, TOKEN_USER* target, LookupAccountSidA_PTR LookupSID)
{
	if (target == nullptr || LookupSID == nullptr)
	{
		if (message_result != nullptr)
		{
			*message_result = "Invalid parameters passed to helper_who_ami_usertoken_stdout";
		}
		if (result != nullptr)
		{
			*result = -1;
		}
		return;
	}
	if (target->User.Sid == nullptr)
	{
		if (message_result != nullptr)
		{
			*message_result = "Token User SID is null";
		}
		if (result != nullptr)
		{
			*result = -2;
		}
		return;
	}
	//if (!helper_lookup_sid(LookupSID, target->User.Sid, 0, "User Name: ", nullptr, "Domain: ", nullptr, true))
	{
		if (message_result != nullptr)
			*message_result = "Failed to lookup Token User SID";

		if (result != nullptr)
			*result = GetLastError();

		return;
	}
}

// NOT INTENTED TO BE CALLED EXCEPT BY helper_WhoAmi_ShowUserPriv
bool ResolveTokenDlls(HMODULE* Advapi32, GetTokenInfoPtr* GetTokenInfoAPI, LookupAccountSidA_PTR* LookupSIDAPI, LookUPPrivnameA* LookUpPriv, const char** result)
{
	*Advapi32 = LoadLibraryA("advapi32.dll");
	*GetTokenInfoAPI = (GetTokenInfoPtr)GetProcAddress(*Advapi32, "GetTokenInformation");
	*LookupSIDAPI = (LookupAccountSidA_PTR)GetProcAddress(*Advapi32, "LookupAccountSidA");
	*LookUpPriv = (LookUPPrivnameA)GetProcAddress(*Advapi32, "LookupPrivilegeNameA");
	if (*Advapi32 == nullptr || *GetTokenInfoAPI == nullptr || *LookupSIDAPI == nullptr || LookUpPriv == nullptr)
	{
		if (result != nullptr)
		{
			*result = "Failed to load advapi32.dll or get GetTokenInformation or LookupAccountSidA or LookupPrivilegeNameA";
		}
		if (*Advapi32 != nullptr) FreeLibrary(*Advapi32);
		return false;
	}
	return true;
}
bool helper_WhoAmi_ShowUserPriv(int* result, const char** message_result, const char* argv[], int argc)
{

#define SELF GetCurrentProcess()
	HANDLE selfToken = 0;
	HMODULE Advapi32 = 0;
	GetTokenInfoPtr GetTokenInfoAPI = 0;
	LookupAccountSidA_PTR LookupSIDAPI = 0;
	LookUPPrivnameA LookUpPriv = 0;
	BYTE Buffer[256];
	if (!ResolveTokenDlls(&Advapi32, &GetTokenInfoAPI, &LookupSIDAPI,&LookUpPriv, message_result))
		return false;

	if (!OpenProcessToken(SELF, TOKEN_QUERY | TOKEN_QUERY_SOURCE, &selfToken))
	{
		return false;// we're dead in the water without that
	}
	else
	{
		TokenInformationUnion tokenInfo;
		tokenInfo.AccessInfo=nullptr;


		DWORD size = 0;
		TOKEN_INFORMATION_CLASS stepper = TokenUser;
		HLOCAL tokenMem = 0;
		for (; stepper != MaxTokenInfoClass; )
		{
			if (!GetTokenInfoAPI(selfToken, stepper, 0, 0, &size) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
			{
				if (tokenMem != 0)
				{
					LocalFree(tokenMem); // free the previous one
					tokenMem = 0;
					tokenInfo.TheVoid = nullptr;
				}
				tokenMem = LocalAlloc(LMEM_ZEROINIT, size);
				tokenInfo.TheVoid = LocalLock(tokenMem);

 				if (GetTokenInfoAPI(selfToken, stepper, tokenInfo.TheVoid, size, &size))
				{
					switch (stepper)
					{
					case TokenUser:
					{
						helper_who_ami_usertoken_stdout(result, message_result, argv, argc, tokenInfo.TokenUser, LookupSIDAPI);


						break;
					}
					case TokenGroups:
					{

					//	helper_who_ami_usertoken_token_groups_stdout(result, message_result, argv, argc, tokenInfo.TokenGroups, LookupSIDAPI);
						break;
					}
					case TokenPrivileges:
					{
						helper_who_ami_priv_display_stdout(result, message_result, argv, argc, tokenInfo.TokenPrivileges, LookUpPriv);

					}
					default:
					{
						break;
					}
					}
				}
				DWORD Debug = GetLastError();
			}
			stepper = (TOKEN_INFORMATION_CLASS)((int)stepper + 1);
		
		}
		CloseHandle(selfToken);
	}
}
#undef SELF

bool helper_WhoAmi_UserTokenDump(int* result, const char** message_result, const char* argv[], int argc, LWAnsiString* Output)
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
							helper_who_ami_usertoken_stdout(result, message_result, argv, argc, tokenInfo.TokenUser, LookupSIDAPI);
							break;
						}
						case TokenGroups:
						{
							helper_who_ami_usertoken_token_groups_stdout(result, message_result, argv, argc, tokenInfo.TokenGroups, LookupSIDAPI, Output);
							break;
						}
						case TokenPrivileges:
						{
							helper_who_ami_priv_display_stdout(result, message_result, argv, argc, tokenInfo.TokenPrivileges, LookUpPriv);
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

bool helper_WhoAmi_UserInformation(int* result, const char** message_result, const char* argv[], int argc, LWAnsiString* Output)
{
	DWORD SIZE;
	SIZE = 50;
	
	if (result == nullptr || message_result == nullptr || Output == nullptr)
	{
		// because there's no meninafly way to set a return falue
		return false;
	}
	LWAnsiString* UserNameStuff = LWAnsiString_CreateString(SIZE);
	*result = 0;
	*message_result = nullptr;
	HMODULE ADVAPI32 = LoadLibraryA("advapi32.dll");
	HMODULE SECUR32 = LoadLibraryA("secur32.dll");
	GetUserNameAPTR GetUserNameAPtr = 0;
	GetUserNameEX_PTR GetUserNameExPtr = 0;
	if (ADVAPI32 != 0)
		GetUserNameAPtr = (GetUserNameAPTR)GetProcAddress(ADVAPI32, "GetUserNameA");
	if (SECUR32 != 0)
		GetUserNameExPtr = (GetUserNameEX_PTR)GetProcAddress(SECUR32, "GetUserNameExA");
	// if we failed to get both, we can't do anything
	if (GetUserNameAPtr == nullptr && GetUserNameExPtr == nullptr)
	{
		*message_result = "Failed to get user name";
		*result = GetLastError();
		LWAnsiString_FreeString(UserNameStuff); 
	}
	else
	{
		
		if (GetUserNameExPtr != 0)
		{
			for (EXTENDED_NAME_FORMAT step = NameFullyQualifiedDN; step <= NameSurname; step = (EXTENDED_NAME_FORMAT)((int)step + 1))
			{
				if (step == 4)
					continue;
				if (step == 5)
					continue;
				SetLastError(0);
				SIZE = 0;
				if ((!GetUserNameExPtr(step, nullptr, &SIZE)))
				{
					if ( (GetLastError() == ERROR_INSUFFICIENT_BUFFER) || (GetLastError() == ERROR_MORE_DATA))
					{
						if (!LWAnsiString_Reserve(UserNameStuff, SIZE))
						{
							FreeLibrary(ADVAPI32);
							FreeLibrary(SECUR32);
							*result = -1;
							*message_result = "Out of memory";
							LWAnsiString_FreeString(UserNameStuff);
							return false;
						}
					}
					else
					{
						// skip it. Mayne not supported
				
	
					}
				}

				LWAnsiString_Reserve(UserNameStuff, SIZE);
				if (SIZE != 0)
				{
					if (GetUserNameExPtr(step, UserNameStuff->Data, &SIZE))
					{
						for (int i = 1; ; i++)
						{
							const char* cdebug = NameTypes[i].DisplayType;
							EXTENDED_NAME_FORMAT kdebug = NameTypes[i].Type;
							if ( (NameTypes[i].Type == NameUnknown) && (NameTypes[i].DisplayType == nullptr))
							{
								// the long line is over. There's no more
								break;
							}
							else
							{
								if (NameTypes[i].Type == step)
								{
									LWAnsiString_Append(Output, NameTypes[i].DisplayType);
									break;
								}
								
							}
						}
						LWAnsiString_Append(Output, ": ");
						LWAnsiString_Append(Output, LWAnsiString_ToCStr(UserNameStuff));
						LWAnsiString_Append(Output, "\r\n");
						LWAnsiString_ZeroString(UserNameStuff);
					}
				}
				
			}
		}
		else
		{
			if (GetUserNameAPtr != 0)
			{
				SetLastError(0);
				SIZE = GetUserNameAPtr(nullptr, &SIZE);
				if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
				{
					// something went wrong
					*message_result = "Failed to get user name";
					*result = GetLastError();
					if (ADVAPI32) FreeLibrary(ADVAPI32);
					if (SECUR32) FreeLibrary(SECUR32);
					LWAnsiString_FreeString(UserNameStuff);
					return false;
				}
				else
				{
					LWAnsiString_Reserve(UserNameStuff, SIZE);
					if(UserNameStuff == nullptr)
					{
						*message_result = "Failed to allocate memory for user name";
						*result = GetLastError();
						if (ADVAPI32) FreeLibrary(ADVAPI32);
						if (SECUR32) FreeLibrary(SECUR32);
						return false;
					}
					else
					{
						SetLastError(0);
						if (GetUserNameAPtr(UserNameStuff->Data, &SIZE))
						{
							LWAnsiString_Append(Output, "self: ");
							LWAnsiString_Append(Output, LWAnsiString_ToCStr(UserNameStuff));
							LWAnsiString_Append(Output, "\r\n");
						}
						else
						{
							*message_result = "Failed to get user name";
							*result = GetLastError();
							if (ADVAPI32) FreeLibrary(ADVAPI32);
							if (SECUR32) FreeLibrary(SECUR32);
							LWAnsiString_FreeString(UserNameStuff);
							return false;
						}
					}
				}
			}
		}
	}

	if (UserNameStuff != 0)
	{
		LWAnsiString_FreeString(UserNameStuff);
		UserNameStuff = nullptr;
	}
	return true;
}
bool Oldhelper_WhoAmi_UserInformation(int* result, const char** message_result, const char* argv[], int argc)
{

	if (result == nullptr || message_result == nullptr)
	{
		// because there's no meninafly way to set a return falue
		return false;
	}
	*result = 0;
	*message_result = nullptr;
	HMODULE ADVAPI32 = LoadLibraryA("advapi32.dll");
	HMODULE SECUR32 = LoadLibraryA("secur32.dll");
	GetUserNameAPTR GetUserNameA = 0;
	GetUserNameEX_PTR GetUserNameExPtr = 0;
	if (ADVAPI32 != 0)
		GetUserNameA = (GetUserNameAPTR)GetProcAddress(ADVAPI32, "GetUserNameA");

	if (SECUR32 != 0)
		GetUserNameExPtr = (GetUserNameEX_PTR)GetProcAddress(SECUR32, "GetUserNameExA");

	// if we failed to get both, we can't do anything
	if (GetUserNameA == nullptr && GetUserNameExPtr == nullptr)
	{
		*message_result = "Failed to get user name";
		*result = GetLastError();
		return false;
	}
	else
	{
		DWORD SIZE;
		SIZE = 256;
	again:
		HGLOBAL UserName = LocalAlloc(LMEM_ZEROINIT, SIZE);

		if (UserName == 0)
		{
			*result = -1;
			*message_result = "Failed to allocate memory for user name";
			return false;
		}

		void* ptr = LocalLock(UserName);
		SetLastError(0);
		if (GetUserNameA != 0)
		{
			if (!GetUserNameA((char*)ptr, &SIZE))
			{
				if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
				{
					LocalFree(UserName);
					UserName = LocalAlloc(LMEM_ZEROINIT, SIZE);
					goto again;
				}
			}
			else
			{
				WriteStdout("self: ");
				WriteStdout((char*)ptr);
				WriteStdout("\r\n");
			}
		}

		{
			if (GetUserNameExPtr != 0)
			{
				int i = 0;
				for (; i < 12; i++)
				{
					SetLastError(0);
					if (GetUserNameExPtr(NameTypes[i].Type, (char*)ptr, &SIZE))
					{
						WriteStdout(NameTypes[i].DisplayType);
						WriteStdout(": ");
						WriteStdout((char*)ptr);
						WriteStdout("\r\n");
					}
					else
					{
						//WriteStdout(NameTypes[i].DisplayType);
						//WriteStdout(": NA or Failure to fetch\r\n");
					}
				}
			}
		}
		helper_WhoAmi_ShowUserPriv(result, message_result, argv, argc); // show user privileges
		LocalFree(UserName);
		if (SECUR32) FreeLibrary(SECUR32);
		if (ADVAPI32) FreeLibrary(ADVAPI32);
		return true;
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
bool WhoAmi_WriteStdout_Priv_common(int* result, const char** message_result, const char* argv[], int argc, LWAnsiString* Output, const char* source_from, bool IncludeSOCDesc, bool IncludeSOCRiskSetNotice, HANDLE TargetToken)
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
	if ( (TargetToken == 0) || (TargetToken == INVALID_HANDLE_VALUE))
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
			if (SizeNeeded==0)
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
					LWAnsiString_Append(Output,  "USED FOR ACCESS");
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
				OutputPrivDescription(& Privs->Privileges[i], Output, PrivName->Data);
			}
		}
	}
	LWAnsiString_Append(Output, "\r\n");
	if (IncludeSOCRiskSetNotice)
	{
		LWAnsiString_Append(Output, "\r\n*****\r\n");
		LWAnsiString_Append(Output, SOCRiskNoticeString );
		LWAnsiString_Append(Output, "\r\n*****\r\n");
	}
	if (Privs != 0) LocalFree(Privs);
	LWAnsiString_FreeString(PrivName);
	FreeLibrary(ADVAPI32);
	FreeLibrary(SECUR32);
	if (TargetToken != selfToken) CloseHandle(selfToken);
	return true;
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
		
		auto res = WhoAmi_WriteStdout_Priv_common(result, message_result, argv, argc, OutputString, "System Token Source\r\n", true, true, GetSystemToken() );
		WriteStdout(LWAnsiString_ToCStr(OutputString));
		LWAnsiString_FreeString(OutputString);
		return res;
	}
	return false;
}
#endif

bool WhoAmi_WriteStdout_Priv(int* result, const char** message_result, const char* argv[], int argc)
{
	LWAnsiString* OutputString = LWAnsiString_CreateFromString("WhoAmI Privs: \r\n");
	if (OutputString != nullptr)
	{
		// 0 as the last are is shortcut for says look at the current process token
		auto res = WhoAmi_WriteStdout_Priv_common(result, message_result, argv, argc, OutputString, "User Token Source\r\n", true, true, 0);
		WriteStdout(LWAnsiString_ToCStr(OutputString));
		LWAnsiString_FreeString(OutputString);
		return res;
	}
	return false;
}
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
	
	// the entry point of whoami - tokens to display values
	if (!helper_WhoAmi_UserInformation(result, message_result, argv, argc, OutputString))
	{
		LWAnsiString_FreeString(OutputString);
		return false;
	}
	if (!helper_WhoAmi_UserTokenDump(result, message_result, argv, argc, OutputString))
	{
		LWAnsiString_FreeString(OutputString);
		return false;
	}
	WriteStdout(LWAnsiString_ToCStr(OutputString));
	LWAnsiString_FreeString(OutputString);
	return true;

}