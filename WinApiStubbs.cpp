#include "common.h"
/*
* This file is a kledge for unit tests. If we'er in unit test mode, we're subbing out the normal stuff and exposing api to let the test code verify stuff.
* Not unit test mode?-> the MyXXXX is redefined to use the normal api
*/
#ifdef  UNIT_TESTS_MODE
HLOCAL MyLocalFree(void* ptr)
{
	return LocalFree(ptr);
}

int MyMultiByteToWideChar(
	UINT CodePage,
	DWORD dwFlags,
	LPCSTR lpMultiByteStr,
	int cbMultiByte,
	LPWSTR lpWideCharStr,
	int cchWideChar
)
{
	return MultiByteToWideChar(CodePage, dwFlags, lpMultiByteStr, cbMultiByte, lpWideCharStr, cchWideChar);
}

HLOCAL MyLocalAlloc(UINT uFlags, SIZE_T uBytes)
{
	return LocalAlloc(uFlags, uBytes);
}

LPWSTR* MyCommandLineToArgvW(LPWSTR lpCmdLine, int* pNumArgs)
{
	return CommandLineToArgvW(lpCmdLine, pNumArgs);
}

void* MyLocalLock(HLOCAL hMem)
{
	return LocalLock(hMem);
}

bool MyLocalUnlock(HLOCAL hMem)
{
	return LocalUnlock(hMem);
}
void* MyLocalFree(HLOCAL hMem)
{
	return LocalFree(hMem);
}

void MyExitProcess(UINT uExitCode)
{
	ExitProcess(uExitCode);
}
#else
#define MyLocalFree LocalFree
#define MyMultiByteToWideChar MultiByteToWideChar
#define MyLocalAlloc LocalAlloc
#define MyCommandLineToArgvW CommandLineToArgvW
#define MyLocalAlloc LocalAlloc
#define MyLocalUnlock LocalUnlock
#define MyExitProcess ExitProcess
#endif