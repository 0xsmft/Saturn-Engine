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
#include "VirtualFS.h"

#include "Profiler.h"

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

#if !defined(SAT_DIST)
#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include <imgui.h>
#endif

namespace Saturn {

	VirtualFS::VirtualFS()
	{
		Init();
	}

	VirtualFS::~VirtualFS()
	{
		Terminate();
	}

	void VirtualFS::Init()
	{
		m_RootDirectory = Ref<VDirectory>::Create( "/" );
	}

	void VirtualFS::Terminate()
	{
		m_RootDirectory->Clear();
		m_RootDirectory = nullptr;
	}

	void VirtualFS::MountBase( const std::string& rID, const std::filesystem::path& rRealPath )
	{
		m_RootDirectory->AddDirectory( rID );
		m_MountBases.emplace( rID, rRealPath );

		SAT_CORE_INFO( "[VFS] Mounted base {0} successfully.", rID );
	}

	bool VirtualFS::Mount( const std::string& rMountBase, const std::filesystem::path& rVirtualPath )
	{
		// Check if the mount base exists.
		const auto Itr = m_MountBases.find( rMountBase );

		if( Itr == m_MountBases.end() )
			return false;

		// Get the mount base folder.
		Ref<VDirectory>& rMountBaseDir = m_RootDirectory->Directories[ rMountBase ];

		VDirectory* pCurrentDir = rMountBaseDir.Get();

		for( auto it = rVirtualPath.begin(); it != rVirtualPath.end(); ++it )
		{
			const std::filesystem::path temporaryPath = *it;
			const std::string segmentStr = temporaryPath.string();

			if( !temporaryPath.has_extension() )
			{
				auto dirItr = pCurrentDir->Directories.find( segmentStr );

				if( dirItr == pCurrentDir->Directories.end() )
				{
					Ref<VDirectory> dir = Ref<VDirectory>::Create( segmentStr, pCurrentDir );

					// Directory does not exist.
					pCurrentDir->Directories.emplace( segmentStr, dir );

					// Only add the last dir path if the last path is not the mount base.
					if( pCurrentDir->GetName() != rMountBase )
					{
						// Build the path for our Path to dir map.
						BuildPath( dir, rMountBase );
					}
					else
					{
						m_PathToDir[ rMountBase ][ segmentStr ] = pCurrentDir->Directories[ segmentStr ];
					}
				}

				pCurrentDir = pCurrentDir->Directories[ segmentStr ].Get();
			}
			else
			{
				const auto fileItr = pCurrentDir->Files.find( segmentStr );
				if( fileItr == pCurrentDir->Files.end() )
				{
					Ref<VFile> file = Ref<VFile>::Create( segmentStr, pCurrentDir );

					// Directory does not exist.
					pCurrentDir->Files.emplace( segmentStr, file );
					
					// Build the path for our Path to dir map.
					BuildPath( file, rMountBase );
				}
			}
		}

		SAT_CORE_INFO( "[VFS]: Successfully mounted {0} to mount base: {1}", rVirtualPath.string(), rMountBase );

		// Mount successful.
		return true;
	}

	void VirtualFS::BuildPath( Ref<VDirectory>& rDir, const std::string& rMountBase )
	{
		// Create a temp copy just so we don't modify the rDir
		VDirectory* currentDir = rDir.Get();

		// Add all of our parents paths onto our one.
		std::string path = "";

		// Recursively get the current directory name and append it on to the path.
		// Then move on to parent and repeat.
		while( true )
		{
			if( currentDir == nullptr || currentDir->GetName() == rMountBase )
				break;

			std::string dirName = currentDir->GetName();
			
			if( path.empty() )
				path = dirName;
			else
				path = dirName + "/" + path;

			currentDir = currentDir->GetParent();
		}

		m_PathToDir[ rMountBase ][ path ] = rDir;
	}

	void VirtualFS::BuildPath( Ref<VFile>& rFile, const std::string& rMountBase )
	{
		// Create a temp copy just so we don't modify the rDir
		VDirectory* currentDir = rFile->ParentDir;

		// Add all of our parents paths onto our one.
		std::string path = rFile->Name;

		// Recursively get the current directory name and append it on to the path.
		// Then move on to parent and repeat.
		while( true )
		{
			if( currentDir == nullptr || currentDir->GetName() == rMountBase )
				break;

			std::string dirName = currentDir->GetName();
			path = dirName + "/" + path;

			currentDir = currentDir->GetParent();
		}

		m_PathToFile[ rMountBase ][ path ] = rFile;
	}

