// LWAnsiString.cpp : Defines the functions for the static library.

/*
 * This file is part of the Midas project and is designed to be self-contained.
 * It presents a lightweight mechanism for creating and managing ANSI strings using
 * a customizable heap-based memory strategy that works cleanly even on Win95-era systems.
 *
 * The core structure, `LWAnsiString`, holds:
 *   - a pointer to the character data,
 *   - the current string length (excluding null terminator),
 *   - the total allocated size (including null),
 *   - and a pointer to an `AllocationHandler` describing how to allocate, free, and reallocate memory.
 *
 * By default, strings are stored in the defailt Windows GetProcessHeap(). When first creatin a strin
 * with this library a bit of internal setup is done to prep to use said heap.
 * This avoids CRT dependency  and ensures compatibility with older systems.
 *
 * === Custom Allocator Usage ===
 * To use a custom allocator, set up an `AllocationHandler` (see LWAnsiString.h), and assign:
 *   - `CustomFirstAlloc`
 *   - `CustomFree`
 *   - `CustomReAlloc`
 *   - `CustomGetHeap`
 * 
 * These should mimic the behavior of HeapAlloc/HeapFree semantics, as documented in MSDN or other Microsoft API references.
 * After passing a custom handler, the string will use that handler for all memory operations.
 * The handler is copied internally into the LWAnsiString struct, so you do not need to keep your own reference.
 * 
 * 
 * === Notes and Contracts ===
 * 1. `CreateLWAnsiString`, `CreateLWAnsiStringEx`, `LWAnsiString_Duplicate`, and `_DuplicateEx`
 *    all return *owned* pointers. The internal struct is heap-allocated via the provided or on
 *	  the default process heap and must only be freed using `FreeLWAnsiString()`.
 *    - Exception: If `DefaultHandler` or null is passed to the ex, the default handler is embedded as a static,
 *      shared instance and should never be freed manually.
 *
 * 2. Ownership: Once created, the string struct and its buffer are owned by the library.
 *    Do not attempt to free or reallocate them manually—doing so risks corruption.
 *
 * 3. `len` should never be manually modified. It's maintained by the library routines and reflects
 *    the logical length (not including the null terminator) ie  len here is like caching strlen("on a string").
 *
 * 3. Continued. IF YOU start with length zero that is gonna modify the length of the string (like added win32 api data),
 *    call 	LWAnsiString_ProbeLength(LWAnsiString* str) to calculate the updated length, return it and store it for future
 * 
 * 4. `AllocatedSize` reflects the full buffer size, *including* null terminator. This allows
 *    safe appends and internal resizing logic. ie AllocatedSize will typically be len + 1 for a string of length `len`.
 *	  unless resized
 *
 * 5. All mutating routines null-terminate the string after modification. Even buffer-growing helpers
 *    maintain a safe trailing null.
 *
 * 6. `LWAnsiString_ToCStr()` exposes the internal buffer as a `const char*` for read-only usage.
 *    This enables clean interop with C functions while preserving encapsulation. It's perfectly
 *	  fine to just refer to the raw buffer, however if the raw buffer exposure changes, this should still work.
 *
 * 7. The raw `data` pointer may be directly read or written (up to `AllocatedSize`),
 *    but never freed or reassigned externally. Doing so will break internal tracking.
 * 
 * 8. As a bit of a hard rule (that is still bendable,
 *    keep your strings the same type ie no trying to compare a unicode string with an ansi string for example)
 *   The code won't stop you BUT IT IS NOT RECOMMANDED!
 * 9.  The 2 main Allocate handlers LWAnsi and LWUnicode have corosponding Append, compare, create stuff
 */

/*
* SOME UNIT TESTing comment related
* UNIT TEST => there's a test for that.
* UNIT TEST indirect -> it does its own trivial thing then calls unit tested code
* Proven in Midas -> thies mean the code is NOT UNIT TESTed but I've used it in my midas app with no issues.
*/

#include "pch.h"
#include "framework.h"
#include <climits>
#include "LWAnsiString.h"
#include "LWAnsiString_Internal.h"
#pragma optimize("", off)


#pragma optimize("", on)
// if defined this does a byte by byte local_memmzero() vs say trying to zero in pointer sized chunks
#define SIMPLE_MEMORY_COPY 

