#include <Windows.h>

extern int main(int argc, const char* argv[]);
extern void SETUP_PIPES();
#define DEBUG


/// <summary>
/// This is a placeholder to have the system do it itself Ie the unicode version of GetCommandLineW -> CommandLineAsArgv; It's currently implewmented via goto unicode and back
/// </summary>
/// <param name="lpCmdLine">from GetCommandLineA()</param>
/// <param name="pArgc">pointer to place argc for main</param>
/// <returns>array of char* each allocated with LocalAlloc and the array itself was.</returns>
/// <remarks>This routine is NOT intended to be called outside of the SystemStart routine below. Why this way. Most main() is not gonna care how it gets the parsed stuff.  </remarks>
LPCSTR* CommandLineAsArgv(LPSTR lpCmdLine, int* pArgc)
{
	int size_needed = MultiByteToWideChar(CP_ACP, 0, lpCmdLine, -1, nullptr, 0);

	HGLOBAL UnicodeString = LocalAlloc(LMEM_ZEROINIT, size_needed * sizeof(char) * 2);
	if (UnicodeString == nullptr)
	{
		return nullptr;
	}
	else
	{
		/*
		* What we're doing here essential is using the Unicode Stuff to get to ANSI and present that to our main.
		* 
		* Eventually this will need to be gutted to be replaced with something that will *not* depend on unicode.
		* Win95 if we're gonna try supported it has no unicode support. We need to have a cli parser that doesn't depend on unicode.
		* and using the Unicode Version while debugging stuff and coding it is fine.
		* 
		* This part of the app essentially is planned to be the only unicode part until we drop it. The rest is ANSI and lets the os
		* determine what to code
		*/
		
		// covert our Command Line to a Unicode string for the shell32 call
		void* pUnicodeString = LocalLock(UnicodeString);
		int result = MultiByteToWideChar(CP_ACP, 0, lpCmdLine, -1, (LPWSTR)pUnicodeString, size_needed);

		auto res = CommandLineToArgvW((LPWSTR)pUnicodeString, pArgc);
		// clean up:: Note why no SEH?  We can't assume the os supports it. 
		LocalUnlock(UnicodeString);
		LocalFree(UnicodeString);
		pUnicodeString = nullptr;

		/*
		* What we're lazy doing is making an array with LocalAlloc of LPCSTR data.
		* 
		* Then allocating a LPCSTR for each entry we find in the unicode one, copying it as ansi to the target and moving on with our life.
		*/
		LPCSTR* Args = (LPCSTR*)LocalAlloc(LMEM_ZEROINIT, sizeof(LPCSTR) * (*pArgc));
		for (int i = 0; i < *pArgc; i++)
		{
			size_needed = WideCharToMultiByte(CP_ACP, 0, res[i], -1, nullptr, 0, nullptr, nullptr);
			HLOCAL ref = LocalAlloc(LMEM_ZEROINIT, size_needed);
			void* target=0;
			if (ref != 0)
			{
				target = LocalLock(ref);
				if (target == 0)
				{
					return nullptr;
				}

				WideCharToMultiByte(CP_ACP, 0, res[i], -1, (LPSTR)target, size_needed, nullptr, nullptr);
				Args[i] = (LPCSTR)target;
			}
			else
			{
				return nullptr;
			}

		}

		return Args;

	}

}

int SystemStart()
{
	int argc = 0;
	auto CmdLine = GetCommandLineA();

	LPCSTR* Args = CommandLineAsArgv(CmdLine, &argc);
	int ret = main(argc, Args);
	for (int i = 0; i < argc; i++)
	{
		LocalFree((void*)Args[i]);
	}
	LocalFree(Args);
	ExitProcess(ret);
}