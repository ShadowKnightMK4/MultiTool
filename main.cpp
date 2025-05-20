#include <windows.h>
#include "common.h"
#include "tool_functions.h"
#include "tool_dispatch.h"

int main(int argc, const char* argv[])
{
	if (argc < 2)
	{
		return -1;
	}
	for (int i = 1; i < argc; i++)
	{
		const char* current = argv[i];
		ToolFunction function = (ToolFunction) GetFunctionPointer(current);
		if (function != 0)
		{
			int result = 0;
			const char* message_result = nullptr;
			bool res = function(&result, &message_result, argv);
			if (res)
			{
				if (message_result != nullptr)
				{
					WriteStdout(message_result);
				}
				return result;
			}
			else
			{
				if (message_result != nullptr)
				{
					WriteStderr(message_result);
				}
				return result;
			}
		}
		else
		{
			WriteStderr("Unknown command line flag");
			return -1;
		}
		continue;
		if (lstrcmpi(argv[i], FlagSilent1) == 0 || lstrcmpi(argv[i], FlagSilent2) == 0)
		{
			SILENCE = true;
		}
		if (lstrcmpi(argv[i], FlagEmptyBin) == 0)
		{
			int result = 0;
			const char* message_result = nullptr;
			if (EmptyBin( & result, &message_result, argv))
			{
				if (message_result != nullptr)
				{
					WriteStdout(message_result);
				}
				return result;
			}
			else
			{
				if (message_result != nullptr)
				{
					WriteStderr(message_result);
				}
				return -1;
			}
		}

		if (lstrcmpi(argv[i], FlagGetVersionOS) == 0)
		{
			int result = 0;
			const char* message_result = nullptr;
			if (ReportVersionStdout(&result, &message_result, argv))
			{
				if (message_result != nullptr)
				{
					WriteStdout(message_result);
				}
				return result;
			}
			else
			{
				if (message_result != nullptr)
				{
					WriteStderr(message_result);
				}
				return -1;
			}

		}
		
	}

	
}