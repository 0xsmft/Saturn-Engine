/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2025 BEAST                                                           *
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

//////////////////////////////////////////////////////////////////////////
// NULL PLATFORM, NOTE: This header does NOT define SAT_PLATFORM_HEADER_DEFINED on purpose because if we get a Null Platform then we don't support that platform.

#if defined(SAT_PLATFORM_HEADER_DEFINED)
#error "SAT_PLATFORM_HEADER_DEFINED was defined before WindowsPlatform.h was included, make sure there is not any other platform header included before the WindowsPlatform header!"
#endif

//////////////////////////////////////////////////////////////////////////

// Standard defines
#define SAT_NOVTABLE 
#define SAT_DLLEXPORT 
#define SAT_DLLIMPORT
#define SAT_NOINLINE
// Windows only defines, however defined on all platforms for compatibility 
#define SAT_ALLOCATOR_ATTR
