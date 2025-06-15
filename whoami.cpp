
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

struct PrivText
{
	const char* Name;
	const char* Discription;
};

PrivText PrivDecrip[] =
{
	{"SeAssignPrimaryTokenPrivilege", "Allows a process to assign a primary token to another process."},
	{"SeAuditPrivilege", "Allows a process to generate audit records."},
	{"SeBackupPrivilege", "Allows a process to back up files and directories."},
	{"SeChangeNotifyPrivilege", "Allows a process to bypass traverse checking."},
	{"SeCreateGlobalPrivilege", "Allows a process to create global objects."},
	{"SeCreatePagefilePrivilege", "Allows a process to create page files."},
	{"SeCreatePermanentPrivilege", "Allows a process to create permanent objects."},
	{"SeCreateSymbolicLinkPrivilege", "Allows a process to create symbolic links."},
	{"SeDebugPrivilege", "Allows a process to debug other processes created by any user."},
	{"SeEnableDelegationPrivilege", "Allows a process to enable delegation."},
};

NameType NameTypes[] = {
	{ NameFullyQualifiedDN, "Fully Qualified Distinguished Name" },
	{ NameSamCompatible, "SAM or Legacy Account Name" },
	{ NameDisplay, "Display Name" },
	{ NameUniqueId, "Unique Account GUID" },
	{ NameCanonical, "Canonical Name" },
	{ NameUserPrincipal, "User Principal Name" },
	{ NameServicePrincipal, "Service Principal Name" },
	{ NameDnsDomain, "DNS Domain Name" },
	{ NameGivenName, "Given Name"},
	{ NameSurname, "Surname " },
	{ NameUnknown, "NameUnknown" }
};

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

bool helper_lookup_sid(LookupAccountSidA_PTR lookup, PSID Target, HANDLE Unused, const char* PrefixName, const char* SuffixName, const char* PrefixDomain, const char* SuffixDomain, bool IncludeDomain)
{
	DWORD TokenNameSize = 0;
	DWORD DomainNameSize = 0;
	SID_NAME_USE SidUse;
	{
		HLOCAL TokenLocal=0;
		HLOCAL DomainLocal=0;
		if ((!lookup(0, Target, nullptr, &TokenNameSize, nullptr, &DomainNameSize, &SidUse) && (GetLastError() != ERROR_INSUFFICIENT_BUFFER)))
		{
			return FALSE;
		}
		else
		{
			TokenLocal = LocalAlloc(LMEM_ZEROINIT, TokenNameSize + 1);
			if (IncludeDomain)
			{
				DomainLocal = LocalAlloc(LMEM_ZEROINIT, DomainNameSize + 1);
			}
			else
			{
				DomainLocal = nullptr;
			}

			char* TokenNameContainer = (char*)LocalLock(TokenLocal);
			char* DomainNameContainer;
			if (IncludeDomain)
			{
				DomainNameContainer = (char*)LocalLock(DomainLocal);
			}
			else
			{
				DomainNameContainer = nullptr;
			}


			int res = lookup(0, Target, TokenNameContainer, &TokenNameSize, DomainNameContainer, &DomainNameSize, &SidUse);


			if (res != 0)
			{
				WriteStdout(PrefixName);
				WriteStdout(TokenNameContainer);
				WriteStdout(SuffixName);
				if (IncludeDomain && DomainNameContainer != nullptr)
				{
					WriteStdout("  ");
					WriteStdout(PrefixDomain);
					WriteStdout(DomainNameContainer);
					WriteStdout(SuffixDomain);
				}
				LocalUnlock(TokenLocal);
				if (DomainNameContainer != nullptr)
				{
					LocalUnlock(DomainLocal);
					LocalFree(DomainLocal);
				}
				LocalUnlock(TokenLocal);
				LocalFree(TokenLocal);
				return TRUE;
			}
			else
			{
				WriteStdout(PrefixName);
				WriteStdout("There was an error obtaining the name for the SID.");
				return FALSE;
			}
		}
	}
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
void helper_who_ami_usertoken_token_groups_stdout(int* result, const char** message_result, const char* argv[], int argc, TOKEN_GROUPS* target, LookupAccountSidA_PTR LookupSid)
{
	for (int i = 0; i < target->GroupCount; i++)
	{
		// emit the display name for the sid
		helper_lookup_sid(LookupSid, target->Groups[i].Sid, 0, "SID: ", "\r\n", "Domain: ", "\r\n", true);
		if (target->Groups[i].Attributes & SE_GROUP_ENABLED)
		{
			WriteStdout("  Attributes: Enabled");
		}
		else if (target->Groups[i].Attributes & SE_GROUP_ENABLED_BY_DEFAULT)
		{
			WriteStdout("  Attributes: Enabled by Default");
		}
		else if (target->Groups[i].Attributes & SE_GROUP_USE_FOR_DENY_ONLY)
		{
			WriteStdout("  Attributes: Use for Deny Only");
		}
		else
		{
			WriteStdout("  Attributes: Not Enabled");
		}
		WriteStdout("\r\n");
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
	if (!helper_lookup_sid(LookupSID, target->User.Sid, 0, "User Name: ", nullptr, "Domain: ", nullptr, true))
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

						helper_who_ami_usertoken_token_groups_stdout(result, message_result, argv, argc, tokenInfo.TokenGroups, LookupSIDAPI);
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

#error helper_WhoAmi_UserInformation is glitchy in resolving display name to proper. It dumps what it finds ok but it's not pretty.
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
			for (EXTENDED_NAME_FORMAT step = NameFullyQualifiedDN; step <= NameSurname; )
			{
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
							if (NameTypes[step - 1].Type == NameUnknown)
							{
								break;
							}
							else
							{
								if (NameTypes[step - 1].Type == step)
								{
									LWAnsiString_Append(Output, NameTypes[step - 1].DisplayType);
								}
								break;
							}
						}
						LWAnsiString_Append(Output, ": ");
						LWAnsiString_Append(Output, LWAnsiString_ToCStr(UserNameStuff));
						LWAnsiString_Append(Output, "\r\n");
						LWAnsiString_ZeroString(UserNameStuff);
					}
				}
				step = (EXTENDED_NAME_FORMAT)((int)step + 1);
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
		return false;
	WriteStdout(LWAnsiString_ToCStr(OutputString));
	LWAnsiString_FreeString(OutputString);
	return true;

}