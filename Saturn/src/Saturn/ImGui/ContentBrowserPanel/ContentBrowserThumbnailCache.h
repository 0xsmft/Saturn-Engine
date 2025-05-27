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

#pragma once

#include "ThumbnailGenerator.h"

#include <queue>

namespace Saturn {

	constexpr int CB_DIRECTORY_ICON = 0;
	constexpr int CB_FILE_ICON = 1;

	class Asset;

	struct CacheData
	{
		int64_t Time = 0;
		Ref<Texture2D> Texture = nullptr;

		std::filesystem::path AssetPath;
		bool ExistsOnFS = false;
	};

	class ContentBrowserThumbnailCache
	{
	public:
		static inline ContentBrowserThumbnailCache& Get() { return *SingletonStorage::GetOrCreateSingleton<ContentBrowserThumbnailCache>(); }
	public:
		void Init();
		void Terminate();

		bool AssetHasThumbnail( AssetID assetID );
		void OnModified( Ref<Asset> asset );
		void Invalidate( Ref<Asset> asset );
		void RemoveThumbnail( AssetID id );

		void UpdateCache();
		void ClearCache();

#if !defined(SAT_DIST)
		void OnImGuiRender( bool* pOpen );
#endif

		[[nodiscard]] Ref<Texture2D> GetDefault( int Identifier );
		[[nodiscard]] Ref<Texture2D> GetFor( const Ref<Asset>& rAsset );
		
	public:
		void SerialiseManifest();
		void DeserialiseManifest();

	private:
		void SerialiseSingleThumbnail( AssetID id, const CacheData& rData );
		[[nodiscard]] bool DeserialiseSingleThumbnail( AssetID id, CacheData& rData );

	private:
		Ref<Texture2D> m_FileIcon;
		Ref<Texture2D> m_FolderIcon;

		bool m_PendingManifestWrite = false;

		std::unordered_map<AssetID, CacheData> m_Cache;

		// Temporary generator queue
		std::queue<ThumbnailCacheQueueData> m_GenerationQueue;

		ContentBrowserThumbnailGenerator m_Generator;
	};

}