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
#include "AssetBundle.h"

#include "Saturn/Asset/Asset.h"
#include "Saturn/Asset/AssetManager.h"

#include "Saturn/Core/VirtualFS.h"

#include "Saturn/Project/Project.h"
#include "Saturn/Serialisation/Raw/RawSerialisation.h"
#include "Saturn/Serialisation/Raw/RawAssetSerialisers.h"

#include "Saturn/NodeEditor/Serialisation/NodeCache.h"

#include "Saturn/Asset/TextureSourceAsset.h"
#include "Saturn/Asset/Prefab.h"
#include "Saturn/Asset/MaterialAsset.h"
#include "Saturn/Audio/SoundSpecification.h"
#include "Saturn/Audio/GraphSound.h"
#include "Saturn/AI/BehaviourTree/BlackboardSpecificationAsset.h"
#include "Saturn/Animation/SkeletonAsset.h"
#include "Saturn/Animation/SkeletalAnimationAsset.h"
#include "Saturn/Physics/PhysicsMaterialAsset.h"
#include "Saturn/Alura/AluraStylingProfile.h"
#include "Saturn/Alura/AluraFont.h"
#include "Saturn/NodeEditor/GlobalNodeEditorTaskCache.h"

#include "Saturn/Serialisation/YAML/SceneSerialiser.h"

#include <zlib.h>

namespace Saturn {

	struct AssetBundleHeader
	{
		// .AB + null
		const unsigned char Magic[ 4 ] = { 0x2E, 0x41, 0x42, 0x00 };
		uint32_t Version = 0u;
		size_t Assets = 0llu;
	};

	struct AssetBundleMinimalHeader
	{
		// .AB + null
		const unsigned char Magic[ 4 ] = { 0x2E, 0x41, 0x42, 0x00 };
		uint32_t Version = 0u;
	};

	struct DumpFileHeader
	{
		// .PAK
		const unsigned char Magic[ 4 ] = { 0x2E, 0x50, 0x41, 0x4B };
		uint32_t Version = 0;
		AssetID Asset = 0;
		uint64_t OrginalSize = 0;
		uint64_t CompressedSize = 0;
		uint64_t Offset = 0;

		DumpFileHeader& operator=( const DumpFileHeader& rOther ) noexcept
		{
			if( &rOther == this )
				return *this;

			Asset = rOther.Asset;
			OrginalSize = rOther.OrginalSize;
			CompressedSize = rOther.CompressedSize;
			Offset = rOther.Offset;
			Version = rOther.Version;

			return *this;
		}
	};

	static void CreateTempDirIfNeeded()
	{
		std::filesystem::path tempDir = Project::GetActiveProject()->GetRootDir();
		tempDir /= "Temp";

		if( !std::filesystem::exists( tempDir ) )
			std::filesystem::create_directories( tempDir );
	}

