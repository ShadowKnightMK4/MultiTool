#pragma once
#include "common.h"
extern "C" {
	extern bool EmptyBin(int* result, const char** message_result, const char* argv[]);

	extern bool ReportVersionStdout(int* result, const char** message_result, const char* argv[]);
	extern bool ReportVersionPlatformIDViaExit(int* result, const char** message_result, const char* argv[]);
	extern bool ReportVersionBuildViaExit(int* result, const char** message_result, const char* argv[]);
	extern bool ReportVersionMinorViaExit(int* result, const char** message_result, const char* argv[]);
	extern bool ReportVersionMajorViaExit(int* result, const char** message_result, const char* argv[]);

}


/// <summary>
/// If true , the tool and this app should not procude output beyond exit code
/// </summary>
extern bool SILENCE;