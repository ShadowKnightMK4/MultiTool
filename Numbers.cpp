#include "common.h"
extern "C" {
	bool NumberToString(int number, char** output, int* output_size)
	{
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

		// and allocate + lock a void* to place
		// note the +1 is for the null terminator
		HLOCAL ret_ptr = LocalAlloc(LMEM_ZEROINIT, digit_count + 1);

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
				tmp = number;
				if (tmp != 0)
				{
					while (digit_count > 0)
					{
						// thank you CoPilot
						// what this code here is doing is started from the ones place and moves up taking the remainding of deviing tmp by 10 and add '0' (the string 0) to 
						// get the digit of it. ie
						// the number 5 would end up being '5' when done.
						ret[digit_count - 1] = (char)((tmp % 10) + '0');
						tmp /= 10;
						digit_count--;
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