	AssetBundleResult AssetBundle::BundleAssets( Ref<JobProgress>& jobProgress )
	{
		jobProgress->Reset();
		jobProgress->SetTitle( "AssetBundle - Init (Minimal)" );

		BundleMinimal();

		std::filesystem::path cachePath = Project::GetActiveProject()->GetFullCachePath();

		if( !std::filesystem::exists( cachePath ) )
			std::filesystem::create_directories( cachePath );

		cachePath /= "AssetBundle.sab";

		Timer timer;

		AssetManager* pAssetManager = AssetManager::Get();
		Ref<Project> ActiveProject = Project::GetActiveProject();

		const std::string& rMountBase = Project::GetActiveConfig().Name;

		Ref<AssetRegistry> AssetBundleRegistry = Ref<AssetRegistry>::Create();
		AssetBundleRegistry->CopyFrom( pAssetManager->GetAssetRegistry() );

		auto& rVFS = VirtualFS::Get();

		// Start by dumping all of the assets
		CreateTempDirIfNeeded();

		std::unordered_map<std::filesystem::path, AssetID> DumpFileToAssetID;

		jobProgress->SetTitle( "Loading assets..." );

		// THREAD-TRANSTION, Block main thread
		Application::Get()->SuspendMainThreadCV();

		for( auto& [id, asset] : AssetBundleRegistry->GetAssetMap() )
		{
			SAT_CORE_INFO( "Dumping asset to disk: {0}", asset->Name );

			RTDumpAsset( asset, AssetBundleRegistry );

			std::filesystem::path p = ActiveProject->GetTempDir() / std::to_string( id );
			asset->Type == AssetType::Sound ? p.replace_extension( ".vfsn" ) : p.replace_extension( ".vfs" );

			DumpFileToAssetID[ p ] = id;
		}

		// Always do scenes last because when we load the Asset Bundle we want all the assets to be loaded before this.
		for( auto& [id, asset] : AssetBundleRegistry->GetAssetMap() )
		{
			if( asset->Type != AssetType::Scene )
				continue;

			Ref<Scene> scene = Ref<Scene>::Create();
			scene->Path = asset->Path;
			scene->ID = asset->ID;
			scene->Name = asset->Name;

			// Load from disk
			SceneSerialiser serialiser( scene );
			serialiser.Deserialise( asset );

			// Serialise
			scene->SerialiseData();
		}

		// THREAD-TRANSTION, Resume main thread
		Application::Get()->ResumeMainThreadCV();

		SAT_CORE_INFO( "Dumped {0} asset(s)", pAssetManager->GetAssetRegistrySize() );

		jobProgress->SetProgress( 10.0f );
		jobProgress->SetTitle( "Building AssetBundle" );

		/////////////////////////////////////

		std::ofstream fout( cachePath, std::ios::binary | std::ios::trunc );

		AssetBundleHeader header{};
		header.Assets = pAssetManager->GetAssetRegistrySize();
		header.Version = SAT_CURRENT_VERSION;

		RawSerialisation::WriteObject( header, fout );

		// Write asset header data.
		for( auto& [id, asset] : AssetBundleRegistry->GetAssetMap() )
		{
			SAT_CORE_INFO( "Writing header information for asset: ASSET/{0} ({1})", id, asset->Name );
			const std::string status = std::format( "Writing header information for asset: ASSET/{0} ({1})", ( uint64_t ) id, asset->Name );

			jobProgress->SetStatus( status );

			asset->SerialiseData( fout );

			rVFS.Mount( rMountBase, asset->Path );
		}

		jobProgress->SetProgress( 35.0f );
		jobProgress->SetStatus( "Writing VFS" );

		/////////////////////////////////////

		VirtualFS::Get().WriteVFS( fout );

		/////////////////////////////////////

		uint64_t offset = 0;

		// Next, now that we have dumped all of the assets we can now pack and compress the assets.
		// And we also make sure that we write the uncompressed/compressed file data + the header into the VFS.
		for( const auto& rEntry : std::filesystem::directory_iterator( Project::GetActiveProject()->GetTempDir() ) )
		{
			if( !rEntry.is_regular_file() )
				continue;

			std::filesystem::path path = rEntry.path();

			Buffer fileBuffer;
			std::ifstream stream( path, std::ios::binary | std::ios::in | std::ios::ate );

			const uint64_t fileSize = stream.tellg();

			DumpFileHeader dfh;
			dfh.Asset = DumpFileToAssetID.at( path );
			dfh.Version = SAT_CURRENT_VERSION;
			dfh.OrginalSize = fileSize;
			dfh.Offset = offset;

			offset += fileSize;

			stream.seekg( 0 );

			fileBuffer.Allocate( ( size_t ) fileSize );
			stream.read( reinterpret_cast< char* >( fileBuffer.Data ), fileBuffer.Size );

			stream.close();

			// Compression, allow for files under 500KB (0.5MB) to not be compressed.
			if( fileSize > 500llu * 1024llu && path.extension() != ".vfsn" )
			{
				SAT_CORE_WARN( "Compressing file: {0} because file is {1} KiB", path.string(), fileSize / 1024 );

				jobProgress->SetStatus( std::format( "Compressing file: {0}", path.string() ) );

				// Compress, file over the limit.
				Buffer TemporaryBuffer;
				TemporaryBuffer.Allocate( ( size_t ) compressBound( ( uLong ) fileSize ) );

				uLongf compressedSize = ( uLongf ) TemporaryBuffer.Size;

				int result = compress(
					( Bytef* ) TemporaryBuffer.Data, &compressedSize,
					( Bytef* ) fileBuffer.Data, static_cast< uLong >( fileBuffer.Size ) );

				if( result != Z_OK )
				{
					SAT_CORE_ERROR( "Failed to compress {0}! zlib error is: {1}. Writing uncompressed data.", path.string(), result );

					RawSerialisation::WriteObject( dfh, fout );
					RawSerialisation::WriteSaturnBuffer( fileBuffer, fout );
				}

				SAT_CORE_INFO( "Compressed file: {0} new file size is {1} KiB", path.string(), compressedSize / 1024 );

				Buffer compressedData = Buffer::Copy( TemporaryBuffer.Data, compressedSize );
				dfh.CompressedSize = compressedSize;

				RawSerialisation::WriteObject( dfh, fout );
				RawSerialisation::WriteSaturnBuffer( compressedData, fout );

				compressedData.Free();
				TemporaryBuffer.Free();
			}
			else
			{
				dfh.CompressedSize = dfh.OrginalSize;

				SAT_CORE_WARN( "Not compressing file: {0} because file size is less than 500 KiB", path.string() );
				jobProgress->SetStatus( std::format( "Not Compressing file because file size is less than 500 KiB: {0}", path.string() ) );

				RawSerialisation::WriteObject( dfh, fout );
				RawSerialisation::WriteSaturnBuffer( fileBuffer, fout );
			}

			jobProgress->AddProgress( ( 1.0f + jobProgress->GetProgress() ) / DumpFileToAssetID.size() );

			fileBuffer.Free();
		}

		SAT_CORE_INFO( "Packaged {0} asset(s)", pAssetManager->GetAssetRegistrySize() );
		SAT_CORE_INFO( "Asset bundle built in {0}s", timer.Elapsed() / 1000 );

		fout.close();

		jobProgress->SetProgress( 100.0f );
		jobProgress->SetStatus( "Done" );

		DumpFileToAssetID.clear();

		// Delete the temp folder.
		std::filesystem::remove_all( ActiveProject->GetTempDir() );

		AssetBundleRegistry = nullptr;

		jobProgress->OnComplete();

		return AssetBundleResult::Success;
	}

