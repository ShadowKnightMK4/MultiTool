#include "pch.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

/* little of chatgpt help here.
* I Just wanted a C# style finally for LWAnsiString
*/
class LWAnsiStringGuard
{
public:
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

	LWAnsiString* get() { return ptr; }

	LWAnsiString* operator->() { return ptr; }
	LWAnsiString& operator*() { return *ptr; }

private:
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
			LWAnsiString* TestMe = Vanguard.get();
			// and we do it by creating a tring
			

			Assert::IsNotNull(TestMe);

		
		}
	};
}
