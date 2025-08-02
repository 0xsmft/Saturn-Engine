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

#include "sppch.h"
#include "Process.h"

namespace Saturn {

	Process::Process( const std::wstring& rCommandLine, const std::wstring& rWorkingDir /*= L""*/, ProcessCreateFlags flags /*= ProcessCreateFlags::Normal */ )
		: m_CommandLine( rCommandLine ), m_Flags( flags )
	{
		Create( rWorkingDir );
	}

	Process::~Process()
	{
		Terminate();
	}

	void Process::Create( const std::wstring& rWorkingDir )
	{
#if defined( SAT_PLATFORM_WINDOWS )
		switch( m_Flags )
		{
			case ProcessCreateFlags::Normal:
				CreateNormal( rWorkingDir );
				break;

			case ProcessCreateFlags::RedirectedStreams:
				CreateRedirectedStream( rWorkingDir );
				break;
		}
#endif
	}

	void Process::CreateNormal( const std::wstring& rWorkingDir )
	{
#if defined( SAT_PLATFORM_WINDOWS )
		STARTUPINFOW StartupInfo = {};
		StartupInfo.cb = sizeof( StartupInfo );
		StartupInfo.hStdOutput = GetStdHandle( STD_OUTPUT_HANDLE );
		StartupInfo.dwFlags = STARTF_USESTDHANDLES;

		PROCESS_INFORMATION ProcessInfo;
		bool result = ::CreateProcessW( 
			nullptr, m_CommandLine.data(), nullptr, nullptr, FALSE, 0, nullptr, 
			rWorkingDir.empty() ? nullptr : rWorkingDir.data(), &StartupInfo, &ProcessInfo );

		::CloseHandle( ProcessInfo.hThread );
		m_Handle = ProcessInfo.hProcess;
#endif
	}

	void Process::CreateRedirectedStream( const std::wstring& rWorkingDir )
	{
#if defined( SAT_PLATFORM_WINDOWS )
		SECURITY_ATTRIBUTES securityAttributes{ .nLength = sizeof( SECURITY_ATTRIBUTES ), .lpSecurityDescriptor = nullptr, .bInheritHandle = TRUE };

		if( ::CreatePipe( &m_ReadHandle, &m_WriteHandle, &securityAttributes, 0 ) ) 
		{
			STARTUPINFOW StartupInfo = {};
			StartupInfo.cb = sizeof( StartupInfo );
			StartupInfo.hStdOutput = GetStdHandle( STD_OUTPUT_HANDLE );
			StartupInfo.dwFlags = STARTF_USESTDHANDLES;
			StartupInfo.hStdError = m_WriteHandle;
			StartupInfo.hStdOutput = m_WriteHandle;

			PROCESS_INFORMATION ProcessInfo;
			bool result = ::CreateProcessW(
				nullptr, m_CommandLine.data(), nullptr, nullptr, TRUE, 0, nullptr,
				rWorkingDir.empty() ? nullptr : rWorkingDir.data(), &StartupInfo, &ProcessInfo );

			::CloseHandle( ProcessInfo.hThread );
			m_Handle = ProcessInfo.hProcess;
		}
#endif
	}

	void Process::Terminate()
	{
#if defined( SAT_PLATFORM_WINDOWS )
		if( m_Handle ) 
		{
			if( m_ReadHandle )
				::CloseHandle( m_ReadHandle );

			::TerminateProcess( m_Handle, 0 );

			m_Handle = nullptr;
		}
#endif
	}

	void Process::WaitForExit()
	{
#if defined( SAT_PLATFORM_WINDOWS )
		bool Result;
		DWORD ExitCode;

		while( Result = ::GetExitCodeProcess( m_Handle, &ExitCode ) && ExitCode == STATUS_PENDING )
		{
			::Sleep( 1 );
		}

		// Process exited somehow... cleanup.
		::CloseHandle( m_Handle );

		if( m_ReadHandle )
			::CloseHandle( m_ReadHandle );

		m_Handle = nullptr;
		m_ReadHandle = nullptr;
		m_ExitCode = ExitCode;
#endif
	}

	int Process::ResultOfProcess()
	{
		WaitForExit();

		return m_ExitCode;
	}

	std::wstring Process::GetCurrentOutput( bool closeHandle )
	{
		if( !m_Handle )
			return std::wstring();

#if defined( SAT_PLATFORM_WINDOWS )
		std::wstring Out;

		DWORD availableBytes;
		if( ::PeekNamedPipe( m_ReadHandle, nullptr, 0, nullptr, &availableBytes, nullptr ) && availableBytes )
		{
			std::string TemporaryBuffer;
			
			Out.resize( availableBytes );
			TemporaryBuffer.resize( availableBytes );
		
			::ReadFile( m_ReadHandle, TemporaryBuffer.data(), availableBytes, nullptr, nullptr );
			::MultiByteToWideChar( CP_ACP, 0, TemporaryBuffer.data(), availableBytes, Out.data(), availableBytes );
		}

		if( closeHandle )
			::CloseHandle( m_ReadHandle );

		return Out;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// DEATCHED PROCESS

	DeatchedProcess::DeatchedProcess( const std::wstring& rCommandLine, const std::wstring& rWorkingDir /*= L"" */ )
	{
		Create( rCommandLine, rWorkingDir );
	}

	DeatchedProcess::~DeatchedProcess()
	{
	}

	void DeatchedProcess::Create( const std::wstring& rCommandLine, const std::wstring& rWorkingDir )
	{
#if defined( SAT_PLATFORM_WINDOWS )
		STARTUPINFOW StartupInfo = {};
		StartupInfo.cb = sizeof( StartupInfo );

		PROCESS_INFORMATION ProcessInfo;
		bool result = ::CreateProcessW(
			nullptr, (LPWSTR)rCommandLine.data(), nullptr, nullptr, FALSE, DETACHED_PROCESS, nullptr,
			rWorkingDir.empty() ? nullptr : rWorkingDir.data(), &StartupInfo, &ProcessInfo );

		::CloseHandle( ProcessInfo.hThread );
		::CloseHandle( ProcessInfo.hProcess );
#endif
	}

}
