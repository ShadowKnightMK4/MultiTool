#include <pch.h>
#include "CppUnitTest.h"
#include "LWAnsiString.h"
#include "LWAnsiStringGuard.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
#define stricmp _stricmp
#define stricmp _stricmp

namespace imports__ {
	// from lwansis
	size_t WINAPI DefaultWriteIndex(void* Target, size_t index, size_t val)
	{
		char* t = (char*)Target;
		((unsigned char*)Target)[index] = (unsigned char)val;
		return 0;
	}
	// from lwansis
	size_t WINAPI DefaultIndexer(void* TARGET, size_t index)
	{
		char* t = (char*)TARGET;
		return (char)t[index];
	}

	// from lwansi
	HANDLE _stdcall DefaultMyHeapGet(DWORD Options, SIZE_T Start, SIZE_T Cap)
	{
		return GetProcessHeap(); // fallback to the default heap
	}

}
namespace LWReserveTest
{
	
	TEST_CLASS(LWReserve_Tests)
	{
	public:
		TEST_METHOD(ReserveTest_NullString_ReturnsNull)
		{
			Assert::IsNull(LWAnsiString_Reserve(nullptr, 255));
		}

		TEST_METHOD(ReserveTest_RiskBufferOverflow_BlocksIt_ReturnsNull_ANSI)
		{
			LWAnsiString* testme = LWAnsiString_CreateStringA(20);
			Assert::IsNotNull(testme);
			Assert::IsNull(LWAnsiString_Reserve(testme, MAXSIZE_T));

			LWAnsiString_FreeString(testme);
		}

		TEST_METHOD(ReserveTest_RiskBufferOverflow_BlocksIt_ReturnsNull_ANSI_HalsSize)
		{
			LWAnsiString* testme = LWAnsiString_CreateStringA(20);
			Assert::IsNotNull(testme);
			Assert::IsNull(LWAnsiString_Reserve(testme,((MAXSIZE_T)/2)+1));

			LWAnsiString_FreeString(testme);
		}

		TEST_METHOD(ReserveTest_RiskBufferOverflow_BlocksIt_ReturnsNOTNull_ANSI_HalsSizeNeg1)
		{
			size_t request = ((MAXSIZE_T) / 2);
			request -= 1;
			LWAnsiString* testme = LWAnsiString_CreateStringA(20);
			Assert::IsNotNull(testme);
			Assert::IsNotNull(LWAnsiString_Reserve(testme, request));
			Assert::IsTrue(testme->AllocatedSize == request);
			LWAnsiString_FreeString(testme);
		}
	};
}


namespace LWAppendFlavor_Native
{
	
