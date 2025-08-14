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
#include "ContentBrowserThumbnailCache.h"

#include "ThumbnailGenerator.h"

#include "Saturn/Asset/Asset.h"
#include "Saturn/Asset/TextureSourceAsset.h"

#include "Saturn/Project/Project.h"

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

#include "Saturn/Core/JobSystem.h"

#include "Saturn/ImGui/EditorIcons.h"
#include "Saturn/ImGui/ImGuiAuxiliary.h"

#include <imgui.h>

namespace Saturn {
	
	void ContentBrowserThumbnailCache::Init()
	{
		DeserialiseManifest();

		m_FileIcon = Ref<Texture2D>::Create( "content/textures/editor/FileIcon.png", AddressingMode::Repeat );
		m_FolderIcon = Ref<Texture2D>::Create( "content/textures/editor/DirectoryIcon.png", AddressingMode::Repeat );

		EditorIcons::AddIcon( m_FileIcon );
		EditorIcons::AddIcon( m_FolderIcon );
	}

	void ContentBrowserThumbnailCache::Terminate()
	{
		SerialiseManifest();

		m_Cache.clear();

		m_FileIcon = nullptr;
		m_FolderIcon = nullptr;
	}

	bool ContentBrowserThumbnailCache::AssetHasThumbnail( AssetID assetID )
	{
		return m_Cache.find( assetID ) != m_Cache.end();
	}

	void ContentBrowserThumbnailCache::UpdateCache()
	{
		// TODO: Fix double call to empty() 

		if( m_PendingManifestWrite && m_GenerationQueue.empty() )
		{
			SerialiseManifest();
			m_PendingManifestWrite = false;
		}

		while( !m_GenerationQueue.empty() )
		{
			auto& rData = m_GenerationQueue.front();

			if( rData.Asset->Type == AssetType::Texture || rData.Asset->Type == AssetType::Material || rData.Asset->Type == AssetType::StaticMesh )
			{
				// If it's somehow already in the cache pop it and move on to the next thumbnail
				const auto Itr = m_Cache.find( rData.Asset->ID );
				if( Itr != m_Cache.end() )
				{
					const auto& rData = Itr->second;
					if( rData.Time == rData.Time && rData.Texture != nullptr )
					{
						m_GenerationQueue.pop();
						continue;
					}
				}

				// Generate texture if not already in cache
				rData.Texture = m_Generator.GenerateForAssetType( rData );

				// Try again next frame (could be still generating), move on to the next thumbnail
				if( !rData.Texture )
				{
					m_GenerationQueue.pop();
					continue;
				}

				// Add to cache
				auto& rCacheData = m_Cache[ rData.Asset->ID ];
				rCacheData.Time = rData.Time;
				rCacheData.Texture = rData.Texture;
				rCacheData.AssetPath = rData.Asset->Path;
				rCacheData.ExistsOnFS = true;

				SerialiseSingleThumbnail(  rData.Asset->ID, rCacheData );

				m_PendingManifestWrite = true;
			}

			m_GenerationQueue.pop();

			break;
		}
	}

	void ContentBrowserThumbnailCache::ClearCache()
	{
		std::filesystem::path thumbnailPath = Project::GetActiveProject()->GetFullCachePath() / "PerUser" / "Thumbnails";

		std::filesystem::remove_all( thumbnailPath );

		m_Cache.clear();
	}

	Ref<Texture2D> ContentBrowserThumbnailCache::GetDefault( int Identifier )
	{
		return Identifier == 0 ? m_FolderIcon : m_FileIcon;
	}

	Ref<Texture2D> ContentBrowserThumbnailCache::GetFor( const Ref<Asset>& rAsset )
	{
		if( !rAsset )
			return GetDefault( 1 );

		Ref<Texture2D> texture = m_FileIcon;
		const auto Itr = m_Cache.find( rAsset->ID );
		
		// TODO: Don't get the last_write_time every frame and instead check once and use FileWatch
		auto fullPath = Project::GetActiveProject()->FilepathAbs( rAsset->Path );
		auto lastWriteTimePoint = std::filesystem::last_write_time( fullPath );
		auto timestamp = std::chrono::duration_cast< std::chrono::milliseconds >( lastWriteTimePoint.time_since_epoch() ).count();

		if( Itr != m_Cache.end() )
		{
			auto& rData = Itr->second;

			if( rData.Time == timestamp && rData.Texture != nullptr && rData.ExistsOnFS )
			{
				return rData.Texture;
			}
			else
			{
				// Load texture from the cache on job system
				if( rData.Time != timestamp || !DeserialiseSingleThumbnail( Itr->first, rData ) )
				{
					// Failed to load from cache, regenerate the image next frame.
					// We can do this by removing the image from the cache
					m_Cache.erase( Itr );
				}

				return texture; 
			}
		}

		// Generate texture & pass in needed information for cache data
		if( rAsset->Type == AssetType::Texture || rAsset->Type == AssetType::Material || rAsset->Type == AssetType::StaticMesh )
			m_GenerationQueue.push( { .Time = timestamp, .Texture = nullptr, .Asset = rAsset } );

		return texture;
	}