	void AssetBundle::RTDumpAsset( const Ref<Asset>& rAsset, Ref<AssetRegistry>& AssetBundleRegistry )
	{
		UUID id = rAsset->ID;
		AssetManager* pAssetManager = AssetManager::Get();

		// Load the asset and dump memory into it's temporary file.
		// NOTE: We use the asset manager but ask it to load the asset into our asset registry.
		// NOTE: Asset manager will use it own importer which is fine as it will be the YAML importer.
		switch( rAsset->Type )
		{
			case Saturn::AssetType::Texture:
			{
				Ref<TextureSourceAsset> textureSourceAsset = pAssetManager->ImportAssetAs<TextureSourceAsset>( AssetBundleRegistry, id );
				if( textureSourceAsset )
				{
					textureSourceAsset->WriteToVFS();
				}
			} break;

			case Saturn::AssetType::StaticMesh:
			{
				Ref<StaticMesh> mesh = pAssetManager->ImportAssetAs<StaticMesh>( AssetBundleRegistry, id );
				if( mesh )
				{
					RawStaticMeshAssetSerialiser serialiser;
					serialiser.DumpAndWriteToVFS( mesh );
				}
			} break;

			case Saturn::AssetType::SkeletalMesh:
			{
				Ref<SkeletalMesh> skMesh = pAssetManager->ImportAssetAs<SkeletalMesh>( AssetBundleRegistry, id );
				if( skMesh )
				{
					RawSkeletalMeshAssetSerialiser serialiser;
					serialiser.DumpAndWriteToVFS( skMesh );
				}
			} break;

			case Saturn::AssetType::Material:
			{
				Ref<MaterialAsset> materialAsset = pAssetManager->ImportAssetAs<MaterialAsset>( AssetBundleRegistry, id );
				if( materialAsset )
				{
					RawMaterialAssetSerialiser serialiser;
					serialiser.DumpAndWriteToVFS( materialAsset );
				}
			} break;

			case Saturn::AssetType::Sound:
			{
				Ref<SoundSpecification> sndSpec = pAssetManager->ImportAssetAs<SoundSpecification>( AssetBundleRegistry, id );
				if( sndSpec )
				{
					RawSoundSpecAssetSerialiser serialiser;
					serialiser.DumpAndWriteToVFS( sndSpec );
				}
			} break;

			case Saturn::AssetType::BehaviourTree:
			{
				// ...otherwise load it from disk.
				const std::string filename = std::format( "{0}.sbt", rAsset->Name );

				// Try to load without touching the disk...
				auto& rCache = GlobalNodeEditorTaskCache::Get().GetOrCreateTaskCache( id );
				if( rCache.IsListEmpty() )
				{
					// Read...
					NodeCacheEditor::ReadNodeTaskCacheOnly( rCache, id, filename );
				}

				// Write...
				std::filesystem::path out = Project::GetActiveProject()->GetTempDir();
				out /= std::to_string( rAsset->ID );
				out.replace_extension( ".vfs" );

				NodeCacheEditor::WriteTaskCacheOnlyDistNC( rCache, out );
			} break;

			case Saturn::AssetType::AnimationController:
			{
				// ...otherwise load it from disk.
				const std::string filename = std::format( "{0}.sac", rAsset->Name );

				// Try to load without touching the disk...
				auto& rCache = GlobalNodeEditorTaskCache::Get().GetOrCreateTaskCache( id );
				if( rCache.IsListEmpty() )
				{
					// Read...
					NodeCacheEditor::ReadNodeTaskCacheOnly( rCache, id, filename );
				}

				// Write...
				std::filesystem::path out = Project::GetActiveProject()->GetTempDir();
				out /= std::to_string( rAsset->ID );
				out.replace_extension( ".vfs" );

				NodeCacheEditor::WriteTaskCacheOnlyDistNC( rCache, out );
			} break;

			case Saturn::AssetType::GraphSound:
			{
				// ...otherwise load it from disk.
				const std::string filename = std::format( "{0}.gsnd", rAsset->Name );

				// Try to load without touching the disk...
				auto& rCache = GlobalNodeEditorTaskCache::Get().GetOrCreateTaskCache( id );
				if( rCache.IsListEmpty() )
				{
					// Read...
					NodeCacheEditor::ReadNodeTaskCacheOnly( rCache, id, filename );
				}

				// Write...
				std::filesystem::path out = Project::GetActiveProject()->GetTempDir();
				out /= std::to_string( rAsset->ID );
				out.replace_extension( ".vfs" );

				NodeCacheEditor::WriteTaskCacheOnlyDistNC( rCache, out );
			} break;

			case Saturn::AssetType::Prefab:
			{
				Ref<Prefab> prefabAsset = pAssetManager->ImportAssetAs<Prefab>( AssetBundleRegistry, id );
				if( prefabAsset )
				{
					RawPrefabSerialiser serialiser;
					serialiser.DumpAndWriteToVFS( prefabAsset );
				}
			} break;

			case Saturn::AssetType::Skeleton:
			{
				Ref<SkeletonAsset> skeletonAsset = pAssetManager->ImportAssetAs<SkeletonAsset>( AssetBundleRegistry, id );
				if( skeletonAsset )
				{
					RawSkeletonAssetSerialiser serialiser;
					serialiser.DumpAndWriteToVFS( skeletonAsset );
				}
			} break;

			case Saturn::AssetType::PhysicsMaterial:
			{
				Ref<PhysicsMaterialAsset> physAsset = pAssetManager->ImportAssetAs<PhysicsMaterialAsset>( AssetBundleRegistry, id );
				if( physAsset )
				{
					RawPhysicsMaterialAssetSerialiser serialiser;
					serialiser.DumpAndWriteToVFS( physAsset );
				}
			} break;

			case Saturn::AssetType::BehaviourTreeMemory:
			{
				Ref<BlackboardSpecificationAsset> btMemAsset = pAssetManager->ImportAssetAs<BlackboardSpecificationAsset>( AssetBundleRegistry, id );
				if( btMemAsset )
				{
					RawBlackboardSpecSerialiser serialiser;
					serialiser.DumpAndWriteToVFS( btMemAsset );
				}
			} break;

			case Saturn::AssetType::SkeletalAnimation:
			{
				Ref<SkeletalAnimationAsset> animAsset = pAssetManager->ImportAssetAs<SkeletalAnimationAsset>( AssetBundleRegistry, id );
				if( animAsset )
				{
					RawSkeletalAnimationSerialiser serialiser;
					serialiser.DumpAndWriteToVFS( animAsset );
				}
			} break;

			case Saturn::AssetType::Font:
			{
				Ref<AluraFont> fontAsset = pAssetManager->ImportAssetAs<AluraFont>( AssetBundleRegistry, id );
				if( fontAsset )
				{
					RawFontSerialiser serialiser;
					serialiser.DumpAndWriteToVFS( fontAsset );
				}
			} break;

			case Saturn::AssetType::StyleProfile:
			{
				Ref<AluraStylingProfile> styleProf = pAssetManager->ImportAssetAs<AluraStylingProfile>( AssetBundleRegistry, id );
				if( styleProf )
				{
					RawSkeletalAnimationSerialiser serialiser;
					serialiser.DumpAndWriteToVFS( styleProf );
				}
			} break;

			case Saturn::AssetType::Scene:
			case Saturn::AssetType::MaterialInstance:
			case Saturn::AssetType::Unknown: break;
			default:
				SAT_CORE_WARN( "Unhandled AssetType! {0}", AssetTypeToString( rAsset->Type ) );
				Core::BreakDebug();
				break;
		}
	}