	TEST_CLASS(AppendNative)
	{
	public:
		TEST_METHOD(AppendNative_DoubleNull_ReturnsNull)
		{
			Assert::IsNull(LWAnsiString_AppendNative(nullptr, nullptr));
		}

		TEST_METHOD(AppendNative_AllocateHandlerNull_ReturnsNull)
		{
			LWAnsiString Dummy;
			Dummy.AllocatedHandle = 0;
			Assert::IsNull(LWAnsiString_AppendNative(&Dummy, nullptr));
		}

		TEST_METHOD(AppendNative_NullAppend_NonNullStr_ReturnsStr)
		{
			LWAnsiString* test = LWAnsiString_CreateString(20);
			Assert::IsNotNull(test);
			auto ret = LWAnsiString_AppendNative(test, nullptr);

			Assert::AreSame((void*)test, (void*)ret);
			LWAnsiString_FreeString(test);
		}

	};
}
namespace LWAppendFlavor_Tests
{
	TEST_CLASS(NewLineTests)
	{
	public:
		TEST_METHOD(AppendNewLineW_Call_Verify)
		{
			LWAnsiString* testme = LWAnsiString_CreateStringW(20);
			Assert::IsNotNull(testme);
			LWAnsiString_AppendNewLineW(testme);

			Assert::IsTrue(testme->W[0] == L'\r');
			Assert::IsTrue(testme->W[0] == L'\n');

			LWAnsiString_FreeString(testme);
		}

		TEST_METHOD(AppendNewLineA_Call_Verify)
		{
			LWAnsiString* testme = LWAnsiString_CreateStringA(20);
			Assert::IsNotNull(testme);
			LWAnsiString_AppendNewLineA(testme);

			Assert::IsTrue(testme->A[0] == '\r');
			Assert::IsTrue(testme->A[0] == '\n');

			LWAnsiString_FreeString(testme);
		}

		TEST_METHOD(AppendNewLineGeneric_Call_Verify)
		{
			LWAnsiString* testme = LWAnsiString_CreateString(20);
			Assert::IsNotNull(testme);
			LWAnsiString_AppendNewLine(testme);

			if (LWAnsiString_IsAnsi(testme))
			{
				Assert::IsTrue(testme->A[0] == '\r');
				Assert::IsTrue(testme->A[0] == '\n');
			}
			else
			{
				if (LWAnsiString_IsUnicode(testme))
				{
					Assert::IsTrue(testme->W[0] == L'\r');
					Assert::IsTrue(testme->W[0] == L'\n');
				}
				else
				{
					Assert::Fail(L"Attempt to append new line to something that's non ansi or unicode handler. A lot of the code assumes it's just these 2 flavors. If custom runner, you'll need your own append function => LWAnsiString_AppendInternal");
				}
			}


			LWAnsiString_FreeString(testme);
		}
	}
	;
}

namespace LWAdjustSize_Tests
{
	TEST_CLASS(Tests)
	{
	public:
		TEST_METHOD(AdjustSize_NullArg_BegetsNegOne)
		{
			Assert::IsTrue(LWAnsiString_AdjustSize(nullptr, 255) == -1);
		}

		TEST_METHOD(AdjustSize_NegSize_BegetsNegOne)
		{
			Assert::IsTrue(LWAnsiString_AdjustSize(nullptr, -255) == -1);
		}
		TEST_METHOD(AdjustSize_ZeroSizeAdjust_BegetsNegOne)
		{
			Assert::IsTrue(LWAnsiString_AdjustSize(nullptr, 0) == -1);
		}


		TEST_METHOD(AdjustSize_EmpyHandler_ReturnsNegOne)
		{
			LWAnsiString dummy;
			dummy.AllocatedHandle = 0;
			Assert::IsTrue(LWAnsiString_AdjustSize(&dummy, 0) == -1);
		}

		TEST_METHOD(AdjustSize_ModestIncrease_ReturnsNewSize_NonZero_ANSI)
		{
			size_t start_size = 255;
			size_t requested_size = 300;
			size_t expected_size = 300;
			LWAnsiString* testme = LWAnsiString_CreateStringA(start_size);
			Assert::IsNotNull(testme);
			int result = LWAnsiString_AdjustSize(testme, requested_size);

			Assert::IsTrue(result == requested_size);
			Assert::IsTrue(testme->AllocatedSize == expected_size);

			LWAnsiString_FreeString(testme);
		}


		TEST_METHOD(AdjustSize_ModestDecrease_SizeDoesntChange)
		{
			size_t start_size = 255;
			size_t requested_size = 100;
			size_t expected_size = start_size;
			LWAnsiString* testme = LWAnsiString_CreateStringA(start_size);
			Assert::IsNotNull(testme);
			int result = LWAnsiString_AdjustSize(testme, requested_size);

			Assert::IsTrue(result == requested_size);
			Assert::IsTrue(testme->AllocatedSize == expected_size);

			LWAnsiString_FreeString(testme);
		}
	};
}


namespace LWStateBasedActions
{
	/// <summary>
	/// a compromise.  hosted in LWAnsiString_Internal.cpp. most should just call ProbeIfDirtyLen()
	/// </summary>
	/// <param name="str"></param>
	/// <returns></returns>
	extern "C" bool LWAnsiString_IsDirtyLen(LWAnsiString* str);

