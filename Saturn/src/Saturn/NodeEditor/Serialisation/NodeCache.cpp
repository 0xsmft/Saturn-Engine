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
#include "NodeCache.h"

#include "Saturn/Asset/AssetManager.h"

#if defined(SAT_DIST)
#include "Saturn/Core/VirtualFS.h"
#include "Saturn/Core/MemoryStream.h"
#endif

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// NODE CACHE | SETTINGS

	struct SettingsFileHeader
	{
		char Magic[ 5 ] = { '.', 'N', 'C', 'S', '\0' };
		size_t SettingsCount = 0;
		uint64_t Version = SAT_CURRENT_VERSION;
	};
	
	bool NodeCacheSettings::WriteEditorSettings( Ref<NodeEditorBase> rNodeEditor )
	{
		std::filesystem::path filepath = Project::GetActiveProject()->GetAppDataFolder();

		if( !std::filesystem::exists( filepath ) )
			std::filesystem::create_directories( filepath );

		filepath /= "NodeCache-Settings";
		filepath.replace_extension( ".ncs" );

		if( CanAppendFile( filepath ) )
			AppendFile( filepath, rNodeEditor );
		else
			OverrideFile( filepath, rNodeEditor );

		return true;
	}

	void NodeCacheSettings::ReadEditorSettings( Ref<NodeEditorBase> rNodeEditor )
	{
		std::filesystem::path filepath = Project::GetActiveProject()->GetAppDataFolder();

		if( !std::filesystem::exists( filepath ) )
			return;

		filepath /= "NodeCache-Settings";
		filepath.replace_extension( ".ncs" );

		std::ifstream stream( filepath, std::ios::binary | std::ios::in );

		SettingsFileHeader fileHeader{};
		RawSerialisation::ReadObject( fileHeader, stream );

		if( strcmp( fileHeader.Magic, ".NCS\0" ) )
		{
			return;
		}

		// Get all currently saved states
		// TODO: Cache in memory
		std::map<uint64_t, std::string> stateMap;

		RawSerialisation::ReadMap( stateMap, stream );

		stream.close();

		const auto Itr = stateMap.find( rNodeEditor->GetAssetID() );

		// New node editor is being cached so update file header
		if( Itr != stateMap.end() )
		{
			rNodeEditor->m_ActiveNodeEditorState = Itr->second;
		}
	}

	bool NodeCacheSettings::CanAppendFile( const std::filesystem::path& rFilepath )
	{
		std::ifstream stream( rFilepath, std::ios::binary | std::ios::in | std::ios::ate );

		auto end = stream.tellg();
		stream.seekg( 0, std::ios::beg );
		auto size = end - stream.tellg();

		stream.close();

		return size != 0;
	}

	void NodeCacheSettings::OverrideFile( const std::filesystem::path& rFilepath, Ref<NodeEditorBase> rNodeEditor )
	{
		SettingsFileHeader fileHeader;
		fileHeader.SettingsCount++;

		std::ofstream fout( rFilepath, std::ios::binary | std::ios::trunc );

		std::map< uint64_t, std::string > stateMap;
		stateMap[ rNodeEditor->GetAssetID() ] = rNodeEditor->m_ActiveNodeEditorState;

		RawSerialisation::WriteObject( fileHeader, fout );
		RawSerialisation::WriteMap( stateMap, fout );

		fout.close();
	}

	void NodeCacheSettings::AppendFile( const std::filesystem::path& rFilepath, Ref<NodeEditorBase> rNodeEditor )
	{
		std::ifstream stream( rFilepath, std::ios::binary | std::ios::in );

		SettingsFileHeader fileHeader{};
		RawSerialisation::ReadObject( fileHeader, stream );

		if( strcmp( fileHeader.Magic, ".NCS\0" ) )
		{
			OverrideFile( rFilepath, rNodeEditor );
			return;
		}

		// Get all currently saved states
		std::map<uint64_t, std::string> stateMap;
		
		RawSerialisation::ReadMap( stateMap, stream );

		stream.close();

		const auto Itr = stateMap.find( rNodeEditor->GetAssetID() );

		// New node editor is being cached so update file header
		if( Itr == stateMap.end() )
		{
			fileHeader.SettingsCount++;

			stateMap[ rNodeEditor->GetAssetID() ] = rNodeEditor->m_ActiveNodeEditorState;
		}
		else
		{
			Itr->second = rNodeEditor->m_ActiveNodeEditorState;
		}

		std::ofstream fout( rFilepath, std::ios::binary | std::ios::trunc );

		RawSerialisation::WriteObject( fileHeader, fout );

		RawSerialisation::WriteMap( stateMap, fout );

		fout.close();
	}

	//////////////////////////////////////////////////////////////////////////
	// NODE CACHE | EDITOR

	struct NodeCacheEditorHeader
	{
		const char Magic[ 6 ] = ".NCE\0";
		AssetID AssetID = 0;
		uint32_t Version = SAT_CURRENT_VERSION;
	};
	
	static void CreateDirIfNeeded()
	{
		std::filesystem::path dir = Project::GetActiveProject()->GetFullCachePath();
		dir /= "NodeEdCache";

		if( !std::filesystem::exists( dir ) )
			std::filesystem::create_directories( dir );
	}

	static std::filesystem::path GetDefaultCachePath()
	{
		CreateDirIfNeeded();

		std::filesystem::path dir = Project::GetActiveProject()->GetFullCachePath();
		dir /= "NodeEdCache";

		return dir;
	}

	void NodeCacheEditor::WriteNodeEditorCache( Ref<NodeEditorBase> nodeEditor, const std::string& rCustomName )
	{
		Ref<Asset> asset = AssetManager::Get().FindAsset( nodeEditor->GetAssetID() );
		std::string filename;
		std::filesystem::path assetPath;

		if( asset )
		{
			filename = std::format( "{0}.{1}.nce", asset->Name, ( uint64_t ) nodeEditor->GetAssetID() );
			
			assetPath = AssetManager::Get().FindAsset( nodeEditor->GetAssetID() )->Path;
			assetPath = assetPath.parent_path();
			assetPath = Project::GetActiveProject()->FilepathAbs( assetPath );
		}
		else
		{
			filename = std::format( "NCEditor.{0}.nce", ( uint64_t ) nodeEditor->GetAssetID() );

			assetPath = GetDefaultCachePath();
		}

		if( !rCustomName.empty() )
			filename = rCustomName;

		assetPath /= filename;

		std::ofstream fout( assetPath, std::ios::binary | std::ios::trunc );

		NodeCacheEditorHeader header{};
		header.AssetID = nodeEditor->GetAssetID();

		RawSerialisation::WriteObject( header, fout );

		nodeEditor->SerialiseData( fout );

		fout.close();
	}

	bool NodeCacheEditor::ReadNodeEditorCache( Ref<NodeEditorBase> nodeEditor, AssetID id, const std::string& rCustomName )
	{
		std::string filename;
		
		Ref<Asset> asset = AssetManager::Get().FindAsset( id );
		std::filesystem::path cachePath;

		if( asset )
		{
			filename = std::format( "{0}.{1}.nce", asset->Name, ( uint64_t ) id );
			cachePath = asset->Path.parent_path();
		}
		else
		{
			filename = std::format( "NCEditor.{0}.nce", ( uint64_t ) id );
			cachePath = GetDefaultCachePath();
		}

		if( !rCustomName.empty() )
			filename = rCustomName;

		cachePath /= filename;

#if defined( SAT_DIST )
		const std::string& rMountBase = Project::GetActiveConfig().Name;
		Ref<VFile> file = VirtualFS::Get().FindFile( rMountBase, cachePath );

		if( !file )
		{
			SAT_CORE_ERROR( "NodeCache VFile does not exist" );
			return false;
		}

		PakFileMemoryBuffer membuf( file->FileContent );

		std::istream stream( &membuf );
#else
		std::filesystem::path cachePathAbs = Project::GetActiveProject()->FilepathAbs( cachePath );

		if( !std::filesystem::exists( cachePathAbs ) )
			return false;

		std::ifstream stream( cachePathAbs, std::ios::binary | std::ios::in );
#endif

		NodeCacheEditorHeader header;
		RawSerialisation::ReadObject( header, stream );

		if( strcmp( header.Magic, ".NCE\0" ) )
		{
			SAT_CORE_ERROR( "Invalid node editor cache file header or corrupt cache file!" );
			return false;
		}

		bool needsUpdate = false;
		if( header.Version != SAT_CURRENT_VERSION )
		{
			std::string decodedAssetBundleVer;
			SAT_DECODE_VER_STRING( header.Version, decodedAssetBundleVer );

			SAT_CORE_WARN( "Node Editor Cache version mismatch! This should not happen. Cache file version is: {0} while current engine version is: {1}.", decodedAssetBundleVer, SAT_CURRENT_VERSION_STRING );
			SAT_CORE_WARN( "The engine will continue to load however this may result in the cache file not loading!" );
		
			needsUpdate = true;
			return false;
		}

		if( header.AssetID != id )
		{
			SAT_CORE_ERROR( "Node editor cache file asset id mismatch! Saved ID was: {0} however ID passed in was {1}", header.AssetID, id );
			return false;
		}

		nodeEditor->m_AssetID = id;
		nodeEditor->DeserialiseData( stream );

#if !defined(SAT_DIST)
		stream.close();
#endif

		return true;
	}

	void NodeCacheEditor::ConvertToDistNC( AssetID id, const std::string& rCustomName /*= "" */ )
	{
		std::string filename;

		Ref<Asset> asset = AssetManager::Get().FindAsset( id );
		std::filesystem::path cachePath;

		if( asset )
		{
			filename = std::format( "{0}.{1}.nce", asset->Name, ( uint64_t ) id );
			cachePath = asset->Path.parent_path();
		}
		else
		{
			filename = std::format( "NCEditor.{0}.nce", ( uint64_t ) id );
			cachePath = GetDefaultCachePath();
		}

		if( !rCustomName.empty() )
			filename = rCustomName;

		cachePath /= filename;

		std::filesystem::path cachePathAbs = Project::GetActiveProject()->FilepathAbs( cachePath );

		if( !std::filesystem::exists( cachePathAbs ) )
			return;

		// Copy for dist
		std::filesystem::path newPath = Project::GetActiveProject()->GetTempDir();
		newPath /= std::to_string( id );
		newPath.replace_extension( ".vfs" );

		std::filesystem::copy_file( cachePathAbs, newPath );
	}

}