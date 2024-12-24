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

#include "sppch.h"
#include "HeaderToolApplication.h"

#include "Errors.h"

#include <iostream>
#include <string>
#include <vector>

static std::vector<std::string> s_ArgumentsMap 
{
	"/NOMSG",
	"/SRC",
	"/OUT",
	"/FC",
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

		{
			auto itr = ParsedMap.find( "/FC" );

			if( itr == ParsedMap.end() )
			{
				result |= false;
				std::cout << s_ErrorsMaps[ HeaderToolError::TR003 ] << "\n";
			}
			else
			{
				std::string path = itr->second;
				m_FileCache.SetLocation( path );
			}
		}

		{
			auto itr = ParsedMap.find( "/NOMSG" );

			if( itr == ParsedMap.end() )
			{
				std::cout << "Saturn Header Tool version " << SAT_CURRENT_VERSION_STRING << "\n";
			}
		}

		m_HeaderTool.SetWorkingDir( m_OutputPath );

		return result;
	}

	bool HeaderToolApplication::Run()
	{
		m_FileCache.Load();

		std::vector<std::filesystem::path> headerFiles = m_FileCache.Analyse();

		m_HeaderTool.SubmitWorkList( headerFiles );
		return m_HeaderTool.StartGeneration();
	}

}