	TEST_CLASS(Tests)
	{
	public:
		TEST_METHOD(MarkDirty_Actually_UpdatesFlags_EmptyString)
		{
			LWAnsiString* testme = LWAnsiString_CreateString(0);
				LWAnsiString_MarkLenDirty(testme);


				Assert::IsTrue(LWAnsiString_IsDirtyLen(testme));

				LWAnsiString_ProbeLength(testme);
				Assert::IsFalse(LWAnsiString_IsDirtyLen(testme));

			LWAnsiString_FreeString(testme);
		}

		TEST_METHOD(LWAnsiString_ProbeLength_ClearsDirtyFlag)
		{
			LWAnsiString* testme = LWAnsiString_CreateString(0);
			LWAnsiString_MarkLenDirty(testme);


			Assert::IsTrue(LWAnsiString_IsDirtyLen(testme));

			LWAnsiString_ProbeLength(testme);
			Assert::IsFalse(LWAnsiString_IsDirtyLen(testme));

			LWAnsiString_FreeString(testme);
		}

		TEST_METHOD(ClampNull_DontCrash_IfNullArg)
		{
			LWAnsiString_ClampNull(0);
		}

		TEST_METHOD(ClampNull_DontCrash_IfNullString)
		{
			LWAnsiString dummy;
			dummy.A = nullptr;
			LWAnsiString_ClampNull(0);
		}
		TEST_METHOD(ClampNull_SetsFirstElement_AsZero)
		{
			const char* base_string = "Hello World!";
			LWAnsiString* testme = LWAnsiString_CreateFromString(base_string);
			Assert::IsNotNull(testme);
			Assert::IsNotNull(testme->A);
			Assert::AreNotEqual(testme->Length, (size_t)0);

			LWAnsiString_ClampNull(testme);
			Assert::IsNotNull(testme);
			Assert::IsNotNull(testme->A);


			// and we zeroed the last allocated byte
			Assert::AreEqual(0, (int) testme->A[testme->Length]);

			// and we didn't zero the 1st few bytes
			Assert::AreNotEqual(0, (int)testme->A[1]);
			Assert::AreNotEqual(0, (int)testme->A[0]);
			LWAnsiString_FreeString(testme);
		}



		TEST_METHOD(LWAnsiString_Length_IsZero_OnNullString)
		{
			Assert::AreEqual(0, LWAnsiString_Length(nullptr));
		}

		TEST_METHOD(LWAnsiString_LengthAndProbeLength_Agree)
		{
			const char* base_string = "Hello World!";
			LWAnsiString* testme = LWAnsiString_CreateFromString(base_string);
			Assert::IsNotNull(testme);



			size_t expect_len = strlen(base_string);
			size_t struct_len = LWAnsiString_Length(testme);
			LWAnsiString_MarkLenDirty(testme);

			Assert::IsTrue(LWAnsiString_IsDirtyLen(testme));

			size_t probe_len = LWAnsiString_ProbeLength(testme);

			Assert::IsFalse(LWAnsiString_IsDirtyLen(testme));

			Assert::AreEqual(expect_len, struct_len);
			Assert::AreEqual(struct_len, probe_len);
			Assert::AreEqual(expect_len, probe_len);

			LWAnsiString_FreeString(testme);
		}





		TEST_METHOD(ZeroString_DropsNullArg_JustDontCrash)
		{
			LWAnsiString_ZeroString(nullptr);
		}

		TEST_METHOD(ZeroString_DropsNullString_JustDontCrash)
		{
			LWAnsiString Dummy;
			Dummy.A = nullptr;
			LWAnsiString_ZeroString(&Dummy);
		}


		TEST_METHOD(ZeroString_DropsZero_AllocatedSize_JustDontCrash)
		{
			LWAnsiString Dummy;
			Dummy.A = nullptr;
			Dummy.AllocatedSize = 0;
			LWAnsiString_ZeroString(&Dummy);
		}


		TEST_METHOD(ZeroString_GenericTest_ANSI)
		{
			const char *base_str = "Hello World!";
			size_t blen = strlen(base_str);
			LWAnsiString* testme = LWAnsiString_CreateFromString(base_str);
			// check base state
			Assert::IsNotNull(testme);
			Assert::IsNotNull(testme->A);
			Assert::AreEqual(strlen(testme->A), blen);
			Assert::IsTrue(strcmp(testme->A, base_str) == 0);

			// call 
			LWAnsiString_ZeroString(testme);

			// now we test
			Assert::IsNotNull(testme);
			Assert::IsNotNull(testme->A);
			Assert::AreNotEqual(strlen(testme->A), blen);
			Assert::AreNotEqual(testme->AllocatedSize, (size_t) 0);
			Assert::AreEqual(testme->Length, (size_t) 0);

			for (size_t i = 0; i < testme->AllocatedSize; i++)
			{
				if (testme->A[i] != 0)
				{
					Assert::Fail(L"LWAnsiString_ZeroString didn't zero all bytes");
				}
			}

			

			LWAnsiString_FreeString(testme);
		}


		TEST_METHOD(ZeroString_GenericTest_UNICODE)
		{
			const wchar_t* base_str = L"Hello Unicode! you ready to rock!";
			size_t blen = wcslen(base_str);
			LWAnsiString* testme = LWAnsiString_CreateFromStringW(base_str);
			// check base state
			Assert::IsNotNull(testme);
			Assert::IsNotNull(testme->A);
			Assert::AreEqual(wcslen(testme->W), blen);
			Assert::IsTrue(wcscmp(testme->W, base_str) == 0);

			// call 
			LWAnsiString_ZeroString(testme);

			// now we test
			Assert::IsNotNull(testme);
			Assert::IsNotNull(testme->A);
			Assert::AreNotEqual(wcslen(testme->W), blen);
			Assert::AreNotEqual(testme->AllocatedSize, (size_t)0);
			Assert::AreEqual(testme->Length, (size_t)0);

			for (size_t i = 0; i < testme->AllocatedSize; i++)
			{
				if (testme->W[i] != 0)
				{
					Assert::Fail(L"LWAnsiString_ZeroString didn't zero all bytes");
				}
			}



			LWAnsiString_FreeString(testme);
		}
	};
}



