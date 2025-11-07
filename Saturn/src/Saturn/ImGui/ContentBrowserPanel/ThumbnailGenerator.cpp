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
#include "Saturn/Vulkan/Renderer2D.h"
#include "Saturn/Vulkan/SceneRenderer.h"

#include "Saturn/Scene/Scene.h"

#include "Saturn/Project/Project.h"

#include <queue>

namespace Saturn {

	constexpr auto THUMBNAIL_SIZE = 512.0F;

	//////////////////////////////////////////////////////////////////////////
	// ContentBrowserThumbnailGenerator

	void ContentBrowserThumbnailGenerator::Initialise()
	{
		m_Generators[ AssetType::Texture    ] = std::make_unique<TextureAssetThumbnailGenerator>();
		m_Generators[ AssetType::Material   ] = std::make_unique<MaterialAssetThumbnailGenerator>();
		m_Generators[ AssetType::StaticMesh ] = std::make_unique<StaticMeshAssetThumbnailGenerator>();
	}

	Ref<Texture2D> ContentBrowserThumbnailGenerator::GenerateForAssetType( ThumbnailCacheQueueData& rData )
	{
		return m_Generators[ rData.Asset->Type ]->Generate( rData );
	}

	void ContentBrowserThumbnailGenerator::OnUpdate( ThumbnailCacheQueueData& rData )
	{
	}

	//////////////////////////////////////////////////////////////////////////
	// TextureAssetThumbnailGenerator

	Ref<Texture2D> TextureAssetThumbnailGenerator::Generate( ThumbnailCacheQueueData& rData )
	{
		Ref<Asset> textureAsset = rData.Asset;

		if( textureAsset->Type != AssetType::Texture )
			return nullptr;

		// Load the texture
		const auto fullPath = Project::GetActiveProject()->FilepathAbs( textureAsset->Path );
		Ref<Texture2D> newTexture = Ref<Texture2D>::Create( fullPath );

		const uint32_t textureWidth = newTexture->Width();
		const uint32_t textureHeight = newTexture->Height();

		uint32_t mipWidth = textureWidth, mipHeight = textureHeight, mip{};
		for( uint32_t i = 0; i < newTexture->GetMipMapLevels(); i++ )
		{
			mipWidth = glm::max( 1u, textureWidth >> i );
			mipHeight = glm::max( 1u, textureHeight >> i );

			if( mipWidth <= THUMBNAIL_SIZE && mipHeight <= THUMBNAIL_SIZE )
			{
				// Found a mip that is the correct size for our thumbnail.
				mip = i;
				break;
			}
		}

		Buffer TemporaryBuffer = newTexture->GetMipTextureData( mipWidth, mipHeight, mip );
		Ref<Texture2D> mippedImage = Ref<Texture2D>::Create( ImageFormat::RGBA8, mipWidth, mipHeight, TemporaryBuffer.Data, false );

		newTexture = nullptr;
		TemporaryBuffer.Free();

		rData.State = ThumbnailState::Generated;
		return mippedImage;
	}

	//////////////////////////////////////////////////////////////////////////
	// MaterialAssetThumbnailGenerator (WIP API)

	struct RendererThumbnailCacheData
	{
		RendererThumbnailCacheData() : Camera( 45.0f, THUMBNAIL_SIZE, THUMBNAIL_SIZE, 0.1f, 1000.0f ) {}

		Ref<SceneRenderer> SceneRenderer;
		Ref<Scene> Scene;
		SharedPtr<Entity> SphereEntity;

		EditorCamera Camera;

		bool AwaitingRender = false;
		bool RenderComplete = false;
	};

	static std::unordered_map<AssetID, RendererThumbnailCacheData> s_RendererThumbnailCache;

