#pragma once
#include <windows.h>
#include "Support/LWAnsiString/LWAnsiString.h"

/// <summary>
/// Write to stdout, will setup pipes if not already done.
/// </summary>
/// <param name="message"></param>
extern void WriteStdout(const wchar_t* message);
/// <summary>
/// Write to stderr, will setup pipes if not already done.
/// </summary>
/// <param name="message"></param>
extern void WriteStderr(const wchar_t* message);
extern void WriteStdout(const char* message);
extern void WriteStderr(const char* message);

extern void WriteStdout(LWAnsiString* str);