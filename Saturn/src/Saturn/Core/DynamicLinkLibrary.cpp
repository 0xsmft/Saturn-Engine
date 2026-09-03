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
#include "DynamicLinkLibrary.h"

namespace Saturn {

	DynamicLinkLibrary::DynamicLinkLibrary()
	{
	}

	DynamicLinkLibrary::~DynamicLinkLibrary()
	{
		Free();
	}

	bool DynamicLinkLibrary::Load( const std::filesystem::path& rPath )
	{
		if( m_Handle ) 
			Free();

#if defined(_WIN32)
		m_Handle = ::LoadLibraryW( rPath.wstring().data() );
#else
		m_Handle = dlopen( rPath.data(), RTLD_LAZY );
#endif

		return m_Handle != nullptr;
	}

	void DynamicLinkLibrary::Free()
	{
		if( !m_Handle )
			return;

#if defined(_WIN32)
		::FreeLibrary( m_Handle );
		m_Handle = nullptr;
#else
		dlclose( m_Handle );
		m_Handle = nullptr;
#endif
	}

#if defined(_WIN32)
	FARPROC DynamicLinkLibrary::GetSymbol( const char* pName ) const
	{
		return ::GetProcAddress( m_Handle, pName );
	}
#else
	void* DynamicLinkLibrary::GetSymbol( const char* pName )
	{
		return dlsym( m_Handle, pName );
	}
#endif

	void DynamicLinkLibrary::SetExisting( LibraryHandle NewHandle )
	{
		Free();
		m_Handle = NewHandle;
	}

}