	AssetBundleResult AssetBundle::ReadBundle()
	{
		std::filesystem::path cachePath = Project::GetActiveProject()->GetFullCachePath();
		cachePath /= "AssetBundle.sab";

		if( !std::filesystem::exists( cachePath ) )
			return AssetBundleResult::FileNotFound;

		Timer timer;

		std::ifstream stream( cachePath, std::ios::binary | std::ios::in );

		AssetBundleHeader header{};
		RawSerialisation::ReadObject( header, stream );

		if( std::memcmp( header.Magic, ".AB", 4 ) != 0 )
		{
			SAT_CORE_ERROR( "Invalid asset bundle file header or corrupt asset bundle file!" );
			return AssetBundleResult::InvalidFileHeader;
		}

		if( header.Version != SAT_CURRENT_VERSION )
		{
			std::string decodedAssetBundleVer;
			SAT_DECODE_VER_STRING( header.Version, decodedAssetBundleVer );

			SAT_CORE_ERROR( "Asset bundle version mismatch! This should not happen. Asset bundle version is: {0} while current Engine version is: {1}.", decodedAssetBundleVer, SAT_CURRENT_VERSION_STRING );

			SAT_CORE_ERROR( "To developers: Have you forgot to rebuilt the Asset Bundle after rebuilding the project for disturbution or was the Asset Bundle built successfully?" );

			return AssetBundleResult::FileVersionMismatch;
		}

		AssetManager* pAssetManager = AssetManager::Get();
		Ref<AssetRegistry> assetRegistry = pAssetManager->GetAssetRegistry();
		VirtualFS& rVFS = VirtualFS::Get();

		const std::string& rMountBase = Project::GetActiveConfig().Name;

		assetRegistry->m_Assets.reserve( header.Assets );

		// Read header information
		for( size_t i = 0; i < header.Assets; ++i )
		{
			Ref<Asset> asset = Ref<Asset>::Create();
			asset->DeserialiseData( stream );

			assetRegistry->m_Assets[ asset->ID ] = asset;

			SAT_CORE_INFO( "Read asset header info: ASSET/{0} ({1})", asset->ID, asset->Name );
		}

		// Load the VFS
		rVFS.LoadVFS( stream );

		std::vector<DumpFileHeader> FileEntries( header.Assets );

		// Iterate over all of the assets again. But this time read compressed file.
		for( size_t i = 0; i < header.Assets; ++i )
		{
			DumpFileHeader dfh;
			RawSerialisation::ReadObject( dfh, stream );

			if( !assetRegistry->DoesIDExists( dfh.Asset ) )
				continue;

			Ref<Asset>& rAsset = assetRegistry->m_Assets[ dfh.Asset ];

			if( std::memcmp( dfh.Magic, ".PAK", 4 ) != 0 )
			{
				SAT_CORE_ERROR( "Invalid pack file header!" );

				return AssetBundleResult::InvalidPackFileHeader;
			}

			if( rAsset->ID != dfh.Asset )
			{
				SAT_CORE_ERROR( "Asset ID's do not match!" );

				return AssetBundleResult::AssetIDMismatch;
			}

			// The version that the file was generated in much also match the engine version.
			if( dfh.Version != SAT_CURRENT_VERSION )
			{
				SAT_CORE_ERROR( "Pack file version mismatch!" );

				return AssetBundleResult::PackFileVersionMismatch;
			}

			// Find the VFile
			Ref<VFile> file = rVFS.FindFile( rMountBase, rAsset->Path );
			if( !file )
				return AssetBundleResult::FileNotFound;

			if( dfh.OrginalSize != dfh.CompressedSize )
			{
				SAT_CORE_INFO( "Decompressing file at offset {0}, AssetID/{1}", dfh.Offset, dfh.Asset );

				// Compression was used, uncompress. 
				Buffer uncompressedData;
				uncompressedData.Allocate( dfh.OrginalSize );

				Buffer compressedData;
				RawSerialisation::ReadSaturnBuffer( compressedData, stream );

				uLongf uncompSize = ( uLongf ) uncompressedData.Size;

				const int result = uncompress(
					( Bytef* ) uncompressedData.Data, &uncompSize,
					( Bytef* ) compressedData.Data, static_cast< const uLong >( compressedData.Size ) );

				if( result != Z_OK )
				{
					SAT_CORE_ERROR( "Failed to uncompress data!" );

					return AssetBundleResult::FailedToUncompress;
				}

				compressedData.Free();

				file->FileContent.resize( uncompSize );
				std::memcpy( file->FileContent.data(), uncompressedData.Data, uncompressedData.Size );

				uncompressedData.Free();
			}
			else
			{
				SAT_CORE_INFO( "Loading uncompressed file at offset {0} AssetID/{1}", dfh.Offset, dfh.Asset );

				Buffer uncompressedData;
				RawSerialisation::ReadSaturnBuffer( uncompressedData, stream );

				file->FileContent.resize( uncompressedData.Size );
				std::memcpy( file->FileContent.data(), uncompressedData.Data, uncompressedData.Size );

				uncompressedData.Free();
			}

			FileEntries[ i ] = dfh;
		}

		SAT_CORE_INFO( "Done reading asset bundle in {0}s", timer.Elapsed() / 1000 );

		stream.close();

		FileEntries.clear();

		return AssetBundleResult::Success;
	}

