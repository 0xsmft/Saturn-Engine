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

#include "Ref.h"

#if defined( SAT_PLATFORM_WINDOWS )
#include <Windows.h>
#endif

namespace Saturn {

#if defined( SAT_PLATFORM_WINDOWS )
	using ProcessHandle = HANDLE;
	using HandleType = HANDLE;
#else
	using ProcessHandle = void*;
	using HandleType = void*;
#endif

	enum class ProcessCreateFlags
	{
		Normal,
		RedirectedStreams
	};

	class Process : public RefTarget
	{
	public:
		Process( const std::wstring& rCommandLine, const std::wstring& rWorkingDir = L"", ProcessCreateFlags flags = ProcessCreateFlags::Normal );
		~Process();

		void WaitForExit();
		[[nodiscard]] int ResultOfProcess();
		[[nodiscard]] std::wstring GetCurrentOutput( bool closeHandle = false );

	private:
		void Create( const std::wstring& rWorkingDir );
		void Terminate();
		void CreateNormal( const std::wstring& rWorkingDir );
		void CreateRedirectedStream( const std::wstring& rWorkingDir );

	private:
		std::wstring m_CommandLine;

		ProcessHandle m_Handle = nullptr;
		ProcessCreateFlags m_Flags = ProcessCreateFlags::Normal;

		int m_ExitCode = 1;
	
		ProcessHandle m_ReadHandle = nullptr;
		ProcessHandle m_WriteHandle = nullptr;
	};

	class DeatchedProcess : public RefTarget
	{
	public:
		DeatchedProcess( const std::wstring& rCommandLine, const std::wstring& rWorkingDir = L"" );
		~DeatchedProcess();

	private:
		void Create( const std::wstring& rCommandLine, const std::wstring& rWorkingDir );
	};
}