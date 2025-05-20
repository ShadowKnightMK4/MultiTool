Welcome to Multitool.


What is it:
	MultiTool is intended to pack as many useful CLI tools as possible into a single app that lives off the land for a poweruser/ dev/ scripting person.

Why do this?
The end goal is demo some of my coding skills and release a live of the land exe file for use in enviroments with minimal overhead. 


Design Contraints (skip if you don't care about dev stuff)

	NO LIBC
	NO C++ Lib
	Raw Windows API
	32bit instread of 64bit (part of the end goal)

	Kernel32 routines are free realestate as it's always loaded.
	ntdll.dll is loaded on most platforms but it's not guarandeed - play defensive

	anything else is only if there's  a finished routine to do it, then it's ok to load and execute.


End goal plan:
	Eventually hit 70+ tools and keep the final release exe under 120kb.
	Unicorn aka reach for the stars goal:  Support Windows ranging from 95 to 11. 

Current tools:
	-EmptyRecyling	-	 load shell32.dll and empty the bin.
	-osver			-	probe for GetVersionEx and conditionally RtlGetVersion.  Write it to the standard output.
	-osMajor		-	produces no stdoutput, returns the OS major version as an exit code
	-osMinor		-	produces no stdoutput, returns the OS minor version as an exit code
	-osBuild		-	produces no stdoutput, returns the OS build version as an ex it code
	-help			-	NOTHING
	--help			-	NOTHING


How to call a tool:
		multitool.exe (or midas if release build)    -toolname

Example:
		midas.exe	-osBuild
		midus.exe	-EmptyRecyling