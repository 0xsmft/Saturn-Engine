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
#include "ShaderIncludeHelper.h"

#include "Saturn/Core/HashFNV1A.h"

#include "Shader.h"
#include "ShaderReader.h"

#include <vulkan.h>

#include "ShaderAuxiliary.h"

namespace Saturn {

	ShaderIncludeHelper::ShaderIncludeHelper()
	{
	}

	ShaderIncludeHelper::~ShaderIncludeHelper()
	{
	}

	std::filesystem::path ShaderIncludeHelper::ResolveInclude( const std::filesystem::path& rIncludeDir )
	{
		if( std::filesystem::exists( rIncludeDir ) )
			return rIncludeDir;

		for( const auto& rIncPath : m_IncludeDirectories )
		{
			const auto reslovedPath = rIncPath / rIncludeDir;

			if( std::filesystem::exists( reslovedPath ) )
				return reslovedPath;
		}

		SAT_CORE_ERROR( "{0} Does not exist and cannot be found with all the provided include directories! Please add a new include path to fix this.", rIncludeDir.string() );
		SAT_CORE_ASSERT( false );
		return {};
	}

	static void ProcessPragmaOnce( std::string& rContent ) 
	{
		// Not all headers may have an include guard so let's check for one first.
		constexpr std::string_view pragmaToken = "#pragma once";
		constexpr size_t pragmaTokenLength = pragmaToken.size();
	
		const size_t pragmaTokenPosition = rContent.find( pragmaToken, 0 );
		if( pragmaTokenPosition != std::string::npos )
		{
			const std::string fileNameAsMacro = std::to_string( FNV1A32( rContent.c_str() ) );
			const std::string headerGuardBegin = std::format( "#ifndef _{0}_H", fileNameAsMacro );
			const std::string headerGuardDefine = std::format( "#define _{0}_H", fileNameAsMacro );
			constexpr std::string_view headerGuardEnd = "#endif";

			// Remove "#pragma once"...
			const size_t begin = pragmaTokenPosition + pragmaTokenLength + Auxiliary::s_ShaderLineEndingTokenSize;
			rContent.erase( 0llu, begin );

			const auto sizeBeforeModification = rContent.size();

			// Insert header guard.
			rContent.insert( 0llu, headerGuardBegin );

			// New line.
			auto insertionPos = headerGuardBegin.size() + Auxiliary::s_ShaderLineEndingTokenSize;
			rContent.insert( insertionPos, Auxiliary::s_ShaderLineEndingToken );

			// Header guard definition.
			rContent.insert( insertionPos, headerGuardDefine );

			// New line after content.
			insertionPos += headerGuardDefine.size() + sizeBeforeModification;
			rContent.insert( insertionPos, Auxiliary::s_ShaderLineEndingToken );

			// #endif
			insertionPos += Auxiliary::s_ShaderLineEndingTokenSize;
			rContent.insert( insertionPos, headerGuardEnd );

			// Ending new line
			insertionPos += headerGuardEnd.size();
			rContent.insert( insertionPos, Auxiliary::s_ShaderLineEndingToken );
		}
		else
		{
			SAT_CORE_WARN( "No #pragma once preprocesser directive found! This file and it's contents will be copied everytime it's included!" );
		}
	}

	shaderc_include_result* ShaderIncludeHelper::GetInclude( 
		const char* pRequestedPath, 
		shaderc_include_type type, 
		const char* pInstigatorPath,
		size_t depth )
	{
		const std::string path = std::string( pRequestedPath );
		const auto resolvedPath = ResolveInclude( path );

		std::string content = ShaderReader::ReadShader( resolvedPath );
		content = ShaderReader::RemoveTypeToken( content );
		
		ProcessPragmaOnce( content );

		if( m_pShader )
		{
			m_pShader->AddDependency( Passkey<ShaderIncludeHelper>(), resolvedPath );
		}

		auto container = new std::array<std::string, 2>;
		( *container )[ 0 ] = path;
		( *container )[ 1 ] = content;

		auto data = new shaderc_include_result();
		data->user_data = container;
		data->source_name = ( *container )[ 0 ].data();
		data->source_name_length = ( *container )[ 0 ].size();
		data->content = ( *container )[ 1 ].data();
		data->content_length = ( *container )[ 1 ].size();

		return data;
	}

	void ShaderIncludeHelper::ReleaseInclude( shaderc_include_result* pData )
	{
		delete static_cast< std::array<std::string, 2>* >( pData->user_data );
		delete pData;
	}

	void ShaderIncludeHelper::AddIncludeDirectory( const std::filesystem::path& rPath )
	{
		auto& rIncludePath = m_IncludeDirectories.emplace_back( rPath );

#if defined(SAT_PLATFORM_WINDOWS)
		auto wPath = rPath.wstring();
		std::replace( wPath.begin(), wPath.end(), '/', '\\' );
		rIncludePath = wPath;
#endif
	}

	void ShaderIncludeHelper::SetShader( Shader* pShader )
	{
		m_pShader = pShader;
	}

}
