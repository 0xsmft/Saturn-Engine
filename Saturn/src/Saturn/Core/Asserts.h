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

#ifndef __CORE_INCLUDED__
#error Include Core.h before including Asserts.h!
#endif // !__CORE_INCLUDE

#define _VA_AGRS_( x ) x

#if defined ( SAT_DEBUG ) || defined ( SAT_RELEASE )
#define SAT_ASSERT_NO_MESSAGE(condition) { if(!(condition)) { SAT_CORE_ERROR("Assertion Failed: {2}, Line {0}, File, {1}", __LINE__, __FILE__, #condition); Saturn::Core::BreakDebug(); } }
#define SAT_ASSERT_MESSAGE(condition, ...) { if(!(condition)) { SAT_CORE_ERROR("Assertion Failed: {2}, Line {0}, File, {1}", __LINE__, __FILE__, __VA_ARGS__); Saturn::Core::BreakDebug(); } }

#define SAT_ASSERT_RESOLVE(arg1, arg2, macro, ...) macro
#define SAT_GET_ASSERT_MACRO(...) _VA_AGRS_(SAT_ASSERT_RESOLVE(__VA_ARGS__, SAT_ASSERT_MESSAGE, SAT_ASSERT_NO_MESSAGE))

#define SAT_ASSERT(...) _VA_AGRS_( SAT_GET_ASSERT_MACRO(__VA_ARGS__)(__VA_ARGS__) )
#define SAT_CORE_ASSERT(...) _VA_AGRS_( SAT_GET_ASSERT_MACRO(__VA_ARGS__)(__VA_ARGS__) )
#else
#define SAT_ASSERT(...)
#define SAT_CORE_ASSERT(...)
#endif