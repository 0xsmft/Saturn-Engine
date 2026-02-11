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
#include "Premake.h"

#include "Saturn/Core/EnvironmentVariables.h"
#include "Saturn/Core/Process.h"

namespace Saturn {

	bool Premake::Launch( const std::filesystem::path& rWorkingDir, const std::wstring& rPremakeFilename, PremakeAction action )
	{
		std::wstring PremakePath = Auxiliary::GetEnvironmentVariableWs( L"SATURN_PREMAKE_PATH" );
		
		// Append premake filename.
		PremakePath += L" --file=" + rPremakeFilename;

		switch( action )
		{
			default: break;
			case PremakeAction::Other:
				break;

			case PremakeAction::Clean:
				PremakePath += L" clean";
				break;

			// 2026 not supported by any premake version yet!
			case PremakeAction::VisualStudio2026:
			case PremakeAction::VisualStudio2022:
				PremakePath += L" vs2022";
				break;

			case PremakeAction::Makefile:
				PremakePath += L" gmake";
				break;
			
			case PremakeAction::Xcode:
				PremakePath += L" xcode4";
				break;

			case PremakeAction::Codelite:
				PremakePath += L" codelite";
				break;
		}

#if defined( _WIN32 )
		std::replace( PremakePath.begin(), PremakePath.end(), L'/', L'\\' );
#endif
		Process premakeProcess( PremakePath, rWorkingDir.wstring() );
		bool res = ( premakeProcess.ResultOfProcess() == 0 ) ? true : false;

		return res;
	}
}