#include "IAT_STRING_STUFF.H"
#include "Support/LWAnsiString/LWAnsiString.h"
#include "Support/LWAnsiString/LWAnsiString_Internal.h"

/// <summary>
/// what this does is configure LwAnsiString  and import the routines in windows to do the thing.
/// </summary>
/// <param name="IncludeUnicode"></param>
/// <param name="getaddr"></param>
void Midas_LWANSISTRING_Init(BOOL IncludeUnicode,
    GetProcAddressPtr getaddr,
    HMODULE kernel32)
{
    if (!getaddr || !kernel32)
        return;

    // --- core heap + conversion APIs (ANSI + Unicode) ---

    IAT_GetProcessHeap = (IAT_GetProcessHeapPtr)getaddr(kernel32, "GetProcessHeap");
    IAT_HeapAlloc = (IAT_HeapAllocPtr)getaddr(kernel32, "HeapAlloc");
    IAT_HeapReAlloc = (IAT_HeapReAllocPtr)getaddr(kernel32, "HeapReAlloc");
    IAT_HeapFree = (IAT_HeapFreePtr)getaddr(kernel32, "HeapFree");

    IAT_MultiByteToWideChar = (IAT_MultiByteToWideCharPtr)getaddr(kernel32, "MultiByteToWideChar");
    IAT_WideCharToMultiByte = (IAT_WideCharToMultiBytePtr)getaddr(kernel32, "WideCharToMultiByte"); 

    // --- ANSI string APIs ---

    IAT_lstrlenA = (IAT_lstrlenAPtr)getaddr(kernel32, "lstrlenA");
    IAT_lstrcpyA = (IAT_lstrcpyAPTR)getaddr(kernel32, "lstrcpyA");
    IAT_lstrcatA = (IAT_lstrcatAPtr)getaddr(kernel32, "lstrcatA");
    IAT_lstrcmpA = (IAT_lstrcmpAPTR)getaddr(kernel32, "lstrcmpA");
    IAT_lstrcmpiA = (IAT_lstrcmpiAPTR)getaddr(kernel32, "lstrcmpiA");

    // --- Unicode string APIs (optional) ---

    if (IncludeUnicode)
    {
        IAT_lstrlenW = (IAT_lstrlenWPtr)getaddr(kernel32, "lstrlenW");
        IAT_lstrcatW = (IAT_lstrcatWPtr)getaddr(kernel32, "lstrcatW");
        IAT_lstrcmpW = (IAT_lstrcmpWPTR)getaddr(kernel32, "lstrcmpW");
        IAT_lstrcmpiW = (IAT_lstrcmpiWPTR)getaddr(kernel32, "lstrcmpiW");
    }
}
