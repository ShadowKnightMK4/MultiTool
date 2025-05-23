#pragma once
#include "common.h"
extern "C" {
	extern bool EmptyBin(int* result, const char** message_result, const char* argv[],  int argc);

	/// <summary>
	/// Reports the OS version data to stdout.
	/// </summary>
	/// <param name="result"></param>
	/// <param name="message_result"></param>
	/// <param name="argv"></param>
	/// <returns></returns>
	extern bool ReportVersionStdout(int* result, const char** message_result, const char* argv[],  int argc);
	extern bool ReportVersionPlatformIDViaExit(int* result, const char** message_result, const char* argv[], int argc);
	extern bool ReportVersionBuildViaExit(int* result, const char** message_result, const char* argv[], int argc);
	extern bool ReportVersionMinorViaExit(int* result, const char** message_result, const char* argv[], int argc);
	extern bool ReportVersionMajorViaExit(int* result, const char** message_result, const char* argv[], int argc);

	extern bool KillProcess(int* result, const char** message_result, const char* argv[], int argc);
}



/// <summary>
/// If true , the tool and this app should not procude output beyond exit code
/// </summary>
extern bool SILENCE;

extern bool VERISON_INFO_IS_UNICODE = false;
extern bool VERSION_INFO_WAS_GOTTON = false;

extern MyOSVERSIONINFO GlobalVersionInfo = { 0, 0, 0, 0 };