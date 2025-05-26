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
			int not= 0;
			int* used = 0;
			StringToNumber(nullptr, nullptr);
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
