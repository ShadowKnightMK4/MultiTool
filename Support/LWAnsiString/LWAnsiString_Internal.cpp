#include "pch.h"
#include "LWAnsiString.h"
#include "LWAnsiString_Internal.h"
#include <climits>
#define AGGRO_REALLOC
#define INTERNALCALL_STREN(str) ALLOC_PTR(str, STRLEN)(str->Data);
#define INTERNALCALL_STRCAT(str) ALLOC_PTR(str, STRCAT)(str->Data, x);

extern "C" {

	IAT_WideCharToMultiBytePtr IAT_WideCharToMultiByte = 0;
	IAT_MultiByteToWideCharPtr IAT_MultiByteToWideChar = 0;
	IAT_GetProcessHeapPtr IAT_GetProcessHeap = 0;
	IAT_HeapAllocPtr IAT_HeapAlloc = 0;
	IAT_HeapReAllocPtr IAT_HeapReAlloc = 0;
	IAT_HeapFreePtr IAT_HeapFree = 0;



	IAT_lstrcatAPtr IAT_lstrcatA = 0;
	IAT_lstrcmpAPTR IAT_lstrcmpA = 0;
	IAT_lstrcmpiAPTR IAT_lstrcmpiA = 0;
	IAT_lstrlenAPtr IAT_lstrlenA = 0;
	IAT_lstrcpyAPTR IAT_lstrcpyA = 0;

	IAT_lstrlenWPtr IAT_lstrlenW = 0;
	IAT_lstrcmpiWPTR IAT_lstrcmpiW = 0;
	IAT_lstrcatWPtr IAT_lstrcatW = 0;
	IAT_lstrcmpWPTR IAT_lstrcmpW = 0;

}

