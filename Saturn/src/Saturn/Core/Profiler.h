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

#pragma once

#if defined(SAT_DEBUG) || defined(SAT_RELEASE)
#define SAT_PROFILER_ENABLE
#endif

#if defined (SAT_PROFILER_ENABLE)

// Please view, chapter "2.1.5 On-demand profiling" of the Tracy Manual,
// by default Tracy will always accumulate profiling events, even when 
// the Tracy client is not open, this means that we basically have to have
// the client open or deal with the leak.
// 
// We could have the profiler off by default and only enable it when needed
// but I think we'd forget to do that sometimes.
// 
// However, TRACY_ON_DEMAND breaks 0.9.1 of Tracy, but stops the leak
// so, in the future I will update Tracy Client to a newer version.
//
// On demand profiling also has has performance hits:
// 
// "The client with on-demand profiling enabled needs to perform additional 
// bookkeeping to present a coherent application state to the profiler. 
// This incurs additional time costs for each profiling event."
// 
// So maybe disable TRACY_ON_DEMAND if you want and then keep SAT_PROFILER_ENABLE
// off when not profiling.
//
#define TRACY_ON_DEMAND

#include <tracy/Tracy.hpp>

#define SAT_PF_EVENT()       ZoneScoped
#define SAT_PF_EVENT_N(x)    ZoneScopedN(x)
#define SAT_PF_FRAME(x)		 FrameMarkNamed(x)
#else 
#define SAT_PF_EVENT(...)       
#define SAT_PF_EVENT_N(x, ...)  
#define SAT_PF_FRAME(x, ...)	
#endif
