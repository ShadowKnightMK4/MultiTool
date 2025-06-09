#include <Windows.h>
/*
* Design is for UNIT_TESTS.           Each tool that is tested will call Begin(), detour as needed and call end when done().
*/
CRITICAL_SECTION ToolMutex; 
BOOL SET = false;

#define OUR_MUTEX_NAME "12340-1898-1523-9135-315-1234567890" 
void BeginToolCritSection()
{
	if (!SET)
	{
		InitializeCriticalSection(&ToolMutex);
		SET = true;
	}
	EnterCriticalSection(&ToolMutex);
}

void EndToolCritSection()
{
	if (SET)
	{
		LeaveCriticalSection(&ToolMutex);
	}
	else
	{
		// we never set it, so no need to leave it
		return;
	}
}