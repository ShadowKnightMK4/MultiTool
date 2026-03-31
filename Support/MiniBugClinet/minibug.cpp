

#include <Windows.h>

/*
* MiniBugClient is a thin wrapper that loads a dll and reports if that worked or not.
* 
* midas.exe uses it to ensure loading a dll doesn't unitinally take down itself.
* 
* for exmaple -whichdll command in midas uses minibug to load the dll and soon as debugger find the event, the process is terminated before loading is done.
* 
* If you run minibugclient.exe *WITHOUT* a debugger, it will write to stdout if the dll wasnlt loaded.
* 
* Why we Load IsDebuggerPresent? In case we're on an ancient exotic windows without that?
*/
HMODULE kernel32 = 0;
typedef BOOL (WINAPI* LocalCheckIsDebugged)();

LocalCheckIsDebugged IsDebugPresent = 0;

bool LoadDebuggerCode()
{
	if (kernel32 == 0)
	{
		kernel32 = LoadLibraryA("kernel32.dll");
		if (kernel32 != 0)
		{
			IsDebugPresent = GetProcAddress(kernel32, "IsDebuggerPresent");
			return true;
		}
	}
	else
	{
		return true;
	}
	return false;
}

bool CallDebugPresent()
{
	if (IsDebugPresent != 0)
	{
		return IsDebugPresent();
	}
	else
	{
		return false;
	}
}

bool myIsSpace(int c)
{
	switch (c)
	{
	case ' ':
	case '\r':
	case '\n':
	case '\t':
		return true;
	}
	return false;
}

void MyMemcpy(char* target, char* source, int count)
{
	while (count > 0)
	{
		*target = *source;
		source++;
		target++;

		count -= 1;
	}
}


char* cmdline = 0;
char* target = 0;
int target_len = 0;
char* finale = 0;

void Write(const char* data)
{
	DWORD written = 0;
	HANDLE StdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	const CHAR* Result = data;
	WriteFile(StdOut, Result, lstrlen(Result), &written, 0);
}

HMODULE TargetLib = 0;
DWORD CheckedLoadArea(char* targetlib)
{
	DWORD ret = 0;
	SetLastError(0);
	TargetLib = LoadLibraryA(targetlib);
	ret = GetLastError();

	return ret;
}


void Start()
{

	LoadDebuggerCode();
	cmdline = GetCommandLineA();
	if (cmdline[0] == '\"')
	{
		bool HitEndQuote = false;
		cmdline++;
		while (true)
		{
			cmdline++;
			if (cmdline[0] == 0)
			{
				break;
			}
			if (cmdline[0] == '\"')
			{
				HitEndQuote = true;
				cmdline++;
				break;
			}
		}
		if (cmdline[0] == 0)
		{
			if (!HitEndQuote)
				ExitProcess(255);
			else
				ExitProcess(254);
		}
		else
		{
			while (myIsSpace(cmdline[0]) == false)
			{
				cmdline++;
			}
			if (cmdline[0] == 0)
			{
				ExitProcess(255);
			}
			else
			{
				while ((cmdline[0] == '\"') || (cmdline[0] == '\''))
				{
					cmdline++;
				}				while (myIsSpace(cmdline[0]) == true)
				{
					cmdline++;
				}
				while ((cmdline[0] == '\"') || (cmdline[0] == '\''))
				{
					cmdline++;
				}
				target = cmdline;
				while (cmdline[0] != 0)
				{
					target_len++;
					cmdline++;
					if ((cmdline[0] == '\"') || (cmdline[0] == '\''))
					{
						break;
					}
				}
			}
		}

	}

	finale = (char*) HeapAlloc(GetProcessHeap(), 0, (target_len + 1) * sizeof(char));
	if (finale != 0)
	{
	}
	else
	{
		ExitProcess(-1);
	}


	MyMemcpy(finale, target, target_len);
	finale[target_len] = 0;



	
	// osver sniff for flags

	// load the dll
	DWORD result = CheckedLoadArea(finale);

	// clean up



	if (!IsDebugPresent())
	{
		// get module file ex.
		//write to stdout
		if (TargetLib != 0)
		{

			Write("SUCESS --> ");
			Write(finale);
			Write(" was loaded ok by loader\r\n");
		}
		else
		{
			Write("FAILED --> ");
			Write(finale);
			Write(" was not found by Windows loader\r\n");
		}
	}
	else
	{
		// debugger should sniff it. we do nothing.
	}
	if (finale != 0)
	{
		HeapFree(GetProcessHeap(), 0, finale);
	}
	Write("Done!");
	if (TargetLib != 0)
	{
		Write("WAS FOUND!");
	}
	else
	{
		Write("WAS NOT!");
	}
	ExitProcess(result);
}