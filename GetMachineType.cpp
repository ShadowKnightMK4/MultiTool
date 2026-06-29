#include "common.h"
#include "PEHeaderHandling.h"
#include "Support/LWAnsiString/LWAnsiString.h"



// USED by GetMachineType to note bad exe
#define HEADER_TYPE_INVALID (0)
// used by GetMachineTypeSupport to not 32-bit header magic id
#define HEADER_TYPE_32BIT (1)
// used by GetMachineTypeSupport to note 64-bit header magic id
#define HEADER_TYPE_64BIT (2)

typedef int HeaderType;

struct machine_type
{
	DWORD type;
	const char* description;
};


#ifndef IMAGE_FILE_MACHINE_ARM64EC
#define IMAGE_FILE_MACHINE_ARM64EC 0xA641
#endif

#ifndef IMAGE_FILE_MACHINE_ARM64X
#define IMAGE_FILE_MACHINE_ARM64X 0xA64E
#endif

#ifndef IMAGE_FILE_MACHINE_RISCV32
#define IMAGE_FILE_MACHINE_RISCV32 0x5032
#endif

#ifndef IMAGE_FILE_MACHINE_RISCV64
#define IMAGE_FILE_MACHINE_RISCV64 0x5064
#endif

#ifndef IMAGE_FILE_MACHINE_RISCV128
#define IMAGE_FILE_MACHINE_RISCV128 0x5128
#endif

#ifndef IMAGE_FILE_MACHINE_LOONGARCH32
#define IMAGE_FILE_MACHINE_LOONGARCH32 0x6232
#endif

#ifndef IMAGE_FILE_MACHINE_LOONGARCH64
#define IMAGE_FILE_MACHINE_LOONGARCH64 0x6264
#endif

#ifndef IMAGE_FILE_MACHINE_R3000BE
#define IMAGE_FILE_MACHINE_R3000BE 0x160
#endif

