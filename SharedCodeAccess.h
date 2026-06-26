#pragma once
#include <Windows.h>

extern "C" {
	/// <summary>
	/// Use the minibug client build into Midas.exe you will need to a sub to call the code. Format is return a string \""ignored" target-dll-to-load\"
	/// </summary>
	/// <param name="CmdLineGet"></param>
	extern void StartMiniBug(LPCSTR ParseMe);

}
