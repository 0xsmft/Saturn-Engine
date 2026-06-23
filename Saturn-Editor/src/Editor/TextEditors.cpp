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
#include "TextEditors.h"

#include "Saturn/Core/Process.h"

namespace Saturn::Auxiliary {

	static std::filesystem::path FindDevenvExe( const std::wstring& rVersion ) 
	{
		const std::wstring vsWherePath = L"C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe";
		if( !std::filesystem::exists( vsWherePath ) ) return {};

		std::wstring commandLine = vsWherePath;
		commandLine += L" -legacy -prerelease -property productPath ";

		if( rVersion == L"latest" )
		{
			commandLine += std::format( L"-latest" );
		}
		else
		{
			commandLine += std::format( L"-version {}", rVersion );
		}

		Process vsWhereProcess( commandLine, {}, ProcessCreateFlags::DelayedStart );

		auto out = vsWhereProcess.StartAndGetOutput( {} );
		
		auto cleanOutput = []( std::wstring str ) -> std::wstring
		{
			// Remove carriage returns and newlines.
			std::erase_if( str, []( wchar_t ch ) { return ch == L'\r' || ch == L'\n'; } );

			// remove leading and trailing spaces
			size_t start = 0;
			while( start < str.size() && iswspace( str[ start ] ) ) ++start;

			size_t end = str.size();
			while( end > start && iswspace( str[ end - 1 ] ) ) --end;

			str = str.substr( start, end - start );

			// remove any embedded nulls
			str.erase( std::remove( str.begin(), str.end(), L'\0' ), str.end() );

			return str;
		};

		// Not ideal.
		out = cleanOutput( out );

		return std::filesystem::path( out );
	}

	static bool CreateVSProcessInternal( const std::filesystem::path& rPath, const TextEditors::OpenOptions& rOptions, const std::filesystem::path& rDevenvPath )
	{
		if( rDevenvPath.empty() )
		{
			return false;
		}

		std::wstring commandLine = rDevenvPath;
		commandLine += std::format( L" \"{}\"", rPath.wstring() );

		if( !rOptions.TextFilePath.empty() )
		{
			commandLine += std::format( L" \"{}\" ", rOptions.TextFilePath.wstring() );
		}

		if( !rOptions.ChildArgs.empty() )
		{
			commandLine += rOptions.ChildArgs;
		}

		DetachedProcess dp( commandLine );

		return true;
	}

	bool TextEditors::OpenVisualStudioLatest( const std::filesystem::path& rPath, const OpenOptions& rOptions )
	{
		const auto devenvPath = FindDevenvExe( L"latest" );
		return CreateVSProcessInternal( rPath, rOptions, devenvPath );
	}

	bool TextEditors::OpenVisualStudio2022( const std::filesystem::path& rPath, const OpenOptions& rOptions )
	{
		const auto devenvPath = FindDevenvExe( L"[17.0,18.0)" );
		return CreateVSProcessInternal( rPath, rOptions, devenvPath );
	}

	bool TextEditors::OpenVisualStudio2019( const std::filesystem::path& rPath, const OpenOptions& rOptions )
	{
		const auto devenvPath = FindDevenvExe( L"[16.0,17.0)" );
		return CreateVSProcessInternal( rPath, rOptions, devenvPath );
	}

}
