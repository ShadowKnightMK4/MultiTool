#include "common.h"
extern "C" {
	bool StringToNumber(const char* input, int* output)
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
		for (;;space_skipper++)
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
				len = i-1;
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
			for (int i = len; i >= space_skipper ; i--)
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
	bool NumberToString(int number, char** output, int* output_size)
	{
		// yes it's an int. Yes we're treating it like a bool.
		int IsPositive = number > 0;
		// arg validation;
		if (output == nullptr || output_size == nullptr)
		{
			return false;
		}
		*output = 0;
		*output_size = 0;

		int tmp = number;
		// count digits
		int digit_count = 0;

		if (!IsPositive)
			tmp *= -1; // we fix in POST

		// count our digits. we need it to figure the buffer
		while (((char)(tmp & 0xFF)) != 0)
		{
			
			digit_count++;
			if (tmp > 9)
			{
				tmp /= 10;
			}
			else
			{
				break;
			}

		}


		// should we have a digit count of 0, we tick it to 1.
		if (digit_count == 0)
			digit_count++;

		if (!IsPositive)
			digit_count++; // for the minus sign;

		// and allocate + lock a void* to place
		// note the +1 is for the null terminator
		// note the IsPositive note is to add a - sign if the number is negative
		HLOCAL ret_ptr = LocalAlloc(LMEM_ZEROINIT, digit_count + 1 );

		if (ret_ptr == 0)
		{
			return false;
		}
		else
		{
			// grab a pointer if possible and bail if not
			char* ret = (char*)LocalLock(ret_ptr);

			if (ret == 0)
			{
				LocalFree(ret_ptr);
				return false;
			}
			else
			{
				if (!IsPositive)
					tmp = number * -1;
				else
					tmp = number;

				if (tmp != 0)
				{
					while (digit_count > 0)
					{
						char debug = (char)((tmp % 10) + '0');
						// thank you CoPilot
						// what this code here is doing is started from the ones place and moves up taking the remainding of deviing tmp by 10 and add '0' (the string 0) to 
						// get the digit of it. ie
						// the number 5 would end up being '5' when done.
						ret[digit_count - 1] = debug;
						tmp /= 10;
						digit_count--;
						if (!IsPositive)
						{
							if (digit_count == 1)
							{ 
								ret[digit_count - 1] = '-';
								digit_count--;
							}
						}
					}
				}
				else
				{
					// the gui complains of def 0,  but currently if we reach here, Code above should be bailing out before this.
					ret[0] = '0';
				}
			}

			// unlock , set output and return
			LocalUnlock(ret_ptr);
			*output = ret;
			*output_size = digit_count + 1;
			return true;
		}
	}
}