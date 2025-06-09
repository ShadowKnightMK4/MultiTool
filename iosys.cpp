#include "common.h"

/// <summary>
/// write ansi or unicode to target
/// </summary>
/// <param name="Target">handle to write</param>
/// <param name="message">if IsUnicode, this is const wchar_t*, otherwise const char*</param>
/// <param name="IsUnicode">contrils if the message arg is treated as unicode</param>
static bool WriteStreamAsString(HANDLE Target, const void* message, bool IsUnicode)
{
	bool yep = false;
	if (Target != nullptr)
	{
		DWORD written=0;

		if (message != nullptr)
		{
			if (IsUnicode)
			{
				yep =WriteFile(Target, message, lstrlenW((const wchar_t*)message), &written, NULL);
			}
			else
			{
				yep =WriteFile(Target, message, lstrlenA((const char*)message), &written, NULL);
			}
		}
	}
	return yep;
}

static void WriteToStream(HANDLE Stream, const wchar_t* message)
{
	if (Stream != 0)
	{
		WriteStreamAsString(Stream, message, true);
	}
}
static void WriteToStream(HANDLE Stream, const char* message)
{
	if (Stream != 0)
	{
		WriteStreamAsString(Stream, message, false);
	}
}

void WriteStdout(const char* message)
{
	if (STDOUT == 0)
	{
		SETUP_PIPES();
	}
		WriteToStream(STDOUT, message);
	
}
void WriteStdout(const wchar_t* message)
{
	if (STDOUT == 0)
	{
		SETUP_PIPES();
	}
		WriteToStream(STDOUT, message);
	
}



void WriteStderr(const char* message)
{
	if (STDERR != 0)
	{
		SETUP_PIPES();
		WriteToStream(STDERR, message);
	}
}
void WriteStderr(const wchar_t* message)
{
	if (STDERR != 0)
	{
		SETUP_PIPES();
		WriteToStream(STDERR, message);
	}
}