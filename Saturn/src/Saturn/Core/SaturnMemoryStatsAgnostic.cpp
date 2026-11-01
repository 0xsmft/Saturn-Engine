/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2026 BEAST                                                           *
*                                                                                           *
* Permission is hereby granted, free of charge, to any person obtaining a copy              *
* of this software and associated documentation files (the "Software"), to deal             *
* in the Software without restriction, including without limitation the rights              *
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell                 *
* copies of the Software, and to permit persons to whom the Software is                     *
* furnished to do so, subject to the following conditions:                                  *
*                                                                                           *
* The above copyright notice and this permission notice shall be included in all            *
* copies or substantial portions of the Software.                                           *
*                                                                                           *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR                *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,                  *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE               *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER                    *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,             *
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE             *
* SOFTWARE.                                                                                 *
*********************************************************************************************
*/

#include "sppch.h"
#include "SaturnMemoryStatsAgnostic.h"

#if defined(SAT_PLATFORM_WINDOWS)
#include <Psapi.h>
#else
#include <sys/resource.h>
#endif

#if defined(SAT_PLATFORM_MACOS)
#include <mach/mach.h>
#endif

namespace Saturn {

#if defined(SAT_PLATFORM_LINUX)
	static uint64_t GetWorkingSet()
	{
		uint64_t workingSet = 0llu;

		std::ifstream statmFile( "/proc/self/statm" );
		if( statmFile.is_open() )
		{
			uint64_t size = 0llu;
			uint64_t resident = 0llu;
			statmFile >> size >> resident;
			workingSet = resident * sysconf( _SC_PAGESIZE );
		}
		
		return workingSet;
	}
#elif defined(SAT_PLATFORM_MACOS)
	static uint64_t GetWorkingSet()
	{
		struct mach_task_basic_info info;
		mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
		
		if( task_info( mach_task_self(), MACH_TASK_BASIC_INFO, ( mach_msg_type_number_t* ) &info, &infoCount ) == KERN_SUCCESS )
		{
			return info.resident_size;
		}

		return 0llu;
	}
#endif

#if defined(SAT_PLATFORM_WINDOWS)
	static SaturnMemoryStatistics SmsGetForWin32() 
	{
		PROCESS_MEMORY_COUNTERS outData{};
		if( !::GetProcessMemoryInfo( ::GetCurrentProcess(), &outData, sizeof( PROCESS_MEMORY_COUNTERS ) ) )
			return {};

		SaturnMemoryStatistics sms{};
		sms.PageFaultCount    = outData.PageFaultCount;
		sms.PeakWorkingSet    = outData.PeakWorkingSetSize;
		sms.CurrentWorkingSet = outData.WorkingSetSize;

		return sms;
	}
#else
	static SaturnMemoryStatistics SmsGetForPOSIX() 
	{
		struct rusage usageData{};
		if( ::getrusage( RUSAGE_SELF, &usageData ) != 0 )
			return {};

		SaturnMemoryStatistics sms{};
		sms.PageFaultCount    = usageData.ru_majflt;

#if defined(SAT_PLATFORM_MACOS)
		sms.PeakWorkingSet    = usageData.ru_maxrss;
#else
		// On linux, ru_maxrss is in kilobytes, but we want it
		// in megabytes to match with Windows and macOS.
		sms.PeakWorkingSet    = usageData.ru_maxrss * 1024llu;
#endif

		sms.CurrentWorkingSet = GetWorkingSet();

		return sms;
	}
#endif

	SaturnMemoryStatistics SaturnMemoryStatisticsHelper::Query()
	{
#if defined(SAT_PLATFORM_WINDOWS)
		return SmsGetForWin32();
#else
		return SmsGetForPOSIX();
#endif
	}

}
