#include "pch.h"
// LWAnsiString.cpp : Defines the functions for the static library.]
#define LWANSISTRING_HARDIMPORTS_VISIBLE
#include "LWAnsiString_Internal.h"
#include "intsafe.h"

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

/* defining this plugs the win32 routines that LWAnsiString uses into its own collection
* UNDEFINE THIS TO GET BLANKS. 
*/
#define LWANSISTRING_HARDIMPORTS 
#define LWANSISTRING_ANSIONLY
#define LWANSISTRING_UNICODE



#ifndef LWANSISTRING_HARDIMPORTS
#else
/*
* The win32 api routines that LWAnsiString depend on or reference are gonna be combined here
* for functionally a light IAT table. 
* Should you want to rip out the hard links fully (say going on the road to kernel mode (NOT TESTED THERE) or linux (or there)
* undefining LWANSISTRING_HARDIMPORTS will get you code that will even get you code to complain of *hey * need this*
*/

void Init_LWAnsiString_Imports()
{


	IAT_WideCharToMultiByte = WideCharToMultiByte;
	IAT_MultiByteToWideChar = MultiByteToWideChar;


	IAT_GetProcessHeap = GetProcessHeap;
	IAT_HeapAlloc = HeapAlloc;
	IAT_HeapReAlloc = HeapReAlloc;
	IAT_HeapFree = HeapFree;


	IAT_lstrcatA = lstrcatA;
	IAT_lstrcmpA = lstrcmpA;
	IAT_lstrcmpiA = lstrcmpiA;
	IAT_lstrlenA = lstrlenA;



	IAT_lstrlenW = lstrlenW;
	IAT_lstrcmpiW = lstrcmpiW;
	IAT_lstrcatW = lstrcatW;
	IAT_lstrcmpW = lstrcmpW;


#endif




}
extern "C" {

}

#include "framework.h"
#include <climits>
#include <cstdint>


#pragma optimize("", off)


#pragma optimize("", on)
// if defined this does a byte by byte local_memmzero() vs say trying to zero in pointer sized chunks
#define SIMPLE_MEMORY_COPY 

// if defined the Append flavors will double the length of the string memory size and take that or the reuqested memory size, whichever is bigger
#define AGGRO_REALLOC


	HANDLE StringHeap = 0;





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
		((unsigned char*)Target)[index] = (unsigned char)val;
		return 0;
	}
	size_t WINAPI DefaultIndexer(void* TARGET, size_t index)
	{
		char* t = (char*)TARGET;
		return (char) t[index];
	}

	size_t WINAPI UnicodeWriteIndex(wchar_t* Target, size_t index, size_t val)
	{
		Target[index] = (wchar_t)val;
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
		return IAT_GetProcessHeap(); // fallback to the default heap
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
		return IAT_HeapAlloc(hHeap, dwFlags, dwBytes);
	}
	LPVOID LW_INTERNAL WINAPI DefaultReAlloc(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, SIZE_T dwBytes)
	{

		if ((DefaultHandler.CustomReAlloc != nullptr) && (DefaultHandler.CustomReAlloc != DefaultReAlloc))
		{
			return DefaultHandler.CustomReAlloc(hHeap, dwFlags, lpMem, dwBytes);
		}
		return IAT_HeapReAlloc(hHeap, dwFlags, lpMem, dwBytes);
	}

	BOOL LW_INTERNAL WINAPI DefaultFree(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem)
	{
		if ((DefaultHandler.CustomFree != nullptr) && (DefaultHandler.CustomFree != DefaultFree))
		{
			return DefaultHandler.CustomFree(hHeap, dwFlags, lpMem);
		}
		return IAT_HeapFree(hHeap, dwFlags, lpMem);
	}


	


	/// <summary>
	/// The creater lwansistring calls this if the string heap is not set up. This intializes default
	/// </summary>
	void LW_INTERNAL SetupLWStringLibrary()
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
				UnicodeHandler.WriteToIndex = (LW_STRING_WRITE_INDEX)UnicodeWriteIndex;
			}
		}
#ifdef LWANSISTRING_HARDIMPORTS
		Init_LWAnsiString_Imports();
#endif
		
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

#ifdef _DEBUG
		auto calc = offset * ALLOC_PTR(str, SingleCharacterLength);
		auto calc_ret = (LWAnsiString_ToCStr(str) + calc);
		auto actual = LWAnsiString_CreateFromStringEx(x,calc_ret);
		return	actual; // create a new string from the offset