	void ContentBrowserThumbnailCache::OnModified( Ref<Asset> asset )
	{
	}

	void ContentBrowserThumbnailCache::Invalidate( Ref<Asset> asset )
	{
		const auto Itr = m_Cache.find( asset->ID );
		if( Itr != m_Cache.end() )
		{
			auto& rData = Itr->second;
		
			// TODO: Don't get the last_write_time every frame and instead check once and use FileWatch
			auto fullPath = Project::GetActiveProject()->FilepathAbs( asset->Path );
			auto lastWriteTimePoint = std::filesystem::last_write_time( fullPath );
			auto timestamp = std::chrono::duration_cast< std::chrono::milliseconds >( lastWriteTimePoint.time_since_epoch() ).count();

			rData.Time = timestamp;
			rData.Texture = nullptr;
			rData.ExistsOnFS = false;

			// Generate texture & pass in needed information for cache data
			if( asset->Type == AssetType::Texture || asset->Type == AssetType::Material || asset->Type == AssetType::StaticMesh )
				m_GenerationQueue.push( { .Time = timestamp, .Texture = nullptr, .Asset = asset } );
		}
	}

	void ContentBrowserThumbnailCache::RemoveThumbnail( AssetID id )
	{
		const auto Itr = m_Cache.find( id );
		if( Itr == m_Cache.end() )
			return;

		m_Cache.erase( Itr );
	
		// Delete file
		std::filesystem::path newPath = Project::GetActiveProject()->GetFullCachePath() / "PerUser" / "Thumbnails" / std::to_string( id );
		newPath.replace_extension( ".stc" );

		std::filesystem::remove( newPath );

		SerialiseManifest();
	}

#if !defined(SAT_DIST)
	//////////////////////////////////////////////////////////////////////////
	// ImGui

