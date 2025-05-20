#include "common.h"
#include <Windows.h>
HANDLE STDIN = 0;
HANDLE STDOUT = 0;
HANDLE STDERR = 0;
void SETUP_PIPES()
{
	STDIN = GetStdHandle(STD_INPUT_HANDLE);
	STDOUT = GetStdHandle(STD_OUTPUT_HANDLE);
	STDERR = GetStdHandle(STD_ERROR_HANDLE);
}