#else
		return LWAnsiString_CreateFromStringEx(x, LWAnsiString_ToCStr(str) + (offset* ALLOC_PTR(str, SingleCharacterLength))); // create a new string from the offset
#endif

	}

	LWAnsiString* LWAnsiString_CreateFromOffset(LWAnsiString* str, int offset)
	{
		ProbeIfDirtyLen(str);
		/* UNIT TESTED.*/
		return LWAnsiString_CreateFromOffsetEx((AllocationHandler*) str->AllocatedHandle, str, offset);
	}



	LWAnsiString* LWAnsiString_CreateString(int len)
	{
#if defined(_UNICODE) || defined(UNICODE)
		return LWAnsiString_CreateStringEx(LWUnicodeHandler, len);
#else
		return LWAnsiString_CreateStringEx(LWAnsiHandler, len);
#endif
	}

	LWAnsiString* LWAnsiString_CreateStringA(int len)
	{
		return LWAnsiString_CreateStringEx(LWAnsiHandler, len);
	}
	LWAnsiString* LWAnsiString_CreateStringW(int len)
	{
		return LWAnsiString_CreateStringEx(LWUnicodeHandler, len);
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

		SetupLWStringLibrary();
#ifndef DEBUG2
		{
			size_t size = sizeof(LWAnsiString);
			size += 1;
			size -= 1;
		}
#endif
		auto Ans = (LWAnsiString*)DefaultHandler.CustomFirstAlloc(StringHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, sizeof(LWAnsiString));
		if (Ans == nullptr)
		{
			return nullptr; // failed to allocate
		}
		else
		{
			size_t char_needed = (len + 1);// chars needed
			size_t bytes_needed = char_needed * x->SingleCharacterLength;
			// convert to actually bytes needed
			//bytes_needed *= (x->SingleCharacterLength);

			
			Ans->Data = (char*)DefaultHandler.CustomFirstAlloc(StringHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, 
				
				bytes_needed); // +1 for null terminator

			Ans->AllocatedSize = char_needed; // +1 for null terminator
			if ( (x != &DefaultHandler) && (x != LWUnicodeHandler))
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


				

				// finally mark the string for probing
				if (x == &DefaultHandler)
				{
					Ans->Flags |= LWANSI_FLAG_ISANSI;
				}
				else
				{
					if (x == &UnicodeHandler)
					{
						Ans->Flags |= LWANSI_FLAG_ISUNICODE;
					}
					else
					{
						Ans->Flags &= ~LWANSI_FLAG_ISANSI;
						Ans->Flags &= ~LWANSI_FLAG_ISUNICODE;
					}

				}



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
		if ( (x == &DefaultHandler) || (x == LWUnicodeHandler))
		{
			SetupLWStringLibrary();
		}
		size_t len = x->STRLEN((char*)str);
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

	LWAnsiString* LWAnsiString_CreateFromString(const wchar_t* str)
	{
		return LWAnsiString_CreateFromStringW(str);
	}

	LWAnsiString* LWAnsiString_CreateFromString(const char* str)
	{
		return LWAnsiString_CreateFromStringA(str);
	}
	


	LWAnsiString* LWAnsiString_CreateFromStringA(const char* str)
	{
		/*  UNIT TESTED. thru the full test collection UnitTestLWString*/
		return LWAnsiString_CreateFromStringEx(&DefaultHandler, str);
	}

	LWAnsiString* LWAnsiString_CreateFromStringW(const wchar_t* str)
	{
		return LWAnsiString_CreateFromStringEx(LWUnicodeHandler, (const char*)str);
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






				if (  (
						(AllocationHandler*)(str->AllocatedHandle) != &DefaultHandler) &&
						(AllocationHandler*)(str->AllocatedHandle) != LWUnicodeHandler) 

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
		if (str && str->Data && ALLOC_PTR(str, WriteToIndex))
		{
			ProbeIfDirtyLen(str);
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
			size_t NewChar = 0;
			size_t NewCharCountInBytes = 0;
			if (!AddOp(new_size, +1, &NewChar))
			{
				return -1; // HELLO OVERFL but possible signed/unsized junk
			}
			if (!MulOp(NewChar, ALLOC_PTR(str, SingleCharacterLength), &NewCharCountInBytes))
			{
				return -1; // still overflow
			}
			
			//char* newPtr = (char*)((AllocationHandler*)(str->AllocatedHandle))->CustomReAlloc(((AllocationHandler*)(str->AllocatedHandle))->CustomGetHeap(0, 0, 0), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, str->Data, new_size + 1); // +1 for null terminator
			char* newPtr = (char*)Handler->CustomReAlloc(Handler->CustomGetHeap(0, 0, 0), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, str->Data, NewCharCountInBytes); // +1 for null terminator

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
				return new_size; // return max buffer size chars
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
		if (str->AllocatedHandle == LWAnsiHandler)
		{
			return LWAnsiString_AppendNewLineA(str);
		}
		else
		{
			if (str->AllocatedHandle == LWUnicodeHandler)
			{
				return LWAnsiString_AppendNewLineW(str);
			}
		}
	}

	void LWAnsiString_AppendNewLineA(LWAnsiString* str)
	{
		ProbeIfDirtyLen(str);
		/* INDRECT UNIT TEST cause LWAnsiString_Append works */
		LWAnsiString_AppendA(str, "\r\n"); // append a new line
	}
	
	void LWAnsiString_AppendNewLineW(LWAnsiString* str)
	{
		ProbeIfDirtyLen(str);
		/* INDRECT UNIT TEST cause LWAnsiString_Append works */
		LWAnsiString_AppendW(str, L"\r\n"); // append a new line
	}


	

	LWAnsiString* LWAnsiString_AppendNative(LWAnsiString* str, const LWAnsiString* append)
	{
		if (str == nullptr)
			return str;
		if (str->AllocatedHandle == 0)
			return nullptr; // WE CAN'T CAUSE there's no table of allocator routines
		if (append == nullptr)
			return str; // nothing to append


		if (str->AllocatedHandle == LWAnsiHandler)
		{
			return LWAnsiString_AppendA(str, LWAnsiString_ToCStr((LWAnsiString*)append));
		}
		else
		{
			if (str->AllocatedHandle == LWUnicodeHandler)
			{
				return LWAnsiString_AppendW(str, (const wchar_t*) LWAnsiString_ToCStr((LWAnsiString*)append));
			}
		}
		return nullptr;
		
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

	

	

	LWAnsiString* LWAnsiString_Reserve(LWAnsiString* str, size_t  new_size)
	{
		size_t SizeCalcInBytes = 0;
		size_t Sizecalc = 0;
		/* UNIT TESTED THRU LWAnsiString_Reserve */
		if (str == nullptr)
			return nullptr; // null string
		/*if (new_size <= 0)
			return str; // no change if size is less than or equal to 0
			uncomment this out if you go back to signed size
			*/
		ProbeIfDirtyLen(str);


		if (new_size > str->AllocatedSize)
		{
			if (!AddOp(new_size, 1, &Sizecalc))
			{
				// unlikey to overflow but play defensie
				return nullptr;
			}
		}
		

		{
			{
				
				if (!MulOp(Sizecalc, ALLOC_PTR(str, SingleCharacterLength), &SizeCalcInBytes))
				{
					return nullptr;// overflow can happen still
				}
			}
		}

		/*
		// the null is added up here
		if (!AddOp(new_size, 0, &Sizecalc))
		{
			return nullptr; // overflow possible.
		}
		else
		{
			// addops already did it.
			if (!MulOp((Sizecalc ), ALLOC_PTR(str, SingleCharacterLength), &Sizecalc))
			{
				return nullptr;// overflow can happen still
			}
		}*/
		/*
		if ((new_size+1) > (MAXSIZE_T/ ALLOC_PTR(str, SingleCharacterLength)))
		{
			return nullptr; //  overflow can happen on this allocate.
		}
		else
		{
			Sizecalc = ((new_size + 1) * ALLOC_PTR(str, SingleCharacterLength)); // +1 for null terminator
		}*/


		if (Sizecalc > str->AllocatedSize)
		{

			
		//	Sizecalc = ((new_size + 1) * ALLOC_PTR(str, SingleCharacterLength)); // +1 for null terminator
			
			
			char* newPtr = (char*)((AllocationHandler*)(str->AllocatedHandle))->CustomReAlloc(((AllocationHandler*)(str->AllocatedHandle))->CustomGetHeap(0, 0, 0), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, str->Data, SizeCalcInBytes); // +1 for null terminator
			if (newPtr == nullptr)
			{
				return nullptr; // failed to reallocate
			}
			else
			{
				
				str->Data = newPtr; // set the new data pointer
				//str->AllocatedSize = new_size ; // update the allocated size
				str->AllocatedSize = Sizecalc;

					/*
					* FALLBACK: If a custom allocator is injected that ignores HEAP_ZERO_MEMORY,
					* uncomment this to manually zero the expanded memory buffer.
					*
					* size_t start_byte = str->Length * ALLOC_PTR(str, SingleCharacterLength);
					* size_t end_byte = str->AllocatedSize * ALLOC_PTR(str, SingleCharacterLength);
					* local_memzero((unsigned char*)str->A + start_byte, end_byte - start_byte);
					*/
				LWAnsiString_ClampNull(str);
				return str; // return the current string


			}
		}
		return str; // no change if size is less than or equal to allocated size
	}

	LWAnsiString* LWAnsiString_AddReserveCap(LWAnsiString* str, size_t new_size, size_t max)
	{
		if (str == nullptr)
			return nullptr; // null string
		/* UNIT TESTED THRU LWAnsiString_Reserve */
		ProbeIfDirtyLen(str);
		size_t calc_size = 0;

		if (new_size > max)
		{
			calc_size = max;
		}
		else
		{
			
			if (!AddOp(str->AllocatedSize, new_size, &calc_size))
			{
				return nullptr;
			}
			else
			{
				if (calc_size > max)
				{
					calc_size = max;
				}
			}
		}
		/*
		if (new_size + str->Length > max)
		{
			calc_size = max;
		}
		else
		{
			calc_size = new_size + str->Length;

			if (calc_size < str->Length) // overflow
			{
				calc_size = max;
			}
		}*/
		

		return LWAnsiString_Reserve(str, calc_size); // reserve the string with the new size plus current length
	}
	LWAnsiString* LWAnsiString_AddReserve(LWAnsiString* str, size_t new_size)
	{
		if (str == nullptr)
			return nullptr; // null string
		ProbeIfDirtyLen(str);



		if ((MAXSIZE_T - str->Length) < new_size)
		{
			return nullptr; 
		}

		size_t calc_size = str->Length + new_size;

	


		/* UNIT TESTED THRU LWAnsiString_Reserve */
		return LWAnsiString_Reserve(str, calc_size); // reserve the string with the new size plus current length
	}


	/// <summary>
	/// Duplicate the passed string
	/// </summary>
	/// <param name="str">string to dup</param>
	/// <returns>null on error and duplicate on ok</returns>
	LWAnsiString* LWAnsiString_Duplicate(LWAnsiString* str)
	{
		if (str == nullptr)
		{
			return nullptr;
		}

		if (str->AllocatedHandle == nullptr)
		{
			return nullptr;
		}
		ProbeIfDirtyLen(str);

		// below routes to the current create from depending on contents
		if (LWAnsiString_IsAnsi(str))
		{
			return LWAnsiString_CreateFromStringA(LWAnsiString_ToCStr(str));
		}

		if (LWAnsiString_IsUnicode(str))
		{
			return LWAnsiString_CreateFromStringW((const wchar_t*) LWAnsiString_ToCStr(str));
		}

		if (LWAnsiString_IsCustomHandler(str))
		{
			return LWAnsiString_CreateFromStringEx(( AllocationHandler*) str->AllocatedHandle, LWAnsiString_ToCStr(str));
		}

		// shouldnt'y striclty need this 
		// but if its none of the above (how?) we need to return something
		return nullptr;

	}

	LWAnsiString* LWAnsiString_DuplicateEx(AllocationHandler* x, LWAnsiString* str)
	{
		if (x == nullptr) return nullptr;
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
	/// <remarks>note well while this returns char*, it is returning the LEST char</remarks>
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

	bool LWAnsiString_AppendNumberA(int number, LWAnsiString* output, int* output_size)
	{
		return  LWAnsiString_AppendNumberInternal(number, output, output_size, (LWAnsiString * (*)(LWAnsiString*, const char*)) LWAnsiString_AppendA, "-2147483648", "0");
	}

	bool LWAnsiString_AppendNumberW(int number, LWAnsiString* output, int* output_size)
	{
		return  LWAnsiString_AppendNumberInternal(number, output, output_size, (LWAnsiString * (*)(LWAnsiString*, const char*)) LWAnsiString_AppendW, L"-2147483648", L"0");
	}



	int LW_INTERNAL LWAnsiString_CompareInternalShim(LWAnsiString* a, const wchar_t* b, bool Case, AllocationHandler* TestHandler, bool* DidCompare = 0)
	{
		if (a->AllocatedHandle == TestHandler)
		{
			if (DidCompare != 0)
			{
				*DidCompare = true;
			}
			return LWAnsiString_CompareInternal(a, (const void*)b, Case);
		}
		if (DidCompare != 0)
		{
			*DidCompare = false;
		}
		return -2;
	}


	/// <summary>
	/// 
	/// </summary>
	/// <param name="a"></param>
	/// <param name="b"></param>
	/// <param name="Case"></param>
	/// <param name="DidCompare">if ask this to compare matching strings, this is true, if say you compare against non matching string, this is set to false</param>
	/// <returns></returns>
	int LWAnsiString_CompareExA(LWAnsiString* a, const char* b, bool Case, bool* DidCompare)
	{
		return LWAnsiString_CompareInternalShim(a, (const wchar_t*)b, Case, LWAnsiHandler, DidCompare);
	}


	int LWAnsiString_CompareExW(LWAnsiString* a, const wchar_t* b, bool Case, bool* DidCompare)
	{
		return LWAnsiString_CompareInternalShim(a, (const wchar_t*)b, Case, LWUnicodeHandler, DidCompare);
	}

	int LWAnsiString_CompareA(LWAnsiString* a, const char* b, bool Case)
	{
		return LWAnsiString_CompareInternalShim(a, (const wchar_t*)b, Case, LWAnsiHandler, 0);
	}


	int LWAnsiString_CompareW(LWAnsiString* a, const wchar_t* b, bool Case)
	{
		return LWAnsiString_CompareInternalShim(a, (const wchar_t*)b, Case, LWUnicodeHandler, 0);
	}






#pragma region


	bool LWAnsiString_TrimEndsWithA(LWAnsiString* str, const char* suffix, bool Case)
	{
		return LWAnsiString_TrimEndsWithInternal(str, suffix, Case, LWAnsiString_EndsAtA);
	}
	bool LWAnsiString_EndsWithA(LWAnsiString* str, const char* suffix, bool Case)
	{
		return LWAnsiString_TrimEndsWithInternal(str, suffix, Case, ((int (*)(LWAnsiString*, const char*, bool)) LWAnsiString_EndsAtW));
	}
	int LWAnsiString_EndsAtA(LWAnsiString* str, const char* suffix, bool Case)
	{
		if (Case)
		{
			return LWAnsiString_EndsAtInternal(str, (const wchar_t*)suffix, false, ALLOC_PTR(str, STRLEN), ALLOC_PTR(str, STRCMP));
		}
		else
		{
			return LWAnsiString_EndsAtInternal(str, (const wchar_t*)suffix, false, ALLOC_PTR(str, STRLEN), ALLOC_PTR(str, STRICMP));
		}
	}

	int LWAnsiString_FindCharExA(LWAnsiString* str, char c, int start)
	{
		return LWAnsiString_FindCharExInternal(str, c, start);

	}
	int LWAnsiString_FindLastExA(LWAnsiString* str, char c, int* count)
	{
		return LWAnsiString_FindLastExInternal(str, c, count);
	}

	int LWAnsiString_FindLastA(LWAnsiString* str, char c)
	{
		return LWAnsiString_FindLastExInternal(str, c, nullptr);
	}



	


	bool LWAnsiString_TrimEndsWithW(LWAnsiString* str, wchar_t* suffix, bool Case)
	{
		return LWAnsiString_TrimEndsWithInternal(str, (const char*)suffix, Case, ((int (*)(LWAnsiString*, const char*, bool)) LWAnsiString_EndsAtW));
	}

	int LWAnsiString_EndsAtW(LWAnsiString* str, const wchar_t* suffix, bool Case)
	{
		if (Case)
		{
			return LWAnsiString_EndsAtInternal(str, suffix, Case, ALLOC_PTR(str, STRLEN), ALLOC_PTR(str, STRCMP));
		}
		else
		{
			return LWAnsiString_EndsAtInternal(str, suffix, Case, ALLOC_PTR(str, STRLEN), ALLOC_PTR(str, STRICMP));
		}
	}
	int LWAnsiString_FindCharExW(LWAnsiString* str, wchar_t c, int start)
	{
		return LWAnsiString_FindCharExInternal(str, c, start);
	}
	int LWAnsiString_FindLastExW(LWAnsiString* str, char c, int* count)
	{
		ProbeIfDirtyLen(str);
		return LWAnsiString_FindLastExInternal(str, c, count);
	}

	int LWAnsiString_FindLastW(LWAnsiString* str, wchar_t c)
	{
		ProbeIfDirtyLen(str);
		return LWAnsiString_FindLastExInternal(str, c, 0);
	}

	int LWAnsiString_FindCharA(LWAnsiString* str, char c)
	{
		ProbeIfDirtyLen(str);
		return LWAnsiString_FindCharExInternal(str, c, 0); // start from the beginning
	}

	int LWAnsiString_FindCharW(LWAnsiString* str, wchar_t c)
	{
		ProbeIfDirtyLen(str);
		return LWAnsiString_FindCharExInternal(str, c, 0); // start from the beginning
	}

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
				res++; // null char.
				char* tmp = (char*)ALLOC_PTR(str, CustomFirstAlloc)(ALLOC_PTR(str, CustomGetHeap)(0,0, res), 0, res);
				LWAnsiString* self = str;
				int do_res = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, append, -1, tmp, res, "?", 0);

				if (do_res != 0)
				{
					self = LWAnsiString_AppendInternal(str, (const void*)tmp, ALLOC_PTR(str, STRLEN));
				}

				if (tmp != 0) {
					ALLOC_PTR(str, CustomFree)(ALLOC_PTR(str, CustomGetHeap)(0,0,0), 0, tmp);
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
#ifdef _DEBUG
			int debug_check = lstrlenA(append);
#endif
			if (res > 0)
			{
				auto Heap = ALLOC_PTR(str, CustomGetHeap)(0, 0, 0);
				auto math_stuff = res;
				math_stuff *= ALLOC_PTR(str, SingleCharacterLength);
				wchar_t* tmp = (wchar_t*)ALLOC_PTR(str, CustomFirstAlloc)(Heap, 0, math_stuff);
				LWAnsiString* self = str;
				if (tmp != 0)
				{
					local_memzero((unsigned char*)tmp, math_stuff);

				
					int do_res = MultiByteToWideChar(CP_ACP, 0, append, -1, tmp, res);

					if (do_res != 0)
					{
						self = LWAnsiString_AppendInternal(str, (const void*)tmp, ALLOC_PTR(str, STRLEN));
					}

				}
				if (tmp != 0) {
					ALLOC_PTR(str, CustomFree)(ALLOC_PTR(str, CustomGetHeap)(0, 0, 0), 0, tmp);
					tmp = 0;
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

	const char* NewLineA_Pading = "\r\n";
	const wchar_t* NewLineW_Padding = L"\r\n";
	LWAnsiString* LWAnsiString_PadNewLineA(LWAnsiString* str, const char c, int len)
	{
		auto ret= LWAnsiString_PadInternal(str, c, len);
		LWAnsiString_AppendA(str, NewLineA_Pading);
		return ret;
	}

	LWAnsiString* LWAnsiString_PadNewLineW(LWAnsiString* str, const wchar_t c, int len)
	{
		auto ret = LWAnsiString_PadInternal(str, c, len);
		LWAnsiString_AppendW(str, NewLineW_Padding);
		return ret;

	}
	LWAnsiString* LWAnsiString_PadA(LWAnsiString* str, char c, int len)
	{
		return LWAnsiString_PadInternal(str, c, len);
	}

	LWAnsiString* LWAnsiString_PadW(LWAnsiString* str, wchar_t c, int len)
	{
		return LWAnsiString_PadInternal(str, c, len);
	}
	bool LWAnsiString_IsUnicode(LWAnsiString* str)
	{
		return str->Flags & LWANSI_FLAG_ISUNICODE;
	}

	bool LWAnsiString_IsAnsi(LWAnsiString* str)
	{
		return str->Flags & LWANSI_FLAG_ISANSI;
	}

	bool LWAnsiString_IsCustomHandler(LWAnsiString* str)
	{
		return (LWAnsiString_IsAnsi(str) == false) && (LWAnsiString_IsUnicode(str) == false);
	}

#pragma endregion
	

	size_t LWAnsiString_GetAllocatedByteSize(LWAnsiString* str)
	{
		return ALLOC_PTR(str, SingleCharacterLength) * str->AllocatedSize;
	}


 
