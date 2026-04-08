#include "pch.h"
#include "LWAnsiString.h"
#include "LWAnsiString_Internal.h"
#define AGGRO_REALLOC

extern "C" {


	LWAnsiString* LWAnsiString_PadInternal(LWAnsiString* str,  DWORD c, int len, const DWORD cutoff_mask)
	{
		LWAnsiString_ZeroString(str);
		c = (c & cutoff_mask);
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
			ALLOC_PTR(str, WriteToIndex)(str->Data, i, c);
		}
		str->Length += len; // update the length
		LWAnsiString_ClampNull(str); // ensure the string is null terminated
		return str; // return the updated string
	}

	LWAnsiString* LWAnsiString_PadA(LWAnsiString* str, char c, int len)
	{
		return LWAnsiString_PadInternal(str, c, len, ALLOC_PTR(str, SingleCharacterLength));
	}

	LWAnsiString* LWAnsiString_PadW(LWAnsiString* str, wchar_t c, int len)
	{
		return LWAnsiString_PadInternal(str, c, len, ALLOC_PTR(str, SingleCharacterLength));
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

}