#include "common.h"
#include <LWAnsiString.h>

extern "C" {

	HMODULE Kernel32 = 0;

	bool SearchPathsForTarget(const char* filename, LWAnsiString* OutputIfOk, LWAnsiString* OutputIfNotFound)
	{
		SetLastError(0);
		DWORD result = SearchPathA(NULL, filename, NULL, OutputIfOk->AllocatedSize, OutputIfOk->Data, NULL);
		if (result != 0)
		{
			LWAnsiString_ProbeLength(OutputIfOk);
			return true;
		}
		else
		{
			if (GetLastError() == ERROR_FILE_NOT_FOUND)
			{
				LWAnsiString_Append(OutputIfNotFound, "[NOT FOUND] ");
				LWAnsiString_Append(OutputIfNotFound, filename);
			}
			else
			{
				LWAnsiString_Append(OutputIfNotFound, "[NOT FOUND] ");
				LWAnsiString_Append(OutputIfNotFound, filename);
				LWAnsiString_Append(OutputIfNotFound, "->last error for search is->");
				int OutputSize = 0;
				LWAnsiString_AppendNumber(GetLastError(), OutputIfNotFound, &OutputSize);
				LWAnsiString_AppendNewLine(OutputIfNotFound);
			}
			return false;
		}
	}

	bool SearchPath_EntryPoint(int* result, const char** message_result, const char* argv[], int argc)
	{
		if ((result == 0) && (message_result == 0))
			return false;

		LWAnsiString* MessageResult = LWAnsiString_CreateString(1);
		LWAnsiString* target = LWAnsiString_CreateString(MAX_PATH);
		LWAnsiString* notfound = LWAnsiString_CreateString(1);
		
		if (target != nullptr)
		{
			if ((MessageResult == 0) || (target == 0) || (notfound == 0))
			{
				*message_result = "Out of Memory for this action";
			}
			else
			{
				LWAnsiString_Append(MessageResult, "Begining Pathfinding (SearthPathA/W)\r\n");
			}
			int count = 0;

			for (int i = 2; i < argc; i++)
			{
				if (SearchPathsForTarget(argv[i], target, notfound))
				{
					count++;
					LWAnsiString_Append(MessageResult, "[Found] ");
					LWAnsiString_Append(MessageResult, argv[i]);
					LWAnsiString_Append(MessageResult, " At ---> ");
					LWAnsiString_Append(MessageResult, LWAnsiString_ToCStr(target));
					LWAnsiString_AppendNewLine(MessageResult);

				}
				else
				{
					LWAnsiString_AppendWithNewLine(MessageResult, LWAnsiString_ToCStr(notfound));
				}
				LWAnsiString_ZeroString(target);
				LWAnsiString_ZeroString(notfound);
			}
			WriteStdout(LWAnsiString_ToCStr(MessageResult));
		 	if (target != nullptr) LWAnsiString_FreeString(target);
			if (MessageResult != nullptr) LWAnsiString_FreeString(MessageResult);
			if (notfound != nullptr) LWAnsiString_FreeString(notfound);
			return true;
		}
		return false;
	}
}