	void VirtualFS::UnmountBase( const std::string& rID )
	{
		// Check if the mount base exists.
		const auto Itr = m_MountBases.find( rID );

		if( Itr == m_MountBases.end() ) 
		{
			SAT_CORE_WARN( "[VFS]: '{0}' was not found, maybe it was already unmounted?", rID );

			return;
		}

		// Get the mount base folder.
		Ref<VDirectory>& rMountBaseDir = m_RootDirectory->Directories[ rID ];

		if( rMountBaseDir->Directories.size() || rMountBaseDir->Files.size() )
		{
			SAT_CORE_WARN( "[VFS]: '{0}' still has directories and/or files mounted to it! Please unmount them before unmounting the base!", rID );
		}

		rMountBaseDir->Clear();

		m_RootDirectory->Directories.erase( rID );
		m_PathToFile.erase( rID );
		m_PathToDir.erase( rID );
	}

	void VirtualFS::Unmount( const std::string& rMountBase, const std::filesystem::path& rVirtualPath )
	{
		// Check if the mount base exists.
		const auto Itr = m_MountBases.find( rMountBase );

		if( Itr == m_MountBases.end() )
		{
			SAT_CORE_WARN( "[VFS]: '{0}' mount base was not found, maybe it was already unmounted?", rMountBase );

			return;
		}

		// Get the mount base folder.
		Ref<VDirectory>& rMountBaseDir = m_RootDirectory->Directories[ rMountBase ];

		// Are we a file?
		if( rVirtualPath.has_extension() )
		{
			Ref<VFile>& rFile = m_PathToFile[ rMountBase ][ rVirtualPath ];
			
			rFile->ParentDir->RemoveFile( rFile->Name );
		}
		else
		{
			Ref<VDirectory>& rDirectory = m_PathToDir[ rMountBase ][ rVirtualPath ];

			rDirectory->Clear();
			rDirectory->GetParent()->RemoveDirectory( rDirectory->GetName() );
		}
	}

	Ref<VFile> VirtualFS::FindFile( const std::string& rMountBase, const std::filesystem::path& rVirtualPath )
	{
		SAT_PF_EVENT();

		// Check if the mount base exists.
		const auto Itr = m_MountBases.find( rMountBase );

		if( Itr == m_MountBases.end() )
			return nullptr;

		if( rVirtualPath.empty() )
			return nullptr;

		return m_PathToFile[ rMountBase ][ rVirtualPath ];
	}

	Ref<VDirectory> VirtualFS::FindDirectory( const std::string& rMountBase, const std::filesystem::path& rVirtualPath )
	{
		SAT_PF_EVENT();

		// Check if the mount base exists.
		const auto Itr = m_MountBases.find( rMountBase );

		if( Itr == m_MountBases.end() )
			return nullptr;

		if( rVirtualPath.empty() )
			return nullptr;

		return m_PathToDir[ rMountBase ][ rVirtualPath ];
	}

	//////////////////////////////////////////////////////////////////////////
	// SERIALILSATION/DESERIALILSATION

	void VirtualFS::WriteDir( const Ref<VDirectory>& rDir, std::ofstream& rStream )
	{
		SAT_CORE_INFO( "Writing Dir with name: {0}", rDir->GetName() );

		size_t mapSize = rDir->Files.size();
		rStream.write( reinterpret_cast< char* >( &mapSize ), sizeof( size_t ) );

		for( const auto& [ k, v ] : rDir->Files )
		{
			RawSerialisation::WriteString( k, rStream );
			VFile::Serialise( v, rStream );
		}

		mapSize = rDir->Directories.size();
		rStream.write( reinterpret_cast< char* >( &mapSize ), sizeof( size_t ) );

		for( const auto& [k, v] : rDir->Directories )
		{
			RawSerialisation::WriteString( k, rStream );
			VDirectory::Serialise( v, rStream );
		}
	}

	void VirtualFS::ReadDir( Ref<VDirectory>& rDir, std::ifstream& rStream )
	{
		SAT_CORE_INFO( "Reading Dir with name: {0}", rDir->GetName() );

		size_t mapSize = 0;
		rStream.read( reinterpret_cast< char* >( &mapSize ), sizeof( size_t ) );

		for( size_t i = 0; i < mapSize; i++ )
		{
			std::string K{};
			K = RawSerialisation::ReadString( rStream );

			Ref<VFile> file = Ref<VFile>::Create();
			file->ParentDir = rDir.Get();

			VFile::Deserialise( file, rStream );

			rDir->Files[ K ] = file;
		}

		mapSize = 0;
		rStream.read( reinterpret_cast< char* >( &mapSize ), sizeof( size_t ) );

		for( size_t i = 0; i < mapSize; i++ )
		{
			std::string K{};
			K = RawSerialisation::ReadString( rStream );

			Ref<VDirectory> dir = Ref<VDirectory>::Create();
			dir->ParentDirectory = rDir.Get();

			VDirectory::Deserialise( dir, rStream );

			rDir->Directories[ K ] = dir;
		}
	}

