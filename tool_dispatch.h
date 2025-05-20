#pragma once


#ifndef CONTAINS_TOOL_FUNCTION_PTR
typedef bool (*ToolFunction)(int* result, const char** message_result, const char* argv[]);
#define CONTAINS_TOOL_FUNCTION_PTR
#endif

/// <summary>
/// lookup the function pointer for the given flag name. Returns 0 if not found
/// </summary>
/// <param name="flag_name">flag to look up</param>
/// <returns>0 on no match, void* to function of bool name(int* exit_code_to_return, const char*argv[])</returns>
ToolFunction GetFunctionPointer(const char* flag_name);