	//////////////////////////////////////////////////////////////////////////
	// MINIMAL ASSET BUNDLE

	AssetBundleResult AssetBundle::BundleMinimal()
	{
		std::filesystem::path cachePath = Project::GetActiveProject()->GetFullCachePath();

		if( !std::filesystem::exists( cachePath ) )
			std::filesystem::create_directories( cachePath );

		cachePath /= "AssetBundle-Minimal.sab";

		Ref<Project> ActiveProject = Project::GetActiveProject();
		auto& rConfig = ActiveProject->GetConfig();

		std::ofstream fout( cachePath, std::ios::binary | std::ios::trunc );

		AssetBundleMinimalHeader header{};
		header.Version = SAT_CURRENT_VERSION;

		RawSerialisation::WriteObject( header, fout );

		// Write Project file & write Startup scene
		RawSerialisation::WriteString( rConfig.Name, fout );
		RawSerialisation::WriteObject( rConfig.StartupSceneID, fout );
		RawSerialisation::WriteObject( ActiveProject->GetDefaultMaterialAsset(), fout );
		RawSerialisation::WriteObject( ActiveProject->GetDefaultPhysicsMaterialAsset(), fout );
		RawSerialisation::WriteObject( ActiveProject->GetDefaultFontAsset(), fout );

		// Action Bindings
		// Write manually, avoid using our T::Serialise functions
		RawSerialisation::WriteObject( ActiveProject->GetActionBindings().size(), fout );

		for( const auto& rBinding : ActiveProject->GetActionBindings() )
		{
			RawSerialisation::WriteString( rBinding.Name, fout );

			RawSerialisation::WriteObject( ( uint16_t ) rBinding.Key, fout );
			RawSerialisation::WriteObject( ( uint8_t ) rBinding.MouseButton, fout );
			RawSerialisation::WriteObject( ( uint8_t ) rBinding.Type, fout );
		}

		// Sound Groups
		// Write manually, avoid using our T::Serialise functions
		RawSerialisation::WriteObject( ActiveProject->GetSoundGroups().size(), fout );

		for( const auto& rSoundGroup : ActiveProject->GetSoundGroups() )
		{
			RawSerialisation::WriteString( rSoundGroup->GetName(), fout );

			RawSerialisation::WriteObject( rSoundGroup->GetVolume(), fout );
			RawSerialisation::WriteObject( rSoundGroup->GetPitch(), fout );
		}

		fout.close();

		return AssetBundleResult::Success;
	}

