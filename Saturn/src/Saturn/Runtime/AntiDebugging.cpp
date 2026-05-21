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
#include "AntiDebugging.h"

#if defined(SAT_PLATFORM_WINDOWS)
#include <winternl.h>
#endif

namespace Saturn {

#if defined(SAT_PLATFORM_WINDOWS)
	// This is by NO means perfect
	// Any good reserve engineer would be able to patch this out.
	static bool PltfCheckDebugger()
	{
		int remoteDbg = 0;
		::CheckRemoteDebuggerPresent( ::GetCurrentProcess(), &remoteDbg );

		const int pebSaysDebugging = ::NtCurrentTeb()->ProcessEnvironmentBlock->BeingDebugged;
		return ::IsDebuggerPresent() || ( remoteDbg == 1 ) || ( pebSaysDebugging == 1 );
	}
#else
	static bool PltfCheckDebugger() { return false; }
#endif

	bool AntiDebugging::CheckForDebugger()
	{
		return PltfCheckDebugger();
	}

}
