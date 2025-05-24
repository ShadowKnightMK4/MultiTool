#pragma once

/*
* include for string_data.h
* 
* This is essentially a central location for all the strings that can be tedious if needing
* to be typed in multiple places. Go here to change the strings if needed (stding_data.cpp)
* 
*/
/// <summary>
/// This is message for when shell32.dll fails to load
/// </summary>
extern const char* Message_CantLoadShell32;
/// <summary>
/// This is a message for when GetProcAddres fails to locate SHEmptyRecycleBinW
/// </summary>
/// 
/// 
extern const char* GetProc_FAIL_ON_ShEmptyBinW;
/// <summary>
/// This is a message for when the recycle bin is emptied successfully
/// </summary>
extern const char* Message_RecycleBin_Empty_Success;

/// <summary>
/// This is the message for when the bin fails to be emptied after a call to SHEmptyRecycleBinW
/// </summary>
extern const char* Message_RecycleBin_Empty_Failure;

extern const char* FlagSilent1;
extern const char* FlagSilent2;
extern const char* FlagHelp1;
extern const char* FlagHelp2;
extern const char* FlagEmptyBin;
extern const char* FlagGetVersionOS;



extern const char* Message_CantLoadKernel32;
extern const char* Message_CantLoadNtdll;

/// <summary>
/// Message displayed if the CLI sees a flag it doesn't know about.
/// </summary>
extern const char* Message_UnknownCommandLineFlag;
