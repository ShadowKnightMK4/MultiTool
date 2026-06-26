

#include <Windows.h>
#include <intrin.h>
/*
* MiniBugClient is a thin wrapper that loads a dll and reports if that worked or not.
* 
* midas.exe uses it to ensure loading a dll doesn't unitinally take down itself.
* 
* for exmaple -whichdll command in midas uses minibug to load the dll and soon as debugger find the event, the process is terminated before loading is done.
* 
* If you run minibugclient.exe *WITHOUT* a debugger, it will write to stdout if the dll wasnlt loaded.
* 
* Why we Load IsDebuggerPresent and NOT hardcoded In case we're on an ancient exotic windows without that?
* 
* Important though with using htis.
* 
* We assume "path to runner"  Target DLL.
* aka we look for q
*/
HMODULE kernel32 = 0;
typedef BOOL (WINAPI* LocalCheckIsDebugged)();

LocalCheckIsDebugged IsDebugPresent = 0;

/* fixed parts we reassume at run time*/
const char* KernelLibrary = "kernel32.dll";


/* helper - how we write our code to the output*/

// NOT THREAD SAFE- this isn't a multi threaded app
DWORD BytesWrote = 0;
void WriteCommon(const VOID* Output, DWORD Bytes)
{
// note MSDN doc says not required to close a GetStdHandle
	WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), Output, Bytes, &BytesWrote, 0);
}
void WriteA(const char* data)
{
	WriteCommon((const VOID*)data, lstrlenA(data));
	/*
	DWORD written = 0;
	HANDLE StdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	const CHAR* Result = data;
	WriteFile(StdOut, Result, lstrlenA(Result), &written, 0);*/
}

bool LoadDebuggerCode()
{
	if (kernel32 == 0)
	{
		kernel32 = LoadLibraryA(KernelLibrary);
		if (kernel32 != 0)
		{
			IsDebugPresent =  (LocalCheckIsDebugged) GetProcAddress(kernel32, "IsDebuggerPresent");
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



HMODULE TargetLib = 0;
DWORD CheckedLoadArea(char* targetlib)
{
	DWORD ret = 0;
	SetLastError(0);
	TargetLib = LoadLibraryA(targetlib);
	ret = GetLastError();

	return ret;
}

#ifdef ASM_CLOBBER
void AsmClobber()
{
	/* to do, write assembly that letms me adjust the import
	table to zero 0 routines that need to not be used
	for example: load internet.dll and making the intialiator
	return falure
	
	WHY?
	****************
	Consider the winsock, and internet other dlls.
	When a dll is loaded? The loader maps it to memory and sets reference count to 1
	wHEN A dll is already loaded and requested to load again, it ticks up reference count, 
	the loader does *not* reload from disk


	Follow me here:
	If we load the internet access dlls already and patch the code to just return failure,
	future requests will fail unless the code is patched back.

	End result idelaly?
	Pointing minibugclient to load malware_that_phones_home.dll.  
	minibug loads
	minbug  preimptly loads the intenerlet libraries and then
	basically rewrites the imports to return failure.

	malware_that_phones_home.dll finds the connection severed.

	
	*/
}
#else
#define AsmClobber(x) 
#endif

void StartMiniBug(LPCSTR Input)
{
	AsmClobber();
	LoadDebuggerCode();
	if (Input != 0)
	{
		cmdline = (LPSTR) Input;
	}
	else
	{
		cmdline = GetCommandLineA();
	}

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
				}
				while (myIsSpace(cmdline[0]) == true)
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


	BOOL skip_load_pov = false;

	if (!CallDebugPresent())
	{
		switch (sizeof(void*))
		{
		case 4: WriteA("x86"); break;
		case 8: WriteA("x64"); break;
		default: WriteA("Unknown (or unknown ptr size)"); break;
		}
		WriteA(" Windows loader location\r\n");

		// get module file ex.
		//write to stdout
		if (TargetLib != 0)
		{

			WriteA("SUCCESS --> ");
			WriteA(finale);
			WriteA(" was loaded ok by loader\r\n");
		}
		else
		{
			WriteA("FAILED --> ");
			WriteA(finale);
			WriteA(" was not found by Windows loader\r\n");
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
	WriteA("Done!");
	if (TargetLib != 0)
	{
		WriteA("WAS FOUND!");
	}
	else
	{
		WriteA("WAS NOT!");
	}
	ExitProcess(result);
}



/// <summary>
/// This is the entry point if compiled solo
/// </summary>
void Start()
{
	StartMiniBug(NULL);
}