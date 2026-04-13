#include "pch.h"
#include "CppUnitTest.h"
#include "LWAnsiString.h"
#include "LWAnsiStringGuard.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;


namespace LWAnsiString_Tests
{
	TEST_CLASS(UnicodeCodeTests)
	{
	public:

		TEST_METHOD(SetupTriggersOk)
		{
			LWAnsiStringGuard Vanguard(TRUE, 1);
			LWAnsiString* TestMe = Vanguard.ptr;
			LWAnsiStringGuard x(nullptr);
			// and we do it by creating a tring
			x.ptr = LWAnsiString_CreateStringEx(LWUnicodeHandler, 2);

			Assert::IsNotNull(TestMe);
			Assert::IsNotNull(x.ptr);


		}

		TEST_METHOD(BlankDefault_Init_StateCheck)
		{
			// string length 1
			LWAnsiStringGuard Vanguard(TRUE, 1);
			LWAnsiString* TestMe = Vanguard.ptr;

			// and we do it by creating a tring


			Assert::IsNotNull(TestMe);
			Assert::IsTrue(LWAnsiString_Length(TestMe) == 0);
			Assert::IsNotNull(TestMe->AllocatedHandle);
			Assert::IsTrue(LWAnsiString_GetAllocatedByteSize(TestMe) == 4); //
		}

		TEST_METHOD(BlankDefaultAllocatesMin1Char)
		{
			// string length 1
			LWAnsiStringGuard Vanguard(TRUE, 1);
			LWAnsiString* TestMe = Vanguard.ptr;
			// and we do it by creating a tring


			Assert::IsNotNull(TestMe);
			Assert::IsTrue(LWAnsiString_Length(TestMe) == 0);
			Assert::IsNotNull(TestMe->AllocatedHandle);
			Assert::IsTrue(LWAnsiString_GetAllocatedByteSize(TestMe) == 4); // the allot plus the null term
		}

		TEST_METHOD(BlankMinAllocatesMin1Char)
		{
			// string length 1
			LWAnsiStringGuard Vanguard(TRUE, 1);
			LWAnsiString* TestMe = Vanguard.ptr;
			// and we do it by creating a tring
//			Vanguard.ptr = LWAnsiString_CreateStringEx(LWUnicodeHandler, 0);


			Assert::IsNotNull(TestMe);
			Assert::IsTrue(LWAnsiString_Length(TestMe) == 0);
			Assert::IsNotNull((TestMe)->AllocatedHandle);
			Assert::IsTrue((TestMe)->AllocatedSize == 2); // the allot plus the null term
			Assert::IsTrue(LWAnsiString_GetAllocatedByteSize(TestMe) == 4); // the allot plus the null term
		}

		TEST_METHOD(BlankStringAnsi_ReserveWorks_NoThru)
		{
			// string length 1
			LWAnsiStringGuard Vanguard(TRUE, 1);
			LWAnsiString* TestMe = Vanguard.ptr;
			// and we do it by creating a tring


			Assert::IsNotNull(TestMe);
			Assert::IsTrue(LWAnsiString_Length(TestMe) == 0);
			Assert::IsNotNull(TestMe->AllocatedHandle);
			Assert::IsTrue(TestMe->AllocatedSize == 2); // the allot plus the null term
			Assert::IsTrue((TestMe)->AllocatedSize == 2); // the allot plus the null term
			Assert::IsTrue(LWAnsiString_GetAllocatedByteSize(TestMe) == 4); // the allot plus the null term

			LWAnsiString_Reserve(TestMe, 512);
			Assert::IsTrue(TestMe->AllocatedSize == 513);
		}


		TEST_METHOD(BlankStringAnsi_ReserveCAPWorks_NoThru)
		{
			// string length 1
			LWAnsiStringGuard Vanguard(TRUE, 1);
			LWAnsiString* TestMe = Vanguard.ptr;
			// and we do it by creating a tring


			Assert::IsNotNull(TestMe);
			Assert::IsTrue(LWAnsiString_Length(TestMe) == 0);
			Assert::IsNotNull(TestMe->AllocatedHandle);
			Assert::IsTrue(TestMe->AllocatedSize == 2); // the allot plus the null term


			LWAnsiString_AddReserveCap(TestMe, 512, 1024);
			Assert::IsTrue(TestMe->AllocatedSize == 513);
		}

		TEST_METHOD(BlankStringAnsi_ReserveCAPWorks_NoThru_TriggersLimit)
		{
			// string length 1
			LWAnsiStringGuard Vanguard(TRUE, 1);
			LWAnsiString* TestMe = Vanguard.ptr;
			// and we do it by creating a tring


			Assert::IsNotNull(TestMe);
			Assert::IsTrue(LWAnsiString_Length(TestMe) == 0);
			Assert::IsNotNull(TestMe->AllocatedHandle);
			Assert::IsTrue(TestMe->AllocatedSize == 2); // the allot plus the null term


			LWAnsiString_AddReserveCap(TestMe, 512, 12);
			Assert::IsFalse(TestMe->AllocatedSize == 513);
			Assert::IsTrue(TestMe->AllocatedSize == 13); // the null
		}

		TEST_METHOD(MarkingStringDirecty_Works)
		{
			// string length 1
			LWAnsiStringGuard Vanguard(TRUE, 1);
			LWAnsiString* TestMe = Vanguard.ptr;

			Assert::IsNotNull(TestMe);
			Assert::IsTrue(LWAnsiString_Length(TestMe) == 0);
			
			Assert::IsNotNull(TestMe->AllocatedHandle);
			Assert::IsTrue(TestMe->AllocatedSize == 2); // the allot plus the null term
			Assert::IsFalse((TestMe->Flags & (LWANSI_FLAG_DIRTY)) == LWANSI_FLAG_DIRTY);

			LWAnsiString_MarkLenDirty(TestMe);
			//Assert::IsTrue(LWAnsiString_Length(TestMe) < 0);
			Assert::IsTrue((TestMe->Flags & (LWANSI_FLAG_DIRTY)) == LWANSI_FLAG_DIRTY);
		}

		TEST_METHOD(CreateOffsetString_Unicode)
		{
			LWAnsiStringGuard AltPtr(LWAnsiString_CreateStringW(255));
			LWAnsiStringGuard Vanguard1(nullptr);
			LWAnsiString** TestMe = &Vanguard1.ptr;
			LWAnsiString* Alt = AltPtr.ptr;

			LWAnsiString_AppendW(Alt, L"Hello World 12");
			*TestMe = LWAnsiString_CreateFromOffset(Alt, 5);

			Assert::IsNotNull(*TestMe);

			int compare = wcscmp(L" World 12", (wchar_t*)LWAnsiString_ToCStr(*TestMe));
			Assert::IsTrue(compare == 0);


			Assert::IsTrue((*TestMe)->AllocatedSize = LWAnsiString_Length(Alt) - 5);
		}



		TEST_METHOD(CreateOffsetString_Unicode_LengthExpected)
		{
			LWAnsiStringGuard AltPtr(TRUE, 255);
			LWAnsiString_AppendA(AltPtr.ptr, "Hello World");
			Assert::IsTrue(LWAnsiString_Length(AltPtr.ptr) == strlen("Hello World"));
		}

		TEST_METHOD(LWAnsiString_ZeroString_Works)
		{
			LWAnsiStringGuard Vanguard(TRUE, 1);
			LWAnsiString* TestMe = Vanguard.ptr;
			LWAnsiString_AppendA(TestMe, "Hello World");
			Assert::IsTrue(LWAnsiString_Length(TestMe) == strlen("Hello World"));
			LWAnsiString_ZeroString(TestMe);
			Assert::IsTrue(LWAnsiString_Length(TestMe) == 0);
			Assert::IsTrue(TestMe->A[0] == 0);
			wchar_t t = (TestMe->W[0]);
			Assert::IsTrue(t == 0); // what Zero does is also zero the memory of the whole string and our example string is larger than sizeof(wchar)
		}

		TEST_METHOD(LWAnsiString_AdjustSize_GoodInput_Grow)
		{
			LWAnsiStringGuard Vanguard(TRUE, 255);
			LWAnsiString* test = Vanguard.ptr;

			Assert::IsTrue(test->AllocatedSize == 255 + 1);
			int res = LWAnsiString_AdjustSize(test, 512);
			Assert::IsTrue(test->AllocatedSize == 513);
			Assert::IsTrue(res == 512);
		}

		TEST_METHOD(LWAnsiString_AdjustSize_GoodInput_Shrink)
		{
			LWAnsiStringGuard Vanguard(TRUE, 255);
			LWAnsiString* test = Vanguard.ptr;

			Assert::IsTrue(test->AllocatedSize == 255 + 1);
			int res = LWAnsiString_AdjustSize(test, 12);
			Assert::IsTrue(test->AllocatedSize == 13);
			Assert::IsTrue(res == 12);
		}

		TEST_METHOD(LWAnsiString_AdjustSize_GoodInput_Shrink_NullsOk)
		{
			LWAnsiStringGuard Vanguard(TRUE, 255);
			LWAnsiString* test = Vanguard.ptr;

			LWAnsiString_AppendA(test, "Hello World");
			Assert::IsTrue(test->AllocatedSize == 255 + 1);
			int res = LWAnsiString_AdjustSize(test, 2);
			Assert::IsTrue(test->AllocatedSize == 3);
			Assert::IsTrue(res == 2);
			Assert::IsTrue(LWAnsiString_GetAllocatedByteSize(test) == 6);
			Assert::IsTrue(_wcsicmp(test->W, L"He") == 0);
		}

		TEST_METHOD(LWAnsiString_AppendNewLine_Works)
		{
			LWAnsiStringGuard Guard(255);
			LWAnsiString* test = Guard.ptr;
			LWAnsiString_AppendNewLine(test);
			Assert::IsTrue(test->Length == 2);
			Assert::IsTrue(test->AllocatedSize == 256); // da null
			Assert::IsTrue(test->A[0] == '\r');
			Assert::IsTrue(test->A[1] == '\n');
			Assert::IsTrue(test->A[2] == 0);
		}

		TEST_METHOD(LWAnsiString_Pad_Works)
		{
			LWAnsiStringGuard Guard(255);
			LWAnsiString* test = Guard.ptr;
			LWAnsiString_PadA(test, '-', 5);
			Assert::IsTrue(LWAnsiString_Length(test) == 5);
			Assert::IsTrue(test->AllocatedSize == 256);
			Assert::IsTrue(_stricmp("-----", LWAnsiString_ToCStr(test)) == 0);
		}

		TEST_METHOD(LWAnsiString_PadNewLine_Works)
		{
			LWAnsiStringGuard Guard(255);
			LWAnsiString* test = Guard.ptr;
			LWAnsiString_PadNewLineA(test, '-', 5);
			Assert::IsTrue(LWAnsiString_Length(test) == 7); // windows new line is 2 chars
			Assert::IsTrue(test->AllocatedSize == 256);
			Assert::IsTrue(_stricmp("-----\r\n", LWAnsiString_ToCStr(test)) == 0);

			Assert::IsTrue(LWAnsiString_EndsWith(test, "\r\n", false));
		}


		TEST_METHOD(LWansiString_AppendCheck)
		{
			LWAnsiStringGuard Guard(255);
			LWAnsiString* test = Guard.ptr;
			LWAnsiString_AppendA(test, "Hello");
			Assert::IsTrue(LWAnsiString_EndsWith(test, LWAnsiString_ToCStr(test), true));
		}

		TEST_METHOD(LWansiString_AppendCheckNewLine)
		{
			LWAnsiStringGuard Guard(255);
			LWAnsiString* test = Guard.ptr;
			LWAnsiString_AppendWithNewLineA(test, "Hello");
			Assert::IsTrue(LWAnsiString_EndsWith(test, LWAnsiString_ToCStr(test), true));
			Assert::IsTrue(LWAnsiString_EndsWith(test, "\r\n", false));
		}

		TEST_METHOD(LWAnsiString_Reserve_Check)
		{
			LWAnsiStringGuard Guard(255);
			LWAnsiString* test = Guard.ptr;
			LWAnsiString_AppendA(test, "Hello");
			LWAnsiString_Reserve(test, 300);
			Assert::IsTrue(test->AllocatedSize == 301);
		}

		TEST_METHOD(LWAnsiString_Duplicate_Check)
		{
			LWAnsiStringGuard Guard(255);
			LWAnsiStringGuard Alt(nullptr);
			LWAnsiString* test = Guard.ptr;
			LWAnsiString_AppendA(test, "Hello");

			Alt.ptr = LWAnsiString_Duplicate(test);
			Assert::IsNotNull(Alt.ptr);
			Assert::IsNotNull(Alt.ptr->Data);
			Assert::IsTrue(strcmp(Alt.ptr->A, Guard.ptr->A) == 0);
		}

		TEST_METHOD(LWAnsi_AppendNumber)
		{
			LWAnsiStringGuard Guard(255);
			LWAnsiStringGuard Alt(nullptr);
			LWAnsiString* test = Guard.ptr;

			LWAnsiString_AppendNumberA(12, test, 0);
			Assert::IsTrue(strcmp("12", test->A) == 0);
			LWAnsiString_AppendNumberA(51, test, 0);
			Assert::IsTrue(strcmp("1251", test->A) == 0);
		}

		TEST_METHOD(LWAnsi_AppendNumber_INTMIN)
		{
			LWAnsiStringGuard Guard(255);
			LWAnsiStringGuard Alt(nullptr);
			LWAnsiString* test = Guard.ptr;

			LWAnsiString_AppendNumberA(INT_MIN, test, 0);
			Assert::IsTrue(strcmp("-2147483648", test->A) == 0);
		}

		TEST_METHOD(LWAnsi_AppendNumber_ZERO)
		{
			LWAnsiStringGuard Guard(255);
			LWAnsiStringGuard Alt(nullptr);
			LWAnsiString* test = Guard.ptr;

			LWAnsiString_AppendNumberA(0, test, 0);
			Assert::IsTrue(strcmp("0", test->A) == 0);
		}



		TEST_METHOD(LWUnicode_JustMakeInstance)
		{
			LWAnsiStringGuard Guard(255);
			LWAnsiStringGuard Alt(nullptr);
			Alt.ptr = LWAnsiString_CreateStringEx(LWUnicodeHandler, 0);

			Assert::IsTrue(Alt.ptr->AllocatedHandle == LWUnicodeHandler);
			Assert::IsTrue(Alt.ptr->AllocatedSize == 1); // reember we're counting characters.
			Assert::IsTrue(LWAnsiString_GetAllocatedByteSize(Alt.ptr) == sizeof(wchar_t));
		}
	};


#include "pch.h"
#include "CppUnitTest.h"
#include "LWAnsiString.h"
	using namespace Microsoft::VisualStudio::CppUnitTestFramework;
#define stricmp _stricmp
#define stricmp _stricmp

