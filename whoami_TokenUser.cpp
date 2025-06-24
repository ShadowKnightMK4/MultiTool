
#include <LWAnsiString.h>
#include <Windows.h>
#include "whoami.h"

struct NameType
{
	EXTENDED_NAME_FORMAT Type;
	const char* DisplayType;
};

NameType NameTypes[] = {
  { NameUnknown, "NameUnknown" },
  { NameFullyQualifiedDN, "Fully Qualified Distinguished Name" },
  { NameSamCompatible, "SAM or Legacy Account Name" },
  { NameDisplay, "Display Name" },
  { NameUniqueId, "Unique Account GUID" },
  { NameCanonical, "Canonical Name" },
  { NameCanonicalEx, "Canonical Name (Extended)" },
  { NameUserPrincipal, "User Principal Name" },
  { NameServicePrincipal, "Service Principal Name" },
  { NameDnsDomain, "DNS Domain Name" },
  { NameGivenName, "Given Name" },
  { NameSurname, "Surname" },
  { NameUnknown, 0 } // sentinel. helper_WhoAmi_UserInformation stops when it resource '0' as a description
}; 

bool helper_WhoAmi_UserAccountName(int* result, const char** message_result, const char* argv[], int argc, LWAnsiString* Output)
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
					if ((GetLastError() == ERROR_INSUFFICIENT_BUFFER) || (GetLastError() == ERROR_MORE_DATA))
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
							if ((NameTypes[i].Type == NameUnknown) && (NameTypes[i].DisplayType == nullptr))
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
					if (UserNameStuff == nullptr)
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