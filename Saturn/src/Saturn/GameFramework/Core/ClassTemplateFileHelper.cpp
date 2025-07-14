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

#include "Saturn/GameFramework/SClass.h"

#include "Saturn/Project/Project.h"

// Template classes
#include "Saturn/GameFramework/Character.h"

namespace Saturn {

	static std::filesystem::path GetTemplateFileForClass( const SClass* pClass ) 
	{
		if( pClass->GetClass() == Character::StaticClass() )
		{
			return "content/Templates/CharacterCode.h";
		}
		else if( pClass->GetClass() == Entity::StaticClass() )
		{
			return "content/Templates/EntityCode.h";
		}
		else
		{
			return "content/Templates/ClassCode.h";
		}
	}

	static std::filesystem::path ReplaceExtension( const std::filesystem::path& rPath, const std::string& rNewExt )
	{
		std::filesystem::path temp = rPath;
		temp.replace_extension( rNewExt );
		return temp;
	}

	void ClassTemplateFileHelper::CreateAndAmendTemplateFile( const SClass* pSelectedClass, const std::filesystem::path& rDestPath, const std::string& rNewClassName )
	{
		// Always returns the path to the template header file
		std::filesystem::path templateFilepath = GetTemplateFileForClass( pSelectedClass );
		std::filesystem::path templateFilepathSource = ReplaceExtension( templateFilepath, ".cpp" );

		std::filesystem::path rDestPathBase = rDestPath / rNewClassName;
		const std::filesystem::path destPathHeader = ReplaceExtension( rDestPathBase, ".h" );
		const std::filesystem::path destPathSource = ReplaceExtension( rDestPathBase, ".cpp" );

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

			// TODO:
			pos = fileData.find( "__SUPER_CLASS_H_PATH__" );
			while( pos != std::string::npos )
			{
//				fileData.replace( pos, 22, pSelectedClass->GetHeaderPath().string() );

				pos = fileData.find( "__SUPER_CLASS_H_PATH__" );
			}

			std::ofstream fout( rFile );
			fout << fileData;
		};

		replaceInFile( destPathHeader );
		replaceInFile( destPathSource );
	}

}
