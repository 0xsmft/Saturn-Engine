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

namespace Saturn {

	enum class ProcessCreateFlags
	{
		Normal,
		DelayedStart,
		RedirectedStreams
	};

	class Process
	{
	public:
		Process( const std::wstring& rCommandLine, const std::wstring& rWorkingDir = L"", ProcessCreateFlags flags = ProcessCreateFlags::Normal );
		~Process();

		void WaitForExit();
		[[nodiscard]] int ResultOfProcess();
		[[nodiscard]] std::wstring GetCurrentOutput( bool closeHandle = false );
		[[nodiscard]] std::wstring StartAndGetOutput( const std::wstring& rWorkingDir );

		inline const std::wstring GetCurrentLine() 
		{
			// Create temporary copy.
			return m_OutputText;
		}

	private:
		void Create( const std::wstring& rWorkingDir );
		void Terminate();
		void CreateNormal( const std::wstring& rWorkingDir );
		void CreateRedirectedStream( const std::wstring& rWorkingDir );

	private:
		std::wstring m_CommandLine;
		std::wstring m_OutputText;

		void* m_Handle = nullptr;
		ProcessCreateFlags m_Flags = ProcessCreateFlags::Normal;

		int m_ExitCode = 1;
	
		void* m_ReadHandle = nullptr;
		void* m_WriteHandle = nullptr;
	};

	class DeatchedProcess
	{
	public:
		DeatchedProcess( const std::wstring& rCommandLine, const std::wstring& rWorkingDir = L"" );
		~DeatchedProcess();

	private:
		void Create( const std::wstring& rCommandLine, const std::wstring& rWorkingDir );
	};
}
