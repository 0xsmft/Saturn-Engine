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
#include "GenerationRecipe.h"

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

#include <glm/glm.hpp>
#include <iostream>

namespace Saturn {

	FGenerationRecipe::FGenerationRecipe( const std::filesystem::path& rCacheLocation )
		: m_Location( rCacheLocation )
	{
	}

	FGenerationRecipe::~FGenerationRecipe()
	{
		m_FilesInCache.clear();
	}

	void FGenerationRecipe::Load()
	{
		if( !std::filesystem::exists( m_Location ) ) 
		{
			std::cout << "Filecache at location: " << m_Location << " does not exist! You may need to run the Build Tool before running the header tool!\n";
			return;
		}

		std::ifstream stream( m_Location, std::ios::binary | std::ios::in );

		std::streampos size = 0;
		size = stream.tellg();
		stream.seekg( 0, std::ios::end );
		size = stream.tellg() - size;

		if( size == 0 )
		{
			return;
		}

		stream.seekg( 0 );

		size_t modules = 0;
		RawSerialisation::ReadObject( modules, stream );

		m_FilesInCache.reserve( modules );

		for( int i = 0; i < modules; ++i )
		{
			std::string key;
			key = RawSerialisation::ReadString( stream );

			m_FilesInCache.push_back( key );
		}

		stream.close();
	}

	void FGenerationRecipe::SetLocation( const std::filesystem::path& rCacheLocation )
	{
		m_Location = rCacheLocation;
	}

	bool FGenerationRecipe::IsCppFile( const std::filesystem::path& rFile )
	{
		const auto ext = rFile.extension();
		return ext == ".cpp" || ext == ".h" || ext == ".hpp";
	}

	bool FGenerationRecipe::IsSourceFile( const std::filesystem::path& rFile )
	{
		const auto ext = rFile.extension();
		return ext == ".cpp";
	}

	bool FGenerationRecipe::HasFileBeenModifed( const std::filesystem::path& rFile )
	{
		return false;
	}

	std::vector<std::filesystem::path> FGenerationRecipe::Analyse()
	{
		return m_FilesInCache;
	}

	/*
	struct FileReference
	{
		std::string Name;
		std::filesystem::path SourcePath;
		std::filesystem::path HeaderPath;

		std::vector<FileReference> References;
	};
	*/
}
