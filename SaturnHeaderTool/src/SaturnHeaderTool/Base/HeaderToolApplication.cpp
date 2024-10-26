/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2024 BEAST                                                           *
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

#include "HeaderToolApplication.h"

#include "SaturnHeaderTool/Errors.h"

#include <iostream>
#include <string>
#include <vector>

static std::vector<std::string> s_ArgumentsMap 
{
	"/NOMSG",
	"/SRC",
	"/OUT",
	"/MT",
	"/VERBOSE"
};

namespace Saturn {

	HeaderToolApplication::HeaderToolApplication( std::span<char*> args )
		: m_Args( args )
	{
	}

	HeaderToolApplication::~HeaderToolApplication()
	{
	}

	bool HeaderToolApplication::ValidateArgs()
	{
		bool result = true;

		std::map<std::string, std::string> ParsedMap;
		
		// Skip program path arg
		for( size_t i = 0; i < m_Args.size(); i++ )
		{
			if( i == 0 ) continue;

			std::string arg = m_Args[ i ];

			if( arg.starts_with( "/" ) )
			{
				// Now, look for the equal sign
				auto equalPos = arg.find( "=" );

				if( equalPos != std::string::npos )
				{
					std::string key = arg.substr( 0, equalPos );
					std::string value = arg.substr( equalPos + 1 );

					//std::transform( key.begin(), key.end(), key.begin(), std::toupper );
					ParsedMap[ key ] = value;
				}
				else // Flag arg
				{
					ParsedMap[ arg ] = "true";
				}
			}
		}
		
		{
			auto itr = ParsedMap.find( "/OUT" );
			
			if( itr == ParsedMap.end() )
			{
				result = false;

				std::cout << s_ErrorsMaps[ HeaderToolError::TR002 ] << "\n";
			}
			else
			{
				std::string path = itr->second;
				m_OutputPath = path;
			}
		}

		{
			auto itr = ParsedMap.find( "/SRC" );

			if( itr == ParsedMap.end() ) 
			{
				result |= false; 
				std::cout << s_ErrorsMaps[ HeaderToolError::TR001 ] << "\n";
			}
			else
			{
				std::string path = itr->second;
				m_SourcePath = path;
			}
		}

		m_HeaderTool.SetWorkingDir( m_OutputPath );

		// result |= std::filesystem::exists( m_OutputPath ) && std::filesystem::exists( m_SourcePath );

		return result;
	}

	bool HeaderToolApplication::Run()
	{
		std::vector<std::filesystem::path> headerFiles;

		// Search source path for all header files.
		for( const auto& rEntry : std::filesystem::recursive_directory_iterator( m_SourcePath ) )
		{
			if( rEntry.is_directory() ) continue;

			auto& rPath = rEntry.path();
			if( rPath.extension() == ".h" || rPath.extension() == ".hpp" )
				headerFiles.push_back( rPath );
		}

		m_HeaderTool.SubmitWorkList( headerFiles );
		return m_HeaderTool.StartGeneration();
	}
}