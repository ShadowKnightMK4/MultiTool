#pragma once
#include "common.h"

/// <summary>
/// the plan is simple - you - yes you return true to continue or false to terminate the process.
/// </summary>
typedef BOOL(WINAPI* MinibugCallBack)(DEBUG_EVENT*, VOID* User, DWORD * SendBack);

extern "C"
{
	/// <summary>
	/// Spin up a worker thread, start your target app + args, mode controls action, feeds events to handler
	/// </summary>
	/// <param name="source"></param>
	/// <param name="argument"></param>
	/// <param name="Mode"></param>
	/// <param name="DebugEventHandler"></param>
	/// <returns></returns>
	extern BOOL StartMiniBugServer(const char* source, const char* argument, VOID* Mode, MinibugCallBack DebugEventHandler, HANDLE* WorkerThreadHandler);


	/// <summary>
	/// A default handler to aid in not forgetting to clean up.
	/// </summary>
	/// <param name=""></param>
	/// <returns>true</returns>
	extern BOOL DefaultMiniBugHandler(DEBUG_EVENT*, VOID*, DWORD* SendBack);
}