	typedef BOOL BoolOrAnsi;
	/* little of chatgpt help here.
	* I Just wanted a C# style finally for LWAnsiString and liternally just have FreeString called at end of tests.
	*
	*/
	class LWAnsiStringGuard
	{
	public:
		explicit LWAnsiStringGuard(BOOL WantUnicode, int size)
		{
			if (WantUnicode)
			{
				ptr = LWAnsiString_CreateStringW(size);
			}
			else
			{
				ptr = LWAnsiString_CreateString(size);
			}
		}
		explicit LWAnsiStringGuard(LWAnsiString* ptr2)
		{
			ptr = ptr;
		}
		explicit LWAnsiStringGuard(int len)
		{

			ptr = LWAnsiString_CreateString(len);
		}


		~LWAnsiStringGuard()
		{
			if (ptr)
			{
				LWAnsiString_FreeString(ptr);
			}
		}


		LWAnsiString* ptr = nullptr;
	};

	namespace LWStringTests
	{
		TEST_CLASS(LWStringTests)
		{
		public:

			TEST_METHOD(SetupTriggersOk)
			{
				LWAnsiStringGuard Vanguard(1);
				LWAnsiString* TestMe = Vanguard.ptr;
				// and we do it by creating a tring


				Assert::IsNotNull(TestMe);


			}

			TEST_METHOD(BlankDefault_Init_StateCheck)
			{
				// string length 1
				LWAnsiStringGuard Vanguard(1);
				LWAnsiString* TestMe = Vanguard.ptr;
				// and we do it by creating a tring


				Assert::IsNotNull(TestMe);
				Assert::IsTrue(LWAnsiString_Length(TestMe) == 0);
				Assert::IsNotNull(TestMe->AllocatedHandle);
				Assert::IsTrue(TestMe->AllocatedSize == 2); // the allot plus the null term
			}

			TEST_METHOD(BlankDefaultAllocatesMin1Char)
			{
				// string length 1
				LWAnsiStringGuard Vanguard(0);
				LWAnsiString* TestMe = Vanguard.ptr;
				// and we do it by creating a tring


				Assert::IsNotNull(TestMe);
				Assert::IsTrue(LWAnsiString_Length(TestMe) == 0);
				Assert::IsNotNull(TestMe->AllocatedHandle);
				Assert::IsTrue(TestMe->AllocatedSize == 1); // the allot plus the null term
			}

			TEST_METHOD(BlankMinAllocatesMin1Char)
			{
				// string length 1
				LWAnsiStringGuard Vanguard(TRUE, 0);
				LWAnsiString* TestMe = Vanguard.ptr;
				// and we do it by creating a tring


				Assert::IsNotNull(TestMe);
				Assert::IsTrue(LWAnsiString_Length(TestMe) == 0);
				Assert::IsNotNull((TestMe)->AllocatedHandle);
				Assert::IsTrue((TestMe)->AllocatedSize == 1); // the allot plus the null term
			}

			TEST_METHOD(BlankStringAnsi_ReserveWorks_NoThru)
			{
				// string length 1
				LWAnsiStringGuard Vanguard(TRUE, 0);
				LWAnsiString* TestMe = Vanguard.ptr;
				// and we do it by creating a tring


				Assert::IsNotNull(TestMe);
				Assert::IsTrue(LWAnsiString_Length(TestMe) == 0);
				Assert::IsNotNull(TestMe->AllocatedHandle);
				Assert::IsTrue(TestMe->AllocatedSize == 1); // the allot plus the null term


				LWAnsiString_Reserve(TestMe, 512);
				Assert::IsTrue(TestMe->AllocatedSize == 513);
			}


			TEST_METHOD(BlankStringAnsi_ReserveCAPWorks_NoThru)
			{
				// string length 1
				LWAnsiStringGuard Vanguard(TRUE, 0);
				LWAnsiString* TestMe = Vanguard.ptr;
				// and we do it by creating a tring


				Assert::IsNotNull(TestMe);
				Assert::IsTrue(LWAnsiString_Length(TestMe) == 0);
				Assert::IsNotNull(TestMe->AllocatedHandle);
				Assert::IsTrue(TestMe->AllocatedSize == 1); // the allot plus the null term


				LWAnsiString_AddReserveCap(TestMe, 512, 1024);
				Assert::IsTrue(TestMe->AllocatedSize == 513);
			}

			TEST_METHOD(BlankStringAnsi_ReserveCAPWorks_NoThru_TriggersLimit)
			{
				// string length 1
				LWAnsiStringGuard Vanguard(TRUE, 0);
				LWAnsiString* TestMe = Vanguard.ptr;
				// and we do it by creating a tring


				Assert::IsNotNull(TestMe);
				Assert::IsTrue(LWAnsiString_Length(TestMe) == 0);
				Assert::IsNotNull(TestMe->AllocatedHandle);
				Assert::IsTrue(TestMe->AllocatedSize == 1); // the allot plus the null term


				LWAnsiString_AddReserveCap(TestMe, 512, 12);
				Assert::IsFalse(TestMe->AllocatedSize == 513);
				Assert::IsTrue(TestMe->AllocatedSize == 13); // the null
			}

			TEST_METHOD(MarkingStringDirecty_Works)
			{
				// string length 1
				LWAnsiStringGuard Vanguard(TRUE,0);
				LWAnsiString* TestMe = Vanguard.ptr;

				Assert::IsNotNull(TestMe);
				Assert::IsTrue(LWAnsiString_Length(TestMe) == 0);
				Assert::IsNotNull(TestMe->AllocatedHandle);
				Assert::IsTrue(TestMe->AllocatedSize == 1); // the allot plus the null term
				Assert::IsFalse((TestMe->Flags & (LWANSI_FLAG_DIRTY)) == LWANSI_FLAG_DIRTY);

				LWAnsiString_MarkLenDirty(TestMe);
				//Assert::IsTrue(LWAnsiString_Length(TestMe) < 0);
				Assert::IsTrue((TestMe->Flags & (LWANSI_FLAG_DIRTY)) == LWANSI_FLAG_DIRTY);
			}

			TEST_METHOD(CreateOffsetString_Default)
			{
				LWAnsiStringGuard AltPtr(TRUE, 255);
				LWAnsiStringGuard Vanguard1(nullptr);
				LWAnsiString** TestMe = &Vanguard1.ptr;
				LWAnsiString* Alt = AltPtr.ptr;

				LWAnsiString_AppendW(Alt, L"Hello World 12");
				*TestMe = LWAnsiString_CreateFromOffset(Alt, 5);

				Assert::IsNotNull(*TestMe);

				int compare = wcscmp(L" World 12", (wchar_t*)LWAnsiString_ToCStr(*TestMe));
				Assert::IsTrue(compare == 0);


				Assert::IsTrue((*TestMe)->AllocatedSize = LWAnsiString_Length(Alt) - 5);
			}

			TEST_METHOD(CreateOffsetString_Ansi)
			{
				LWAnsiStringGuard AltPtr(TRUE, 255);
				LWAnsiStringGuard Vanguard1(nullptr);
				LWAnsiString** TestMe = &Vanguard1.ptr;
				LWAnsiString* Alt = AltPtr.ptr;

				LWAnsiString_AppendW(Alt, L"Hello World 12");
				*TestMe = LWAnsiString_CreateFromOffsetEx(LWUnicodeHandler, Alt, 5);

				Assert::IsNotNull(*TestMe);

				int compare = wcscmp(L" World 12", (wchar_t*)LWAnsiString_ToCStr(*TestMe));
				Assert::IsTrue(compare == 0);


				Assert::IsTrue((*TestMe)->AllocatedSize = LWAnsiString_Length(Alt) - 5);
			}

			TEST_METHOD(CreateOffsetString_Ansi_LengthExpected)
			{
				LWAnsiStringGuard Guard(TRUE, 0);
				LWAnsiString_AppendW(Guard.ptr, L"Hello World");
				Assert::IsTrue(LWAnsiString_Length(Guard.ptr) == wcslen(L"Hello World"));
			}

			TEST_METHOD(LWAnsiString_ZeroString_Works)
			{
				LWAnsiStringGuard Guard(TRUE, 0);
				LWAnsiString_AppendW(Guard.ptr, L"Hello World");
				Assert::IsTrue(LWAnsiString_Length(Guard.ptr) == wcslen(L"Hello World"));
				LWAnsiString_ZeroString(Guard.ptr);
				Assert::IsTrue(LWAnsiString_Length(Guard.ptr) == 0);
				Assert::IsTrue(Guard.ptr->A[0] == 0);
				wchar_t t = (Guard.ptr->W[0]);
				Assert::IsTrue(t == 0); // what Zero does is also zero the memory of the whole string and our example string is larger than sizeof(wchar)
			}

			TEST_METHOD(LWAnsiString_AdjustSize_GoodInput_Grow)
			{
				LWAnsiStringGuard Guard(TRUE, 0);
				LWAnsiString* test = Guard.ptr;

				Assert::IsTrue(test->AllocatedSize == 255 + 1);
				int res = LWAnsiString_AdjustSize(test, 512);
				Assert::IsTrue(test->AllocatedSize == 513);
				Assert::IsTrue(res == 512);
			}

			TEST_METHOD(LWAnsiString_AdjustSize_GoodInput_Shrink)
			{
				LWAnsiStringGuard Guard(TRUE, 0);
				LWAnsiString* test = Guard.ptr;

				Assert::IsTrue(test->AllocatedSize == 255 + 1);
				int res = LWAnsiString_AdjustSize(test, 12);
				Assert::IsTrue(test->AllocatedSize == 13);
				Assert::IsTrue(res == 12);
			}

			TEST_METHOD(LWAnsiString_AdjustSize_GoodInput_Shrink_NullsOk)
			{
				LWAnsiStringGuard Guard(TRUE, 0);
				LWAnsiString* test = Guard.ptr;
				LWAnsiString_AppendW(test, L"Hello World");
				Assert::IsTrue(test->AllocatedSize == 255 + 1);
				int res = LWAnsiString_AdjustSize(test, 2);
				Assert::IsTrue(test->AllocatedSize == 3);
				Assert::IsTrue(res == 2);

				Assert::IsTrue(wcscmp(test->W, L"He") == 0);
			}

			TEST_METHOD(LWAnsiString_AppendNewLine_Works)
			{
				LWAnsiStringGuard Guard(TRUE, 0);
				LWAnsiString* test = Guard.ptr;
				LWAnsiString_AppendNewLine(test);
				Assert::IsTrue(test->Length == 2);
				Assert::IsTrue(test->AllocatedSize == 256); // da null
				Assert::IsTrue(test->A[0] == '\r');
				Assert::IsTrue(test->A[1] == '\n');
				Assert::IsTrue(test->A[2] == 0);
			}

			TEST_METHOD(LWAnsiString_Pad_Works)
			{
				LWAnsiStringGuard Guard(TRUE, 0);
				LWAnsiString* test = Guard.ptr;
				LWAnsiString_PadW(test, '-', 5);
				Assert::IsTrue(LWAnsiString_Length(test) == 5);
				Assert::IsTrue(test->AllocatedSize == 256);
				Assert::IsTrue(wcscmp(L"-----", (wchar_t*)LWAnsiString_ToCStr(test)) == 0);
			}

			TEST_METHOD(LWAnsiString_PadNewLine_Works)
			{
				LWAnsiStringGuard Guard(TRUE, 0);
				LWAnsiString* test = Guard.ptr;
				LWAnsiString_PadNewLineA(test, '-', 5);
				Assert::IsTrue(LWAnsiString_Length(test) == 7); // windows new line is 2 chars
				Assert::IsTrue(test->AllocatedSize == 256);
				Assert::IsTrue(wcscmp(L"-----\r\n", (const wchar_t*)LWAnsiString_ToCStr(test)) == 0);

				Assert::IsTrue(LWAnsiString_EndsWith(test, "\r\n", false));
			}


			TEST_METHOD(LWansiString_AppendCheck)
			{
				LWAnsiStringGuard Guard(TRUE, 0);
				LWAnsiString* test = Guard.ptr;
				LWAnsiString_AppendW(test, L"Hello");
				Assert::IsTrue(LWAnsiString_EndsWith(test, LWAnsiString_ToCStr(test), true));
			}

			TEST_METHOD(LWansiString_AppendCheckNewLine)
			{
				LWAnsiStringGuard Guard(TRUE, 0);
				LWAnsiString* test = Guard.ptr;
				LWAnsiString_AppendWithNewLineW(test, L"Hello");
				Assert::IsTrue(LWAnsiString_EndsWith(test, LWAnsiString_ToCStr(test), true));
				Assert::IsTrue(LWAnsiString_EndsWith(test, "\r\n", false));
			}

			TEST_METHOD(LWAnsiString_Reserve_Check)
			{
				LWAnsiStringGuard Guard(TRUE, 0);
				LWAnsiString* test = Guard.ptr;
				LWAnsiString_AppendW(test, L"Hello");
				LWAnsiString_Reserve(test, 300);
				Assert::IsTrue(test->AllocatedSize == 301);
			}

			TEST_METHOD(LWAnsiString_Duplicate_Check)
			{
				LWAnsiStringGuard Guard(TRUE, 0);
				LWAnsiStringGuard Alt(nullptr);
				LWAnsiString* test = Guard.ptr;
				LWAnsiString_AppendW(test, L"Hello");

				Alt.ptr = LWAnsiString_Duplicate(test);
				Assert::IsNotNull(Alt.ptr);
				Assert::IsNotNull(Alt.ptr->Data);
				Assert::IsTrue(wcscmp(Alt.ptr->W, Guard.ptr->W) == 0);
			}

			TEST_METHOD(LWAnsi_AppendNumber)
			{
				LWAnsiStringGuard Guard(TRUE, 255);
				LWAnsiStringGuard Alt(nullptr);
				LWAnsiString* test = Guard.ptr;

				LWAnsiString_AppendNumberW(12, test, 0);
				Assert::IsTrue(wcscmp(L"12", test->W) == 0);
				LWAnsiString_AppendNumberW(51, test, 0);
				Assert::IsTrue(wcscmp(L"1251", test->W) == 0);
			}

			TEST_METHOD(LWAnsi_AppendNumber_INTMIN)
			{
				LWAnsiStringGuard Guard(TRUE, 255);
				LWAnsiStringGuard Alt(nullptr);
				LWAnsiString* test = Guard.ptr;

				LWAnsiString_AppendNumberW(INT_MIN, test, 0);
				Assert::IsTrue(wcscmp(L"-2147483648", test->W) == 0);
			}

			TEST_METHOD(LWAnsi_AppendNumber_ZERO)
			{
				LWAnsiStringGuard Guard(TRUE, 255);
				LWAnsiStringGuard Alt(nullptr);
				LWAnsiString* test = Guard.ptr;

				LWAnsiString_AppendNumberW(0, test, 0);
				Assert::IsTrue(wcscmp(L"0", test->W) == 0);
			}



			TEST_METHOD(LWUnicode_JustMakeInstance)
			{
				LWAnsiStringGuard Guard(TRUE,255);
				LWAnsiStringGuard Alt(nullptr);
				Alt.ptr = LWAnsiString_CreateStringEx(LWUnicodeHandler, 0);

				Assert::IsTrue(Alt.ptr->AllocatedHandle == LWUnicodeHandler);
				Assert::IsTrue(Alt.ptr->AllocatedSize == 1); // reember we're counting characters.
				Assert::IsTrue(LWAnsiString_GetAllocatedByteSize(Alt.ptr) == sizeof(wchar_t));
			}
		};
	}

}