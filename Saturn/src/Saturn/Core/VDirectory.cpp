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
#include "VDirectory.h"

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

#include "VFile.h"

#include "StringAuxiliary.h"

namespace Saturn {

	VDirectory::VDirectory( const std::wstring& rName )
		: m_Name( Auxiliary::ConvertWString( rName ) )
	{
	}

	VDirectory::VDirectory( const std::string& rName ) 
		: m_Name( rName )
	{
	}

	VDirectory::VDirectory( const std::string& rName, VDirectory* parentDirectory )
		: m_Name( rName ), ParentDirectory( parentDirectory )
	{

	}

	VDirectory::~VDirectory()
	{
		Clear();
	}

	void VDirectory::RemoveFile( const std::string& rName )
	{
		Files.erase( rName );
	}

	void VDirectory::AddDirectory( const std::string& rName )
	{
		Ref<VDirectory> dir = Ref<VDirectory>::Create( rName, nullptr );
		Directories.emplace( rName, dir );
	}

	void VDirectory::RemoveDirectory( const std::string& rName )
	{
		Directories.erase( rName );
	}

	void VDirectory::Clear()
	{
		Files.clear();

		for( auto&& [k, dir] : Directories )
		{
			dir->Clear();
		}

		Directories.clear();
	}

	void VDirectory::Serialise( const Ref<VDirectory>& rObject, std::ofstream& rStream )
	{
		RawSerialisation::WriteString( rObject->m_Name, rStream );

		// Serialise the map manually.
		size_t mapSize = rObject->Files.size();
		rStream.write( reinterpret_cast<char*>( &mapSize ), sizeof( size_t ) );

		for( const auto& [k, v] : rObject->Files )
		{
			RawSerialisation::WriteString( k, rStream );
			VFile::Serialise( v, rStream );
		}

		mapSize = rObject->Directories.size();
		rStream.write( reinterpret_cast<char*>( &mapSize ), sizeof( size_t ) );

		for( const auto& [k, v] : rObject->Directories )
		{
			RawSerialisation::WriteString( k, rStream );
			VDirectory::Serialise( v, rStream );
		}
	}

	void VDirectory::Deserialise( Ref<VDirectory>& rObject, std::ifstream& rStream )
	{
		rObject->m_Name = RawSerialisation::ReadString( rStream );

		// Deserialise the map manually.
		size_t mapSize = 0;
		rStream.read( reinterpret_cast< char* >( &mapSize ), sizeof( size_t ) );

		for( size_t i = 0; i < mapSize; i++ )
		{
			std::string K{};
			K = RawSerialisation::ReadString( rStream );

			Ref<VFile> V = Ref<VFile>::Create();
			V->ParentDir = rObject.Get();

			VFile::Deserialise( V, rStream );

			rObject->Files[ K ] = V;
		}

		mapSize = 0;
		rStream.read( reinterpret_cast< char* >( &mapSize ), sizeof( size_t ) );

		for( size_t i = 0; i < mapSize; i++ )
		{
			std::string K{};
			K = RawSerialisation::ReadString( rStream );

			Ref<VDirectory> V = Ref<VDirectory>::Create();
			V->ParentDirectory = rObject.Get();

			VDirectory::Deserialise( V, rStream );

			rObject->Directories[ K ] = V;
		}
	}
}