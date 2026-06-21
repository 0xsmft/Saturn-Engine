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
#include "ProjectZipper.h"

#include "Saturn/Project/Project.h"

#include <libzip/zip.h>

namespace Saturn {

	struct ZipFilters
	{
		std::vector<std::string> FoldersToExclude;
		std::vector<std::string> FileExtensionsToExclude;
	};

	static bool ZipDirectoryRec( zip_t* pZip, const std::filesystem::path& rPath, const std::filesystem::path& rName, const ZipFilters& rFilters )
	{
		auto relPathForNewFile = std::filesystem::relative( rPath, Project::GetActiveProjectRootPath() );

		if( zip_dir_add( pZip, relPathForNewFile.string().c_str(), ZIP_FL_ENC_UTF_8 ) < 0 )
		{
			SAT_CORE_ERROR( "[ProjectZipper]: Failed to add directory into archive! error: {0}", zip_strerror( pZip ) );
			return false;
		}

		for( const auto& rEntry : std::filesystem::directory_iterator( rPath ) )
		{
			const auto& rEntryPath = rEntry.path();
			const auto& rEntryPathStr = rEntry.path().string();
			const auto& rFilenameStr = rEntryPath.filename();
		
			if( rEntry.is_directory() )
			{
				const auto itr = std::find_if( rFilters.FoldersToExclude.begin(), rFilters.FoldersToExclude.end(),
					[ rFilenameStr ]( const auto& rCandidate )
				{
					return rCandidate == rFilenameStr;
				} );

				if( itr != rFilters.FoldersToExclude.end() )
					continue;

				if( !ZipDirectoryRec( pZip, rPath / rFilenameStr, rFilenameStr.string(), rFilters ) )
					return false;
			}
			else
			{
				const auto& rExtStr = rEntryPath.extension().string();

				const auto itr = std::find_if( rFilters.FileExtensionsToExclude.begin(), rFilters.FileExtensionsToExclude.end(),
					[ rExtStr ]( const auto& rCandidate )
				{
					return rCandidate == rExtStr;
				} );

				if( itr != rFilters.FileExtensionsToExclude.end() )
					continue;

				zip_source_t* pSource = zip_source_file( pZip, rEntryPathStr.c_str(), 0, 0 );
				if( !pSource )
				{
					SAT_CORE_ERROR( "[ProjectZipper]: libzip failed to add file: {0} into the zip archive!", rEntryPathStr );

					return false;
				}

				// We want just the file name.
				const auto relPathForZip = relPathForNewFile / rFilenameStr;

				if( zip_file_add( pZip, relPathForZip.string().c_str(), pSource, ZIP_FL_ENC_UTF_8 ) < 0 )
				{
					zip_source_free( pSource );
				}
			}
		}

		return true;
	}

	static bool ZipDirectory( zip_t* pZip, const std::filesystem::path& rPath, const std::string& rNameInZip, const ZipFilters& rFilters )
	{
		for( const auto& rEntry : std::filesystem::directory_iterator( rPath ) )
		{
			const auto& rEntryPath = rEntry.path();
			const auto& rEntryPathStr = rEntry.path().string();

			if( rEntry.is_directory() )
			{
				const auto filename = rEntryPath.filename().string();

				const auto itr = std::find_if( rFilters.FoldersToExclude.begin(), rFilters.FoldersToExclude.end(),
					[ filename ]( const auto& rCandidate )
				{
					return rCandidate == filename;
				} );

				if( itr != rFilters.FoldersToExclude.end() )
					continue;

				const auto relPath = std::filesystem::relative( rPath / rEntryPath.filename(), Project::GetActiveProjectRootPath() );

				ZipDirectoryRec( pZip, rEntry.path(), relPath.string(), rFilters );
			}
			else
			{
				const auto& rFilenameStr = rNameInZip / rEntryPath.filename();
				const auto& rExtStr = rEntryPath.extension().string();

				const auto itr = std::find_if( rFilters.FileExtensionsToExclude.begin(), rFilters.FileExtensionsToExclude.end(), 
					[ rExtStr ](const auto& rCandidate)
				{
					return rCandidate == rExtStr;
				} );

				if( itr != rFilters.FileExtensionsToExclude.end() )
					continue;

				zip_source_t* pSource = zip_source_file( pZip, rEntryPathStr.c_str(), 0, 0 );
				if( !pSource )
				{
					SAT_CORE_ERROR( "[ProjectZipper]: libzip failed to add file: {0} into the zip archive!", rEntryPathStr );
					return false;
				}

				// We want just the file name.
				if( zip_file_add( pZip, rFilenameStr.string().c_str(), pSource, ZIP_FL_ENC_UTF_8 ) < 0 )
				{
					zip_source_free( pSource );
				}
			}
		}

		return true;
	}

