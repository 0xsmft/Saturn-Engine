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
#include "ThumbnailGenerator.h"

#include "ContentBrowserThumbnailCache.h"

#include "Saturn/Core/Ruby/RubyWindow.h"

#include "Saturn/Core/JobSystem.h"
#include "Saturn/Core/Renderer/RenderThread.h"
#include "Saturn/Core/Renderer/EditorCamera.h"

#include "Saturn/Asset/Prefab.h"
#include "Saturn/Asset/AssetManager.h"

#include "Saturn/Vulkan/DefaultMeshes.h"
#include "Saturn/Vulkan/SceneRenderer.h"
#include "Saturn/Scene/Scene.h"

#include <queue>

namespace Saturn {

	constexpr auto THUMBNAIL_SIZE = 512.0F;

	//////////////////////////////////////////////////////////////////////////
	// ContentBrowserThumbnailGenerator

	void ContentBrowserThumbnailGenerator::Initialise()
	{
		m_Generators[ AssetType::Texture        ] = std::make_unique<TextureAssetThumbnailGenerator>();
		m_Generators[ AssetType::Material       ] = std::make_unique<MaterialAssetThumbnailGenerator>();
	}

	Ref<Texture2D> ContentBrowserThumbnailGenerator::GenerateForAssetType( ThumbnailCacheQueueData& rData )
	{
		return m_Generators[ rData.Asset->Type ]->Generate( rData );
	}

	void ContentBrowserThumbnailGenerator::OnUpdate( ThumbnailCacheQueueData& rData )
	{
		m_Generators[ rData.Asset->Type ]->OnUpdate( rData );
	}

	//////////////////////////////////////////////////////////////////////////
	// TextureAssetThumbnailGenerator

	Ref<Texture2D> TextureAssetThumbnailGenerator::Generate( ThumbnailCacheQueueData& rData )
	{
		Ref<Asset> textureAsset = rData.Asset;

		if( textureAsset->Type != AssetType::Texture )
			return nullptr;

		// Load the texture
		auto fullPath = Project::GetActiveProject()->FilepathAbs( textureAsset->Path );

		Ref<Texture2D> newTexture = Ref<Texture2D>::Create( fullPath );
		rData.State = ThumbnailState::Generated;

		return newTexture;
	}

	//////////////////////////////////////////////////////////////////////////
	// MaterialAssetThumbnailGenerator (WIP API)

	struct RendererThumbnailCacheData
	{
		RendererThumbnailCacheData() : Camera( 45.0f, 1280.0f, 720.0f, 0.1f, 10000.0f ) {}

		Ref<SceneRenderer> SceneRenderer;
		Ref<Scene> Scene;
		Ref<Entity> SphereEntity;

		EditorCamera Camera;

		bool CanRender = false;
		bool Complete = false;
	};

	static std::unordered_map<AssetID, RendererThumbnailCacheData> s_RendererThumbnailCache;

	Ref<Texture2D> MaterialAssetThumbnailGenerator::Generate( ThumbnailCacheQueueData& rData )
	{
		// Load material
		Ref<MaterialAsset> materialAsset = AssetManager::Get().GetAssetAs<MaterialAsset>( rData.Asset->ID );

		if( rData.Asset->Type != AssetType::Material )
			return nullptr;

		// Check if we are waiting for the renderer to start
		if( !s_RendererThumbnailCache.empty() )
		{
			auto& rCacheData = s_RendererThumbnailCache[ rData.Asset->ID ];
			if( rCacheData.CanRender )
			{
				// Start the render
				rCacheData.Camera.SetActive( true );
				rCacheData.Camera.OnUpdate( Application::Get().Time() );
				rCacheData.Scene->OnRenderEditor( rCacheData.Camera, Application::Get().Time(), *rCacheData.SceneRenderer );
				rCacheData.CanRender = false;

//				RenderThread::Get().Queue( []()
				{
					auto& rCacheData = s_RendererThumbnailCache[ rData.Asset->ID ];
					rCacheData.SceneRenderer->RenderScene();
					rCacheData.Complete = true;
				}// );

				return nullptr;
			}

			if( rCacheData.Complete )
			{
				// Get final composite image and save it to buffer then create create texture
				Ref<Image2D> finalImage = rCacheData.SceneRenderer->CompositeImage();
				Buffer buffer = finalImage->CopyToBuffer();

				Ref<Texture2D> texture = Ref<Texture2D>::Create( finalImage->GetImageFormat(), finalImage->GetWidth(), finalImage->GetHeight(), (const void*)buffer.Data, false );

				rData.State = ThumbnailState::Generated;
				RenderThread::Get().Queue( [ rData ]()
				{
					s_RendererThumbnailCache.erase( rData.Asset->ID );
				} );

				buffer.Free();
				return texture;
			}
		}

		{
			RendererThumbnailCacheData cacheData{};
			cacheData.SceneRenderer = Ref<SceneRenderer>::Create( SceneRendererFlag_NoFlags );
			cacheData.SceneRenderer->SetDynamicSky( 2.0f, 0.0f, 0.0f );
			cacheData.Camera.SetActive( true );

			cacheData.Scene = Ref<Scene>::Create();
			cacheData.SceneRenderer->SetCurrentScene( cacheData.Scene.Get() );

			cacheData.SphereEntity = Ref<Entity>::Create( cacheData.Scene.Get() );
			cacheData.SphereEntity->AddComponent<StaticMeshComponent>().Mesh = Auxiliary::DefaultMeshes::CreateSphere( 1.0f );
			cacheData.SphereEntity->GetComponent<StaticMeshComponent>().Mesh->GetMaterialRegistry()->AddAsset( materialAsset );
			
			s_RendererThumbnailCache[ rData.Asset->ID ] = cacheData;
			RenderThread::Get().Queue( [rData]()
			{
				auto& rCacheData = s_RendererThumbnailCache[ rData.Asset->ID ];
				rCacheData.SceneRenderer->SetViewportSize( THUMBNAIL_SIZE, THUMBNAIL_SIZE );
				rCacheData.Camera.SetViewportSize( THUMBNAIL_SIZE, THUMBNAIL_SIZE );
				rCacheData.Camera.SetDistance( 4.0f );
				// Update to change the distance
				rCacheData.Camera.OnUpdate( Application::Get().Time() );

				rCacheData.CanRender = true;
			} );
		}

		rData.State = ThumbnailState::Generating;

		rData.Asset = materialAsset;

		return nullptr;
	}

	//////////////////////////////////////////////////////////////////////////
	// RendererThumbnailGenerator

	Ref<Texture2D> RendererThumbnailGenerator::Generate( ThumbnailCacheQueueData& rData )
	{
		return nullptr;
	}

}