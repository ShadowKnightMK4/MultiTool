#include "common.h"
#include "Support/LWAnsiString/LWAnsiString.h"
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
				yep =WriteFile(Target, message, lstrlenW((const wchar_t*)message) *sizeof(wchar_t), &written, NULL);
			}
			else
			{
				yep =WriteFile(Target, message, lstrlenA((const char*)message) * sizeof(char), &written, NULL);
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


/// <summary>
/// this flavor of WriteStdout sees if unicode or ansi in there then branches to approporate function/ IMPORTANT THIS DOES NOT HANDLE Non Unicode or Ansi char sets. Needs to be the ones built into the lwansi library
/// </summary>
/// <param name="str"></param>
void WriteStdout(LWAnsiString* str)
{
	if (STDOUT == 0)
	{
		SETUP_PIPES();
	}
	if (LWAnsiString_IsAnsi(str))
	{
		WriteToStream(STDOUT, LWAnsiString_ToCStr(str));
	}
	else
	{
		if (LWAnsiString_IsUnicode(str))
		{
			WriteToStream(STDOUT, (const wchar_t*) LWAnsiString_ToCStr(str));
		}
		else
		{
			// TO DO some way to tell caller that they need to do custom output.
		}
	}

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