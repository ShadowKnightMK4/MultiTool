#include "string_tools.h"

/* Why there are 2.
Because for some awful reason 
Visual studio 2022 ses the original one - the killprocess uses that.
And yet doesn't compile it.

StringToNumber_ForKillProcess
*/
bool _cdecl StringToNumber_ForKillProcess(const char* input, int* output)
{
	bool IsPositive = false;
	int space_skipper = 0;
	int mul = 1;
	int len = 0;
	if (input == nullptr)
		return false;
	if (output == nullptr)
		return false;

	// eat any spaces at the start 
	for (;; space_skipper++)
		if ((!(IsSpace(input[space_skipper]))))
		{
			break;
		}

	if (input[space_skipper] == '-')
	{
		IsPositive = false;
		space_skipper++;
	}
	else
	{
		// why not if (x), then if (x) - what if the person passed "+-5" or "-+5"
		if (input[space_skipper] == '+')
		{
			IsPositive = true;
			space_skipper++;
		}
		else
		{
			IsPositive = true;
		}
	}
	for (int i = space_skipper; ; i++)
	{
		if (input[i] == 0)
		{
			// to the future:  we sub 1 from i so the code below DOESN'T start at null
			len = i - 1;
			break;
		}
		else
		{
			if (!IsDigit(input[i]))
			{
				return false;
			}
		}
	}
	if (len == 0)
		return false;
	else
	{
		*output = 0;
		for (int i = len; i >= space_skipper; i--)
		{
			if ((input[i] < '0') || (input[i] > '9'))
			{
				return false;
			}
			else
			{
				*output += (input[i] - '0') * mul;
				mul *= 10;
			}
		}
		if (!IsPositive)
		{
			*output *= -1;
		}
		return true;
	}

}