	static void InitNewRenderThumbnail( ThumbnailCacheQueueData& rData, Ref<MaterialAsset> materialAsset )
	{
		// Complete init on render thread
		// We must do this as we are going to create vulkan objects
		RenderThread::Get().Queue( [ rData, materialAsset ]()
		{
			// INIT
			RendererThumbnailCacheData cacheData{};
			cacheData.SceneRenderer = Ref<SceneRenderer>::Create( SceneRendererFlag_NoFlags );
			cacheData.SceneRenderer->SetDynamicSky( 2.0f, 0.0f, 0.0f );
			cacheData.Camera.SetActive( true );

			cacheData.Scene = Ref<Scene>::Create();
			cacheData.SceneRenderer->SetCurrentScene( cacheData.Scene.Get() );

			// Create entity
			cacheData.SphereEntity = cacheData.Scene->CreateEntity();
			auto& mc = cacheData.SphereEntity->AddComponent<StaticMeshComponent>();
			mc.Mesh = Auxiliary::DefaultMeshes::CreateSphere( 1.0f );
			mc.Mesh->GetMaterialRegistry()->AddAsset( materialAsset );

			// INIT (PreRender)
			cacheData.SceneRenderer->SetViewportSize( ( uint32_t ) THUMBNAIL_SIZE, ( uint32_t ) THUMBNAIL_SIZE );

			cacheData.Camera.SetViewportSize( ( uint32_t ) THUMBNAIL_SIZE, ( uint32_t ) THUMBNAIL_SIZE );
			cacheData.Camera.SetDistance( 4.0f );

			// Update to change the distance
			cacheData.Camera.OnUpdate( Application::Get().Time() );

			cacheData.AwaitingRender = true;

			s_RendererThumbnailCache[ rData.Asset->ID ] = cacheData;
		} );
	}

	static void StartFirstRender( RendererThumbnailCacheData& rCacheData )
	{
		// Start the render
		rCacheData.Camera.OnUpdate( Application::Get().Time() );
		rCacheData.Scene->OnRenderEditor( rCacheData.Camera, Application::Get().Time(), *rCacheData.SceneRenderer );
		
		rCacheData.AwaitingRender = false;

		// We cannot render on the render thread because the command buffer will no longer be active
		// As when StartFirstRender is called we are on the ImGui part.
		rCacheData.SceneRenderer->RenderScene();
		rCacheData.RenderComplete = true;
	}

	static Ref<Texture2D> CreateTextureFromFBImage( Ref<Image2D> image ) 
	{
		Buffer TemporaryBuffer = image->CopyToBuffer();

		Ref<Texture2D> texture = Ref<Texture2D>::Create( image->GetImageFormat(), image->GetWidth(), image->GetHeight(), ( const void* ) TemporaryBuffer.Data, false );

		TemporaryBuffer.Free();

		return texture;
	}

	Ref<Texture2D> MaterialAssetThumbnailGenerator::Generate( ThumbnailCacheQueueData& rData )
	{
		if( rData.Asset->Type != AssetType::Material )
			return nullptr;

		// If we already exist then we could be waiting on render or we can get the final image if we are complete
		// TRANSITION: MAIN THREAD
		const auto itr = s_RendererThumbnailCache.find( rData.Asset->ID );
		if( itr != s_RendererThumbnailCache.end() )
		{
			auto& rCacheData = itr->second;

			if( rCacheData.AwaitingRender )
			{
				StartFirstRender( rCacheData );

				// Return no texture as it has not been generated.
				return nullptr;
			}

			if( rCacheData.RenderComplete )
			{
				rData.State = ThumbnailState::Generated;

				// Destroy on render thread
				// TRANSITION: MAIN THREAD (RenderThread)
				RenderThread::Get().Queue( [ rData ]()
				{
					s_RendererThumbnailCache.erase( rData.Asset->ID );
				} );

				return CreateTextureFromFBImage( rCacheData.SceneRenderer->CompositeImage() );
			}
		}

		// Load and prepare SceneRenderer on JobSystem
		// TRANSITION: JOB SYSTEM THREAD
		JobSystem::Get().QueueJob( [&]() 
		{
			// Load material
			Ref<MaterialAsset> materialAsset = AssetManager::Get().GetAssetAs<MaterialAsset>( rData.Asset->ID );

			InitNewRenderThumbnail( rData, materialAsset );
			rData.State = ThumbnailState::Generating;
			rData.Asset = materialAsset;
		} );

		// Return no texture as it has not been generated.
		return nullptr;
	}

