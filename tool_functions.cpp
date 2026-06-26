#include "common.h"
#include "tool_functions.h"
bool SILENCE = false;

/*
* Noteice most of the tool functions are moved to be defined in individual source files such as killprocess, osver and recylcinbin
*/
extern "C" {
	bool ShowInternalPointerSize(int* result, const char** message_result, const char* argv[], int argc)
	{
		/* we liternally just do size(INT*) */
		const char* size_str;

		switch (sizeof(INT*))
		{
			case 4: size_str = "4\0";
			case 8: size_str = "8\0";
			default: size_str = NULL;
		}


		WriteStdout("Self Pointer Size is ");
		if (size_str == 0)
		{
			WriteStdout("unknown");
		}
		else
		{
			WriteStdout(size_str);
		}
		WriteStdout(".\r\n");
		return true;
	}
}