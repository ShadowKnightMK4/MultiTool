

***

# Midas / MultiTool
**A "Live-Off-The-Land", CRT-Independent OS Interrogation & Poweruser Toolkit**

**Midas** (Release build) / **MultiTool** (Debug build) is a command-line Swiss Army knife being designed to pack as many useful low-level Windows tools as possible into a few, ultra-lightweight executables. 

Built for powerusers, sysadmins, and researchers, Midas is being designed to "live off the land" in environments with minimal overhead. It interacts directly with the Windows API at user level (ring 3) to provide raw, unfiltered insights into the operating system, process profiling, and security token interrogation.

## 🎯 The "Why" and Project Goals
The primary goal of this project is to demonstrate advanced coding skills by building a highly capable, zero-dependency executable for restrictive environments.

*   **The End Goal:** Pack 70+ useful CLI tools into a single release executable while keeping the final file size **under 120kb**.
*   **The Unicorn Goal:** Flawless execution across the entire modern Windows lineage—from **Windows 95 to Windows 11**.


## ⚠️ Disclaimer
**For Educational and Defensive Analysis Purposes Only.**
Midas is a low-level system interrogation tool. It interacts heavily with Windows security tokens, the debugging API, and raw memory manipulation. This tool will **not** ship with methods to try to trick Windows to gain access beyond what the user is logged in as. This toolkit is provided "as-is". You are solely responsible for ensuring that your use of this tool complies with all applicable local, state, and federal laws. Do not use this tool on systems or networks you do not own or have explicit authorization to audit. Additionally, Midas is and likely will always be a work in progress of some extent. It's just as much a learning tool for me.


---

## ⚙️ Design Constraints & Philosophy (The "Dev Stuff")
To achieve the extreme size and compatibility goals, this project strictly adheres to the following constraints:

*   **NO added LIBC & NO C++ Standard Library:** The tool completely rejects modern C-Runtime (CRT) abstractions, STL, RAII, and C++ exceptions to eliminate dependency hell and binary bloat.
*   **Raw Windows API Only:** C and a dash of C++ . Memory is managed via `LocalAlloc`/`HeapAlloc`, and I/O is piped manually.
*   **32-Bit Architecture:** Compiled exclusively as 32-bit to ensure execution on legacy 32-bit and modern 64-bit (WoW64) operating systems alike.
*   **Defensive API Loading:** 
    *   `kernel32.dll` is considered free real estate (always loaded) but the preference is late binding (GetProcAddress) to load routines as needed.
    *   `ntdll.dll` is never guaranteed. Midas plays defensively, dynamically probing for APIs like `RtlGetVersion` and degrading gracefully if they don't exist.
    *   Other DLLs (like `advapi32.dll` or `shell32.dll`) are loaded dynamically only when executing a specific finished routine (e.g., emptying the recycle bin or checking tokens).
*   **Zero-Trust Analysis:** Tools like `-whichdll` force the OS to resolve paths inside a heavily restricted Mini-Debugger sandbox (Job Objects and UI restrictions if supported, `KILL_ON_JOB_CLOSE`) using the Windows Debugger API to  help analyze potentially malicious payloads.

---

## 🛠️ Current Tool List & Capabilities

*Note: Debug builds resolve to `multitool.exe`, while Release builds resolve to `midas.exe`.*

Short note: tools tagged as **(Experimental)**  require to be built with the Experimental MACRO.

### Process & Dependency Analysis
| Command | Description |
| :--- | :--- |
| `-whichdll [dll_name]` | **(Experimental)** Uses a custom Mini-Debugger to force the Windows OS Loader to resolve a DLL's true path based on complex search orders. Operates safely within a zero-trust sandbox. |
| `-processprofile` | Walks the modules and heaps of the target process to find the main executable spawn point and flags security vulnerabilities (e.g., writable main executable folders, DLL hijacking risks). |
| `-killprocess -PID <id>` | **(Requires both flags)** Aggressively terminates target processes. Plays nice first via standard termination, then falls back to a "Sniper" method (attaching as a debugger and exiting to force a kernel-level kill). |
| `-which [name]` | Wrapper around `SearchPathA`. Asks Windows directly where a specific file or folder is located across the system's path environment. |

