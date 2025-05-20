#include "common.h"

typedef int (WINAPI* SHEmptyRecycleBinW_PTR)(
	HWND   hwnd,
	LPCWSTR pszRootPath,
	DWORD  dwFlags
	);
extern "C" {
	bool EmptyBin(int* result, const char** message_result, const char* argv[], int argc)
	{
		SHEmptyRecycleBinW_PTR WideCode = nullptr;
		// load shell32 and handle failure stuff
		HMODULE hModule = LoadLibraryA("shell32.dll");
		if (hModule == NULL)
		{
			if (message_result != nullptr)
			{

				*message_result = Shell32_DLL_FAIL_LOAD;
			}
			if (result != nullptr)
			{
				*result = -1;
			}
			return false;
		}

		// get ShEmptyRecycleBinW handle failure
		WideCode = (SHEmptyRecycleBinW_PTR)GetProcAddress(hModule, "SHEmptyRecycleBinA");
		if (WideCode == nullptr)
		{
			if (message_result != nullptr)
			{
				*message_result = GetProc_FAIL_ON_ShEmptyBinW;
			}
			if (result != nullptr)
			{
				*result = -2;
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
				*message_result = RecycleBin_Empty_Success;
			}
			return true;
		}
		else
		{
			if (message_result != nullptr)
			{
				*message_result = RecycleBin_Empty_Failure;
			}
			return false;
		}
	}
}