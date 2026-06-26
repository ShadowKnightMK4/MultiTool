#pragma once

/*
* Our game plan is put all the strings we create and grow into single heap, created on first alloc.
*/
#include "framework.h"

extern "C" {
	/*
	* What this is is a loose fitting wrapper around a char* string that takes care of reallocs so the user don't have too.
	*
	* There's 2 main components
	*
	* The LWAnsiString struct which houses the struct tracking allocation size, current len of the string and the pointer to the string data. It'll also hold a pointer to an AllocationHandler string (below).
	*
	*
	* The AllocationHandler struct which is a set of function pointers that default to small stubs that call the Windows Heap routines on top of the current process heap. Why?
	* this will let us customize how data is allocated, maybe sub in a different heap plan or more. The default routines will ask the Heap to trigger exceptions and zero memory on allocaiton.
	*
	* the inpact is we get a functional low level string type built on raw Windows API that lets use inject *how* to allocate memory and how to free it WITHOUT libc or the normal C++ stuff.
	* Suitable for when one wants small footprint and no dependencies on C++ or libc.  This is a C style string type that can be used in C++ code.
	*
	*
	* If You want to Inject a custom Allocation Handler use the Ex routines such as  LWAnsiString* CreateLWAnsiStringFromStringEx(AllocationHandler* x, const char* str);
	*
	* IMPORTANT: Do not cache the internal string pointer (LWAnsiString->Data).
	* Always fetch a fresh pointer when needed, either via LWAnsiString->Data or LWAnsiString_ToCStr().
	*/

	/*
	* Design phylospohy is interided some from my larger Midas Project.
	*
	* NO LIBC, STL or same code. (using data types and #defines from normal include file is fine)
	* We're aming for Win95-Win11 era. (partially why just raw Win32 Heaps, and the lstrlen kernel32 exports that existed for like near ever..
	*
	* LWAnsiString  itself is used extensively thru Midas as effecticely a flat growing array or Euphoria style sequence (dynamic array) - in fect that's why some of the code uses Append as  a name like LWAnsiString_AppendWithNewLine()
	*
	*/

	/* Example
	#include "LWAnsiString.h"
#include <windows.h>

// so we don't pull the ful llibrary of libc/stl. (Windows platform)
//
void PrintString(LWAnsiString* s) {
	DWORD written;
	WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),
			  LWAnsiString_ToCStr(s),
			  LWAnsiString_Length(s),
			  &written,
			  NULL);
}

int main() {
	LWAnsiString* myString = LWAnsiString_CreateFromString("Hello, World!\r\n");
	PrintString(myString);
	LWAnsiString_FreeString(myString);
	return 0;
}

	*/

#define LWANSISTRING_H
#include <Windows.h>
	/// <summary>
	/// See MSDN HeapAlloc for an idea of what do implementation for this.
	/// </summary>
	typedef void* (WINAPI* MyAlloc)(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
	/// <summary>
	/// See MSDN HeapCreate for an idea of what do implementation for this WITH ONE VERY IMPORTANT EXCEPTION, receiving (0,0,0) should be equivalent to GetProcessHeap() and return the current process heap desired
	/// </summary>
	typedef HANDLE(WINAPI* MyHeapGet)(DWORD Options, SIZE_T Start, SIZE_T Cap);

	/// <summary>
	/// See MSDN HeapFree for an idea of what do implementation for this.
	/// </summary>
	typedef BOOL(WINAPI* MyHeapFree)(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem);

	/// <summary>
	/// See MSDN HeapReAlloc for an idea of what do implementation for this.
	/// </summary>
	typedef LPVOID(WINAPI* MyHeapReAlloc)(
		HANDLE                 hHeap,
		DWORD                  dwFlags,
		LPVOID lpMem,
		SIZE_T                 dwBytes
		);

#define LWANSI_FLAG_DIRTY (1)
#define LWANSI_FLAG_ISANSI (2)
#define LWANSI_FLAG_ISUNICODE (4)
	typedef int LWAnsiStringFlags;

	typedef struct LWAnsiString
	{
		// Pointer to the actual string data, allocated on the heap.
		// Do not modify this pointer. Modifying the contents is fine.
		// IMPORTANT: Do not cache this pointer. Always fetch a fresh pointer when needed,
		// either via LWAnsiString->Data or LWAnsiString_ToCStr().
		union
		{
			void* Data;
			char* A;
			wchar_t* W;
			char* AnsiData;
			wchar_t* UnicodeData;
		};
		// Logical length of the string in CHARS. Do not modify this directly.
		// All length changes should happen through the library functions.
		size_t Length;

		// Allocated size of the string buffer IN CHARS. Internal use only.
		size_t AllocatedSize;

		/// <summary>
		/// Handle to the heap used for allocation. Do not modify. (In practice, this is AllocationHandler*) struct
		/// </summary>
		void* AllocatedHandle;
		LWAnsiStringFlags Flags;
	} LWAnsiString;


	typedef int (WINAPI* LW_STRING_strlen)(VOID* lpString);
	typedef	void* (WINAPI* LW_STRING_strcat)(VOID* target, VOID* source);
	typedef void* (WINAPI* LW_STRING_strcpy)(VOID* target, VOID* source);
	typedef size_t(WINAPI* LW_STRING_INDEXOR)(VOID* Target, size_t Index);
	typedef size_t(WINAPI* LW_STRING_WRITE_INDEX)(VOID* Target, size_t Index, size_t val);
	typedef int (WINAPI* LWSTRING_ICMP)(
		const char* string1,
		const char* string2
		);
	typedef int (WINAPI* LWSTRING_CMP)(
		const char* string1,
		const char* string2
		);

	typedef struct AllocationHandler
	{
		MyAlloc CustomFirstAlloc;
		MyHeapGet CustomGetHeap;
		MyHeapFree CustomFree;
		MyHeapReAlloc CustomReAlloc;
		DWORD SingleCharacterLength;
		LW_STRING_strlen STRLEN;
		LW_STRING_strcat STRCAT;
		LW_STRING_strcpy STRCPY;
		LW_STRING_INDEXOR INDEX;
		LW_STRING_WRITE_INDEX WriteToIndex;
		LWSTRING_ICMP STRICMP;
		LWSTRING_CMP STRCMP;
	} AllocationHandler;

	/// <summary>
	/// This is the handler to pass for creating Ansi Strings
	/// </summary>
	extern AllocationHandler* LWAnsiHandler;
	/// <summary>
	/// This is the handler to pass for creating Unicode Strings
	/// </summary>
	extern AllocationHandler* LWUnicodeHandler;

	extern AllocationHandler DefaultHandler;

	// LWAnsiString.cpp : Defines the functions for the static library.
	//

	

		bool LWAnsiString_AppendNumberA(int number, LWAnsiString* output, int* output_size);
		bool LWAnsiString_AppendNumberW(int number, LWAnsiString* output, int* output_size);

		/// <summary>
		/// For char strings, this is just functionally fetching the allocate in the struct. For non byte strings (ie wchar_t*) it multiples by single char in allocate struct
		/// </summary>
		/// <param name="str"></param>
		/// <returns></returns>
		size_t LWAnsiString_GetAllocatedByteSize(LWAnsiString* str);


		/// <summary>
		/// Shortcut for using the unicode defines for creating len (in WIDE_CHARS)
		/// </summary>
		/// <param name="len"></param>
		/// <returns></returns>
		LWAnsiString* LWAnsiString_CreateStringW(int len);

		/// <summary>
		/// Trigger a realloc if needed to ensure the string has at least new_size characters available. If the string is null, it will return null. If new_size is less than or equal to the current size, it will return the string unchanged. Also allocated extra memory will zero it out
		/// </summary>
		/// <param name="str"></param>
		/// <param name="new_size"></param>
		/// <returns></returns>
		/// <remarks>IMPORTANT. As it stands atm, this does NOT zero stuff out.</remarks>
		LWAnsiString* LWAnsiString_Reserve(LWAnsiString* str, size_t new_size);

		/// <summary>
		/// This varient of LWAnsiString_Reserve will do new_size PLUS current_size but will also clamp the new size to max. If new_size + current_size is greater than max, it will set new_size to max - current_size.
		/// </summary>
		/// <param name="str"></param>
		/// <param name="new_size"></param>
		/// <param name="max"></param>
		/// <returns></returns>
		LWAnsiString* LWAnsiString_AddReserveCap(LWAnsiString* str, size_t new_size, size_t max);

		/// <summary>
		/// This will duplicate the passed string starting at offset, allocating a new string and copying the contents of the passed string into it starting at the char directly after offset. The new string will be null terminated.
		/// </summary>
		/// <param name="str">str to duplicatie</param>
		/// <param name="offset">positive offset.</param>
		/// <returns>returns new duplicate of the string </returns>
		LWAnsiString* LWAnsiString_CreateFromOffset(LWAnsiString* str, int offset);

		/// <summary>
		/// This will duplicate the passed string starting at offset, allocating a new string and copying the contents of the passed string into it starting at offset. The new string will be null terminated.
		/// </summary>
		/// <param name="x">allocation handler</param>
		/// <param name="str">str</param>
		/// <param name="offset"></param>
		/// <returns></returns>
		LWAnsiString* LWAnsiString_CreateFromOffsetEx(AllocationHandler* x, LWAnsiString* str, int offset);


		/// <summary>
		/// This will conver the int passed to an 0-0 number, taking care of the sign and appending it to the end of the output string. 
		/// </summary>
		/// <param name="number">number to convert</param>
		/// <param name="output">out string</param>
		/// <param name="output_size">optional, will be how many digits processed</param>
		/// <returns></returns>
		/// <remarks>HEX this isn't.  This is closer to decimal ie +123 -> 123,  -1000  -> -1000</remarks>
		//bool LWAnsiString_AppendNumber(int number, LWAnsiString* output, int* output_size);
		/// <summary>
		/// This varient of LWAnsiString_Reserve will do new_size PLUS current_size
		/// </summary>
		/// <param name="str"></param>
		/// <param name="new_size"></param>
		/// <returns></returns>
		LWAnsiString* LWAnsiString_AddReserve(LWAnsiString* str, size_t new_size);
		/// <summary>
		/// While the memory adjusting strings clamp, this will let you do that in a single call
		/// </summary>
		/// <param name="str">string to clamp</param>
		/// <remarks>Is it recommanded to call this if you're not sure of what kind of string. It'll look into the Ansi/unicde and set index[length] to 0</remarks>
		void LWAnsiString_ClampNull(LWAnsiString* str);

		/// <summary>
		/// DEPENDS ON COMPILE SETTING. If _UNICODE || UNICODE DEFINED, this ends up being CreateStringW,  if not CreateStringA
		/// </summary>
		/// <param name="len">length to do</param>
		/// <returns>pointer to new string. </returns>
		LWAnsiString* LWAnsiString_CreateString(int len);

		LWAnsiString* LWAnsiString_CreateStringEx(AllocationHandler* x, int len);
		/// <summary>
		/// Create a string to hold the c string
		/// </summary>
		/// <param name="str">starting value of the string.</param>
		/// <returns>pointer to new string. </returns>
		LWAnsiString* LWAnsiString_CreateFromString(const char* str);

		/// <summary>
		/// Create a string to hold the c string and specify a different allocator 
		/// </summary>
		/// <param name="x">replacement allocator. </param>
		/// <param name="str">starting value of the string.</param>
		/// <returns>pointer to new string. </returns>
		LWAnsiString* LWAnsiString_CreateFromStringEx(AllocationHandler* x, const char* str);
		/// <summary>
		/// free our string data we prev allocated. If there is some corruption or the string is null, this will return false but will attempt to free as much as possible.
		/// </summary>
		/// <param name="str">string to free</param>
		/// <returns>true if fully freed. false if something wasn't</returns>
		/// <remarks>depends on the AllocateHandle agnostic pointer being a valid AllocaitonHandle. That incorrect? then fail.     Otherwise if buffer is null, we don't free it. if the struct is null. we don't free it. If ths allocator is not the default, we free it</remarks>
		bool LWAnsiString_FreeString(LWAnsiString* str);


		/// <summary>
		/// how long is this string?
		/// </summary>
		/// <param name="str"></param>
		/// <returns>returns the stored length of the string</returns>
		int LWAnsiString_Length(LWAnsiString* str);

		/// <summary>
		/// Sets stored length of string to be invalid (-1). The other routines will check and call LWAnsiString_ProbeLength
		/// </summary>
		/// <param name="str">str to mark</param>
		/// <returns>returns passed str</returns>
		/// <remarks>stricklt speaking currently the code that triggers call to ProbeLength checks if len is less than or equal to -1</remarks>
		LWAnsiString* LWAnsiString_MarkLenDirty(LWAnsiString* str);
		/// <summary>
		/// how long is the string? Update the stored length if *not* empty
		/// </summary>
		/// <param name="str"></param>
		/// <returns>returns the stored length of the string</returns>
		/// <remarks>picking between LWAnsiString_ProbeLength and  LWAnsiString_Length. If you're using the buffer directly as a target and not append or create, go for Probe, that ensures the length int is synched. If you're only mutating directly and ensuring the length is synched - go for length, it's probably faster </remarks>
		int LWAnsiString_ProbeLength(LWAnsiString* str);
		/// <summary>
		/// Wipe the buffer to zero. keep it allocated and null terminate it.
		/// </summary>
		/// <param name="str">strign to do</param>
		/// <returns></returns>
		void LWAnsiString_ZeroString(LWAnsiString* str);

		/// <summary>
		/// Adjust str to new size. If new_size is less than current size, it will truncate the string and null terminate it. If more than current size, we realloc and update.
		/// </summary>
		/// <param name="str">str to reallocation</param>
		/// <param name="new_size">size to adjust too</param>
		/// <returns>returns current buffer_size-1 (ready to just use as quick strcpy into) after resizing on success and -1 on error</returns>
		int LWAnsiString_AdjustSize(LWAnsiString* str, int new_size);

		LWAnsiString* LWAnsiString_PadA(LWAnsiString* str, char c, int len);

		LWAnsiString* LWAnsiString_PadW(LWAnsiString* str, wchar_t c, int len);
		/// <summary>
		/// Add an \r\n to this string
		/// </summary>
		/// <param name="str"></param>
		//void LWAnsiString_AppendNewLine(LWAnsiString* str);
		/// <summary>
		/// pad string at the right with c, len times
		/// </summary>
		/// <param name="str"></param>
		/// <param name="c"></param>
		/// <param name="len"></param>
		/// <returns></returns>
		/// <remarks>UNIT TEST</remarks>
		//LWAnsiString* LWAnsiString_Pad(LWAnsiString* str, const char c, int len);

		/// <summary>
		/// Varient of LWAnsiString_Pad that will also add a new line after padding
		/// </summary>
		/// <param name="str"></param>
		/// <param name="c"></param>
		/// <param name="len"></param>
		/// <returns></returns>
		//LWAnsiString* LWAnsiString_PadNewLine(LWAnsiString* str, const char c, int len);

		/// <summary>
		/// Varient of append that adds a new line also
		/// </summary>
		/// <param name="str"></param>
		/// <param name="append"></param>
		/// <returns></returns>
		//LWAnsiString* LWAnsiString_AppendWithNewLine(LWAnsiString* str, const char* append);
		/// <summary>
		/// Append a string to the end of the LWAnsiString, realloc as needed and null terminate
		/// </summary>
		/// <param name="str">string to append to</param>
		/// <param name="append">c string to read from</param>
		/// <returns>if str is null, returns null, if append is null, returns string unchanged, otherwise reallocs as needed does strcpy and then appends</returns>
		//LWAnsiString* LWAnsiString_Append(LWAnsiString* str, const char* append);

		/// <summary>
		/// Duplicate the passed 
		/// </summary>
		/// <param name="str">string to dup</param>
		/// <returns>null on error and duplicate on ok</returns>
		LWAnsiString* LWAnsiString_Duplicate(LWAnsiString* str);



		/// <summary>
		/// Retrive the raw underlying buffer for the string. Note while  the buffer contents can be changed the pointer itself SHOULD NOT BE
		/// </summary>
		/// <param name="str">type to get buffer to</param>
		/// <returns>buffer value or null</returns>
		const char* LWAnsiString_ToCStr(LWAnsiString* str);

		/// <summary>
		/// returns pointer to the null character add end of this string. BE Sure that the allocated memory is large enough for your incoming strcat.
		/// </summary>
		/// <param name="str">type to get buffer to</param>
		/// <returns>pointer to the null char ending the string.</returns>
		const char* LWAnsiString_EndingOffset(LWAnsiString* str);
		/// <summary>
		/// Compare a string with a char. Uses case sensitivity if Case is true, otherwise case insensitive
		/// </summary>
		/// <param name="a">furst </param>
		/// <param name="b">next</param>
		/// <param name="Case">care matters</param>
		/// <returns>if both are null, returns 0, if a is null, returns -1, if b is null returns 1, otherwise it's stricmp vs strcmp</returns>
		int LWAnsiString_Compare(LWAnsiString* a, const char* b, bool Case);


		/// <summary>
		/// Search for c c starting at start
		/// </summary>
		/// <param name="str">str to search</param>
		/// <param name="c">c look for for</param>
		/// <param name="start">start here. </param>
		/// <returns>-1 on failure and index on yay</returns>
		int LWAnsiString_FindCharEx(LWAnsiString* str, char c, int start);

		/// <summary>
		/// Search for c char stasrting at position 0.
		/// </summary>
		/// <param name="str">str to search</param>
		/// <param name="c">c look for for</param>
		/// <returns>-1 on failure and index on yay</returns>
		int LWAnsiString_FindChar(LWAnsiString* str, char c);

		/// <summary>
		/// return the locaiton of the last char c in the string
		/// </summary>
		/// <param name="str"></param>
		/// <param name="c"></param>
		/// <returns></returns>
		//int LWAnsiString_FindLast(LWAnsiString* str, char c);

		/// <summary>
		/// find the location of the last char c in the passed string with optionally, counting number of its.
		/// </summary>
		/// <param name="str"></param>
		/// <param name="c"></param>
		/// <param name="count"></param>
		/// <returns></returns>
		int LWAnsiString_FindLastEx(LWAnsiString* str, char c, int* count);
		/// <summary>
		/// 
		/// </summary>
		/// <param name="str"></param>
		/// <param name="suffix"></param>
		/// <param name="Case"></param>
		/// <returns></returns>
		bool LWAnsiString_EndsWith(LWAnsiString* str, const char* suffix, bool Case);


		//int LWAnsiString_EndsAt(LWAnsiString* str, const char* suffix, bool Case);
		/// <summary>
		/// If the string ends with the given suffix, trim it off. If it does not end with the suffix, the string remains unchanged.
		/// </summary>
		/// <param name="str"></param>
		/// <param name="suffix"></param>
		/// <param name="Case"></param>
		/// <returns></returns>
		bool LWAnsiString_TrimEndsWith(LWAnsiString* str, const char* suffix, bool Case);





		void LWAnsiString_AppendNewLine(LWAnsiString* str);
		/// <summary>
		/// Append one string to another. IF appending cross ANSI/Unicode, we're prompting the source append to target 
		/// </summary>
		/// <param name="str">target</param>
		/// <param name="append">source</param>
		/// <returns>returns source</returns>
		/// 
		LWAnsiString* LWAnsiString_AppendNative(LWAnsiString* str, const LWAnsiString* append);



	
		LWAnsiString* LWAnsiString_CreateStringW(int len);

		LWAnsiString* LWAnsiString_PadNewLineW(LWAnsiString* str, const wchar_t c, int len);
		LWAnsiString* LWAnsiString_AppendWithNewLineW(LWAnsiString* str, const wchar_t* append);
		LWAnsiString* LWAnsiString_AppendW(LWAnsiString* str, const wchar_t* append);
		bool LWAnsiString_TrimEndsWithW(LWAnsiString* str, wchar_t* suffix, bool Case);
		void LWAnsiString_AppendNewLine(LWAnsiString* str);
		void LWAnsiString_AppendNewLineW(LWAnsiString* str);

		bool LWAnsiString_EndsWithW(LWAnsiString* str, const wchar_t* suffix, bool Case);
		int LWAnsiString_EndsAtW(LWAnsiString* str, const wchar_t* suffix, bool Case);
		int LWAnsiString_FindCharExW(LWAnsiString* str, wchar_t c, int start);
		int LWAnsiString_FindLastExW(LWAnsiString* str, char c, int* count);
		int LWAnsiString_FindCharW(LWAnsiString* str, wchar_t c);
		int LWAnsiString_FindLastW(LWAnsiString* str, wchar_t c);
		int LWAnsiString_CompareW(LWAnsiString* a, const wchar_t* b, bool Case);
		int LWAnsiString_CompareExW(LWAnsiString* a, const wchar_t* b, bool Case, bool* DidCompare);





		LWAnsiString* LWAnsiString_CreateStringA(int len);
		LWAnsiString* LWAnsiString_PadNewLineA(LWAnsiString* str, const char c, int len);
		LWAnsiString* LWAnsiString_AppendWithNewLineA(LWAnsiString* str, const char* append);
		LWAnsiString* LWAnsiString_AppendA(LWAnsiString* str, const char* append);
		void LWAnsiString_AppendNewLineA(LWAnsiString* str);

		bool LWAnsiString_TrimEndsWithA(LWAnsiString* str, const char* suffix, bool Case);
		bool LWAnsiString_EndsWithA(LWAnsiString* str, const char* suffix, bool Case);
		int LWAnsiString_EndsAtA(LWAnsiString* str, const char* suffix, bool Case);
		int LWAnsiString_FindCharExA(LWAnsiString* str, char c, int start);
		int LWAnsiString_FindLastExA(LWAnsiString* str, char c, int* count);
		int LWAnsiString_FindCharA(LWAnsiString* str, char c);
		int LWAnsiString_FindLastA(LWAnsiString* str, char c);

		int LWAnsiString_CompareA(LWAnsiString* a, const char* b, bool Case);
		int LWAnsiString_CompareExA(LWAnsiString* a, const char* b, bool Case, bool* DidCompare);



		bool LWAnsiString_IsUnicode(LWAnsiString* str);
		bool LWAnsiString_IsAnsi(LWAnsiString* str);
		bool LWAnsiString_IsCustomHandler(LWAnsiString* str);

}
#define LWANSISTRING_FORMAT_ANSI
#define LWANSISTRING_FORMAT_UNICODE

#ifndef LWANSISTRING_FORMAT_NO_SUBS
#if defined(_UNICODE) || defined (_UNICODE)
#define LWANSISTRING_DEFAULT  LWANSISTRING_FORMAT_UNICODE

#else
#define LWANSISTRING_DEFAULT LWANSISTRING_FORMAT_ANSI

#endif

#endif