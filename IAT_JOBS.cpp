#include "common.h"
#include "osver.h"
#include <LWAnsiString.h>


#include "IAT_DLLS.H"
#include "IAT_JOBS.H"


extern "C" {

	 CreateJobAnsi IAT_CreateJobObjectA=0;
	 CreateJobAnsi IAT_CreateJobObjectW_DONOTUSEYET=0;
	 AssignToJob IAT_AssignProcessToJobObject=0;
	 SetJobLimit IAT_SetInformationJobObject=0;

	 DWORD IAT_DynamicLinkJob_Cleanup()
	 {
		 DWORD ret = 0;
		 if (iatKernel32 != 0)
		 {
			 ret =FreeLibrary(iatKernel32);
			 iatKernel32 = 0;
		 }
		 IAT_CreateJobObjectA = nullptr;
		 IAT_AssignProcessToJobObject = IAT_CreateJobObjectW_DONOTUSEYET = nullptr;
		 IAT_SetInformationJobObject = nullptr;
		 return ret;
	 }
	DWORD IAT_DynamicLinkJob(DWORD IAT_settings)
	{
		DWORD ret = 0;
		if (iatKernel32 == 0)
		{
			iatKernel32 = LoadLibraryA("kernel32.dll");
		}

		if (iatKernel32 == 0)
		{
			return 0;
		}

		if ( (IAT_settings & IAT_JOB_CREATEJOBA) == (IAT_JOB_CREATEJOBA))
		{
			IAT_CreateJobObjectA = (CreateJobAnsi) GetProcAddress(iatKernel32, "CreateJobObjectA");
			if (IAT_CreateJobObjectA != 0)
			{
				ret |= IAT_JOB_CREATEJOBA;
			}
		}

		if ((IAT_settings & (IAT_JOB_CREATEJOBW)) == (IAT_JOB_CREATEJOBW))
		{
			IAT_CreateJobObjectA = (CreateJobAnsi)GetProcAddress(iatKernel32, "CreateJobObjectW");
			if (IAT_CreateJobObjectA != 0)
			{
				ret |= IAT_JOB_CREATEJOBW;
			}
		}

		if ((IAT_settings & (IAT_JOB_ASSIGN_PROCESS)) == (IAT_JOB_ASSIGN_PROCESS))
		{
			IAT_AssignProcessToJobObject = (AssignToJob)GetProcAddress(iatKernel32, "AssignProcessToJobObject");
			if (IAT_AssignProcessToJobObject != 0)
			{
				ret |= IAT_JOB_ASSIGN_PROCESS;
			}
		}

		if ((IAT_settings & (IAT_JOB_SETINFO)) == (IAT_JOB_SETINFO))
		{
			IAT_SetInformationJobObject = (SetJobLimit)GetProcAddress(iatKernel32, "SetInformationJobObject");
			if (IAT_SetInformationJobObject != 0)
			{
				ret |= IAT_JOB_SETINFO;
			}
		}
		if (ret != IAT_settings)
		{
			ret = 0;
			IAT_DynamicLinkJob_Cleanup();
		}
		return ret;
	}
 }