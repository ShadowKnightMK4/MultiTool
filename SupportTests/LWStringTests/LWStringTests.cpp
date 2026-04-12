#include "pch.h"
#include "CppUnitTest.h"
#include "LWAnsiString.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
#define stricmp _stricmp
#define stricmp _stricmp

/* little of chatgpt help here.
* I Just wanted a C# style finally for LWAnsiString and liternally just have FreeString called at end of tests.
* 
*/
class LWAnsiStringGuard
{
public:
	explicit LWAnsiStringGuard(int len)
	{
		
		ptr = LWAnsiString_CreateString(len);
	}

	/// <summary>
	/// Set to blank
	/// </summary>
	/// <param name=""></param>
	explicit LWAnsiStringGuard(void*)
	{
		ptr = nullptr;
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
			LWAnsiStringGuard Vanguard(nullptr);
			LWAnsiString** TestMe = &Vanguard.ptr;
			// and we do it by creating a tring
			Vanguard.ptr = LWAnsiString_CreateStringEx(LWAnsiHandler, 0);


			Assert::IsNotNull(*TestMe);
			Assert::IsTrue(LWAnsiString_Length(*TestMe) == 0);
			Assert::IsNotNull((*TestMe)->AllocatedHandle);
			Assert::IsTrue((*TestMe)->AllocatedSize == 1); // the allot plus the null term
		}

		TEST_METHOD(BlankStringAnsi_ReserveWorks_NoThru)
		{
			// string length 1
			LWAnsiStringGuard Vanguard(0);
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
			LWAnsiStringGuard Vanguard(0);
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
			LWAnsiStringGuard Vanguard(0);
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
			LWAnsiStringGuard Vanguard(0);
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
			LWAnsiStringGuard AltPtr(250);
			LWAnsiStringGuard Vanguard1(nullptr);
			LWAnsiString** TestMe = &Vanguard1.ptr;
			LWAnsiString* Alt = AltPtr.ptr;

			LWAnsiString_AppendA(Alt, "Hello World 12");
			*TestMe = LWAnsiString_CreateFromOffset(Alt, 5);

			Assert::IsNotNull(*TestMe);

			int compare = strcmp(" World 12", LWAnsiString_ToCStr(*TestMe));
			Assert::IsTrue(compare == 0);


			Assert::IsTrue((*TestMe)->AllocatedSize = LWAnsiString_Length(Alt) - 5);
		}

		TEST_METHOD(CreateOffsetString_Ansi)
		{
			LWAnsiStringGuard AltPtr(250);
			LWAnsiStringGuard Vanguard1(nullptr);
			LWAnsiString** TestMe = &Vanguard1.ptr;
			LWAnsiString* Alt = AltPtr.ptr;

			LWAnsiString_AppendA(Alt, "Hello World 12");
			*TestMe = LWAnsiString_CreateFromOffsetEx(LWAnsiHandler, Alt, 5);

			Assert::IsNotNull(*TestMe);

			int compare = strcmp(" World 12", LWAnsiString_ToCStr(*TestMe));
			Assert::IsTrue(compare == 0);


			Assert::IsTrue((*TestMe)->AllocatedSize = LWAnsiString_Length(Alt) - 5);
		}

		TEST_METHOD(CreateOffsetString_Ansi_LengthExpected)
		{
			LWAnsiStringGuard AltPtr(255);
			LWAnsiString_AppendA(AltPtr.ptr, "Hello World");
			Assert::IsTrue(LWAnsiString_Length(AltPtr.ptr) == strlen("Hello World"));
		}

		TEST_METHOD(LWAnsiString_ZeroString_Works)
		{
			LWAnsiStringGuard AltPtr(255);
			LWAnsiString_AppendA(AltPtr.ptr, "Hello World");
			Assert::IsTrue(LWAnsiString_Length(AltPtr.ptr) == strlen("Hello World"));
			LWAnsiString_ZeroString(AltPtr.ptr);
			Assert::IsTrue(LWAnsiString_Length(AltPtr.ptr) == 0);
			Assert::IsTrue(AltPtr.ptr->A[0]  == 0);
			wchar_t t = (AltPtr.ptr->W[0]);
			Assert::IsTrue(t ==  0); // what Zero does is also zero the memory of the whole string and our example string is larger than sizeof(wchar)
		}

		TEST_METHOD(LWAnsiString_AdjustSize_GoodInput_Grow)
		{
			LWAnsiStringGuard Guard(255);
			LWAnsiString* test = Guard.ptr;

			Assert::IsTrue(test->AllocatedSize == 255 + 1);
			int res = LWAnsiString_AdjustSize(test, 512);
			Assert::IsTrue(test->AllocatedSize == 513);
			Assert::IsTrue(res == 512);
		}

		TEST_METHOD(LWAnsiString_AdjustSize_GoodInput_Shrink)
		{
			LWAnsiStringGuard Guard(255);
			LWAnsiString* test = Guard.ptr;

			Assert::IsTrue(test->AllocatedSize == 255 + 1);
			int res = LWAnsiString_AdjustSize(test, 12);
			Assert::IsTrue(test->AllocatedSize == 13);
			Assert::IsTrue(res == 12);
		}

		TEST_METHOD(LWAnsiString_AdjustSize_GoodInput_Shrink_NullsOk)
		{
			LWAnsiStringGuard Guard(255);
			LWAnsiString* test = Guard.ptr;
			LWAnsiString_AppendA(test, "Hello World");
			Assert::IsTrue(test->AllocatedSize == 255 + 1);
			int res = LWAnsiString_AdjustSize(test, 2);
			Assert::IsTrue(test->AllocatedSize == 3);
			Assert::IsTrue(res == 2);

			Assert::IsTrue(stricmp(test->A, "He")==0);
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
			Assert::IsTrue(stricmp("-----", LWAnsiString_ToCStr(test)) ==0);
		}

		TEST_METHOD(LWAnsiString_PadNewLine_Works)
		{
			LWAnsiStringGuard Guard(255);
			LWAnsiString* test = Guard.ptr;
			LWAnsiString_PadNewLineA(test, '-', 5);
			Assert::IsTrue(LWAnsiString_Length(test) == 7); // windows new line is 2 chars
			Assert::IsTrue(test->AllocatedSize == 256);
			Assert::IsTrue(stricmp("-----\r\n", LWAnsiString_ToCStr(test)) == 0);
			
			Assert::IsTrue(LWAnsiString_EndsWith(test, "\r\n", false));
		}


		TEST_METHOD(LWansiString_AppendCheck)
		{
			LWAnsiStringGuard Guard(255);
			LWAnsiString* test = Guard.ptr;
			LWAnsiString_AppendA(test, "Hello");
			Assert::IsTrue(LWAnsiString_EndsWith(test, LWAnsiString_ToCStr(test),true));
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
}
