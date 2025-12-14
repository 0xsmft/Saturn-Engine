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

#include "ThumbnailCacheQueueData.h"

namespace Saturn {

	class ContentBrowserThumbnailGeneratorBase
	{
	public:
		virtual Ref<Texture2D> Generate( ThumbnailCacheQueueData& rData ) = 0;
	};
	
	class TextureAssetThumbnailGenerator : public ContentBrowserThumbnailGeneratorBase
	{
	public:
		Ref<Texture2D> Generate( ThumbnailCacheQueueData& rData ) override;

		static inline AssetType GetStaticType() { return AssetType::Texture; }
	};

	class MaterialAssetThumbnailGenerator : public ContentBrowserThumbnailGeneratorBase
	{
	public:
		Ref<Texture2D> Generate( ThumbnailCacheQueueData& rData ) override;

		static inline AssetType GetStaticType() { return AssetType::Material; }
	};

	class StaticMeshAssetThumbnailGenerator : public ContentBrowserThumbnailGeneratorBase
	{
	public:
		Ref<Texture2D> Generate( ThumbnailCacheQueueData& rData ) override;

		static inline AssetType GetStaticType() { return AssetType::StaticMesh; }
	};

	class SkeletalMeshAssetThumbnailGenerator : public ContentBrowserThumbnailGeneratorBase
	{
	public:
		Ref<Texture2D> Generate( ThumbnailCacheQueueData& rData ) override;

		static inline AssetType GetStaticType() { return AssetType::SkeletalMesh; }
	};

	//////////////////////////////////////////////////////////////////////////
	// ContentBrowserThumbnailGenerator

	class ContentBrowserThumbnailGenerator
	{
	public:
		ContentBrowserThumbnailGenerator() { Initialise(); }
		~ContentBrowserThumbnailGenerator() = default;
		
		Ref<Texture2D> GenerateForAssetType( ThumbnailCacheQueueData& rData );

		void OnUpdate( ThumbnailCacheQueueData& rData );

	private:
		void Initialise();
	
	private:
		// TODO: Maybe change to Ref?
		std::unordered_map<AssetType, std::unique_ptr<ContentBrowserThumbnailGeneratorBase>> m_Generators;
	};

}