#include <windows.h>
#include "common.h"
#include "tool_functions.h"
#include "tool_dispatch.h"

extern "C" {
#ifdef EXPERIMENT
#ifdef DEBUG
	#define WaterMarkString  "DebugBuild,ExperimentOn";
#else
#error AS a preference, release build and EXPERIMENT flag not supported.
#define  WaterMarkString  "WARNINGReleaseBuild,ExperimentOn";
#endif 


#else
#ifdef DEBUG
#define  WaterMarkString "DebugBuild,ExperimentOff";
#else
#define  WaterMarkString "ReleaseBuild,ExperimentOff";

#endif
#endif
}

extern "C" __declspec(dllexport) const char WaterMark[] = WaterMarkString;
#undef WaterMarkString

int main(int argc, const char* argv[])
{
	if (argc < 2)
	{
		return -1;
	}

	// first check for the help flag;
	bool HelpWanted = false;
	for (int i = 1; i < argc; i++)
	{
		if (lstrcmpiA(argv[i], "-help") == 0)
		{
			HelpWanted = true;
			break;
		}
	}

	for (int i = 1; i < argc; i++)
	{
		const char* current = argv[i];
		if (HelpWanted)
		{
			if (lstrcmpiA(argv[i], "-help") != 0)
			{
				const char* help_text = GetFunctionHelp(current);
				if (help_text != nullptr)
				{
					WriteStdout(help_text);
				}
				else
				{
					WriteStderr(Message_UnknownCommandLineFlag);
				}
			}
			continue;
		}
		else
		{
			ToolFunction function = (ToolFunction)GetFunctionPointer(current);
			if (function != 0)
			{
				int result = 0;
				const char* message_result = nullptr;
				bool res = function(&result, &message_result, argv, argc);
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
				WriteStderr(Message_UnknownCommandLineFlag);
				return -1;
			}
		}
		break;
	}

	
}