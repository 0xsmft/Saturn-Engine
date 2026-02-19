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
#include "Process.h"

#include "StringAuxiliary.h"

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
			default: break;

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
			::SetHandleInformation( m_ReadHandle, HANDLE_FLAG_INHERIT, 0 );

			STARTUPINFOW StartupInfo = {};
			StartupInfo.cb = sizeof( StartupInfo );
			StartupInfo.dwFlags = STARTF_USESTDHANDLES;
			StartupInfo.hStdError = m_WriteHandle;
			StartupInfo.hStdOutput = m_WriteHandle;

			PROCESS_INFORMATION ProcessInfo;
			bool result = ::CreateProcessW(
				nullptr, m_CommandLine.data(), nullptr, nullptr, TRUE, 0, nullptr,
				rWorkingDir.empty() ? nullptr : rWorkingDir.data(), &StartupInfo, &ProcessInfo );

			::CloseHandle( ProcessInfo.hThread );
			::CloseHandle( m_WriteHandle );

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
		if( !m_ReadHandle )
			return std::wstring();

#if defined( SAT_PLATFORM_WINDOWS )
		constexpr DWORD MAX_BUFFER_SIZE = 4096;
		std::vector<wchar_t> tempBuffer;

		while( true )
		{
			DWORD bytesAvailable = 0;
			if( !::PeekNamedPipe( m_ReadHandle, nullptr, 0, nullptr, &bytesAvailable, nullptr ) )
				break;

			// Nothing left to read
			if( !bytesAvailable )
				break;

			const DWORD bytesToBeRead = std::min<DWORD>( bytesAvailable, MAX_BUFFER_SIZE * sizeof( wchar_t ) );
			DWORD bytesRead = 0;

			if( ::ReadFile( m_ReadHandle, tempBuffer.data(), bytesToBeRead, &bytesRead, nullptr ) && bytesRead > 0 )
			{
				const size_t charCount = bytesRead / sizeof( wchar_t );

				m_OutputText.append( tempBuffer.data(), charCount );
			}
			else
			{
				DWORD error = ::GetLastError();
				SAT_CORE_ERROR( "Win32 Error: {0}", error );

				break;
			}
		}

		if( closeHandle && m_ReadHandle )
		{
			::CloseHandle( m_ReadHandle );
			m_ReadHandle = nullptr;
		}

		// Return final buffer when we are done
		return m_OutputText;
#endif
	}

	std::wstring Process::StartAndGetOutput( const std::wstring& rWorkingDir )
	{
		if( m_Flags != ProcessCreateFlags::DelayedStart ) 
			return std::wstring();

		CreateRedirectedStream( rWorkingDir );

		// Wait
		bool result;
		DWORD exitCode;

		while( result = ::GetExitCodeProcess( m_Handle, &exitCode ) && exitCode == STATUS_PENDING )
		{
			::Sleep( 1 );
		}

		// Process exited somehow... cleanup.
		std::vector<char> tempBuffer( 4096 );
		std::string out;

		DWORD bytesRead = 0;
		while( ::ReadFile( m_ReadHandle, tempBuffer.data(), ( DWORD ) ( tempBuffer.size() * sizeof( char ) ), &bytesRead, nullptr ) && bytesRead > 0 )
		{
			out.append( tempBuffer.data(), bytesRead / sizeof( char ) );
		}

		// Close any open handles
		::CloseHandle( m_ReadHandle );
		::CloseHandle( m_Handle );

		m_Handle = nullptr;
		m_ReadHandle = nullptr;
		m_WriteHandle = nullptr;
		m_ExitCode = exitCode;

		int wideLen = ::MultiByteToWideChar( CP_ACP, 0, tempBuffer.data(), ( int ) tempBuffer.size(), nullptr, 0 );
		std::wstring output( wideLen, L'\0' );
		::MultiByteToWideChar( CP_ACP, 0, tempBuffer.data(), ( int ) tempBuffer.size(), output.data(), wideLen );

		return output;
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
