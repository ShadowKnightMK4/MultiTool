#include "common.h"

extern "C"
{
	bool IsDigit(int c)
	{
		return (c >= '0' && c <= '9');
	}
	/// <summary>
	/// our own is space
	/// </summary>
	/// <param name="c"></param>
	/// <returns></returns>
	bool IsSpace(int c)
	{
		switch (c)
		{
		case ' ':
		case '\t':
		case '\n':
		case '\v':
		case '\f':
		case '\r':
			return true;
		}
		return false;
	}
}