// if defined the Append flavors will double the length of the string memory size and take that or the reuqested memory size, whichever is bigger
#define AGGRO_REALLOC
extern "C" {
	HANDLE StringHeap = 0;

	//#define CONST_CAST(type, val) ((type)(uintptr_t)(const void *)(val))


	///<summary>
	/// This macro is used to access the AllocationHandler functions from the LWAnsiString struct without excessively verbose code.
	/// </summary>




#pragma optimize("", off)

	/// <summary>
	/// our local memory zero
	/// </summary>
	/// <param name="target">target to zero</param>
	/// <param name="size">how many bytes</param>
	void LW_INTERNAL local_memzero(unsigned char* target, size_t size)
	{
#ifdef SIMPLE_MEMORY_COPY

		volatile unsigned char* v_target = target;
		if (target == nullptr || size == 0)
			return; // nothing to do
		else
		{
			while (size > 0)
			{
				*v_target = 0;
				size--;
				v_target++;
			}
		}

#else

		if (target == nullptr || size == 0)
			return; // nothing to do
		union shim {
			DWORD x86;
			ULONGLONG x64;
		} f;
		f.x64 = 0;


		while (size > 0)
		{
			if ((size % sizeof(void*)) == 0)
			{
				if (sizeof(void*) == 4)
				{
					(*(DWORD*)target) = f.x86; // zero out the DWORD
					target += sizeof(DWORD); // move the target pointer forward
				}
				else if (sizeof(void*) == 8)
				{
					(*(ULONGLONG*)target) = f.x64; // zero out the ULONGLONG
					target += sizeof(ULONGLONG); // move the target pointer forward
				}
				size -= sizeof(void*);
			}
			else
			{
				*target = 0; // zero out the byte
				size--;
			}
		}
#endif
	}
#pragma optimize("", on)  


	size_t WINAPI DefaultWriteIndex(void* Target, size_t index, size_t val)
	{
		char* t = (char*)Target;
		t[index] = val;
		return 0;
	}
	size_t WINAPI DefaultIndexer(void* TARGET, size_t index)
	{
		char* t = (char*)TARGET;
		return (char) t[index];
	}

	size_t WINAPI UnicodeWriteIndex(wchar_t* Target, size_t index, size_t val)
	{
		Target[index] = val;
		return 0;
	}
	wchar_t WINAPI UnicodeIndexer(wchar_t* TARGET, size_t index)
	{
		return TARGET[index];
	}
	/// <summary>
	/// The default allocaiton handler, intiailized on the first call to any of the Create LWAnsiString routines
	/// </summary>
	AllocationHandler DefaultHandler = { 0 };
	AllocationHandler UnicodeHandler{ 0 };

	AllocationHandler* LWAnsiHandler = &DefaultHandler;
	AllocationHandler* LWUnicodeHandler = &UnicodeHandler;

	HANDLE LW_INTERNAL WINAPI DefaultMyHeapGet(DWORD Options, SIZE_T Start, SIZE_T Cap)
	{
		return GetProcessHeap(); // fallback to the default heap
	}

	/*
	* Why the check if null or self, so we don't accidently go infiniate in stack use and recursion. In otherwords, this happens call default instead
	*/
	HANDLE LW_INTERNAL WINAPI DefaultAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes)
	{
		if ((DefaultHandler.CustomFirstAlloc != nullptr) && (DefaultHandler.CustomFirstAlloc != DefaultAlloc))
		{
			return DefaultHandler.CustomFirstAlloc(hHeap, dwFlags, dwBytes);
		}
		return HeapAlloc(hHeap, dwFlags, dwBytes);
	}
	LPVOID LW_INTERNAL WINAPI DefaultReAlloc(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, SIZE_T dwBytes)
	{
		if ((DefaultHandler.CustomReAlloc != nullptr) && (DefaultHandler.CustomReAlloc != DefaultReAlloc))
		{
			return DefaultHandler.CustomReAlloc(hHeap, dwFlags, lpMem, dwBytes);
		}
		return HeapReAlloc(hHeap, dwFlags, lpMem, dwBytes);
	}

	BOOL LW_INTERNAL WINAPI DefaultFree(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem)
	{
		if ((DefaultHandler.CustomFree != nullptr) && (DefaultHandler.CustomFree != DefaultFree))
		{
			return DefaultHandler.CustomFree(hHeap, dwFlags, lpMem);
		}
		return HeapFree(hHeap, dwFlags, lpMem);
	}


	


	/// <summary>
	/// The creater lwansistring calls this if the string heap is not set up. This intializes default
	/// </summary>
	void LW_INTERNAL SetupAnsiString()
	{
		{
			if (StringHeap == 0)
			{
				StringHeap = GetProcessHeap();
				DefaultHandler.CustomFirstAlloc = DefaultAlloc;
				DefaultHandler.CustomGetHeap = DefaultMyHeapGet;
				DefaultHandler.CustomFree = DefaultFree;
				DefaultHandler.CustomReAlloc = DefaultReAlloc;
				//DefaultHandler.STRCAT = (LW_STRING_strcat)lstrcatA;
				DefaultHandler.STRCAT = (LW_STRING_strcat) lstrcatA;
				//DefaultHandler.STRCPY = (LW_STRING_strcpy)lstrcpyA;
				DefaultHandler.STRCPY = (LW_STRING_strcpy)lstrcpyA;
				//DefaultHandler.STRLEN = (LW_STRING_strlen)lstrlenA;
				DefaultHandler.STRLEN = (LW_STRING_strlen)lstrlenA;
				DefaultHandler.SingleCharacterLength = sizeof(char);
				DefaultHandler.INDEX = DefaultIndexer;
				DefaultHandler.STRCMP = lstrcmpA;
				DefaultHandler.STRICMP = lstrcmpiA;
				DefaultHandler.WriteToIndex = DefaultWriteIndex;

				/*
				UnicodeHandler.CustomFirstAlloc = DefaultAlloc;
				UnicodeHandler.CustomGetHeap = DefaultMyHeapGet;
				UnicodeHandler.CustomFree = DefaultFree;
				UnicodeHandler.CustomReAlloc = DefaultReAlloc;
				//DefaultHandler.STRCAT = (LW_STRING_strcat)lstrcatA;
				UnicodeHandler.STRCAT = (LW_STRING_strcat)lstrcatW;
				//DefaultHandler.STRCPY = (LW_STRING_strcpy)lstrcpyA;
				UnicodeHandler.STRCPY = (LW_STRING_strcpy)lstrcpyW;
				//DefaultHandler.STRLEN = (LW_STRING_strlen)lstrlenA;
				UnicodeHandler.STRLEN = (LW_STRING_strlen)lstrlenW;
				UnicodeHandler.SingleCharacterLength = sizeof(wchar_t);
				UnicodeHandler.INDEX = (LW_STRING_INDEXOR) UnicodeIndexer;
				UnicodeHandler.STRICMP = (LWSTRING_CMP)lstrcmpiW;
				UnicodeHandler.STRCMP = (LWSTRING_ICMP)lstrcmpW;
				UnicodeHandler.WriteToIndex = (LW_STRING_WRITE_INDEX)UnicodeWriteIndex;*/
			}
		}

		
	}



	


	LWAnsiString* LWAnsiString_MarkLenDirty(LWAnsiString* str)
	{
		if (str != nullptr)
		{
			str->Length = -1;
			str->Flags |= LWANSI_FLAG_DIRTY;
		}
		return str;
	}
	LWAnsiString* LWAnsiString_CreateFromOffsetEx(AllocationHandler* x, LWAnsiString* str, int offset)
	{
		ProbeIfDirtyLen(str);
		/* indirect UNIT TESTED.*/
		if (str == nullptr || offset < 0 || offset >= str->Length)
		{
			return nullptr; // invalid string or offset
		}

		if (offset == 0)
			return LWAnsiString_CreateFromStringEx(x, LWAnsiString_ToCStr(str)); // if offset is 0, just duplicate original

		return LWAnsiString_CreateFromStringEx(x, LWAnsiString_ToCStr(str) + (offset* ALLOC_PTR(str, SingleCharacterLength))); // create a new string from the offset

	}

	LWAnsiString* LWAnsiString_CreateFromOffset(LWAnsiString* str, int offset)
	{
		ProbeIfDirtyLen(str);
		/* UNIT TESTED.*/
		return LWAnsiString_CreateFromOffsetEx(&DefaultHandler, str, offset);
	}


	LWAnsiString* LWAnsiString_CreateString(int len)
	{
		/* UNIT TESTED*/
		return LWAnsiString_CreateStringEx(&DefaultHandler, len);
	}
	LWAnsiString* LWAnsiString_CreateStringEx(AllocationHandler* x, int len)
	{
		/* indirect UNIT TESTED.*/
		if (x == nullptr)
		{
			x = &DefaultHandler;
		}
		if (len < 0)
		{
			return nullptr; // invalid length
		}

		SetupAnsiString();
		auto Ans = (LWAnsiString*)DefaultHandler.CustomFirstAlloc(StringHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, sizeof(LWAnsiString));
		if (Ans == nullptr)
		{
			return nullptr; // failed to allocate
		}
		else
		{
			Ans->Data = (char*)DefaultHandler.CustomFirstAlloc(StringHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, (len +1 )*x->SingleCharacterLength); // +1 for null terminator

			Ans->AllocatedSize = len + 1; // +1 for null terminator
			if (x != &DefaultHandler)
			{
				// if the default handler is NOT passed, we create a copy of it and stash it as  a pointer in the void* thing.
				Ans->AllocatedHandle = x->CustomFirstAlloc(StringHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, sizeof(AllocationHandler));

				ALLOC_PTR(Ans, CustomFirstAlloc) = x->CustomFirstAlloc;
				ALLOC_PTR(Ans, CustomGetHeap) = x->CustomGetHeap;
				ALLOC_PTR(Ans, CustomFree) = x->CustomFree;
				ALLOC_PTR(Ans, CustomReAlloc) = x->CustomReAlloc; // copy 



			}
			else
			{
				Ans->AllocatedHandle = x;
			}
			if (Ans->Data == nullptr)
			{
				DefaultHandler.CustomFree(StringHeap, 0, Ans);
				Ans = nullptr; // failed to allocate data
				return nullptr; // failed to allocate data
			}
			else
			{
				Ans->Length = 0;
				//Ans->Data[len] = 0; // null terminate
				LWAnsiString_ClampNull(Ans);


				return Ans;
			}
		}
	}


	LWAnsiString* LWAnsiString_CreateFromStringEx(AllocationHandler* x, const char* str)
	{
		/* indirect UNIT TESTED.*/
		if (str == nullptr)
		{
			return nullptr; // null string
		}
		size_t len = lstrlenA(str);
		LWAnsiString* Ans = LWAnsiString_CreateStringEx(x, len);
		if (Ans != nullptr)
		{

			//lstrcpyA((char*) Ans->Data, str); // copy the string
			/*Ans->Data[len] = 0; // null terminate
			Ans->Length = len;*/
			ALLOC_PTR(Ans, STRCPY)(Ans->Data, (void*)str);
			Ans->Length = len;
			LWAnsiString_ClampNull(Ans);
			
		}
		return Ans;
	}
	LWAnsiString* LWAnsiString_CreateFromString(const char* str)
	{
		/*  UNIT TESTED. thru the full test collection UnitTestLWString*/
		return LWAnsiString_CreateFromStringEx(&DefaultHandler, str);
	}

	bool LWAnsiString_FreeString(LWAnsiString* str)
	{
		/* indirect UNIT TESTED.  thru the full test collection UnitTestLWString*/
		bool StrFree, StructFree, AllotFree;
		StructFree = StrFree = AllotFree = false; // init to false
		AllocationHandler* handler = (AllocationHandler*)(str ? str->AllocatedHandle : nullptr); // get the handler if it exists
		if (str == nullptr)
		{
			return false; // nothing to free
		}
		else
		{
			if (handler == 0)
			{
				return false; // WE DON'T KNOW HOW TO FREE THIS hense we can't
			}
			else
			{
				// first free the string. Then free the buffer.
				if (str->Data != nullptr)
				{
					//	StrFree = ((AllocationHandler*)(str->AllocatedHandle))->CustomFree(((AllocationHandler*)(str->AllocatedHandle))->CustomGetHeap(0, 0, 0), 0, str->Data); // free the data
					StrFree = handler->CustomFree(handler->CustomGetHeap(0, 0, 0), 0, str->Data); // free the data
					str->Data = nullptr; /* honestly unneeded , the struct holding it will be free shortly*/
				}
				else
				{
					StrFree = true; // no data to free, so we consider it freed
				}






				if ((AllocationHandler*)(str->AllocatedHandle) != &DefaultHandler)
				{
					//AllotFree = ((AllocationHandler*)(str->AllocatedHandle))->CustomFree(((AllocationHandler*)(str->AllocatedHandle))->CustomGetHeap(0, 0, 0), 0, str->AllocatedHandle); // free the structure itself
					handler->CustomFree(handler->CustomGetHeap(0, 0, 0), 0, str->AllocatedHandle);
				}
				else
				{
					// the default handler is STATIC AND GLOBAL TO THE FILE DON"T FREE IT
					AllotFree = true;
				}


				/* free our structure taking care of ensuring we GOT THE BACKUP PTR to the handler. That's our ticket.*/
				StructFree = ((AllocationHandler*)(str->AllocatedHandle))->CustomFree(((AllocationHandler*)(str->AllocatedHandle))->CustomGetHeap(0, 0, 0), 0, str); // free the structure itself


				if (!StrFree || !StructFree || !AllotFree)
				{
					return false; // failed to free either the data or the structure or the handler
				}
				return true;
			}
		}
	}



	int LWAnsiString_Length(LWAnsiString* str)
	{
		if (str == nullptr)
			return 0; // null string
		ProbeIfDirtyLen(str);
		return str->Length; // return the length
	}

	int LWAnsiString_ProbeLength(LWAnsiString* str)
	{
		if (str == nullptr)
		{
			return 0;
		}

		if (str->Data != 0)
		{
			str->Flags &= ~LWANSI_FLAG_DIRTY;
			if (ALLOC_PTR(str, INDEX)(str->Data, 0) == 0)
			{
				str->Length = 0;
				return 0;
			}
			else
			{
				str->Length = ALLOC_PTR(str, STRLEN)(str->Data);
				return str->Length;
			}
		}

		/*
		if (str->Data != 0)
		{
			if (str->Data[0] == 0)
			{
				str->Length = 0;
				return 0;
			}
			else
			{
				str->Length = lstrlenA(str->Data);
				return str->Length;
			}
		}
		*/
		str->Length = 0;
		return 0;
	}

	void  LWAnsiString_ClampNull(LWAnsiString* str)
	{
		if (str && str->Data)
		{
			ALLOC_PTR(str, WriteToIndex)(str->Data, str->Length, 0);
		//	str->Data[str->Length] = 0;
		}
	}


	/// <summary>
	/// Wipe the buffer to zero. keep it allocated and null terminate it.
	/// </summary>
	/// <param name="str"></param>
	/// <returns></returns>
	void LWAnsiString_ZeroString(LWAnsiString* str)
	{
		/* NOT UNIT TESTED BUT PROVEN IN MIDAS project*/
		if ((str == nullptr) || (str->Data == nullptr) || (str->AllocatedSize <= 0))
			return; // null string or null data or invalid size

		/*
		if (str->AllocatedSize % sizeof(void*) == 0)
		{
			local_memzero((unsigned char*)str->Data, str->AllocatedSize); // zero out the buffer using our local_memzero function
			// what we're going is are we allocated a chunjk of data divisible by 4 or 8,  we fill in by that value
			union shim {
				DWORD x86;
				ULONGLONG x64;
			} f;
			f.x64 = 0;
			char* step = str->Data;

			for (int i = 0; i < str->AllocatedSize; i += sizeof(void*))
			{
				if (sizeof(void*) == 8)
				{
					*(ULONGLONG*)(step + i) = f.x64; // zero out the buffer
				}
				else
				{
					*(DWORD*)(step + i) = f.x86; // zero out the buffer
				}
			}
		}
		else
			for (int i = 0; i < str->AllocatedSize; i++)
			{
				str->Data[i] = 0; // zero out the buffer
			}
			*/
		local_memzero((unsigned char*)str->Data, (str->AllocatedSize * ALLOC_PTR(str,SingleCharacterLength))); // zero out the buffer using our local_memzero function
		str->Length = 0;
	}

	int LWAnsiString_AdjustSize(LWAnsiString* str, int new_size)
	{
		/* UNIT TESTED to truncate str and enloarge buffer.*/
		AllocationHandler* Handler = nullptr;
		if (str == nullptr)
			return -1; // null string
		if (new_size <= 0)
			return -1; // no change if size is less than or equal to 0

		ProbeIfDirtyLen(str);

		Handler = (AllocationHandler*)str->AllocatedHandle;
		if (Handler == 0)
			return -1; // WE CAN'T CAUSE there's no table of allocator routines

		if (new_size != str->AllocatedSize)
		{
			//char* newPtr = (char*)((AllocationHandler*)(str->AllocatedHandle))->CustomReAlloc(((AllocationHandler*)(str->AllocatedHandle))->CustomGetHeap(0, 0, 0), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, str->Data, new_size + 1); // +1 for null terminator
			char* newPtr = (char*)Handler->CustomReAlloc(Handler->CustomGetHeap(0, 0, 0), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, str->Data, ( (new_size + 1) * ALLOC_PTR(str, SingleCharacterLength))); // +1 for null terminator

			if (newPtr == nullptr)
			{
				return -1; // failed to reallocate
			}
			else
			{
				/* zero new size. we don't assume it happens*/
				if (new_size > str->AllocatedSize)
				{
					unsigned char* offsetBound = (unsigned char*)newPtr + str->AllocatedSize; // this is for zeroing out the new size and not blinding asusming the heap function did.
					local_memzero(offsetBound + str->Length, new_size - str->Length); // zero out the new size
				}
				if (new_size < str->Length)
					str->Length = new_size;

				str->Data = newPtr; // set the new data pointer
				str->AllocatedSize = new_size + 1; // update the allocated size
//				str->Data[str->AllocatedSize - 1] = 0; // null terminate
				LWAnsiString_ClampNull(str);
				return new_size; // return max buffer size
			}
		}
		else
		{
			LWAnsiString_ClampNull(str); // ensure the string is null terminated
		}
		return str->AllocatedSize; // no change if size is less than or equal to allocated size
	}

	void LWAnsiString_AppendNewLine(LWAnsiString* str)
	{
		ProbeIfDirtyLen(str);
		/* INDRECT UNIT TEST cause LWAnsiString_Append works */
		LWAnsiString_Append(str, "\r\n"); // append a new line
	}


	LWAnsiString* LWAnsiString_PadInternal(LWAnsiString* str, const char c, int len)
	{
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

	LWAnsiString* LWAnsiString_PadNewLineInternal(LWAnsiString* str, const wchar_t c, int len)
	{
		LWAnsiString* r = LWAnsiString_Pad(str, c, len); // pad the string with the character
		ProbeIfDirtyLen(r);
		LWAnsiString_AppendNewLine(r); // append a new line
		return r;
	}

	

	LWAnsiString* LWAnsiString_AppendNative(LWAnsiString* str, const LWAnsiString* append)
	{
		/* INIT TESTED CAUSE WE USE IT THRUOUT MIDAS AND the unit tests for other stuff*/
		if (str == nullptr)
			return str;
		if (append == nullptr)
			return str; // nothing to append
		if (str->AllocatedHandle == 0)
			return nullptr; // WE CAN'T CAUSE there's no table of allocator routines
		// TODO: Add unit test to test for this.
		int append_len = LWAnsiString_Length((LWAnsiString*) append);
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


	
	

	

	LWAnsiString* LW_INTERNAL LWAnsiString_AppendOld(LWAnsiString* str, const char* append)
	{
		/* INIT TESTED CAUSE WE USE IT THRUOUT MIDAS AND the unit tests for other stuff*/
		if (str == nullptr)
			return str;
		if (append == nullptr)
			return str; // nothing to append
		if (str->AllocatedHandle == 0)
			return nullptr; // WE CAN'T CAUSE there's no table of allocator routines
		// TODO: Add unit test to test for this.
		size_t append_len = lstrlenA(append);
		if (append_len == 0)
			return str; // nothing to append
		int new_size = 0;
		int required_size = 0;
		ProbeIfDirtyLen(str);
#ifndef AGGRO_REALLOC
		required_size = str->Length + append_len;
#else
		
		if ((str->Length + append_len+1) < (str->AllocatedSize))
		{
			required_size = str->AllocatedSize;
		}
		else
		{
			if ((str->Length + append_len+1) < (str->AllocatedSize * 2))
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

	LWAnsiString* LWAnsiString_AppendWithNewLine(LWAnsiString* str, const char* append)
	{
		ProbeIfDirtyLen(str);
		LWAnsiString* r = LWAnsiString_Append(str, append); // append the string
		if (r == nullptr)
			return nullptr; // failed to append
		LWAnsiString_AppendNewLine(r); // append a new line
		return r; // return the updated string
	}


	LWAnsiString* LWAnsiString_Reserve(LWAnsiString* str, int new_size)
	{
		/* UNIT TESTED THRU LWAnsiString_Reserve */
		if (str == nullptr)
			return nullptr; // null string
		if (new_size <= 0)
			return str; // no change if size is less than or equal to 0
		ProbeIfDirtyLen(str);
		if (new_size > str->AllocatedSize)
		{
			char* newPtr = (char*)((AllocationHandler*)(str->AllocatedHandle))->CustomReAlloc(((AllocationHandler*)(str->AllocatedHandle))->CustomGetHeap(0, 0, 0), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, str->Data, new_size + ALLOC_PTR(str, SingleCharacterLength)); // +1 for null terminator
			if (newPtr == nullptr)
			{
				return nullptr; // failed to reallocate
			}
			else
			{
				
				str->Data = newPtr; // set the new data pointer
				str->AllocatedSize = new_size + 1; // update the allocated size
				LWAnsiString_ClampNull(str);
				local_memzero((unsigned char*) str->A + (str->Length*ALLOC_PTR(str, SingleCharacterLength)), str->AllocatedSize + (- 1 - str->Length)* ALLOC_PTR(str, SingleCharacterLength)); // zero out the rest of the buffer
				LWAnsiString_ClampNull(str);
				return str; // return the current string

				/*
				str->Data = newPtr; // set the new data pointer
				str->AllocatedSize = new_size + 1; // update the allocated size
				str->Data[str->AllocatedSize - 1] = 0; // null terminate
				local_memzero((unsigned char*)str->Data + str->Length, str->AllocatedSize - 1 - str->Length); // zero out the rest of the buffer
				str->Data[str->Length] = 0; // ensure the string is null terminated
				return str; // return the current string*/
			}
		}
		return str; // no change if size is less than or equal to allocated size
	}

	LWAnsiString* LWAnsiString_AddReserveCap(LWAnsiString* str, int new_size, int max)
	{
		if (str == nullptr)
			return nullptr; // null string
		/* UNIT TESTED THRU LWAnsiString_Reserve */
		if (new_size + str->Length > max)
		{
			new_size = max;
		}
		ProbeIfDirtyLen(str);

		return LWAnsiString_Reserve(str, new_size); // reserve the string with the new size plus current length
	}
	LWAnsiString* LWAnsiString_AddReserve(LWAnsiString* str, int new_size)
	{
		ProbeIfDirtyLen(str);
		/* UNIT TESTED THRU LWAnsiString_Reserve */
		return LWAnsiString_Reserve(str, str->Length + new_size); // reserve the string with the new size plus current length
	}


	/// <summary>
	/// Duplicate the passed string
	/// </summary>
	/// <param name="str">string to dup</param>
	/// <returns>null on error and duplicate on ok</returns>
	LWAnsiString* LWAnsiString_Duplicate(LWAnsiString* str)
	{
		if (str == nullptr) return nullptr;
		/* UNIT TESTED THRU LWAnsiString_CreateFromString and the */
		ProbeIfDirtyLen(str);
		return LWAnsiString_CreateFromString(LWAnsiString_ToCStr(str));
	}

	LWAnsiString* LWAnsiString_DuplicateEx(AllocationHandler* x, LWAnsiString* str)
	{
		if (str == nullptr) return nullptr; // null string
		ProbeIfDirtyLen(str);
		/* UNIT TESTED THRU LWAnsiString_CreateFromStringEx and the */
		return LWAnsiString_CreateFromStringEx(x, LWAnsiString_ToCStr(str));
	}

	/// <summary>
	/// Retrive the raw underlying buffer for the string. Note while  the buffer contents can be changed the pointer itself SHOULD NOT BE
	/// </summary>
	/// <param name="str">type to get buffer to</param>
	/// <returns>buffer value or null</returns>
	const char* LWAnsiString_ToCStr(LWAnsiString* str)
	{
		if (str == nullptr)
		{
			return nullptr; // null string
		}
		ProbeIfDirtyLen(str);
		return str->A; // return the data
	}

	/// <summary>
	/// returns point to the null character add end of this string
	/// </summary>1
	/// <param name="str">type to get buffer to</param>
	/// <returns>pointer to the null char ending the string.</returns>
	const char* LWAnsiString_EndingOffset(LWAnsiString* str)
	{
		if (str == nullptr)
		{
			return nullptr;
		}
		ProbeIfDirtyLen(str);
		// future me: goal is we get the null end of char the rest of the librarby enforces to let
		// it be easy to strcat from an external api
		char* ret = (char*) (((unsigned char*)str->Data) + (ALLOC_PTR(str, SingleCharacterLength) * str->Length));
		return ret;
	}


	bool LWAnsiString_AppendNumber(int number, LWAnsiString* output, int* output_size)
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
			LWAnsiString_Append(output, "-2147483648");
			if (output_size != 0)
			{
				*output_size = ( 12 + 
					ALLOC_PTR(output, SingleCharacterLength) // this odd wayt lets it theory scale for say unicode and ansi
					);
			}; // 11 digits + null terminator
			return true;
		}

		if (number == 0)
		{
			// special case for zero
			LWAnsiString_Append(output, "0");
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


	int LWAnsiString_Compare(LWAnsiString* a, const char* b, bool Case)
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
		if (Case)
		{
			//return lstrcmpA(a->Data, b); // case sensitive
			return ALLOC_PTR(a, STRICMP)((const char*) a->Data, b);
		}
		else
		{
			//int tmp = lstrcmpiA(a->Data, b);
			return ALLOC_PTR(a, STRCMP)((const char*)a->Data, b);
			//return tmp; // case insensitive
		}
	}


	int LWAnsiString_FindChar(LWAnsiString* str, char c)
	{
		ProbeIfDirtyLen(str);
		return LWAnsiString_FindCharEx(str, c, 0); // start from the beginning
	}
	int LWAnsiString_FindLast(LWAnsiString* str, char c)
	{
		ProbeIfDirtyLen(str);
		return LWAnsiString_FindLastEx(str, c, nullptr); 
	}
	int LWAnsiString_FindLastEx(LWAnsiString* str, char c, int * count)
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

	int LWAnsiString_FindCharEx(LWAnsiString* str, char c, int start)
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

	int LWAnsiString_EndsAt(LWAnsiString* str, const char* suffix, bool Case)
	{
		if (str == nullptr || suffix == nullptr)
		{
			return false; // invalid string or suffix
		}
		ProbeIfDirtyLen(str);
		int res = 0;
		int suffix_len = lstrlenA(suffix);
		int str_len = LWAnsiString_Length(str);
		if (suffix_len > str->Length)
		{
			return false; // suffix is longer than the string
		}
		if (Case)
		{
			//res = lstrcmpA(str->Data + str_len - suffix_len, suffix) == 0 ? str_len - suffix_len : -1; // case sensitive
			res = ALLOC_PTR(str, STRCMP)((const char*) str->Data + str_len - suffix_len, suffix) == 0 ? str_len - suffix_len : -1; // case sensitive
		}
		else
		{
			//res = lstrcmpiA(str->Data + str_len - suffix_len, suffix) == 0 ? str_len - suffix_len : -1; // case insensitive
			res = ALLOC_PTR(str, STRICMP)((const char*)str->Data + str_len - suffix_len, suffix) == 0 ? str_len - suffix_len : -1; // case sensitive
		}
		return res;
	}
	bool LWAnsiString_EndsWith(LWAnsiString* str, const char* suffix, bool Case)
	{
		/* UNIT TESTED THRU LWAnsiString_EndsAt */
		if (str == nullptr || suffix == nullptr)
		{
			return false; // invalid string or suffix
		}
		ProbeIfDirtyLen(str);
		int res = LWAnsiString_EndsAt(str, suffix, Case);
		return res != -1;
	}

	/// <summary>
	/// If the string ends with the given suffix, trim it off.
	/// </summary>
	/// <param name="str"></param>
	/// <param name="suffix"></param>
	/// <param name="Case"></param>
	/// <returns></returns>
	bool LWAnsiString_TrimEndsWith(LWAnsiString* str, const char* suffix, bool Case)
	{/* UNIT TESTED THRU LWAnsiString_EndsAt */
		if (str == nullptr || suffix == nullptr)
		{
			return false; // invalid string or suffix
		}
		ProbeIfDirtyLen(str);
		int res = LWAnsiString_EndsAt(str, suffix, Case);
		if (res != -1)
		{
			str->Length = res; // trim the string
			//str->Data[str->Length] = 0; // null terminate
			LWAnsiString_ClampNull(str);
			return true; // trimmed successfully
		}
		return false; // not trimmed

	}

#pragma region

	LWAnsiString* LWAnsiString_AppendW(LWAnsiString* str, const wchar_t* append)
	{
		if (str->AllocatedHandle == LWUnicodeHandler)
		{
			LWAnsiString_AppendInternal(str, (const void*)append, ALLOC_PTR(str, STRLEN));
		}
		else
		{

			int res = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, append, -1, 0, 0, "?", 0);
			if (res > 0)
			{
				char* tmp = (char*)ALLOC_PTR(str, CustomFirstAlloc)(ALLOC_PTR(str, CustomGetHeap), 0, res);
				LWAnsiString* self = str;
				int do_res = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, append, -1, tmp, res, "?", 0);

				if (do_res != 0)
				{
					self = LWAnsiString_AppendInternal(str, (const void*)append, ALLOC_PTR(str, STRLEN));
				}

				if (tmp != 0) {
					ALLOC_PTR(str, CustomFree)(ALLOC_PTR(str, CustomGetHeap), 0, tmp);
				}
				return self;
			}
		}
		return nullptr;
	}

	LWAnsiString* LWAnsiString_AppendA(LWAnsiString* str, const char* append)
	{
		if (str->AllocatedHandle == LWAnsiHandler)
		{
			return LWAnsiString_AppendInternal(str, (const void*)append, ALLOC_PTR(str, STRLEN));
		}
		else
		{

			int res = MultiByteToWideChar(CP_ACP, 0, append, -1, 0, 0);
			if (res > 0)
			{
				wchar_t* tmp = (wchar_t*)ALLOC_PTR(str, CustomFirstAlloc)(ALLOC_PTR(str, CustomGetHeap), 0, res);
				LWAnsiString* self = str;
				int do_res = MultiByteToWideChar(CP_ACP, 0, append, -1, tmp, -1);

				if (do_res != 0)
				{
					 	self = LWAnsiString_AppendInternal(str, (const void*)append, ALLOC_PTR(str, STRLEN));
				}

				if (tmp != 0) {
					ALLOC_PTR(str, CustomFree)(ALLOC_PTR(str, CustomGetHeap), 0, tmp);
				}
				return self;
			}

		}
		return nullptr;
	}

	LWAnsiString* LWAnsiString_AppendWithNewLineA(LWAnsiString* str, const char* append)
	{
		LWAnsiString_AppendA(str, append);
		return LWAnsiString_AppendA(str, "\r\n");
	}

	LWAnsiString* LWAnsiString_AppendWithNewLineW(LWAnsiString* str, const wchar_t* append)
	{
		LWAnsiString_AppendW(str, append);
		return LWAnsiString_AppendW(str, L"\r\n");
	}

	LWAnsiString* LWAnsiString_PadNewLineA(LWAnsiString* str, const char c, int len)
	{
		return LWAnsiString_PadNewLineInternal(str, c, len);
	}

	LWAnsiString* LWAnsiString_PadNewLineW(LWAnsiString* str, const wchar_t c, int len)
	{
		return LWAnsiString_PadNewLineInternal(str, (const wchar_t)c, len);

	}
#pragma endregion
	
}


