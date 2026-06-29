#pragma once
#include <Windows.h>>
#include "IAT_BOOSTRAP_LOADER.H"


/// <summary>
/// what this does is configure LwAnsiString  and import the routines in windows to do the thing.
/// </summary>
/// <param name="IncludeUnicode"></param>
/// <param name="getaddr"></param>
void Midas_LWANSISTRING_Init(BOOL IncludeUnicode,
    GetProcAddressPtr getaddr,
    HMODULE kernel32);

