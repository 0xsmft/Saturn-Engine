/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2023 BEAST                                                           		*
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

#pragma once

#include "RubyCore.h"

#include <chrono>

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace Saturn {

	class RubyPerfTimer
	{
	public:
		RubyPerfTimer()
		{
#if defined( _WIN32 )
			QueryPerformanceFrequency( ( LARGE_INTEGER* ) &m_Frequency );
			QueryPerformanceCounter( ( LARGE_INTEGER* ) &m_InitTime );
#endif
		}

		double GetTicks() const
		{
#if defined(_WIN32)
			uint64_t ticks;
			QueryPerformanceCounter( ( LARGE_INTEGER* ) &ticks );
			return ( double ) ( ticks - m_InitTime ) / m_Frequency;
#endif
		}

	private:
#if defined(_WIN32)
		uint64_t m_Frequency = 0;
		uint64_t m_InitTime = 0;
#endif
	};

}