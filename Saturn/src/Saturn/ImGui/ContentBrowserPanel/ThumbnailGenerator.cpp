/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2024 BEAST                                                           *
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
#include "ThumbnailGenerator.h"

#include "ContentBrowserThumbnailCache.h"

#include "Saturn/Core/JobSystem.h"
#include "Saturn/Core/Renderer/RenderThread.h"

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// ContentBrowserThumbnailGeneratorBase

	Ref<Texture2D> ContentBrowserThumbnailGeneratorBase::GenerateForAssetType( Ref<Asset> asset )
	{
		switch( asset->Type )
		{
			case AssetType::Texture:
			{
				return TextureAssetThumbnailGenerator::Generate( asset );
			}

			case AssetType::Unknown:
			case AssetType::COUNT:
			default: break;
		}

		return nullptr;
	}

	//////////////////////////////////////////////////////////////////////////
	// TextureAssetThumbnailGenerator

	Ref<Texture2D> TextureAssetThumbnailGenerator::Generate( Ref<Asset> textureAsset )
	{
		if( textureAsset->Type != AssetType::Texture )
			return nullptr;

		// Add a null texture and wait for job system
		ContentBrowserThumbnailCache::InsertNew( textureAsset->Path, 0, nullptr );

		// Load the texture
		auto fullPath = Project::GetActiveProject()->FilepathAbs( textureAsset->Path );

		Ref<Texture2D> newTexture = Ref<Texture2D>::Create( fullPath );
		return newTexture;
	}

}