#define machine_type_eol ((DWORD)-1)
machine_type lookup_index[] =
{
	{ IMAGE_FILE_MACHINE_UNKNOWN,    "Unknown machine" },

	{ IMAGE_FILE_MACHINE_I386,       "Intel 386 (x86)" },
	{ IMAGE_FILE_MACHINE_AMD64,      "x64 (AMD64)" },

	{ IMAGE_FILE_MACHINE_ARM,        "ARM (little endian)" },
	{ IMAGE_FILE_MACHINE_ARMNT,      "ARM Thumb-2 (ARMv7)" },
	{ IMAGE_FILE_MACHINE_ARM64,      "ARM64 (AArch64)" },
	{ IMAGE_FILE_MACHINE_ARM64EC,    "ARM64EC (x64 compat on ARM64)" },
	{ IMAGE_FILE_MACHINE_ARM64X,     "ARM64X (hybrid ARM64/x64 binary)" },

	{ IMAGE_FILE_MACHINE_IA64,       "Intel Itanium (IA-64)" },

	{ IMAGE_FILE_MACHINE_ALPHA,      "DEC Alpha AXP (32-bit)" },
	{ IMAGE_FILE_MACHINE_ALPHA64,    "DEC Alpha AXP (64-bit)" },
	{ IMAGE_FILE_MACHINE_AXP64,      "DEC Alpha AXP (64-bit alias)" },

	{ IMAGE_FILE_MACHINE_MIPS16,     "MIPS16" },
	{ IMAGE_FILE_MACHINE_MIPSFPU,    "MIPS with FPU" },
	{ IMAGE_FILE_MACHINE_MIPSFPU16,  "MIPS16 with FPU" },
	{ IMAGE_FILE_MACHINE_R3000,      "MIPS R3000 (little endian)" },
	{ IMAGE_FILE_MACHINE_R3000BE,    "MIPS R3000 (big endian)" },
	{ IMAGE_FILE_MACHINE_R4000,      "MIPS R4000" },
	{ IMAGE_FILE_MACHINE_R10000,     "MIPS R10000" },
	{ IMAGE_FILE_MACHINE_WCEMIPSV2,  "MIPS WinCE v2" },

	{ IMAGE_FILE_MACHINE_POWERPC,    "PowerPC (little endian)" },
	{ IMAGE_FILE_MACHINE_POWERPCFP,  "PowerPC with FPU" },

	{ IMAGE_FILE_MACHINE_SH3,        "Hitachi SH3" },
	{ IMAGE_FILE_MACHINE_SH3DSP,     "Hitachi SH3 DSP" },
	{ IMAGE_FILE_MACHINE_SH4,        "Hitachi SH4" },
	{ IMAGE_FILE_MACHINE_SH5,        "Hitachi SH5" },

	{ IMAGE_FILE_MACHINE_THUMB,      "ARM Thumb" },

	{ IMAGE_FILE_MACHINE_EBC,        "EFI Byte Code (EBC)" },

	{ IMAGE_FILE_MACHINE_RISCV32,    "RISC-V 32-bit" },
	{ IMAGE_FILE_MACHINE_RISCV64,    "RISC-V 64-bit" },
	{ IMAGE_FILE_MACHINE_RISCV128,   "RISC-V 128-bit" },

	{ IMAGE_FILE_MACHINE_LOONGARCH32,"LoongArch 32-bit" },
	{ IMAGE_FILE_MACHINE_LOONGARCH64,"LoongArch 64-bit" },

	{ IMAGE_FILE_MACHINE_AM33,       "Matsushita AM33" },

	{ machine_type_eol, NULL}
};
extern "C" {
	bool ShowMachineType(int* result, const char** message_result, const char* argv[], int argc)
	{
		HeaderType Kind = 0;
		if (argc < 3)
		{
			return false;
		}
		else
		{
			LWAnsiString* Output = LWAnsiString_CreateString(255);
			const char* use_string = 0;
			if (Output != 0)
			{
				DWORD MachineType = GetMachineTypeSupport(&Kind, argv[2]);

				for (int i = 0; ; i++)
				{
					if (lookup_index[i].type == machine_type_eol)
					{
						break;
					}
					else
					{
						if (lookup_index[i].type == MachineType)
						{
							use_string = lookup_index[i].description;
							break;
						}
					}
				}
				if (use_string == 0)
				{
					LWAnsiString_AppendWithNewLineA(Output, "MachineType of the PE file unknown or not defined yet in midas.");
				}
				else
				{
					LWAnsiString_AppendA(Output, "MachineType of the PE File is ");
					LWAnsiString_AppendA(Output, use_string);
					LWAnsiString_AppendWithNewLineA(Output, ".");
				}

				WriteStdout(Output);
				LWAnsiString_FreeString(Output);
				return true;
			}
			else
			{
				return false;
			}

		}
	}

	BOOL GetFileSizeShim(PLARGE_INTEGER x, HANDLE FN)
	{
		if (!x)
		{
			return FALSE;
		}
		else
		{
			if (FN == INVALID_HANDLE_VALUE)
			{
				return FALSE;
			}
			if (FN == 0)
			{
				return FALSE;
			}

			(x)->HighPart = (x)->LowPart = 0;
			SetLastError(0);
			DWORD ret = GetFileSize(FN, (DWORD*)x + offsetof(LARGE_INTEGER, HighPart));

			if (ret != INVALID_FILE_SIZE)
			{
				(x)->LowPart = ret;
				return TRUE;
			}
			else
			{
				if (GetLastError() != NO_ERROR)
				{
					return FALSE;
				}
				else
				{
					(x)->LowPart = ret;
					return TRUE;
				}
				return FALSE;
			}
		}
	}
	/// <summary>
	/// Returns the machine type, and which PE header we found
	/// </summary>
	/// <param name="Kind"></param>
	/// <returns></returns>
	DWORD GetMachineTypeSupport(HeaderType* Kind, const char* ExeOnDisk)
	{
		if (ExeOnDisk == 0)
			return 0;
		if (Kind != 0)
		{
			*Kind = HEADER_TYPE_INVALID;
		}
		DWORD MachineType = 0;
		union shim
		{
			VOID* TheVoid;
			IMAGE_DOS_HEADER* DosHeader;
			IMAGE_NT_HEADERS32* Pe32;
			IMAGE_NT_HEADERS64* Pe64;
			IMAGE_OPTIONAL_HEADER32* Op32;
			IMAGE_OPTIONAL_HEADER64* Op64;
			unsigned char* ByteList;
		} MagicPtr;
		MagicPtr.TheVoid = (VOID*)0;


		HANDLE FileHandle = INVALID_HANDLE_VALUE;
		HANDLE Mapped = 0;
		LPCVOID MapData = 0;
		LARGE_INTEGER FileSize;
		FileSize.HighPart = FileSize.LowPart = 0;

		FileHandle = CreateFileA(ExeOnDisk, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (FileHandle != INVALID_HANDLE_VALUE)
		{
			Mapped = CreateFileMappingA(FileHandle, 0, PAGE_READONLY, 0, 0, 0);
			if (Mapped != 0)
			{
				MapData = MapViewOfFile(Mapped, FILE_MAP_READ, 0, 0, 0);
				if (MapData != 0)
				{
					// The base image;
					MagicPtr.TheVoid = (VOID*)MapData;
					if (MagicPtr.DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
					{
						// ABORT - NOT A EXE
						goto safe_clean;
					}
					else
					{

						// OK we're exe.
						MagicPtr.ByteList += MagicPtr.DosHeader->e_lfanew;
						if (!GetFileSizeShim(&FileSize, FileHandle))
						{
							// no way to ensure we don't read pass eof in a malice mapped exe/dll
							goto safe_clean;
						}
						if ((LONGLONG) MagicPtr.ByteList > FileSize.QuadPart)
						{
							// possible malice exe/dll, ABORT
							goto safe_clean;
						}

						if (MagicPtr.Pe32->Signature != IMAGE_NT_SIGNATURE)
						{
							// ABORT - not an exe
							goto safe_clean;
						}
						else
						{

							if (MagicPtr.Pe64->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
							{
								if (Kind != 0)
								{
									*Kind = HEADER_TYPE_64BIT;
								}
								MachineType = MagicPtr.Pe64->FileHeader.Machine;
							}
							else if (MagicPtr.Pe32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
							{
								if (Kind != 0)
								{
									*Kind = HEADER_TYPE_32BIT;
								}
								MachineType = MagicPtr.Pe32->FileHeader.Machine;
							}
							else
							{
								if (Kind != 0)
								{
									*Kind = HEADER_TYPE_INVALID;
								}
								MachineType = 0;
							}

						}

					}

					UnmapViewOfFile(MapData);
					MapData = 0;
				}
				CloseHandle(Mapped);
				Mapped = 0;
			}
			CloseHandle(FileHandle);
			FileHandle = 0;


			return MachineType;
		}


	safe_clean:
		if (MapData != 0)
		{
			UnmapViewOfFile(MapData);
		}
		if (Mapped != 0)
		{
			CloseHandle(Mapped);
		}
		if (FileHandle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(FileHandle);
		}
		return 0;

	}



}