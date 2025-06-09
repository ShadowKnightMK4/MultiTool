#include "CppUnitTest.h"
#include "common.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
#include <stdlib.h>
#include "MutexContainer.h"
#include "DetoursHelp/src/detours.h"
namespace Tests
{
	typedef HMODULE(WINAPI* LoadLibraryA_PTR)(LPCSTR lpLibFileName);
	typedef int (WINAPI* SHEmptyRecycleBinA_PTR)(
		HWND   hwnd,
		LPCWSTR pszRootPath,
		DWORD  dwFlags
		);
	typedef FARPROC(WINAPI* GetProcPtr)(HMODULE hModule, LPCSTR lpProcName);

	LoadLibraryA_PTR NativeLoadLibraryPtr;
	SHEmptyRecycleBinA_PTR SHEmptyRecycleBinAPtr;
	GetProcPtr NativeGetProcAddressPtr;

	FARPROC WINAPI GetProcAddress_mia_shell_empty_bin(HMODULE hModule, LPCSTR lpProcName)
	{
		if (hModule != nullptr && _stricmp(lpProcName, "SHEmptyRecycleBinA") == 0)
		{
			// return null to simulate no SHEmptyRecycleBinA
			return nullptr;
		}
		else
		{
			return NativeGetProcAddressPtr(hModule, lpProcName);
		}
	}
	HMODULE WINAPI NoShell32_LoadLibraryA(const char* loadme)
	{
		if (loadme != 0)
		{
			if (_stricmp(loadme, "shell32.dll") == 0)
			{
				// return null to simulate no shell32.dll
				return nullptr;
			}
			else
			{
				// otherwise call the original LoadLibraryA		
				return NativeLoadLibraryPtr(loadme);
			}
		}
		return NativeLoadLibraryPtr(loadme);
	}



	TEST_CLASS(SHEmptyRecycleBinTests)
	{
	public:
		TEST_METHOD(EmptyRecycleBinTest_missing_shell32)
		{
			HMODULE KERN = LoadLibraryA("kernel32.dll");
			NativeLoadLibraryPtr =  (LoadLibraryA_PTR) GetProcAddress (KERN, "LoadLibraryA");
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());
			DetourAttach(&(PVOID&)NativeLoadLibraryPtr, NoShell32_LoadLibraryA);
			if (DetourTransactionCommit() != 0)
			{
				Assert::Fail(L"Failed to attach the detour for LoadLibraryA. Test not done.");
			}

			// this is a test for the EmptyBin function in recylcinbin.cpp
			BeginToolCritSection();
			int result = 0;
			const char* message_result = nullptr;
			bool res = EmptyBin(&result, &message_result, nullptr, 0);
			Assert::IsFalse(res);
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());
			DetourDetach(&NativeLoadLibraryPtr, NoShell32_LoadLibraryA);
			if (DetourTransactionCommit() != 0)
			{
				Assert::Fail(L"Failed to detach the detour for LoadLibraryA. Test done but others may have issues");
			}
			EndToolCritSection();
		}

		TEST_METHOD(EmptyRecycleBinTest_missing_emptybina)
		{
			HMODULE KERN = LoadLibraryA("kernel32.dll");
			NativeGetProcAddressPtr = (GetProcPtr)GetProcAddress(KERN, "GetProcAddress");
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());
			DetourAttach(&(PVOID&)NativeGetProcAddressPtr, GetProcAddress_mia_shell_empty_bin);
			if (DetourTransactionCommit() != 0)
			{
				Assert::Fail(L"Failed to attach the detour for LoadLibraryA. Test not done.");
			}

			// this is a test for the EmptyBin function in recylcinbin.cpp
			BeginToolCritSection();
			int result = 0;
			const char* message_result = nullptr;
			bool res = EmptyBin(&result, &message_result, nullptr, 0);
			Assert::IsFalse(res);
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());
			DetourDetach(&NativeLoadLibraryPtr, NoShell32_LoadLibraryA);
			if (DetourTransactionCommit() != 0)
			{
				Assert::Fail(L"Failed to detach the detour for LoadLibraryA. Test done but others may have issues");
			}
			EndToolCritSection();
		}
	};
}
