#pragma once
#include "LWAnsiString.h"


///<summary>
/// This blank macro is for me to mark certain functions as internal only. 
///</summary>
#define LW_INTERNAL

#define ALLOC_PTR( LW, Function)  ((AllocationHandler*)LW->AllocatedHandle)->Function

extern "C"
{
	//int LWAnsiString_EndsAtInternal(LWAnsiString* str, const char* suffix, bool Case, LWSTRING_CMP CompareFunction);

	/// <summary>
/// Internal version of LWansiString_EndsAt
/// </summary>
/// <param name="str">str </param>
/// <param name="suffix">TOP LEVEL routines use this for Char or wchar_t</param>
/// <param name="Case">IGNORED -> always uses STRCMP. The TopLevel ones LWAnsiString_EndsAt() pass the correct version if bool is right or not</param>
/// <param name="STRLEN">how to check length of incomming string</param>
/// <param name="STRCMP">you should pass STRICMP to do that, otherwase pass STRCMP</param>
/// <returns></returns>
	bool LWAnsiString_TrimEndsWithInternal(LWAnsiString* str, const char* suffix, bool Case, int (*SuppliedLWAnsiString_EndsAt)(LWAnsiString*, const char*, bool));

	int LWAnsiString_EndsAtInternal(LWAnsiString* str, const wchar_t* suffix, bool Case, LW_STRING_strlen STRLEN, LWSTRING_CMP STRCMP);

	int LWAnsiString_FindCharExInternal(LWAnsiString* str, wchar_t c, int start);

	int LWAnsiString_CompareInternal(LWAnsiString* a, const void* b, bool Case);

	LWAnsiString* LWAnsiString_PadInternal(LWAnsiString* str, DWORD c, int len);

	int LWAnsiString_FindLastExInternal(LWAnsiString* str, wchar_t c, int* count);


	bool LWAnsiString_AppendNumberInternal(int number,
		LWAnsiString* output,
		int* output_size,
		LWAnsiString* (*appendFunc)(LWAnsiString* str, const char* append),
		const void* IntMaxPlugin);


	/// <summary>
	/// Internal version of Append.
	/// </summary>
	/// <param name="str">str to append too. </param>
	/// <param name="append">wchar_t or char string to read from</param>
	/// <param name="STRLEN">how to get the end of the string</param>
	/// <returns>string</returns>
	/// <remarks>DO NOT try appending a unicode string to an Actual ansi string with this. Use the AppendW instead</remarks>
	LWAnsiString* LW_INTERNAL LWAnsiString_AppendInternal(LWAnsiString* str, const void* append, LW_STRING_strlen STRLEN);

	/// <summary>
	/// What this does if check if the len of the string is directy and triggers call to ProbeLength if so.
	/// </summary>
	/// <param name="str"></param>
	/// <returns></returns>
	LW_INTERNAL void ProbeIfDirtyLen(LWAnsiString* str);
}


typedef int(_stdcall* IAT_WideCharToMultiBytePtr)(
	UINT   CodePage,
	DWORD  dwFlags,
	LPCWCH lpWideCharStr,
	int    cchWideChar,
	LPSTR  lpMultiByteStr,
	int    cbMultiByte,
	LPCCH  lpDefaultChar,
	LPBOOL lpUsedDefaultChar
	);
typedef int(_stdcall* IAT_MultiByteToWideCharPtr)(
	UINT                              CodePage,
	DWORD                             dwFlags,
	LPCCH lpMultiByteStr,
	int                               cbMultiByte,
	LPWSTR                            lpWideCharStr,
	int                               cchWideChar
	);

typedef HANDLE(_stdcall* IAT_GetProcessHeapPtr)();

typedef LPVOID(_stdcall* IAT_HeapAllocPtr)(
	HANDLE hHeap,
	DWORD  dwFlags,
	SIZE_T dwBytes
	);
typedef LPVOID(_stdcall* IAT_HeapReAllocPtr)(
	HANDLE                 hHeap,
	DWORD                  dwFlags,
	LPVOID lpMem,
	SIZE_T                 dwBytes
	);

typedef BOOL(_stdcall* IAT_HeapFreePtr)(
	HANDLE                 hHeap,
	DWORD                  dwFlags,
	LPVOID lpMem
	);

typedef LPWSTR(_stdcall* IAT_lstrcatWPtr)(
	LPWSTR  lpString1,
	PCWSTR lpString2
	);
typedef LPSTR(_stdcall* IAT_lstrcatAPtr)(
	LPSTR  lpString1,
	LPCSTR  lpString2
	);

typedef LPSTR(_stdcall* IAT_lstrcpyAPTR)(
	LPSTR  lpString1,
	LPCSTR lpString2
	);

typedef LPWSTR(_stdcall* IAT_lstrcpyWPTR)(
	LPWSTR  lpString1,
	LPCWSTR lpString2
	);

typedef int(_stdcall* IAT_lstrlenAPtr)(LPCSTR lpString);
typedef int(_stdcall* IAT_lstrlenWPtr)(LPCWSTR lpString);


typedef int(_stdcall* IAT_lstrcmpiAPTR)(
	LPCSTR  lpString1,
	LPCSTR lpString2
	);

typedef int(_stdcall* IAT_lstrcmpiWPTR)(
	LPCWSTR  lpString1,
	LPCWSTR lpString2
	);

typedef int(_stdcall* IAT_lstrcmpAPTR)(
	LPCSTR  lpString1,
	LPCSTR lpString2
	);

typedef int(_stdcall* IAT_lstrcmpWPTR)(
	LPCWSTR  lpString1,
	LPCWSTR lpString2
	);

typedef void(_stdcall* IAT_SleepPtr)(DWORD dwSecs);



#ifndef LWANSISTRING_HARDIMPORTS
/* IF the build undefines LWANSISTRING_HARDIMPORTS and includes this include, we expose the IAT pointers that will normally point to Win32 code. */

extern IAT_WideCharToMultiBytePtr IAT_WideCharToMultiByte;
extern IAT_MultiByteToWideCharPtr IAT_MultiByteToWideChar;
extern IAT_GetProcessHeapPtr IAT_GetProcessHeap;
extern IAT_HeapAllocPtr IAT_HeapAlloc;
extern IAT_HeapReAllocPtr IAT_HeapReAlloc;
extern IAT_HeapFreePtr IAT_HeapFree;



	#ifdef LWANSISTRING_ANSI_TABLE
extern	IAT_lstrcatAPtr IAT_lstrcatA;
extern IAT_lstrcmpAPTR IAT_lstrcmpA;
extern IAT_lstrcmpiAPTR IAT_lstrcmpiA;
extern IAT_lstrlenAPtr IAT_lstrlenA = lstrlenA;
	#endif

#ifdef LWANSISTRING_UNICODE_TABLE
extern IAT_lstrlenW IAT_lstrlenA;
extern IAT_lstrcmpiWPTR IAT_lstrcmpiW;
extern IAT_lstrcatWPtr IAT_lstrcatW;
extern IAT_lstrcmpWPtr IAT_lstrcmpW;
#endif

#endif