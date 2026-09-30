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
#include "ShaderReader.h"

namespace Saturn {

	std::string ShaderReader::ReadShader( const std::filesystem::path& rPath )
	{
		std::string content;

		std::ifstream stream( rPath, std::ios::ate | std::ios::binary );
		if( !stream.is_open() )
		{
			SAT_CORE_ERROR( "Failed to open shader file \"{0}\"", rPath.string() );
			SAT_CORE_ASSERT( false );
			return content;
		}

		const auto fileSize = ( size_t ) stream.tellg();
		content.resize( fileSize );

		stream.seekg( 0 );
		stream.read( content.data(), fileSize );
		stream.close();

		return content;
	}

	std::string ShaderReader::RemoveTypeToken( const std::string& rContent )
	{
		constexpr std::string_view typeToken = "#type";
		constexpr size_t typeTokenLength = typeToken.size();

		size_t typeTokenPosition = rContent.find( typeToken, 0 );
		SAT_CORE_ASSERT( typeTokenPosition != std::string::npos, "Missing type token in shader! All shaders must have a \"#type <shader_type>\" token in the shader!" );

		while( typeTokenPosition != std::string::npos )
		{
			std::string temporaryFileCopy;
			std::copy( rContent.begin(), rContent.end(), std::back_inserter( temporaryFileCopy ) );

			// On windows we'll use CRLF...
#if defined(SAT_PLATFORM_WINDOWS)
			const size_t typeTokenEnd = temporaryFileCopy.find( "\r\n", typeTokenPosition );

			//... so we'll need to assert if we do not find CRLF characters.
			SAT_CORE_ASSERT( typeTokenEnd != std::string::npos, "Shader must be CRLF!" );
#else 
			// And on other platforms, we'll use LF...
			const size_t typeTokenEnd = temporaryFileCopy.find( "\n", typeTokenPosition );

			//... so we'll need to assert if we do not find LF characters.
			SAT_CORE_ASSERT( typeTokenEnd != std::string::npos, "Shader must be LF!" );
#endif

			const size_t begin = typeTokenPosition + typeTokenLength + 1;
			const std::string type = temporaryFileCopy.substr( begin, typeTokenEnd - begin );

			const size_t nextLinePos = temporaryFileCopy.find_first_not_of( "\r\n", typeTokenEnd );
			typeTokenPosition = temporaryFileCopy.find( typeToken, nextLinePos );

			const auto rawShaderCode = temporaryFileCopy.substr( nextLinePos, typeTokenPosition - ( nextLinePos == std::string::npos ? temporaryFileCopy.size() - 1 : nextLinePos ) );

			return rawShaderCode;
		}

		return rContent;
	}

}
