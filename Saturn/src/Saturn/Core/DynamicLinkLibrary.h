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

#if defined(_WIN32)
#include <Windows.h>
using LibraryHandle = HMODULE;
#else
#include <dlfcn.h>
using LibraryHandle = void*;
#endif

namespace Saturn {

	// DynamicLinkLibrary
	// 
	// Encapsulates the low level loading and unloading of dynamic libraries.
	//
	class DynamicLinkLibrary
	{
		SAT_DISABLE_COPY( DynamicLinkLibrary );
	public:
		DynamicLinkLibrary();

		// Automatically calls Free() to release any loaded library.
		~DynamicLinkLibrary();

		// Load the actual .dll file, automatically calls Free() if it's already loaded.
		[[nodiscard]] bool Load( const std::filesystem::path& rPath );
		
		// Unloads the .dll file, if any.
		void Free();

#if defined(_WIN32)
		FARPROC GetSymbol( const char* pName ) const;
#else
		void* GetSymbol( const char* pName );
#endif

		// Set the existing library handle.
		// This function is internal and should not typically be used, however it can be used if you must.
		// Automatically calls Free().
		void SetExisting( LibraryHandle newHandle );

	private:
		// Handle to the platform specific type.
		LibraryHandle m_Handle = nullptr;
	};
}
