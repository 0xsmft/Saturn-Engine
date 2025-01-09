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
#include "FileCache.h"

#include "Saturn/Serialisation/RawSerialisation.h"

#include <glm/glm.hpp>

namespace Saturn {

	FileCache::FileCache( const std::filesystem::path& rCacheLocation )
		: m_Location( rCacheLocation )
	{
	}

	FileCache::~FileCache()
	{
		m_FilesInCache.clear();
	}

	void FileCache::Load()
	{
		if( !std::filesystem::exists( m_Location ) )
			return;

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

		int count = 0;
		RawSerialisation::ReadObject( count, stream );

		m_FilesInCache.reserve( count );

		for( int i = 0; i < count; i++ )
		{
			std::string key;
			key = RawSerialisation::ReadString( stream );

			int64_t ticks = 0;
			RawSerialisation::ReadObject( ticks, stream );
		
			int64_t time = 0;
			RawSerialisation::ReadObject( time, stream );

			FileCacheTime fileCacheTime{ .Ticks = ticks, .Time = time };
			m_FilesInCache.emplace( key, fileCacheTime );
		}

		stream.close();
	}

	void FileCache::SetLocation( const std::filesystem::path& rCacheLocation )
	{
		m_Location = rCacheLocation;
	}

	bool FileCache::IsCppFile( const std::filesystem::path& rFile )
	{
		auto ext = rFile.extension();
		return ext == ".cpp" || ext == ".h" || ext == ".hpp";
	}

	bool FileCache::IsSourceFile( const std::filesystem::path& rFile )
	{
		auto ext = rFile.extension();
		return ext == ".cpp";
	}

	bool FileCache::HasFileBeenModifed( const std::filesystem::path& rFile )
	{
		return false;
	}

	std::vector<std::filesystem::path> FileCache::Analyse()
	{
		std::vector<std::filesystem::path> files;

		for( const auto& [rFile, time] : m_FilesInCache )
		{
			if( rFile.filename().string().contains( ".Entry.cpp" ) ) continue;
			if( rFile.filename().string().contains( ".Load.cpp" ) ) continue;
			if( rFile.filename().string().contains( ".Gen.cpp" ) ) continue;
			if( rFile.filename().string().contains( ".Gen.h" ) ) continue;

			if( IsSourceFile( rFile ) )
			{
				auto fsLastWriteTime = std::filesystem::last_write_time( rFile );

				auto systemClock = std::chrono::clock_cast< std::chrono::system_clock >( fsLastWriteTime );
				auto systemTime = std::chrono::duration_cast<std::chrono::milliseconds>( systemClock.time_since_epoch() ).count();

				if( time.Time != systemTime )
				{
					std::filesystem::path headerPath = rFile;
					headerPath.replace_extension( ".h" );

					auto Itr = std::find( files.begin(), files.end(), headerPath );

					if( Itr == files.end() )
					{
						files.push_back( headerPath );
					}
				}
			}
			else if( IsCppFile(rFile) )
			{
				// If its a still a cpp file then its most likely a header file
				// So try to find the source counterpart and add it to the cache
	
				auto fsLastWriteTime = std::filesystem::last_write_time( rFile );

				auto systemClock = std::chrono::clock_cast< std::chrono::system_clock >( fsLastWriteTime );
				auto systemTime = std::chrono::duration_cast< std::chrono::milliseconds >( systemClock.time_since_epoch() ).count();

				if( time.Time != systemTime )
				{
					auto Itr = std::find( files.begin(), files.end(), rFile );

					if( Itr == files.end() )
					{
						files.push_back( rFile );
					}
				}
			}
		}

		return files;
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