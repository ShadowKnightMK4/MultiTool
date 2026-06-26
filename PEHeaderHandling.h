#pragma once


// USED by GetMachineType to note bad exe
#define HEADER_TYPE_INVALID (0)
// used by GetMachineTypeSupport to not 32-bit header magic id
#define HEADER_TYPE_32BIT (1)
// used by GetMachineTypeSupport to note 64-bit header magic id
#define HEADER_TYPE_64BIT (2)

typedef int HeaderType;

extern "C" {
	/// <summary>
	/// Returns the machine type, and which PE header we found
	/// </summary>
	/// <param name="Kind">can be null. IF NOT int sized value is written (HEADER_TYPE_INVALID, TYPE 32BIT , TYPE_64BT</param>
	/// <returns>Speficially returns the part of the image file header of the loaded exe if the header magic vlaues match that specs the machine type</returns>
	DWORD GetMachineTypeSupport(HeaderType* Kind, const char* ExeOnDisk);


	/// <summary>
	/// When given a target PE/DLL on the desk, read what the value of it's machine type is.
	/// </summary>
	/// <param name="result"></param>
	/// <param name="message_result"></param>
	/// <param name="argv">always reads argv[2]</param>
	/// <param name="argc"></param>
	/// <returns></returns>
	bool ShowMachineType(int* result, const char** message_result, const char* argv[], int argc);
}