#pragma once
#include <Windows.h>
extern "C" {
	typedef HANDLE(WINAPI* CreateJobAnsi)(VOID*, VOID*);
	typedef HANDLE(WINAPI* AssignToJob)(HANDLE, HANDLE);
	typedef BOOL(WINAPI* SetJobLimit)(HANDLE, JOBOBJECTINFOCLASS, VOID*, DWORD);

	extern CreateJobAnsi IAT_CreateJobObjectA;
	extern CreateJobAnsi IAT_CreateJobObjectW_DONOTUSEYET;
	extern AssignToJob IAT_AssignProcessToJobObject;
	extern SetJobLimit IAT_SetInformationJobObject;


	/// <summary>
	/// Import routines related to jobs.
	/// </summary>
	/// <param name="IAT_settings">use the IAT_JOB_XXXXX stuff defined</param>
	/// <returns>Returns equivlent flag if set. IE if you pass IAT_JOB_CREATEJOBA and that flag is set on return, then IAT_CreateJobObjectA is set.
	extern DWORD IAT_DynamicLinkJob(DWORD IAT_settings);
	/// <summary>
	/// Freew what IAT_DynamicLinkJob has sewt
	/// </summary>
	/// <returns></returns>
	extern DWORD IAT_DynamicLinkJob_Cleanup();
}
#ifndef IAT_JOB_OBJECT_LINKING
#define IAT_JOB_OBJECT_LINKING
#define IAT_JOB_CREATEJOBA (1)
#define IAT_JOB_CREATEJOBW (2)
#define IAT_JOB_ASSIGN_PROCESS (4)
#define IAT_JOB_SETINFO (8)
#define IAT_JOB_ALL_A  (IAT_JOB_CREATEJOBA |IAT_JOB_ASSIGN_PROCESS | IAT_JOB_SETINFO)
#define IAT_JOB_ALL_W  (IAT_JOB_CREATEJOBW |IAT_JOB_ASSIGN_PROCESS | IAT_JOB_SETINFO)
#endif





