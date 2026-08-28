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

#if defined(SAT_PLATFORM_HEADER_DEFINED)
#error "SAT_PLATFORM_HEADER_DEFINED was defined before WindowsPlatform.h was included, make sure there is not any other platform header included before the WindowsPlatform header!"
#endif

//////////////////////////////////////////////////////////////////////////

#define SAT_PLATFORM_HEADER_DEFINED 1

// Standard defines
#define SAT_NOVTABLE  __declspec(novtable)
#define SAT_NOINLINE  __declspec(noinline) 
// Windows only defines, however defined on all platforms for compatibility 
#define SAT_ALLOCATOR_ATTR __declspec(allocator)
#define SAT_PLATFORM_FRIENDLY_NAME "Windows"

#if !defined(SAT_DIST)
#define SAT_DLLEXPORT __declspec(dllexport)
#define SAT_DLLIMPORT __declspec(dllimport)
#else
#define SAT_DLLEXPORT
#define SAT_DLLIMPORT
#endif

// The vulkan surface extension name.
#define SAT_PLATFORM_VULKAN_SURFACE_NAME "VK_KHR_win32_surface"

#define SAT_PLATFORM_DYNALIB_FILE_EXT ".dll"
#define SAT_PLATFORM_STLIB_FILE_EXT ".lib"
#define SAT_PLATFORM_EXE_FILE_EXT ".exe"

#define SAT_CLANG_TYPENAME 
#define SAT_GCC_TYPENAME 
#define SAT_MSVC_TYPENAME typename

// We only support x86_64
#if defined(_MSC_VER) && defined(_M_X64) || defined(__x86_64__)
#define SAT_PLATFORM_BINARY_FOLDER "windows-x86_64"
#elif defined(_MSC_VER) && defined(_M_ARM64)
#define SAT_PLATFORM_BINARY_FOLDER "windows-AARCH64"
#else
#define SAT_PLATFORM_BINARY_FOLDER "windows-ArchUnk"
#endif

// SAT_PLATFORM_WINDOWS is defined from CLI