namespace LWCreateStringTests {

	TEST_CLASS(Tests)
	{
	public:

		TEST_METHOD(DefaultCreateStringEmpty_ValidateUnicodeOrAnsi_Ok)
		{
			LWAnsiString* testme = LWAnsiString_CreateString(0);

			Assert::IsNotNull(testme);
			Assert::IsNotNull(testme->A);
#ifdef _UNICODE
			Assert::AreEqual((const void*)testme->AllocatedHandle, (const void*)LWUnicodeHandler);
			Assert::AreEqual(testme->AllocatedSize, (size_t)1);
#else
			Assert::AreEqual((const void*)testme->AllocatedHandle, (const void*)LWAnsiHandler);
			Assert::AreEqual(testme->AllocatedSize, (size_t)1);
#endif
			Assert::AreEqual(testme->Length, (size_t)0);

			LWAnsiString_FreeString(testme);

		}

		TEST_METHOD(CreateUnicodeStringEmpty_ValidateOk)
		{
			LWAnsiString* testme = LWAnsiString_CreateStringW(0);

			Assert::IsNotNull(testme);

			Assert::AreEqual((const void*)testme->AllocatedHandle, (const void*)LWUnicodeHandler);
			Assert::AreEqual(testme->AllocatedSize, (size_t) 1);
			Assert::AreEqual(testme->Length, (size_t)0);


			LWAnsiString_FreeString(testme);

		}

		TEST_METHOD(CreateAnsiStringEmpty_ValidateOk)
		{
			LWAnsiString* testme = LWAnsiString_CreateStringA(0);
			Assert::IsNotNull(testme);
			Assert::AreEqual((const void*)testme->AllocatedHandle, (const void*)LWAnsiHandler);
			Assert::AreEqual(testme->AllocatedSize, sizeof(char));
			Assert::AreEqual(testme->Length, (size_t)0);
			LWAnsiString_FreeString(testme);
		}

		TEST_METHOD(CreateStringEX_NullHandler_DefaultsToAnsi)
		{
			LWAnsiString* testme = LWAnsiString_CreateStringEx(nullptr, 0);
			Assert::AreEqual((const void*)testme->AllocatedHandle, (const void*)LWAnsiHandler);
			Assert::AreEqual(testme->AllocatedSize, sizeof(char));
			Assert::AreEqual(testme->Length, (size_t)0);
			LWAnsiString_FreeString(testme);
		}

		TEST_METHOD(CreateStringEX_NullHandler_NegativeLength_ReturnsNull)
		{
			LWAnsiString* testme = LWAnsiString_CreateStringEx(nullptr, -1);
			Assert::IsNull(testme);
		}

		TEST_METHOD(CreateStringEX_DefaultHandler_NegativeLength_ReturnsNull)
		{
			LWAnsiString* testme = LWAnsiString_CreateStringEx(LWAnsiHandler, -1);
			Assert::IsNull(testme);
		}

		TEST_METHOD(CreateStringEX_UnicodeHandler_NegativeLength_ReturnsNull)
		{
			LWAnsiString* testme = LWAnsiString_CreateStringEx(LWUnicodeHandler, -1);
			Assert::IsNull(testme);
		}


		TEST_METHOD(CreateFromStringEx_BlankString_ReturnsNull)
		{
			LWAnsiString* testme = LWAnsiString_CreateFromStringEx(&DefaultHandler, nullptr);
			Assert::IsNull(testme);
		}

		TEST_METHOD(CreateFromStringA_BlankString_ReturnsNull)
		{
			LWAnsiString* testme = LWAnsiString_CreateFromStringA(nullptr);
			Assert::IsNull(testme);
		}

		TEST_METHOD(CreateFromStringW_BlankString_ReturnsNull)
		{
			LWAnsiString* testme = LWAnsiString_CreateFromStringW(nullptr);
			Assert::IsNull(testme);
		}
		

		

		TEST_METHOD(CreateFromStringA_UsesAnsi_SourceString)
		{
			char* source_string = "Hello World!";
			LWAnsiString* testme = LWAnsiString_CreateFromStringA(source_string);

			// check if allowed and we used current handler
			Assert::IsNotNull(testme);
			Assert::AreEqual((const void*)testme->AllocatedHandle, (const void*)LWAnsiHandler);

			// and we didn't blindly just set pointer to const
			Assert::AreNotSame(source_string, testme->A);

			// and length is same
			Assert::AreEqual(testme->Length, strlen(source_string));

			// and allocated size includes null (defualt lenght+1)
			Assert::AreEqual(testme->AllocatedSize, strlen(source_string) + 1);

			// and the actual byte size is right
			Assert::AreEqual(testme->AllocatedSize * sizeof(char), LWAnsiString_GetAllocatedByteSize(testme));



			Assert::AreEqual(testme->AllocatedSize, strlen(source_string)+1);

			auto ptr = LWAnsiString_ToCStr(testme);
			Assert::IsNotNull(ptr);

			if (strcmp(source_string, ptr) != 0)
			{
				Assert::Fail(L"LWAnsiString_CreateFromStringA and by ext LWAnsiString_CreateFromStringEx failed to copy ok the source string to the target as perfect duplicate");
			}

		}
		


		TEST_METHOD(CreateFromStringW_UsesUnicode_SourceString)
		{
			wchar_t* source_string = L"Hello Unicode!";
			LWAnsiString* testme = LWAnsiString_CreateFromStringW(source_string);


			// check if allowed and we used current handler
			Assert::IsNotNull(testme);
			Assert::AreEqual((const void*)testme->AllocatedHandle, (const void*)LWUnicodeHandler);

			// and we didn't blindly just set pointer to const
			Assert::AreNotSame(source_string, testme->W);

			// and lengths are same
			Assert::AreEqual(testme->Length, wcslen(source_string));
			// as well as byte size
			Assert::AreEqual(testme->AllocatedSize, (wcslen(source_string) + 1));

			// and the allocated byte size owrks
			Assert::AreEqual(testme->AllocatedSize * sizeof(wchar_t), LWAnsiString_GetAllocatedByteSize(testme));


			


			// importanted, ALlocated
			Assert::AreEqual(testme->Length, wcslen(source_string) );

			auto ptr = (const wchar_t*) LWAnsiString_ToCStr(testme);
			Assert::IsNotNull(ptr);

			if (wcscmp(source_string, ptr) != 0)
			{
				Assert::Fail(L"LWAnsiString_CreateFromStringW and by ext LWAnsiString_CreateFromStringEx failed to copy ok the source string to the target as perfect duplicate");
			}

		}
		TEST_METHOD(CreateStringEX_CustomHandler_CopiesCustomHandler)
		{
			AllocationHandler Dummy;
			Dummy.CustomFirstAlloc = HeapAlloc;
			Dummy.CustomGetHeap = imports__::DefaultMyHeapGet;
			Dummy.CustomFree = HeapFree;
			Dummy.CustomReAlloc = HeapReAlloc;
			//DefaultHandler.STRCAT = (LW_STRING_strcat)lstrcatA;
			Dummy.STRCAT = (LW_STRING_strcat)lstrcatA;
			//DefaultHandler.STRCPY = (LW_STRING_strcpy)lstrcpyA;
			Dummy.STRCPY = (LW_STRING_strcpy)lstrcpyA;
			//DefaultHandler.STRLEN = (LW_STRING_strlen)lstrlenA;
			Dummy.STRLEN = (LW_STRING_strlen)lstrlenA;
			Dummy.SingleCharacterLength = sizeof(char);
			Dummy.INDEX = imports__::DefaultIndexer;
			Dummy.STRCMP = lstrcmpA;
			Dummy.STRICMP = lstrcmpiA;
			Dummy.WriteToIndex = imports__::DefaultWriteIndex;



			LWAnsiString* testme = LWAnsiString_CreateStringEx(&Dummy, 0);
			Assert::AreNotSame((const void*)testme->AllocatedHandle, (const void*)LWAnsiHandler);
			Assert::AreEqual(testme->AllocatedSize, sizeof(char));
			Assert::AreEqual(testme->Length, (size_t)0);
			LWAnsiString_FreeString(testme);
		}


		TEST_METHOD(CreateFromOffsetEx_NullArg_ReturnsNull)
		{
			LWAnsiString* testme = LWAnsiString_CreateFromOffsetEx(&DefaultHandler, nullptr, 0);
			Assert::IsNull(testme);
		}

		TEST_METHOD(CreateFromOffsetEx_NegLength_ReturnsNull)
		{
			LWAnsiString* testme = LWAnsiString_CreateFromOffsetEx(&DefaultHandler, nullptr, -1);
			Assert::IsNull(testme);
		}

		TEST_METHOD(CreateFromOffsetEx_AnsiTest_OffsetPastLength_ReturnsNull)
		{
			LWAnsiString* Original = LWAnsiString_CreateFromStringA("Hello World!");
			LWAnsiString* testme = LWAnsiString_CreateFromOffsetEx(&DefaultHandler, Original, 255);

			Assert::IsNotNull(Original);
			Assert::IsNull(testme);


			LWAnsiString_FreeString(Original);
		}

		TEST_METHOD(CreateFromOffsetEx_UnicodeTest_OffsetPastLength_ReturnsNull)
		{
			LWAnsiString* Original = LWAnsiString_CreateFromStringW(L"Hello World!");
			LWAnsiString* testme = LWAnsiString_CreateFromOffsetEx(&DefaultHandler, Original, 255);

			Assert::IsNotNull(Original);
			Assert::IsNull(testme);


			LWAnsiString_FreeString(Original);
		}

		TEST_METHOD(CreateFromOffsetEx_AnsiTest_OffsetNOTEndOfString_IsOdd_MatchesExpected)
		{
			const char* original_string = "Hello World";
			const char* offset_expected = " World";
			const int offset_set = 5;

			LWAnsiString* Original = LWAnsiString_CreateFromStringA(original_string);
			Assert::IsNotNull(Original);
			LWAnsiString* testme = LWAnsiString_CreateFromOffsetEx(&DefaultHandler, Original, offset_set);

			
			Assert::IsNotNull(testme);


			const char* source = LWAnsiString_ToCStr(Original);
			const char* offset_source = LWAnsiString_ToCStr(testme);

			if (strcmp(offset_expected, offset_source) != 0)
			{
				Assert::Fail(L"LWAnsiString_CreateFromOffsetEx thru LWAnsiString_CreateFromStringA pathway did not slice correctly! ");
			}

			LWAnsiString_FreeString(Original);
			LWAnsiString_FreeString(testme);
		}


		TEST_METHOD(CreateFromOffsetEx_UnicodeTest_OffsetNOTEndOfString_IsOdd_MatchesExpected)
		{
			const wchar_t* original_string = L"Hello World";
			const wchar_t* offset_expected = L" World";
			const int offset_set = 5;

			LWAnsiString* Original = LWAnsiString_CreateFromStringW(original_string);
			Assert::IsNotNull(Original);
			LWAnsiString* testme = LWAnsiString_CreateFromOffsetEx(LWUnicodeHandler, Original, offset_set);


			Assert::IsNotNull(testme);


			const char* source = LWAnsiString_ToCStr(Original);
			const wchar_t* offset_source = (wchar_t*) LWAnsiString_ToCStr(testme);

			if (wcscmp(offset_expected, offset_source) != 0)
			{
				Assert::Fail(L"LWAnsiString_CreateFromOffsetEx thru LWAnsiString_CreateFromStringA pathway did not slice correctly! ");
			}

			LWAnsiString_FreeString(Original);
			LWAnsiString_FreeString(testme);
		}








		TEST_METHOD(CreateFromOffsetEx_AnsiTest_OffsetNOTEndOfString_IsEven_MatchesExpected)
		{
			const char* original_string = "Hello World";
			const char* offset_expected = "World";
			const int offset_set = 6;

			LWAnsiString* Original = LWAnsiString_CreateFromStringA(original_string);
			Assert::IsNotNull(Original);
			LWAnsiString* testme = LWAnsiString_CreateFromOffsetEx(&DefaultHandler, Original, offset_set);


			Assert::IsNotNull(testme);


			const char* source = LWAnsiString_ToCStr(Original);
			const char* offset_source = LWAnsiString_ToCStr(testme);

			if (strcmp(offset_expected, offset_source) != 0)
			{
				Assert::Fail(L"LWAnsiString_CreateFromOffsetEx thru LWAnsiString_CreateFromStringA pathway did not slice correctly! ");
			}

			LWAnsiString_FreeString(Original);
			LWAnsiString_FreeString(testme);
		}


		TEST_METHOD(CreateFromOffsetEx_UnicodeTest_OffsetNOTEndOfString_IsEven_MatchesExpected)
		{
			const wchar_t* original_string = L"Hello World";
			const wchar_t* offset_expected = L"World";
			const int offset_set = 6;

			LWAnsiString* Original = LWAnsiString_CreateFromStringW(original_string);
			Assert::IsNotNull(Original);
			LWAnsiString* testme = LWAnsiString_CreateFromOffsetEx(LWUnicodeHandler, Original, offset_set);


			Assert::IsNotNull(testme);


			const char* source = LWAnsiString_ToCStr(Original);
			const wchar_t* offset_source = (wchar_t*)LWAnsiString_ToCStr(testme);

			if (wcscmp(offset_expected, offset_source) != 0)
			{
				Assert::Fail(L"LWAnsiString_CreateFromOffsetEx thru LWAnsiString_CreateFromStringA pathway did not slice correctly! ");
			}

			LWAnsiString_FreeString(Original);
			LWAnsiString_FreeString(testme);
		}
		
	}
	;
}