### Security & Token Interrogation
| Command | Description |
| :--- | :--- |
| `-whoami` | Performs a comprehensive dump of the current process/user security token. Outputs SIDs, Group attributes, and Token Elevation details. |
| `-whoami_priv` | Extracts and parses the specific privileges assigned to the application's token. Maps privileges against a custom SOC risk matrix to flag dangerous abilities (e.g., `SeDebugPrivilege`). |
| `-elevated` | Queries the Token Elevation Type to determine if the application is running with full Administrator privileges, limited privileges, or default scoping. |
| `-whoami_user_group` | Outputs the specific User Group information and attributes mapped to the current token. |
| `-checkSafeLoadPath` | Verifies if `SafeDllSearchMode` is active in the Windows Registry. Flags systems that are vulnerable to decoy DLL-hijacking. |
| `-whoami_priv_system`| **(Experimental)** Hard-hooks into `winlogon.exe` to extract and analyze a raw `SYSTEM` level token. |

### System & OS Information
| Command | Description |
| :--- | :--- |
| `-OsVer` | Reports the exact version of Windows running to `stdout`. Bypasses Windows 8.1+ manifest lies by probing `ntdll.dll` for `RtlGetVersion` if necessary. |
| `-osMajor` | Silently reports the Major version of Windows via the process Exit Code. |
| `-osMinor` | Silently reports the Minor version of Windows via the process Exit Code. |
| `-osBuild` | Silently reports the OS Build version via the process Exit Code. |
| `-osPlatform` | Silently reports the OS Platform ID (NT-based vs older Win9x) via the process Exit Code. |
| `-upTime` | Reports exactly how long the system has been running (in milliseconds) to `stdout`. |
| `-upTimeExitCode`| Silently reports the system uptime via the process Exit Code (capped to 32-bit). |

### Utilities
| Command | Description |
| :--- | :--- |
| `-EmptyRecyling` | Loads `shell32.dll` to empty the Windows Recycle Bin programmatically. |
| `-?` or `-help` | Displays the help menu and a list of all available commands. |

---

## 🚀 Usage

Execute `midas.exe` (Release) or `multitool.exe` (Debug) from the command line followed by the desired tool flag.

**Basic Usage:**
```cmd
midas.exe -toolname [optional_arguments]
```

**Examples:**
```cmd
:: Dump detailed token privileges and SOC risk ratings
midas.exe -whoami_priv

:: Safely resolve the true OS load path of a DLL using the MiniBug sandbox
midas.exe -whichdll gdi32.dll

:: Empty the recycle bin silently
midas.exe -EmptyRecyling --silent

:: Force-kill a stubborn process by PID
midas.exe -killprocess -PID 4094

:: Check the major OS version via exit code
midas.exe -osMajor
echo %errorlevel%
```


## 🧪 Advanced Unit Testing and Building
Despite being a zero-dependency C/C++ application, testing Midas utilizes the **Microsoft Detours** library for advanced unit testing, and the testes need not skip libc/stl. Tests can actively hook and mock core OS APIs (such as `LoadLibraryA` and `GetProcAddress`) to simulate extreme edge-cases—such as missing system DLLs on ancient operating systems—ensuring the application degrades gracefully without catastrophic failure.
Building Midas yields the apps below. It iw

midas.exe - the main tool
freeze32.exe / freeze64.exe - a currently standalone debug attach and exit. There's plans for this to be used for when 32-bit midas use attempting to stop a 64-bit app.
minibugclient.exe - the app midas.exe uses to contain dlls that are loaded that might not be safe to directly load in midas. It spawns this and attaches to debug it.

total size of release build 3/24/26 -> approx 40kb.