	void VirtualFS::WriteVFS( std::ofstream& rStream )
	{
		SAT_CORE_INFO( "VFS: Writing to AssetBundle..." );

		RawSerialisation::WriteUnorderedMap( m_MountBases, rStream );
		WriteDir( m_RootDirectory, rStream );
	}

	void VirtualFS::LoadVFS( std::ifstream& rStream )
	{
		SAT_CORE_INFO( "VFS: Loading from AssetBundle..." );

		RawSerialisation::ReadUnorderedMap( m_MountBases, rStream );
		ReadDir( m_RootDirectory, rStream );

		// Build paths because we do not serialise them.
		for( auto& [ name, dir ] : m_RootDirectory->Directories )
		{
			// Skip mount base folder, move into the mount base and start building.
			for( auto& [subName, subDir] : dir->Directories )
			{
				BuildAllPathsInDir( subDir, name );
			}
		}
	}

	void VirtualFS::BuildAllPathsInDir( Ref<VDirectory>& rDir, const std::string& rMountBase )
	{
		BuildPath( rDir, rMountBase );

		for( auto& [name, file] : rDir->Files )
		{
			BuildPath( file, rMountBase );
		}

		for( auto& [name, dir] : rDir->Directories )
		{
			BuildAllPathsInDir( dir, rMountBase );
		}
	}

	//////////////////////////////////////////////////////////////////////////

	size_t VirtualFS::GetMountBases()
	{
		return m_MountBases.size();
	}

	size_t VirtualFS::GetMounts()
	{
		size_t mounts = 0;

		for( auto&& [name, rDir] : m_RootDirectory->Directories )
			mounts += GetMountsForDir( rDir );

		return mounts;
	}

	size_t VirtualFS::GetMountsForDir( Ref<VDirectory>& rDirectory )
	{
		size_t mounts = 0;

		for( auto&& [name, rDir] : rDirectory->Directories )
		{
			mounts += rDir->Files.size();
			mounts += rDir->Directories.size();

			mounts += GetMountsForDir( rDir );
		}

		return mounts;
	}

#if !defined(SAT_DIST)
	//////////////////////////////////////////////////////////////////////////
	// IMGUI

	void VirtualFS::DrawDirectory( const Ref<VDirectory>& rDirectory )
	{
		if( ImGui::TreeNode( rDirectory->GetName().c_str() ) )
		{
			for( const auto& [name, rDirectory] : rDirectory->Directories )
			{
				DrawDirectory( rDirectory );
			}

			for( auto&& [name, rFile] : rDirectory->Files )
			{
				ImGui::Selectable( name.c_str() );
			}

			ImGui::TreePop();
		}
	}

	void VirtualFS::ImGuiRender()
	{
		if( Auxiliary::TreeNode( "Mount Bases", false ) ) 
		{
			for( const auto& [name, rPath] : m_MountBases )
			{
				if( ImGui::TreeNode( name.c_str() ) )
				{
					ImGui::Text( "ID: %s", name.c_str() );
					ImGui::Text( "Path: %s", rPath.string().c_str() );

					ImGui::TreePop();
				}
			}

			Auxiliary::EndTreeNode();
		}

		if( Auxiliary::TreeNode( "Mounts", false ) )
		{
			for( const auto& [name, rDirectory] : m_RootDirectory->Directories )
			{
				DrawDirectory( rDirectory );
			}

			Auxiliary::EndTreeNode();
		}

		if( Auxiliary::TreeNode( "Path to Dir & File Map", false ) )
		{
			for( const auto& [mountBase, pathToDir] : m_PathToDir )
			{
				if( Auxiliary::TreeNode( mountBase.c_str(), false ) )
				{
					for( const auto& [path, dir] : pathToDir )
					{
						ImGui::Text( "Mapped to Dir name: %s", dir->GetName().c_str() );
						ImGui::Text( "Path: %s", path.string().c_str() );
					}

					Auxiliary::EndTreeNode();
				}
			}

			for( const auto& [mountBase, pathToDir] : m_PathToFile )
			{
				if( Auxiliary::TreeNode( mountBase.c_str(), false ) )
				{
					for( const auto& [path, file] : pathToDir )
					{
						ImGui::Text( "Mapped to Dir name: %s", file->Name.c_str() );
						ImGui::Text( "Path: %s", path.string().c_str() );
					}

					Auxiliary::EndTreeNode();
				}
			}

			Auxiliary::EndTreeNode();
		}
	}
#endif

}
