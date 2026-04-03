
#include "IAT_REGISTRY.H"

extern "C"
{
	RegOpenKeyA_Ptr IAT_RegOpenKeyA = 0;
	RegEnumKeyA_Ptr IAT_RegEnumKeyA = 0;
	RegCloseKey_Ptr IAT_RegCloseKey = 0;
	RegEnumValueA_Ptr IAT_RegEnumValueA = 0;
	RegQueryValueExA_Ptr IAT_RegQueryValueExA = 0;
	
	DWORD IAT_DynamicLinkRegistry_Cleanup()
	{
		DWORD ret =0;

		IAT_RegCloseKey = 0;
		IAT_RegEnumKeyA = 0;
		IAT_RegEnumValueA = 0;
		IAT_RegQueryValueExA = 0;
		IAT_RegOpenKeyA = 0;
		return ret;
	}
	DWORD IAT_DynamicLinkRegistry(DWORD IAT_settings)
	{
		DWORD ret = 0;
		if (iatAdvapi32 == 0)
		{
			iatAdvapi32 = LoadLibraryA("advapi32.dll");
		}
		if (iatAdvapi32 == 0)
		{
			return 0;
		}
		else
		{
			if ( (IAT_settings & IAT_REGISTRY_ENUMKEYA) == IAT_REGISTRY_ENUMKEYA)
			{
				IAT_RegEnumKeyA = (RegEnumKeyA_Ptr)GetProcAddress(iatAdvapi32, "RegEnumKeyA");
				if (IAT_RegEnumKeyA != 0)
				{
					ret |= IAT_REGISTRY_ENUMKEYA;
				}
			}
			
			if ((IAT_settings & IAT_REGISTRY_OPENKEYA) == IAT_REGISTRY_OPENKEYA)
			{
				IAT_RegOpenKeyA = (RegOpenKeyA_Ptr)GetProcAddress(iatAdvapi32, "RegOpenKeyA");
				if (IAT_RegOpenKeyA != 0)
				{
					ret |= IAT_REGISTRY_OPENKEYA;
				}
			}


			if ((IAT_settings & IAT_REGISTRY_CLOSEKEY) == IAT_REGISTRY_CLOSEKEY)
			{
				IAT_RegCloseKey = (RegCloseKey_Ptr)GetProcAddress(iatAdvapi32, "RegCloseKey");
				if (IAT_RegCloseKey != 0)
				{
					ret |= IAT_REGISTRY_CLOSEKEY;
				}
			}

			if ((IAT_settings & IAT_REGISTRY_ENUMKEYA) == IAT_REGISTRY_ENUMKEYA)
			{
				IAT_RegEnumValueA = (RegEnumValueA_Ptr)GetProcAddress(iatAdvapi32, "RegEnumValueA");
				if (IAT_RegEnumValueA != 0)
				{
					ret |= IAT_REGISTRY_ENUMKEYA;
				}
			}
			if ((IAT_settings & IAT_REGISTRY_QUERYVALUEEXA) == IAT_REGISTRY_QUERYVALUEEXA)
			{
				IAT_RegQueryValueExA = (RegQueryValueExA_Ptr)GetProcAddress(iatAdvapi32, "RegQueryValueExA");
				if (IAT_RegQueryValueExA != 0)
				{
					ret |= IAT_REGISTRY_QUERYVALUEEXA;
				}
			}

			return ret;
			

		}
	}
}