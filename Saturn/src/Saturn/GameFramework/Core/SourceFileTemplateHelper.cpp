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
#include "SourceFileTemplateHelper.h"

#include "Saturn/Project/Project.h"

namespace Saturn {

	void SourceFileTemplateHelper::CreateEntitySourceFiles( const std::filesystem::path& rPath, const char* pName )
	{
		auto prjRootDir = Project::GetActiveProject()->GetRootDir();

		// Copy entity code from templates.
		std::filesystem::copy_file( "content/Templates/EntityCode.cpp", rPath / "EntityCode.cpp" );
		std::filesystem::copy_file( "content/Templates/EntityCode.h", rPath / "EntityCode.h" );

		std::filesystem::path src = rPath.string();
		src.append( pName );
		src.replace_extension( ".cpp" );

		std::filesystem::path header = rPath.string();
		header.append( pName );
		header.replace_extension( ".h" );

		std::filesystem::rename( rPath / "EntityCode.cpp", src );
		std::filesystem::rename( rPath / "EntityCode.h", header );

		// Wait for rename.
		std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

		auto loadFn = [&]( bool IsHeader ) 
		{
			std::string fileData;

			std::ifstream stream( IsHeader ? header : src );

			// Load the file.

			if( stream )
			{
				stream.seekg( 0, std::ios_base::end );
				auto size = static_cast< size_t >( stream.tellg() );
				stream.seekg( 0, std::ios_base::beg );

				fileData.reserve( size );
				fileData.assign( std::istreambuf_iterator<char>( stream ), std::istreambuf_iterator<char>() );
			}

			size_t pos = fileData.find( "__FILE_NAME__" );

			while( pos != std::string::npos )
			{
				fileData.replace( pos, 13, pName );

				pos = fileData.find( "__FILE_NAME__" );
			}

			std::ofstream fout( IsHeader ? header : src );
			fout << fileData;
		};

		loadFn( false );
		loadFn( true );
	}

}