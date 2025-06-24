#pragma once

#include "common.h"
#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif 

#include <windows.h>
#include <security.h>


#include <ntsecapi.h> // For TOKEN_MANDATORY_LABEL
#include <winnt.h>    // Core token structures
#include <securitybaseapi.h> // TOKEN_GROUPS_AND_PRIVILEGES, etc.

#include <LWAnsiString.h>


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

// Union that can hold any GetTokenInformation return type (by value)
typedef union TokenInformationUnion {
	TOKEN_USER* TokenUser;
	TOKEN_GROUPS* TokenGroups;
	TOKEN_PRIVILEGES* TokenPrivileges;
	TOKEN_OWNER* TokenOwner;
	TOKEN_PRIMARY_GROUP* TokenPrimaryGroup;
	TOKEN_DEFAULT_DACL* TokenDefaultDacl;
	TOKEN_SOURCE* TokenSource;
	TOKEN_TYPE* TokenType;
	SECURITY_IMPERSONATION_LEVEL* ImpersonationLevel;
	TOKEN_STATISTICS* TokenStatistics;
	TOKEN_GROUPS_AND_PRIVILEGES* TokenGroupsAndPrivileges;
	TOKEN_ORIGIN* TokenOrigin;
	TOKEN_ELEVATION_TYPE* ElevationType;
	TOKEN_LINKED_TOKEN* LinkedToken;
	TOKEN_ELEVATION* Elevation;
	TOKEN_ACCESS_INFORMATION* AccessInfo;
	TOKEN_MANDATORY_LABEL* IntegrityLevel;
	TOKEN_MANDATORY_POLICY* MandatoryPolicy;
	TOKEN_APPCONTAINER_INFORMATION* AppContainer;
	CLAIM_SECURITY_ATTRIBUTES_INFORMATION* UserClaims;
	//TOKEN_PROCESS_TRUST_LEVEL *      TrustLevel;

	DWORD* DwordValue;
	VOID* TheVoid;
	//BYTE                            RawBuffer[4096]; // For unknown/undocumented types or overflows
} TokenInformationUnion;



/// <summary>
/// For the priv struct. indicates if we do [!!!] [!!] or [ ] on descrption output
/// </summary>
typedef int FlagType;

#define FLAG_INFORMAL 0
#define FLAG_YELLOW 1
#define RED_RISK 2

/// <summary>
/// Format of how the priv names , descipr, risk and flag are displayed
/// </summary>
struct PrivText
{
	const char* Name;
	const char* Description;
	const char* Risk;
	FlagType Flags;
};

extern "C" {
	extern bool WhoAmI_WriteStdout(int* result, const char** message_result, const char* argv[], int argc);

	/// <summary>
	/// This will loop thru https://learn.microsoft.com/en-us/windows/win32/api/secext/nf-secext-getusernameexa and it's possible choices, output descriptions of what's reported if valid to the output string
	/// </summary>
	/// <param name="result"></param>
	/// <param name="message_result"></param>
	/// <param name="argv"></param>
	/// <param name="argc"></param>
	/// <param name="Output"></param>
	/// <remarks>https://learn.microsoft.com/en-us/windows/win32/api/secext/ne-secext-extended_name_format</remarks>
	extern bool helper_WhoAmi_UserAccountName(int* result, const char** message_result, const char* argv[], int argc, LWAnsiString* Output);

	extern bool helper_WhoAmi_UserAccountName(int* result, const char** message_result, const char* argv[], int argc, LWAnsiString* Output);

	extern void helper_who_ami_usertoken_token_groups_string(int* result, const char** message_result, const char* argv[], int argc, TOKEN_GROUPS* target, LookupAccountSidA_PTR LookupSid, LWAnsiString* Output);

	extern void OutputPrivDescription(PLUID_AND_ATTRIBUTES x, LWAnsiString* output, const char* name);
	extern bool ResolveTokenDlls(HMODULE* Advapi32, GetTokenInfoPtr* GetTokenInfoAPI, LookupAccountSidA_PTR* LookupSIDAPI, LookUPPrivnameA* LookUpPriv, const char** result);
	extern bool helper_WhoAmi_PrivString(int* result, const char** message_result, const char* argv[], int argc, const char* TokenName, HANDLE Token, LWAnsiString* Output);


	extern void helper_who_ami_usertoken_token_groups_string(int* result, const char** message_result, const char* argv[], int argc, TOKEN_GROUPS* target, LookupAccountSidA_PTR LookupSid, LWAnsiString* Output);
	extern void helper_who_ami_usertoken_token_groups_string(int* result, const char** message_result, const char* argv[], int argc, TOKEN_GROUPS* target, LookupAccountSidA_PTR LookupSid, LWAnsiString* Output);

	extern bool WhoAmi_Write_Priv_common_string(int* result, const char** message_result, const char* argv[], int argc, LWAnsiString* Output, const char* source_from, bool IncludeSOCDesc, bool IncludeSOCRiskSetNotice, HANDLE TargetToken);

	extern bool helper_WhoAmi_PrivString(int* result, const char** message_result, const char* argv[], int argc, const char* TokenName, HANDLE Token, LWAnsiString* Output);

	extern void helper_who_ami_usertoken_token_groups_string(int* result, const char** message_result, const char* argv[], int argc, TOKEN_GROUPS* target, LookupAccountSidA_PTR LookupSid, LWAnsiString* Output);


	extern bool helper_lookup_sid(LookupAccountSidA_PTR lookup, PSID Target, HANDLE Unused, const char* PrefixName, const char* SuffixName, const char* PrefixDomain, const char* SuffixDomain, bool IncludeDomain, LWAnsiString* Output);


	extern bool WhoAmi_WriteStdout_Priv(int* result, const char** message_result, const char* argv[], int argc);


	extern bool WhoAmi_Writestdout_TokenElevatedQuestion(int* result, const char** message_result, const char* argv[], int argc);


	extern bool WhoAmi_Write_TokenElevatedQuestion_string(int* result, const char** message_result, const char* argv[], int argc, LWAnsiString* Output, const char* Prefix, HANDLE TargetToken);


}