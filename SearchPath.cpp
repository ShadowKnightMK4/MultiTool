#include "common.h"
#include "Support\\LWAnsiString\\LWAnsiString.h">

extern "C" {

	HMODULE Kernel32 = 0;
//#warning SearchPathsForTarget needs check and balances to ensure no acciently buffer overflow
	bool SearchPathsForTarget(const char* filename, LWAnsiString* OutputIfOk, LWAnsiString* OutputIfNotFound)
	{
		DWORD already_happened = 0;
		again:
		SetLastError(0);
		DWORD result = SearchPathA(NULL, filename, NULL, OutputIfOk->AllocatedSize, OutputIfOk->AnsiData, NULL);

		if ((result != 0) &&  (result < OutputIfOk->AllocatedSize))
		{
			LWAnsiString_ProbeLength(OutputIfOk);
			return true;
		}
		else
		{
			if (OutputIfOk->AllocatedSize < result)
			{
				LWAnsiString_AddReserve(OutputIfOk, result );
				if (!already_happened)
				{
					already_happened = true;
					goto again;
				}
				else
				{
					return false;
				}

			}

			if (GetLastError() == ERROR_FILE_NOT_FOUND)
			{
				LWAnsiString_AppendA(OutputIfNotFound, "[NOT FOUND] ");
				LWAnsiString_AppendA(OutputIfNotFound, filename);
			}
			else
			{
				

				LWAnsiString_AppendA(OutputIfNotFound, "[NOT FOUND] ");
				LWAnsiString_AppendA(OutputIfNotFound, filename);
				LWAnsiString_AppendA(OutputIfNotFound, "->last error for search is->");
				int OutputSize = 0;
				LWAnsiString_AppendNumberA(GetLastError(), OutputIfNotFound, &OutputSize);
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
				LWAnsiString_AppendA(MessageResult, "Begining Pathfinding (SearthPathA/W)\r\n");
			}
			int count = 0;

			for (int i = 2; i < argc; i++)
			{
				if (SearchPathsForTarget(argv[i], target, notfound))
				{
					count++;
					LWAnsiString_AppendA(MessageResult, "[Found] ");
					LWAnsiString_AppendA(MessageResult, argv[i]);
					LWAnsiString_AppendA(MessageResult, " At ---> ");
					LWAnsiString_AppendA(MessageResult, LWAnsiString_ToCStr(target));
					LWAnsiString_AppendNewLine(MessageResult);

				}
				else
				{
					LWAnsiString_AppendWithNewLineA(MessageResult, LWAnsiString_ToCStr(notfound));
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