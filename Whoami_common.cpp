#include "whoami.h"

extern "C" {
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

		if (IncludeDomain)
		{

			helper_lookup_sid_domain_name(lookup, Target, DomainName);
			if (DomainName->Length != 0)
			{
				if (PrefixDomain != nullptr)
				{
					LWAnsiString_Append(Output, PrefixName);
				}

				LWAnsiString_Append(Output, LWAnsiString_ToCStr(DomainName));
				if (SuffixDomain != nullptr)
				{
					LWAnsiString_Append(Output, SuffixName);
				}
				LWAnsiString_Append(Output, "\\\\");
			}

		}
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

		LWAnsiString_FreeString(SIDNAME);
		LWAnsiString_FreeString(DomainName);
		return true;
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


}