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

#include "ShaderReader.h"

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

	shaderc_include_result* ShaderIncludeHelper::GetInclude( 
		const char* pRequestedPath, 
		shaderc_include_type type, 
		const char* pInstigatorPath,
		size_t depth )
	{
		const std::string name = std::string( pRequestedPath );
		std::string content = ShaderReader::ReadShader( ResolveInclude( name ) );
		content = ShaderReader::RemoveTypeToken( content );

		auto container = new std::array<std::string, 2>;
		( *container )[ 0 ] = name;
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

}
