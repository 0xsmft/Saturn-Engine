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

#pragma once

#include <unordered_map>
#include <filesystem>

namespace Saturn {

	class FileCache
	{
	public:
		struct FileCacheTime
		{
			FileCacheTime( int64_t ticks, int64_t time )
				: Ticks( ticks ), Time( time )
			{
			}

			// C# DateTime.Ticks
			int64_t Ticks;

			// Unix Timestamp (system time from C# Build Tool)
			int64_t Time;

			bool operator==( const FileCacheTime& rOther )
			{
				return Time == rOther.Time;
			}

			bool operator!=( const FileCacheTime& rOther )
			{
				return Time != rOther.Time;
			}
		};

	public:
		FileCache() = default;
		FileCache( const std::filesystem::path& rCacheLocation );

		~FileCache();

		void Load();
		void SetLocation( const std::filesystem::path& rCacheLocation );

	public:
		bool IsCppFile( const std::filesystem::path& rFile );
		bool IsSourceFile( const std::filesystem::path& rFile );
		bool HasFileBeenModifed( const std::filesystem::path& rFile );
		
		std::vector<std::filesystem::path> Analyse();

	private:
		std::unordered_map<std::filesystem::path, FileCacheTime> m_FilesInCache;
	
	private:
		std::filesystem::path m_Location;
	};

}
