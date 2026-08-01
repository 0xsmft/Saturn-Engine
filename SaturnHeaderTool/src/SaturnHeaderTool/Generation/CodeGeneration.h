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

#include "SaturnHeaderTool/Base/Core.h"

#include <vector>
#include <filesystem>

namespace Saturn {

	enum class HeaderToolParseResult
	{
		ParseSkipped = 1 << 0, // No SClass/GENERATED_BODY
		NoSClass = 1 << 1, // GENERATED_BODY was implied however no SCLASS
		NoGeneratedBody = 1 << 2, // SCLASS was implied but no GENERATED_BODY 
		FailedToParse = 1 << 3, // General error -- more detailed one would of been outputted
		ClassAlreadyExists = 1 << 4, // A class with the same name was already found in the current file.
		SPropertyFieldNotInClass = 1 << 5, // SPROPERTY() macro defined outside of the class.
		GeneratedBodyNotInClass = 1 << 6, // GENERATED_BODY macro defined outside of the class.
		Success = 1 << 7
	};

	class HeaderTool 
	{
	public:
		HeaderTool();
		~HeaderTool();

		void SetIsHotReload( bool val ) { m_IsHotReload = val; }
		void SetPCHPath( const std::filesystem::path& rPath ) { m_PCHPath = rPath; }
		void SetWorkingDir( const std::filesystem::path& rPath );
		void SubmitWorkList( const std::vector<std::filesystem::path>& rCommands, HeaderToolConfigKind config );
		[[nodiscard]] bool StartGeneration();

	private:
		HeaderToolParseResult GenerateHeader( HeaderToolCommand& rCommand );
		HeaderToolParseResult ParseHeaderFile( HeaderToolCommand& rCommand );

		bool GenerateSource( HeaderToolCommand& rCommand );
	
	private:
		std::vector<HeaderToolCommand> m_Commands;
		std::filesystem::path m_WorkingDir;
		std::filesystem::path m_PCHPath;
		bool m_IsHotReload = false;
	};
}
