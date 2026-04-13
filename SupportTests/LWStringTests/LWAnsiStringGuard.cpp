#include <pch.h>
#include "LWAnsiStringGuard.h"

LWAnsiStringGuard::LWAnsiStringGuard(int len)
{
    ptr = LWAnsiString_CreateString(len);
}

LWAnsiStringGuard::LWAnsiStringGuard(BOOL UseUnit, int len)
{
    if (UseUnit)
    {
        ptr = LWAnsiString_CreateStringW(len);
    }
    else
    {
        ptr = LWAnsiString_CreateString(len);
    }
}

LWAnsiStringGuard::LWAnsiStringGuard(LWAnsiString* Input)
{
    ptr = Input;
}

LWAnsiStringGuard::~LWAnsiStringGuard()
{
    if (ptr)
    {
        LWAnsiString_FreeString(ptr);
    }
}