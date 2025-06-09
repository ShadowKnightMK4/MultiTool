#include "common.h"

typedef int (WINAPI* SHEmptyRecycleBinA_PTR)(
	HWND   hwnd,
	LPCWSTR pszRootPath,
	DWORD  dwFlags
	);

extern "C" {
#define SHELL32_FAIL -1
#define SHELL32_NO_EMPTYBINA -2
	/// <summary>
	/// Empty the recycle bin using SHEmptyRecycleBinA.  If SILENCE is true, it will not show any UI
	/// </summary>
	/// <param name="result">if not null, will have a negative value on error and 0 on ok.</param>
	/// <param name="message_result">If not null, will be pointed to a read only ANSI string stating what happened.</param>
	/// <param name="argv">part of the tool protocol. not actually used currently with this. yes pass the argv from main()</param>
	/// <param name="argc">part of the tool protocol.  Yes pass argc from main()</param>
	/// <returns>True if it worked and false if something went bad.  Message is set to something suitable for console/stdout. Result too.</returns>
	/// <remarks> if the return value is not -1, -2, lookup the return values for SHEmptyRecycleBinA.</remarks>
	bool EmptyBin(int* result, const char** message_result, const char* argv[], int argc)
	{
		SHEmptyRecycleBinA_PTR WideCode = nullptr;
		// load shell32 and handle failure stuff
		HMODULE hModule = LoadLibraryA("shell32.dll");
		if (message_result != nullptr)
		{
			*message_result = nullptr; // reset the message result to null
		}
		if (result != nullptr)
		{
			*result = 0; 
		}
		if (hModule == NULL)
		{
			if (message_result != nullptr)
			{
				*message_result = Message_CantLoadShell32;
			}
			if (result != nullptr)
			{
				*result = SHELL32_FAIL;
			}
			return false;
		}

		// get ShEmptyRecycleBinW handle failure
		WideCode = (SHEmptyRecycleBinA_PTR)GetProcAddress(hModule, "SHEmptyRecycleBinA");
		if (WideCode == nullptr)
		{
			if (message_result != nullptr)
			{
				*message_result = GetProc_FAIL_ON_ShEmptyBinW;
			}
			if (result != nullptr)
			{
				*result = SHELL32_NO_EMPTYBINA;
			}
			// NOTE WE DON'T free the library here as we do assume the workflow of run a tool and exit vs persist	
			return false;
		}
		int r = 0;
		// call the function without noise
		if (SILENCE)
		{
			r = WideCode(0, 0, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI);
		}
		else
		{
			r = WideCode(0, 0, 0);
		}
		// assisn the result
		if (result != nullptr)
		{
			*result = r;
		}

		/// set the yay message or boo message
		if (r == S_OK)
		{

			if (message_result != nullptr)
			{
				*message_result = Message_RecycleBin_Empty_Success;
			}
			return true;
		}
		else
		{
			if (message_result != nullptr)
			{
				*message_result = Message_RecycleBin_Empty_Failure;
			}
			return false;
		}
	}
}