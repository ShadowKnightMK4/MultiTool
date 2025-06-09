#include "CppUnitTest.h"
#include "common.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
#include <stdlib.h>
namespace Tests
{
	TEST_CLASS(StringToNumberTests)
	{
	public:
		TEST_METHOD(NullFailChecks)
		{
			const char* not= 0;
			int used = 0;
			Assert::IsFalse(StringToNumber(nullptr, nullptr));
			Assert::IsFalse(StringToNumber(nullptr, &used));
			Assert::IsFalse(StringToNumber(not, nullptr));
		}

		TEST_METHOD(SimpleNoSign12345)
		{
			const char* First = "12345";
			int output = 0;
			Assert::IsTrue(StringToNumber(First, &output));
			Assert::IsTrue(output == 12345);
		}

		TEST_METHOD(SimpleNoSignButSpace12345)
		{
			const char* First = "  \t  12345";
			int output = 0;
			Assert::IsTrue(StringToNumber(First, &output));
			Assert::IsTrue(output == 12345);
		}


		TEST_METHOD(SimplePlusSignButSpace12345)
		{
			const char* First = "  \t  +12345";
			int output = 0;
			Assert::IsTrue(StringToNumber(First, &output));
			Assert::IsTrue(output == +12345);
		}

		TEST_METHOD(SimpleNegSignButSpace12345)
		{
			const char* First = "  \t  -12345";
			int output = 0;
			Assert::IsTrue(StringToNumber(First, &output));
			Assert::IsTrue(output == -12345);
		}

		TEST_METHOD(Simple12345WithBothMinusAndPlus_RoutineShouldRejectIt)
		{
			const char* First = "  \t  +-12345";
			int output = 0;
			Assert::IsFalse(StringToNumber(First, &output));
			Assert::IsTrue(output == 0); // output should be 0 if it fails		}
		}


		TEST_METHOD(SimpleWithSignPlus124)
		{
			const char* First = "+124";
			int output = 0;
			Assert::IsTrue(StringToNumber(First, &output));
			Assert::IsTrue(output == 124);
		}

		TEST_METHOD(SimpleWithSignMinus1412)
		{
			const char* First = "-1412";
			int output = 0;
			Assert::IsTrue(StringToNumber(First, &output));
			Assert::IsTrue(output == -1412);
		}

	};
	TEST_CLASS(NumberToStringTests)
	{
		/*
		* This is a test class for NumberToString and is intended to verify below
			
			#1 NumberToString rejects null
		*/
	public:
		
		TEST_METHOD(NullFailChecks)
		{
			int not= 0;
			char* used = 0;
			Assert::IsFalse(NumberToString(0, nullptr, nullptr));
			Assert::IsFalse(NumberToString(0, &used, nullptr));
			Assert::IsFalse(NumberToString(0, nullptr, &not));
		}

		TEST_METHOD(PositiveNumber)
		{
			int number = 123456789;
			int Out = 0;
			char* output = nullptr;

			Assert::IsTrue(NumberToString(number, &output, &Out));
			Assert::IsTrue(output != nullptr);
			Assert::IsTrue(atoi(output) == number);
			LocalFree(output);
		}

		TEST_METHOD(NegativeNumber)
		{
			int number = -123456789;
			int Out = 0;
			char* output = nullptr;

			Assert::IsTrue(NumberToString(number, &output, &Out));
			Assert::IsTrue(output != nullptr);
			Assert::IsTrue(atoi(output) == number);
			LocalFree(output);
		}


	};
}
