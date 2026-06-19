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

#include "Saturn/Core/Base.h"

#include <filesystem>

namespace Saturn {

	enum class PremakeAction : uint8_t
	{
		Other, // other specified elsewhere
		Clean,
		VisualStudio2022,
		VisualStudio2026,
		Makefile, // gmake
		Xcode,
		Codelite,
	};

#if defined(SAT_PLATFORM_WINDOWS)
	static constexpr PremakeAction PREFERED_PREMAKE_ACTION_FOR_OS = PremakeAction::VisualStudio2022;
#elif defined(SAT_PLATFORM_MACOS)
	static constexpr PremakeAction PREFERED_PREMAKE_ACTION_FOR_OS = PremakeAction::Xcode;
#else // any other OS use make files
	static constexpr PremakeAction PREFERED_PREMAKE_ACTION_FOR_OS = PremakeAction::Makefile;
#endif

	class Premake
	{
	public:
		static bool Launch( const std::filesystem::path& rWorkingDir, const std::wstring& rPremakeFilename, PremakeAction action );
	};

}