extern "C" {
	/// <summary>
/// If the string ends with the given suffix, trim it off.
/// </summary>
/// <param name="str"></param>
/// <param name="suffix"></param>
/// <param name="Case"></param>
/// <returns></returns>
	bool LWAnsiString_TrimEndsWithInternal(LWAnsiString* str, const char* suffix, bool Case, int (*SuppliedLWAnsiString_EndsAt)(LWAnsiString*, const char*, bool))
	{/* UNIT TESTED THRU LWAnsiString_EndsAt */
		if (str == nullptr || suffix == nullptr || SuppliedLWAnsiString_EndsAt == nullptr )
		{
			return false; // invalid string or suffix
		}
		ProbeIfDirtyLen(str);
		int res = SuppliedLWAnsiString_EndsAt(str, suffix, Case);
		if (res != -1)
		{
			str->Length = res; // trim the string
			//str->Data[str->Length] = 0; // null terminate
			LWAnsiString_ClampNull(str);
			return true; // trimmed successfully
		}
		return false; // not trimmed

	}

	bool LWAnsiString_EndsWithInternal(LWAnsiString* str, const char* suffix, bool Case, int (*SuppliedLWAnsiString_EndsAt)(LWAnsiString*, const char*, bool))
	{
		/* UNIT TESTED THRU LWAnsiString_EndsAt */
		if (str == nullptr || suffix == nullptr || SuppliedLWAnsiString_EndsAt == nullptr)
		{
			return false; // invalid string or suffix
		}
		ProbeIfDirtyLen(str);
		int res = SuppliedLWAnsiString_EndsAt(str, suffix, Case);
		return res != -1;
	}





	/// <summary>
/// If the string ends with the given suffix, trim it off.
/// </summary>
/// <param name="str"></param>
/// <param name="suffix"></param>
/// <param name="Case"></param>
/// <returns></returns>
	bool LWAnsiString_TrimEndsWithOLD(LWAnsiString* str, const char* suffix, bool Case)
	{/* UNIT TESTED THRU LWAnsiString_EndsAt */
		if (str == nullptr || suffix == nullptr)
		{
			return false; // invalid string or suffix
		}
		ProbeIfDirtyLen(str);
		int res = LWAnsiString_EndsAtA(str, suffix, Case);
		if (res != -1)
		{
			str->Length = res; // trim the string
			//str->Data[str->Length] = 0; // null terminate
			LWAnsiString_ClampNull(str);
			return true; // trimmed successfully
		}
		return false; // not trimmed

	}

	/// <summary>
	/// Internal common code for LWAnsiString_EndsAt
	/// </summary>
	/// <param name="str">check this</param>
	/// <param name="suffix">the type is a lie, we're ether char* or wchar_t*, and depend on passed STRLEN and STRCMP</param>
	/// <param name="Case">ignored</param>
	/// <param name="STRLEN">STRLEN to use like wcslen , strlen</param>
	/// <param name="STRCMP">one of between stricmp, strcmp, wcsmp , wcsicmp flavors</param>
	/// <returns></returns>
	int LWAnsiString_EndsAtInternal(LWAnsiString* str, const wchar_t* suffix, bool Case, LW_STRING_strlen STRLEN, LWSTRING_CMP STRCMP)
	{
		if (str == nullptr || suffix == nullptr || STRLEN == nullptr || STRCMP == nullptr )
		{
			return false; // invalid string or suffix
		}
		ProbeIfDirtyLen(str);
		int res = 0;
		int suffix_len = STRLEN((char*)suffix);
		int str_len = LWAnsiString_Length(str);
		if (suffix_len > str->Length)
		{
			return false; // suffix is longer than the string
		}
		res = STRCMP((const char*)str->Data + str_len - suffix_len, (char*) suffix) == 0 ? str_len - suffix_len : -1; // case sensitive;
		return res;


	}


	int LWAnsiString_FindLast(LWAnsiString* str, char c)
	{
		ProbeIfDirtyLen(str);
		return LWAnsiString_FindLastExA(str, c, nullptr);
	}



	int LWAnsiString_CompareInternal(LWAnsiString* a, const void* b, bool Case)
	{
		if (a == nullptr && b == nullptr)
		{
			return 0;
		}
		if (a == nullptr && b != nullptr)
		{
			return -1;
		}
		if (a != nullptr && b == nullptr)
		{
			return 1;
		}
		if (!Case)
		{
			//return lstrcmpA(a->Data, b); // case sensitive
			return ALLOC_PTR(a, STRICMP)((const char*)a->Data, (char*)b);
		}
		else
		{
			//int tmp = lstrcmpiA(a->Data, b);
			return ALLOC_PTR(a, STRCMP)((const char*)a->Data, (char*) b);
			//return tmp; // case insensitive
		}
	}


	
	bool LWAnsiString_EndsWith(LWAnsiString* str, const char* suffix, bool Case)
	{
		/* UNIT TESTED THRU LWAnsiString_EndsAt */
		if (str == nullptr || suffix == nullptr)
		{
			return false; // invalid string or suffix
		}
		ProbeIfDirtyLen(str);
		int res = LWAnsiString_EndsAtA(str, suffix, Case);
		return res != -1;
	}


	int LWAnsiString_FindCharExInternal(LWAnsiString* str, wchar_t c, int start)
	{
		if (str == nullptr || str->Data == nullptr || start < 0 || start >= str->Length)
		{
			return -1; // invalid string or start position
		}
		ProbeIfDirtyLen(str);
		for (int i = start; i < str->Length; i++)
		{
			if (ALLOC_PTR(str, INDEX)(str->Data, i) == c)
				//if (str->Data[i] == c)
			{
				return i; // found the character
			}
		}
		return -1; // character not found
	}

	int LWAnsiString_FindLastExInternal(LWAnsiString* str, wchar_t c, int* count)
	{
		if (count != 0) *count = 0;
		if (str == nullptr || str->Data == nullptr)
		{
			return -1; // invalid string
		}
		ProbeIfDirtyLen(str);
		for (int i = str->Length - 1; i >= 0; i--)
		{
			//if (str->Data[i] == c)
			if (ALLOC_PTR(str, INDEX)(str->Data, i) == c)
			{
				if (count != 0)
				{
					*count += 1; // increment the count of found characters
				}
				else
				{
					return i; // found the char
				}

			}
		}
		return -1; // character not found
	}

	


	LWAnsiString* LWAnsiString_PadInternal(LWAnsiString* str,  DWORD c, int len)
	{
		
		//LWAnsiString_ZeroString(str);
		/* NOT UNITED TESTED - but proven in midas by appending '-' symboles*/
		if (str == nullptr)
			return nullptr; // null string

		ProbeIfDirtyLen(str);


		if (len <= 0)
			return str; // no change if size is less than or equal to 0
		if (len + str->Length > str->AllocatedSize)
		{
			LWAnsiString_Reserve(str, len + str->Length); // ensure we have enough space
		}
		if (str->Length + len > str->AllocatedSize)
		{
			return nullptr; // not enough space to pad
		}
		for (int i = str->Length; i < str->Length + len; i++)
		{
			//str->Data[i] = c; // pad the string with the character
			ALLOC_PTR(str, WriteToIndex)(str->Data,i, c);
		}
		str->Length += len; // update the length
		LWAnsiString_ClampNull(str); // ensure the string is null terminated
		return str; // return the updated string
	}

	
	LWAnsiString* LWAnsiString_PadNewLineInternalW(LWAnsiString* str, const wchar_t c, int len, DWORD CullSize)
	{
		LWAnsiString* r = LWAnsiString_PadInternal(str, (const  wchar_t) c, len ); // pad the string with the character
		ProbeIfDirtyLen(r);
		LWAnsiString_AppendNewLine(r); // append a new line
		return r;
	}

	LWAnsiString* LW_INTERNAL LWAnsiString_AppendInternal(LWAnsiString* str, const void* append, LW_STRING_strlen STRLEN)
	{
		/* INIT TESTED CAUSE WE USE IT THRUOUT MIDAS AND the unit tests for other stuff*/
		if (str == nullptr)
			return str;
		if (append == nullptr)
			return str; // nothing to append
		if (str->AllocatedHandle == 0)
			return nullptr; // WE CAN'T CAUSE there's no table of allocator routines
		// TODO: Add unit test to test for this.
		size_t append_len = STRLEN((void*)append);
		if (append_len == 0)
			return str; // nothing to append
		int new_size = 0;
		int required_size = 0;
		ProbeIfDirtyLen(str);
#ifndef AGGRO_REALLOC
		required_size = str->Length + append_len;
#else

		if ((str->Length + append_len + 1) < (str->AllocatedSize))
		{
			required_size = str->AllocatedSize;
		}
		else
		{
			if ((str->Length + append_len + 1) < (str->AllocatedSize * 2))
			{
				required_size = str->AllocatedSize * 2;
			}
			else
			{
				required_size = str->AllocatedSize + append_len + ALLOC_PTR(str, SingleCharacterLength);
			}
		}

#endif

		if (required_size <= str->AllocatedSize)
		{
			// if you remove the +1 in the code above, ensure we add it here for the null term
			new_size = str->AllocatedSize;
		}
		else
		{
			new_size = required_size;
		}

		if (new_size >= str->AllocatedSize)
		{
			LWAnsiString_ClampNull(str);
			//new_size++; // +1 for null terminator 
			// reason above is commented out is we're assuming the original is null term
			char* newPtr = 0;
			if (new_size != str->AllocatedSize)
			{
				newPtr = (char*)((AllocationHandler*)(str->AllocatedHandle))->CustomReAlloc(((AllocationHandler*)(str->AllocatedHandle))->CustomGetHeap(0, 0, 0), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, str->Data, new_size);
			}
			else
			{
				newPtr = str->A; /// now A, W and Data are unamed union.
			}


			if (newPtr == nullptr)
			{
				return nullptr; // failed to reallocate
			}
			else
			{
				str->Data = newPtr;
				LWAnsiString_ClampNull(str);
				ALLOC_PTR(str, STRCAT)(str->Data, (void*)append);
				str->Length += ALLOC_PTR(str, STRLEN)((void*)append);
				LWAnsiString_ClampNull(str);
				str->AllocatedSize = new_size;
				return str;
				/*
				str->Data = newPtr; // set the new data pointer
				str->Data[str->Length] = 0; // null terminate the existing string

				str->Data[str->Length] = 0; // null terminate the new length on the string we requested
				lstrcatA(str->Data, append); // append the string
				str->Length += lstrlenA(append); // update the length
				str->Data[str->Length] = 0; // null terminate
				str->AllocatedSize = new_size; // update the allocated size
				return str; // return the updated string	*/
			}
		}
		else
		{
			ALLOC_PTR(str, STRCAT)(str->Data, (void*)append);
			str->Length += append_len;
			LWAnsiString_ClampNull(str);
			for (size_t i = str->Length; i < str->AllocatedSize; i++)
			{
				ALLOC_PTR(str, WriteToIndex)(str->Data, i, 0);
			}
			return str;
			/*str->Length += append_len; // update the lengt
			lstrcatA(str->Data, append); // append the string

			str->Data[str->Length] = 0; // null terminate
			for (int i = str->Length; i < str->AllocatedSize; i++)
			{
				str->Data[i] = 0; // zero out the rest of the buffer
			}
			return str; // return the updated string*/
		}
	}


	LW_INTERNAL void ProbeIfDirtyLen(LWAnsiString* str)
	{

		if (str != 0)
		{
			if ((str->Length < 0) || ((str->Flags & LWANSI_FLAG_DIRTY) == LWANSI_FLAG_DIRTY))
			{
				LWAnsiString_ProbeLength(str);
			}
		}
	}

	
	bool LWAnsiString_AppendNumberInternal(int number, 
		LWAnsiString* output, 
		int* output_size,
		LWAnsiString* (*appendFunc)(LWAnsiString* str, const char* append),
		const void* IntMaxPlugin,
		const void* zeroPlugin)
	{
		// yes it's an int. Yes we're treating it like a bool.
		int IsPositive = number > 0;
		// arg validation;
		if (output == nullptr)
		{
			return false;
		}

		ProbeIfDirtyLen(output);
		if (number == INT_MIN)
		{
			// special case for the minimum int value, which cannot be represented as a positive number
			//LWAnsiString_AppendA(output, "-2147483648");
			appendFunc(output, (char*)IntMaxPlugin);
			if (output_size != 0)
			{
				*output_size = (12 +
					ALLOC_PTR(output, SingleCharacterLength) // this odd wayt lets it theory scale for say unicode and ansi
					);
			}; // 11 digits + null terminator
			return true;
		}

		if (number == 0)
		{
			// special case for zero
			appendFunc(output, (const char*) zeroPlugin);
			if (output_size != 0) { *output_size = 2; }; // 1 digit + null terminator
			return true;
		}
		int tmp = number;
		// count digits
		int digit_count = 0;

		if (!IsPositive)
			tmp *= -1; // we fix in POST

		// count our digits. we need it to figure the buffer
		while (((char)(tmp & 0xFF)) != 0)
		{

			digit_count++;
			if (tmp > 9)
			{
				tmp /= 10;
			}
			else
			{
				break;
			}

		}


		// should we have a digit count of 0, we tick it to 1.
		if (digit_count == 0)
			digit_count++;

		if (!IsPositive)
			digit_count++; // for the minus sign;

		// and allocate + lock a void* to place
		// note the +1 is for the null terminator
		// note the IsPositive note is to add a - sign if the number is negative
		LWAnsiString_AddReserve(output, digit_count + 1); // reserve the string with the new size + 1 for null terminator


		{
			LWAnsiString* tmp_buff = LWAnsiString_CreateString(digit_count);
			// grab a pointer if possible and bail if not
			char* ret = (char*)LWAnsiString_ToCStr(tmp_buff);

			if (ret == 0)
			{
				return false;
			}
			else
			{
				if (!IsPositive)
					tmp = number * -1;
				else
					tmp = number;

				if (tmp != 0)
				{
					while (digit_count > 0)
					{
						char debug = (char)((tmp % 10) + '0');
						// what this code here is doing is started from the ones place and moves up taking the remainding of diviing tmp by 10 and add '0' (the string 0) to 
						// get the digit of it. ie
						// the number 5 would end up being '5' when done.
						ret[digit_count - 1] = debug;
						tmp /= 10;
						digit_count--;
						if (!IsPositive)
						{
							if (digit_count == 1)
							{
								ret[digit_count - 1] = '-';
								digit_count--;
							}
						}
					}
				}
				else
				{
					/* code above should be counting digits and ect... if code gets here, aka the int was 0, return 0*/
					ret[0] = '0';
				}
				LWAnsiString_AppendNative(output, tmp_buff); // append the string to the output
				LWAnsiString_FreeString(tmp_buff); // free the temporary buffer
			}


			if (output_size != 0) { *output_size = digit_count + 1; };
			return true;
		}
	}


}