	//////////////////////////////////////////////////////////////////////////
	// Static mesh

	Ref<Texture2D> StaticMeshAssetThumbnailGenerator::Generate( ThumbnailCacheQueueData& rData )
	{
		// Invalid type
		if( rData.Asset->Type != AssetType::StaticMesh )
			return nullptr;

		// If we already exist then we could be waiting on render or we can get the final image if we are complete
		// We also want to exit if we exist in the cache because we could still be waiting for the mesh to be loaded as meshes may take longer to load.
		const auto itr = s_RendererThumbnailCache.find( rData.Asset->ID );
		if( itr != s_RendererThumbnailCache.end() )
		{
			auto& rCacheData = itr->second;

			if( rCacheData.AwaitingRender )
			{
				StartFirstRender( rCacheData );

				// Return no texture as it has not been generated.
				return nullptr;
			}

			if( rCacheData.RenderComplete )
			{
				rData.State = ThumbnailState::Generated;

				// Destroy on render thread
				RenderThread::Get().Queue( [ rData ]()
				{
					s_RendererThumbnailCache.erase( rData.Asset->ID );
				} );

				return CreateTextureFromFBImage( rCacheData.SceneRenderer->CompositeImage() );
			}

			// Awaiting init
			// Return no texture as it has not been generated.
			return nullptr;
		}

		// Execute init on JobSystem Thread
		JobSystem::Get().QueueJob( [&]() 
		{
			RenderThread::Get().Queue( [ rData ]()
			{
				Ref<StaticMesh> staticMesh = AssetManager::Get().GetAssetAs<StaticMesh>( rData.Asset->ID );

				auto& cacheData = s_RendererThumbnailCache[ rData.Asset->ID ];
				cacheData.SceneRenderer = Ref<SceneRenderer>::Create( SceneRendererFlag_NoFlags );
				cacheData.SceneRenderer->SetDynamicSky( 2.0f, 0.0f, 0.0f );
				cacheData.Camera.SetActive( true );

				cacheData.Scene = Ref<Scene>::Create();
				cacheData.SceneRenderer->SetCurrentScene( cacheData.Scene.Get() );

				// Create entity with the static mesh
				cacheData.SphereEntity = cacheData.Scene->CreateEntity();
				auto& mc = cacheData.SphereEntity->AddComponent<StaticMeshComponent>();
				mc.Mesh = staticMesh;

				cacheData.SceneRenderer->SetViewportSize( ( uint32_t ) THUMBNAIL_SIZE, ( uint32_t ) THUMBNAIL_SIZE );
				cacheData.Camera.SetViewportSize( ( uint32_t ) THUMBNAIL_SIZE, ( uint32_t ) THUMBNAIL_SIZE );

				// Move the camera back to contain the whole mesh.
				auto& rBoundingBox = staticMesh->GetBoundingBox();

				// Set the distance based on the bounding box and make sure that we are not too close so the min is 4.0f
				const glm::vec3 size = rBoundingBox.Extent();
				const float maxSize = std::max( size.x, std::max( size.y, size.z ) );

				float distance = maxSize * 2.0f;
				distance = std::max( distance + 4.0f, 4.0f );

				cacheData.Camera.SetDistance( distance );

				// Greater than the far clip
				if( distance > 1000.0f )
				{
					cacheData.Camera.SetProjectionMatrix( 45.0f, THUMBNAIL_SIZE, THUMBNAIL_SIZE, 0.1f, distance * 10.0f );
				}

				// Update to change the distance
				cacheData.Camera.OnUpdate( Application::Get().Time() );
				cacheData.AwaitingRender = true;
			} );

			Ref<StaticMesh> staticMesh = AssetManager::Get().GetAssetAs<StaticMesh>( rData.Asset->ID );
			// Ready to render next frame by the MainThread (RenderThread)
			rData.State = ThumbnailState::Generating;
			rData.Asset = staticMesh;
		} );

		// Return no texture as it has not been generated.
		return nullptr;
	}

}
