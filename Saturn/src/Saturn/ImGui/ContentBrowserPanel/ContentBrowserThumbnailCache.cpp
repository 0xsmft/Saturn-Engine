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

#include "Saturn/Serialisation/RawSerialisation.h"

#include <queue>

namespace Saturn {

	static Ref<Texture2D> s_FileIcon;
	static Ref<Texture2D> s_FolderIcon;

	// Stored for the lifetime of the content browser
	// Stored per asset
	struct CacheData
	{
		int64_t Time = 0;
		Ref<Texture2D> Texture = nullptr;
	};

	// Stored per asset when generation is needed and then popped from the queue
	// 	Time & Texture are passed to the Asset's CacheData in s_Cache hence why we need it the QueueData
	struct QueueData
	{
		int64_t Time = 0;
		Ref<Texture2D> Texture = nullptr;

		Ref<Asset> Asset = nullptr;
	};

	static std::unordered_map<std::filesystem::path, CacheData> s_Cache;
	static std::queue<QueueData> s_Queue;

	void ContentBrowserThumbnailCache::Init()
	{
		Deserialise();

		s_FileIcon = Ref<Texture2D>::Create( "content/textures/editor/FileIcon.png", AddressingMode::Repeat );
		s_FolderIcon = Ref<Texture2D>::Create( "content/textures/editor/DirectoryIcon.png", AddressingMode::Repeat );
	}

	void ContentBrowserThumbnailCache::Terminate()
	{
		Serialise();

		s_Cache.clear();
		s_FileIcon = nullptr;
		s_FolderIcon = nullptr;
	}

	void ContentBrowserThumbnailCache::InsertNew( const std::filesystem::path& rPath, int64_t time, Ref<Texture2D> texture )
	{
		s_Cache.insert( { rPath, { time, texture } } );
	}

	bool ContentBrowserThumbnailCache::AssetHasThumbail( const std::filesystem::path& rPath )
	{
		return s_Cache.find( rPath ) != s_Cache.end();
	}

	void ContentBrowserThumbnailCache::OnUpdate()
	{
		while( !s_Queue.empty() )
		{
			auto& rData = s_Queue.front();

			// temp
			if( rData.Asset->Type == AssetType::Texture )
			{
				// If it's somehow already in the cache pop it and move on to the next thumbnail
				const auto Itr = s_Cache.find( rData.Asset->Path );
				if( Itr != s_Cache.end() )
				{
					const auto& rData = Itr->second;
					if( rData.Time == rData.Time && rData.Texture != nullptr )
					{
						s_Queue.pop();
						continue;
					}
				}

				rData.Texture = ContentBrowserThumbnailGeneratorBase::GenerateForAssetType( rData.Asset );

				if( !rData.Texture )
				{
					s_Queue.pop();
					continue;
				}

				// Add to cache
				auto& rCacheData = s_Cache[ rData.Asset->Path ];
				rCacheData.Time = rData.Time;
				rCacheData.Texture = rData.Texture;
			}

			s_Queue.pop();
			break;
		}
	}

	Ref<Texture2D> ContentBrowserThumbnailCache::GetDefault( int Identifier )
	{
		return Identifier == 0 ? s_FolderIcon : s_FileIcon;
	}

	Ref<Texture2D> ContentBrowserThumbnailCache::GetFor( const Ref<Asset>& rAsset )
	{
		Ref<Texture2D> texture = s_FileIcon;
		const auto Itr = s_Cache.find( rAsset->Path );
		
		// TODO: Don't get the last_write_time every frame and instead check once and use FileWatch
		auto fullPath = Project::GetActiveProject()->FilepathAbs( rAsset->Path );
		auto lastWriteTimePoint = std::filesystem::last_write_time( fullPath );
		auto timestamp = std::chrono::duration_cast< std::chrono::milliseconds >( lastWriteTimePoint.time_since_epoch() ).count();

		if( Itr != s_Cache.end() )
		{
			const auto& rData = Itr->second;
			if( rData.Time == timestamp && rData.Texture != nullptr )
			{
				return rData.Texture;
			}
		}

		// Temp
		if( rAsset->Type != AssetType::Texture )
			return texture;

		// Generate texture & pass in needed information for cache data
		s_Queue.push( { .Time = timestamp, .Texture = nullptr, .Asset = nullptr } );

		return texture;
	}

	//////////////////////////////////////////////////////////////////////////
	// Serialisation

	struct CacheHeader
	{
		const char Magic[ 6 ] = ".STC\0";
		size_t Size = 0;
		uint32_t Version = SAT_CURRENT_VERSION;
	};

	void ContentBrowserThumbnailCache::Serialise()
	{
		CacheHeader header{ .Size = s_Cache.size() };

		std::filesystem::path cachePath = Project::GetActiveProject()->GetFullCachePath() / "Thumbnails.stc";
		std::ofstream fout( cachePath, std::ios::binary | std::ios::trunc );

		RawSerialisation::WriteObject( header, fout );

		for( const auto& [path, data] : s_Cache )
		{
			RawSerialisation::WriteString( path, fout );

			RawSerialisation::WriteObject( data.Time, fout );
		}

		fout.close();
	}

	void ContentBrowserThumbnailCache::Deserialise()
	{
		std::filesystem::path cachePath = Project::GetActiveProject()->GetFullCachePath() / "Thumbnails.stc";
		
		if( !std::filesystem::exists( cachePath ) )
			return;

		std::ifstream stream( cachePath, std::ios::binary | std::ios::in );

		CacheHeader header{};
		RawSerialisation::ReadObject( header, stream );

		if( strcmp( header.Magic, ".STC\0" ) )
		{
			return;
		}

		s_Cache.reserve( header.Size );

		for( size_t i = 0; i < header.Size; i++ )
		{
			CacheData data{};

			std::filesystem::path path = RawSerialisation::ReadString( stream );

			RawSerialisation::ReadObject( data.Time, stream );

			s_Cache[ path ] = data;
		}

		stream.close();
	}

}