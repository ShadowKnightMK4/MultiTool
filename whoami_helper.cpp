#pragma once
#include "whoami.h"

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