#include "common.h"	

typedef struct MIDAS_STRING
{
	HLOCAL StringHandle;
	char* String;
	int Length;
	int MaxLength;
} MIDAS_STRING;

/// <summary>
/// Allocate a string with a max length of Max.  The string is zeroed out and null terminated.
/// </summary>
/// <param name="Max"></param>
/// <returns></returns>
MIDAS_STRING* MidasStringCreate(int Max)
{
	MIDAS_STRING* ret = nullptr;
	HLOCAL ptr =LocalAlloc(LMEM_ZEROINIT, sizeof(MIDAS_STRING));
	if (ptr)
	{
		ret = (MIDAS_STRING*)LocalLock(ptr);
		ret->Length = 0;
		ret->MaxLength = Max;
		ret->StringHandle = LocalAlloc(LMEM_ZEROINIT, Max + 1); // +1 for null terminator
		if (ret->StringHandle)
		{
			ret->String = (char*)LocalLock(ret->StringHandle);
			ret->String[0] = 0; // null terminate
		}
		else
		{
			LocalUnlock(ptr);
			LocalFree(ptr);
			ret = nullptr;
		}
	}
	return ret;
}

/// <summary>
/// Free a prev allocated blank string from <see cref="MidasStringCreate(int)"/>.
/// </summary>
/// <param name="str"></param>
/// <returns></returns>
BOOL MidasStringFree(MIDAS_STRING** str)
{
	if (str == nullptr)
		return FALSE;
	if ((*str)->StringHandle != nullptr)
	{
		LocalUnlock((*str)->StringHandle);
		LocalFree((*str)->StringHandle);
		(*str)->StringHandle = nullptr;
	}
	if ((*str)->String != nullptr)
	{
		LocalUnlock((*str)->String);
		(*str)->String = nullptr;
	}
	LocalFree((*str));
	*str = nullptr; // set to null so we don't try to free it again
	return TRUE;
}


MIDAS_STRING* MidasStringCreateFromString(char* ptr)
{
	if (ptr == nullptr)
		return nullptr;
	int size = lstrlenA(ptr);
	MIDAS_STRING* ret = MidasStringCreate(size);
	if (ret)
	{
		if (size > ret->MaxLength)
		{
			ret->MaxLength = size;
			HLOCAL new_handle = LocalReAlloc(ret->StringHandle, size + 1, LMEM_ZEROINIT);
			if (new_handle)
			{
				ret->StringHandle = new_handle;
				ret->String = (char*)LocalLock(new_handle);
			}
			else
			{
				MidasStringFree(&ret);
				return nullptr;
			}
		}
		lstrcpyA(ret->String, ptr);
		ret->Length = size;
	}
	
}