#pragma once
#include "LWAnsiString.h"

class LWAnsiStringGuard
{
public:
    explicit LWAnsiStringGuard(int len);
    explicit LWAnsiStringGuard(BOOL UseUnit, int len);
    explicit LWAnsiStringGuard(LWAnsiString* Input);
    ~LWAnsiStringGuard();

    LWAnsiString* ptr = nullptr;
};