	AssetBundleResult AssetBundle::ReadMinimal()
	{
		std::filesystem::path cachePath = std::filesystem::current_path() / "Cache" / "AssetBundle-Minimal.sab";

		if( !std::filesystem::exists( cachePath ) )
			return AssetBundleResult::FileNotFound;

		ProjectConfig newConfig{};

		std::ifstream stream( cachePath, std::ios::binary | std::ios::in );

		AssetBundleMinimalHeader header;
		RawSerialisation::ReadObject( header, stream );

		if( std::memcmp( header.Magic, ".AB", 4 ) != 0 )
		{
			SAT_CORE_ERROR( "Invalid AB-Minimal file header or corrupt asset bundle file!" );
			return AssetBundleResult::InvalidFileHeader;
		}

		if( header.Version != SAT_CURRENT_VERSION )
		{
			return AssetBundleResult::FileVersionMismatch;
		}

		newConfig.Name = RawSerialisation::ReadString( stream );
		RawSerialisation::ReadObject( newConfig.StartupSceneID, stream );

		// Create project
		Ref<Project> newProject = Ref<Project>::Create( newConfig );

		UUID defMatAsset = 0;
		RawSerialisation::ReadObject( defMatAsset, stream );
		newProject->SetDefaultMaterialAsset( defMatAsset );

		RawSerialisation::ReadObject( defMatAsset, stream );
		newProject->SetDefaultPhysicsMaterialAsset( defMatAsset );

		RawSerialisation::ReadObject( defMatAsset, stream );
		newProject->SetDefaultFontAsset( defMatAsset );

		size_t actionBindings = 0;
		RawSerialisation::ReadObject( actionBindings, stream );

		newProject->GetActionBindings().reserve( actionBindings );

		for( size_t i = 0; i < actionBindings; ++i )
		{
			ActionBindingData ab;

			ab.Name = RawSerialisation::ReadString( stream );

			RawSerialisation::ReadObject( ab.Key, stream );
			RawSerialisation::ReadObject( ab.MouseButton, stream );
			RawSerialisation::ReadObject( ab.Type, stream );

			newProject->AddActionBinding( ab );
		}

		// TODO: Read Audio groups
		// Disabled for now anyways.

		stream.close();

		Project::SetActiveProject( newProject );

		return AssetBundleResult::Success;
	}

}