	void ContentBrowserThumbnailCache::OnImGuiRender( bool* pOpen )
	{
		if( ImGui::Begin( "ContentBrowserThumbnailCache", pOpen ) ) 
		{
			ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX | ImGuiTableFlags_NoBordersInBody;

			if( ImGui::BeginTable( "##DebugCBThumbnails", 5, tableFlags ) )
			{
				ImGui::TableSetupColumn( "Last Write Time" );
				ImGui::TableSetupColumn( "In Manifest" );
				ImGui::TableSetupColumn( "Asset ID" );
				ImGui::TableSetupColumn( "Delete", ImGuiTableColumnFlags_NoHeaderLabel );
				ImGui::TableSetupColumn( "Regen", ImGuiTableColumnFlags_NoHeaderLabel );

				ImGui::TableHeadersRow();

				for( const auto& [rID, rData] : m_Cache )
				{
					ImGui::PushID( static_cast< int >( rID ) );

					ImGui::TableNextRow();

					ImGui::TableNextColumn();
					ImGui::Text( "%llu", rData.Time );

					ImGui::TableNextColumn();
					ImGui::Text( "%s", rData.ExistsOnFS ? "true" : "false" );

					ImGui::TableNextColumn();
					ImGui::Text( "%llu", rID );

					ImGui::TableNextColumn();
					if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Bin" ), { 24.0f, 24.0f } ) )
					{
						RemoveThumbnail( rID );
						ImGui::PopID();
						break;
					}

					ImGui::TableNextColumn();
					if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Sync" ), { 24.0f, 24.0f } ) )
					{
						Ref<Asset> asset = AssetManager::Get().FindAsset( rID );
						Invalidate( asset );
					}

					ImGui::PopID();
				}

				ImGui::EndTable();
			}

			if( ImGui::Button( "Clear All" ) )
			{
				ClearCache();
			}
	
			ImGui::End();
		}
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Serialisation

	struct CacheManifestHeader
	{
		const char Magic[ 6 ] = ".STM\0";
		size_t Thumbnails = 0;
		uint32_t Version = SAT_CURRENT_VERSION;
	};

	static void CreateDirectoriesIfNeeded() 
	{
		std::filesystem::path newPath = Project::GetActiveProject()->GetFullCachePath() / "PerUser" / "Thumbnails";

		if( !std::filesystem::exists( newPath ) )
		{
			std::filesystem::create_directories( newPath );
		}
	}

	void ContentBrowserThumbnailCache::SerialiseManifest()
	{
		CreateDirectoriesIfNeeded();

		std::filesystem::path cachePath = Project::GetActiveProject()->GetFullCachePath() / "PerUser" / "Thumbnails" / "Manifest.stm";

		std::ofstream fout( cachePath, std::ios::binary | std::ios::trunc );

		// Write manifest
		CacheManifestHeader header{ .Thumbnails = m_Cache.size() };
		RawSerialisation::WriteObject( header, fout );

		for( const auto& [id, data] : m_Cache )
		{
			RawSerialisation::WriteObject( id, fout );
			RawSerialisation::WriteObject( data.Time, fout );
		}

		fout.close();

		SAT_CORE_INFO( "Serialised content browser thumbnail manifest!" );
	}

	void ContentBrowserThumbnailCache::SerialiseSingleThumbnail( AssetID id, const CacheData& rData )
	{
		CreateDirectoriesIfNeeded();

		std::filesystem::path newPath = Project::GetActiveProject()->GetFullCachePath() / "PerUser" / "Thumbnails" / std::to_string( id );
		newPath.replace_extension( ".stc" );

		std::ofstream tfout( newPath, std::ios::binary | std::ios::trunc );

		// Write Texture buffer
		Ref<Texture2D> texture = rData.Texture;

		uint32_t width = texture->Width();
		uint32_t height = texture->Height();

		RawSerialisation::WriteObject( width, tfout );
		RawSerialisation::WriteObject( height, tfout );

		Buffer TemporaryBuffer = texture->X31CopyToBuffer();
		RawSerialisation::WriteSaturnBuffer( TemporaryBuffer, tfout );
		TemporaryBuffer.Free();

		tfout.close();
	}

	void ContentBrowserThumbnailCache::DeserialiseManifest()
	{
		std::filesystem::path cachePath = Project::GetActiveProject()->GetFullCachePath() / "PerUser" / "Thumbnails" / "Manifest.stm";
		
		if( !std::filesystem::exists( cachePath ) )
			return;

		std::ifstream stream( cachePath, std::ios::binary | std::ios::in );

		CacheManifestHeader header{};
		RawSerialisation::ReadObject( header, stream );

		if( strcmp( header.Magic, ".STM\0" ) )
		{
			SAT_CORE_ERROR( "Invalid CB Thumbnail Manifest file header!" );
			return;
		}

		m_Cache.reserve( header.Thumbnails );

		for( size_t i = 0; i < header.Thumbnails; i++ )
		{
			CacheData data{};
			data.ExistsOnFS = true;

			AssetID id{};
			RawSerialisation::ReadObject( id, stream );
			RawSerialisation::ReadObject( data.Time, stream );

			m_Cache[ id ] = data;
		}

		stream.close();
	}

	bool ContentBrowserThumbnailCache::DeserialiseSingleThumbnail( AssetID id, CacheData& rData )
	{
		std::filesystem::path newPath = Project::GetActiveProject()->GetFullCachePath() / "PerUser" / "Thumbnails" / std::to_string( id );
		newPath.replace_extension( ".stc" );

		if( !std::filesystem::exists( newPath ) )
			return false;

		std::ifstream stream( newPath, std::ios::binary | std::ios::in );

		uint32_t width = 0;
		uint32_t height = 0;

		RawSerialisation::ReadObject( width, stream );
		RawSerialisation::ReadObject( height, stream );

		Buffer TemporaryBuffer;
		RawSerialisation::ReadSaturnBuffer( TemporaryBuffer, stream );
		
		uint32_t expectedImageSize = width * height * 4;
		if( TemporaryBuffer.Size != expectedImageSize )
		{
			SAT_CORE_ERROR( "Image Size does not match!, Expected: {0}, Got: {1} ( ASSET/{2}, PATH/{3} )", expectedImageSize, TemporaryBuffer.Size, id, newPath.string() );
			return false;
		}

		rData.Texture = Ref<Texture2D>::Create( ImageFormat::RGBA8, width, height, TemporaryBuffer.Data, TemporaryBuffer.Size );
		
		TemporaryBuffer.Free();

		stream.close();

		return true;
	}

}