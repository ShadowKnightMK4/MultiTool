#pragma once
#ifndef OSVER_H
#define OSVER_H
#include "Windows.h"	
extern bool VERISON_INFO_IS_UNICODE;
extern bool VERSION_INFO_WAS_GOTTON;

typedef union MyOSVERSIONINFO
{
	OSVERSIONINFOA A;
	OSVERSIONINFOW W;
};

/// <summary>
/// If VERSION_INFO_WAS_GOTTON is true, this will be the version info.  If not, this is undefined .
/// </summary>
extern MyOSVERSIONINFO GlobalVersionInfo;


/// <summary>
/// bit of an internal routine.  We copy the version info to a global variable and also set this to the same.  Unicode vs ANSI will state which union part
/// </summary>
/// <param name="Output"></param>
/// <param name="UseUnicode"></param>
/// <returns></returns>
/// <remarks>The variable GlobalVersionInfo is where this will copy its sucesfull result to if you pass something other than that. The osver tool also uses this routine </remarks>
int FetchVersionInfo(MyOSVERSIONINFO* Output, bool* UseUnicode);
#endif
