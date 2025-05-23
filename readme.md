
What is it:
* MultiTool is intended to pack as many useful CLI tools as possible into a single app that lives off the land for a poweruser/ dev/ scripting person.
* Please note the debug build resolves to multitool.exe while the release build resolves to midas.exe.  
* If your grabbing a release from the github section (nothing is there yet), it will be midas.exe.  
Why do this?
* The end goal is demo some of my coding skills and release a live of the land exe file for use in enviroments with minimal overhead. 


Design Contraints (skip if you don't care about dev stuff)
* NO added LIBC
* NO added C++ Lib
* Raw Windows API (C from there is fine)
* 32bit instread of 64bit (part of the end goal)
* Kernel32 routines are free realestate as it's always loaded.
* ntdll.dll is loaded on most platforms but it's not guarandeed - play defensive, we can't assume it exists.
* anything else off limits except if it's a finished routine to do it (like empty recyling bin)


End goal plan:
* Eventually hit 70+ tools and keep the final release exe under 120kb.
* Unicorn aka reach for the stars goal:  Support Windows ranging from 95 to 11. 

Current tools:
*	-EmptyRecyling	    -	 load shell32.dll and empty the bin.
*	-osver			    -	probe for GetVersionEx and conditionally RtlGetVersion.  Write it to the standard output.
*	-osMajor		    -	produces no stdoutput, returns the OS major version as an exit code
*	-osMinor		    -	produces no stdoutput, returns the OS minor version as an exit code
*	-osBuild		    -	produces no stdoutput, returns the OS build version as an exit code

Expirmental tools:
*	-killprocess -PID	-	You need to pass BOTH flags. Pass -killprocess first.   Given a series of process IDs, attempt to force kill them all.
How to call a tool:
*	multitool.exe (or midas if release build)    -toolname

Example:
*	midas.exe	-osBuild
*	midus.exe	-EmptyRecyling