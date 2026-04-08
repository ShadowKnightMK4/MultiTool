#pragma once
#include "LWAnsiString.h"


///<summary>
/// This blank macro is for me to mark certain functions as internal only. 
///</summary>
#define LW_INTERNAL

#define ALLOC_PTR( LW, Function)  ((AllocationHandler*)LW->AllocatedHandle)->Function

extern "C"
{
	/// <summary>
	/// Internal version of Append.
	/// </summary>
	/// <param name="str">str to append too. </param>
	/// <param name="append">wchar_t or char string to read from</param>
	/// <param name="STRLEN">how to get the end of the string</param>
	/// <returns>string</returns>
	/// <remarks>DO NOT try appending a unicode string to an Actual ansi string with this. Use the AppendW instead</remarks>
	LWAnsiString* LW_INTERNAL LWAnsiString_AppendInternal(LWAnsiString* str, const void* append, LW_STRING_strlen STRLEN);
	LW_INTERNAL void ProbeIfDirtyLen(LWAnsiString* str);
}