	static bool AddRootLevelDirectory( zip_t* pZip, const std::string& rNameInZip, const std::filesystem::path& rRealPath, const ZipFilters& rFilters = {} )
	{
		// Now we can add the Source, Assets, and Cache
		if( zip_dir_add( pZip, rNameInZip.c_str(), ZIP_FL_ENC_UTF_8 ) < 0 )
		{
			SAT_CORE_ERROR( "[ProjectZipper]: Failed to add directory into archive! error: {0}", zip_strerror( pZip ) );
			return false;
		}

		return ZipDirectory( pZip, rRealPath, rNameInZip, rFilters );
	}

	bool ProjectZipper::ZipActiveProject( const std::filesystem::path& rPath )
	{
		const auto& rProject = Project::GetActiveProject();

		auto fullPath = rPath / rProject->GetConfig().Name;
		fullPath.replace_extension( ".zip" );

		// libzip doesn't seem to support overwriting.
		// So, we'll just delete it if it exists.
		if( std::filesystem::exists( fullPath ) )
		{
			std::filesystem::remove( fullPath );
		}

		int errnum;
		zip_t* pZipper = zip_open( fullPath.string().c_str(), ZIP_CREATE | ZIP_EXCL | ZIP_TRUNCATE, &errnum );

		if( !pZipper )
		{
			zip_error_t error;
			zip_error_init_with_code( &error, errnum );

			SAT_CORE_ERROR( "[ProjectZipper]: libzip error: {0}", zip_error_strerror( &error ) );
			return false;
		}

		// Now, after we've opened the zip file successfully, we can now start adding files.
		// First, we add any files in the project root dir.
		for( const auto& rEntry : std::filesystem::directory_iterator( rProject->GetRootDir() ) )
		{
			if( rEntry.is_directory() )
				continue;

			const auto& rEntryPath = rEntry.path();
			const auto& rEntryPathStr = rEntryPath.string();
			const auto& rFilenameStr = rEntryPath.filename().string();
			const auto& rExt = rEntryPath.extension().string();

			// Skip sln file
			if( rExt == ".sln" )
				continue;

			zip_source_t* pSource = zip_source_file( pZipper, rEntryPathStr.c_str(), 0, 0 );
			if( !pSource )
			{
				SAT_CORE_ERROR( "[ProjectZipper]: libzip failed to add file: {0} into the zip archive!", rEntryPathStr );
				break;
			}

			// We want just the file name.
			if( zip_file_add( pZipper, rFilenameStr.c_str(), pSource, ZIP_FL_ENC_UTF_8 ) < 0 )
			{
				zip_source_free( pSource );
			}
		}

		AddRootLevelDirectory( pZipper, "Assets", rProject->GetAbsoluteAssetPath() );
		AddRootLevelDirectory( pZipper, "Source", rProject->GetRootDir() / "Source" );

		ZipFilters filters;
		filters.FoldersToExclude.push_back( "PerUser" );
		filters.FileExtensionsToExclude.push_back( ".sab" );
		filters.FileExtensionsToExclude.push_back( ".ssb" );

		AddRootLevelDirectory( pZipper, "Cache", rProject->GetFullCachePath(), filters );

		zip_close( pZipper );

		return true;
	}

}
