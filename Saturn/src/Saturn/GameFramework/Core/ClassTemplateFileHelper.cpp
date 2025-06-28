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
#include "ClassTemplateFileHelper.h"

#include "Saturn/Project/Project.h"

namespace Saturn {

	void ClassTemplateFileHelper::CreateAndAmendTemplateFile( const SClass* pSelectedClass, const std::filesystem::path& rDestPath, const std::string& rNewClassName )
	{
		auto prjRootDir = Project::GetActiveProject()->GetRootDir();

//		std::filesystem::path templateFilepath = rMetadata.TemplateFile.empty() ? "content/Templates/ClassCode.h" : rMetadata.TemplateFile;

		std::filesystem::path templateFilepath = "";

		std::filesystem::path templateFilepathSource = templateFilepath.replace_extension( ".cpp" );

		std::filesystem::path rDestPathBase = rDestPath / rNewClassName;
		std::filesystem::path destPathHeader = rDestPathBase.replace_extension( ".h" );
		std::filesystem::path destPathSource = rDestPathBase.replace_extension( ".cpp" );

		std::filesystem::copy_file( templateFilepath, destPathHeader );
		std::filesystem::copy_file( templateFilepathSource, destPathSource );

		auto replaceInFile = [ & ]( const std::filesystem::path& rFile )
		{
			std::string fileData;

			std::ifstream stream( rFile );

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
				fileData.replace( pos, 13, rNewClassName );

				pos = fileData.find( "__FILE_NAME__" );
			}

			pos = fileData.find( "__SUPER_CLASS__" );
			while( pos != std::string::npos )
			{
				fileData.replace( pos, 15, pSelectedClass->GetName() );

				pos = fileData.find( "__SUPER_CLASS__" );
			}

			pos = fileData.find( "__SUPER_CLASS_H_PATH__" );
			while( pos != std::string::npos )
			{
//				fileData.replace( pos, 22, pSelectedClass().HeaderPath.string() );

				pos = fileData.find( "__SUPER_CLASS_H_PATH__" );
			}

			std::ofstream fout( rFile );
			fout << fileData;
		};

		replaceInFile( destPathHeader );
		replaceInFile( destPathSource );
	}

}
