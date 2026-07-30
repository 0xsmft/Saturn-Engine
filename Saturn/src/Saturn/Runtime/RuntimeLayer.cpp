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
#include "RuntimeLayer.h"

#include "Saturn/Project/Project.h"

#include "Saturn/Core/VirtualFS.h"
#include "Saturn/Core/Ruby/RubyWindow.h"

#include "Saturn/Serialisation/YAML/SceneSerialiser.h"
#include "Saturn/Serialisation/AssetBundle.h"

#include "Saturn/GameFramework/Core/GameModule.h"

#include "Saturn/Alura/AluraCanvas.h"

#include "Saturn/Vulkan/AluraRenderer.h"
#include "Saturn/Vulkan/SceneRenderer.h"

#include "Saturn/Core/Renderer/RenderThread.h"

#include "Saturn/Asset/AssetManager.h"

#if defined(SAT_DIST)
#include "Saturn/GameFramework/Core/SClassDistReference.h"
#endif

namespace Saturn {

	RuntimeLayer::RuntimeLayer()
		: m_RuntimeScene( Ref<Scene>::Create() )
	{
		Scene::SetActiveScene( m_RuntimeScene.Get() );

		// Init Physics
		m_PhysicsFoundation.Init();

		VirtualFS::Get().MountBase( Project::GetActiveConfig().Name, Project::GetActiveProject()->GetRootDir() );

		// Load Asset bundle.
		if( const auto result = AssetBundle::ReadBundle(); result != AssetBundleResult::Success )
		{
			const std::string errMsg = std::format( "Asset Bundle could not be read. Error code: {0}", result );
			SAT_CORE_VERIFY( false, errMsg );
		}

		// "Load" the Game Module
		m_GameModule = std::make_unique<class GameModule>();

		SceneRendererSpecification spec{
			.Width = Application::Get()->GetWindow()->GetWidth(),
			.Height = Application::Get()->GetWindow()->GetHeight(),
			.AOTechnique = AOTechnique::SSAO,
			.Flags = SceneRendererFlag_MasterInstance | SceneRendererFlag_SwapchainTarget,
			.TargetScene = m_RuntimeScene };

		m_SceneRenderer = Ref<SceneRenderer>::Create( spec );

#if defined(SAT_DIST)
		SClassDistReferencer::Reference();
#endif

		// Create online API but not init it yet.
		m_OnlineAPI = OnlineAPI::CreateOnlineSystemAPI( Project::GetActiveProject()->GetOnlineAPIType() );
	}

	void RuntimeLayer::OnAttach()
	{
		// Create canvas
		AluraCanvasSpecification canvasSpecification{};
		canvasSpecification.Size = glm::vec2{ Application::Get()->GetWindow()->GetWidth(), Application::Get()->GetWindow()->GetHeight() };
		canvasSpecification.Position = glm::vec2{ 0.0f };
		canvasSpecification.MasterFontAssetID = Project::GetActiveProject()->GetDefaultFontAsset();

		if( g_AluraCanvas )
			delete g_AluraCanvas;

		g_AluraCanvas = new AluraCanvas( canvasSpecification );
		g_AluraCanvas->SetContext( m_SceneRenderer->GetAluraRenderer() );

		OpenFile( Project::GetActiveProject()->GetConfig().StartupSceneID );

		Application::Get()->GetWindow()->Show();

		if( m_OnlineAPI )
			m_OnlineAPI->Initialise();

		SAT_CORE_VERIFY( m_RuntimeScene->OnRuntimeStart(), "Initial runtime request failed!" );
	}

	void RuntimeLayer::OnDetach()
	{
		if( g_AluraCanvas ) 
		{
			delete g_AluraCanvas;
			g_AluraCanvas = nullptr;
		}

		if( m_OnlineAPI )
		{
			m_OnlineAPI->Terminate();
		}
	}

	RuntimeLayer::~RuntimeLayer()
	{
		m_RuntimeScene->OnRuntimeEnd();
		m_RuntimeScene = nullptr;

		m_SceneRenderer = nullptr;
	}

	void RuntimeLayer::OpenFile( AssetID id )
	{
		const Ref<Asset> asset = AssetManager::Get()->FindAsset( id );

		Ref<Scene> newScene = Ref<Scene>::Create();
		newScene->Path = asset->Path;

		Scene::SetActiveScene( newScene.Get() );
		
		newScene->DeserialiseData();

		m_RuntimeScene = nullptr;
		m_RuntimeScene = newScene;

		m_RuntimeScene->Name = asset->Name;
		m_RuntimeScene->Path = asset->Path;
		m_RuntimeScene->ID = asset->ID;
		m_RuntimeScene->Type = asset->Type;

		Scene::SetActiveScene( m_RuntimeScene.Get() );

		newScene = nullptr;

		m_SceneRenderer->SetCurrentScene( m_RuntimeScene.Get() );
	}

	void RuntimeLayer::OpenFileInRuntime( AssetID id )
	{
		Ref<Scene> temporaryScene = Ref<Scene>::Create();
		Scene::SetActiveScene( temporaryScene.Get() );

		m_RuntimeScene->OnRuntimeEnd();

		const Ref<Asset> asset = AssetManager::Get()->FindAsset( id );

		SceneSerialiser serialiser( temporaryScene );
		serialiser.Deserialise( asset );

		m_RuntimeScene = temporaryScene;

		m_RuntimeScene->Name = asset->Name;
		m_RuntimeScene->Path = asset->Path;
		m_RuntimeScene->ID = asset->ID;
		m_RuntimeScene->Type = asset->Type;

		Scene::SetActiveScene( m_RuntimeScene.Get() );

		temporaryScene = nullptr;

		m_SceneRenderer->SetCurrentScene( m_RuntimeScene.Get() );

		// If we fail to start runtime, terminate it for good.
		if( !m_RuntimeScene->OnRuntimeStart() )
		{
//			CleanupRuntimeWhenFailed( RuntimeState::Running );
		}
	}

	void RuntimeLayer::HandleSceneTravel( SceneTravelEvent& rEvent )
	{
		AssetID destinationID = rEvent.GetID();
		Ref<Asset> sceneAsset = AssetManager::Get()->FindAsset( destinationID );

		if( !sceneAsset )
		{
			SAT_CORE_ERROR( "Failed to travel as {0} is not a valid scene ID", destinationID );
			SAT_CORE_VERIFY( false, "Failed to travel scene ID is not valid or not found in AssetRegistry" );
		}
		
		OpenFileInRuntime( destinationID );
	}

	void RuntimeLayer::OnUpdate( Timestep time )
	{
		m_SceneRenderer->PreRender();

		m_RuntimeScene->OnUpdate( time );
		m_RuntimeScene->OnRenderRuntime( time, m_SceneRenderer );

		// Online subsystem update...
		if( m_OnlineAPI )
			m_OnlineAPI->Tick();

		RenderThread::Get().Queue( [ = ]()
		{
			m_SceneRenderer->RenderScene();
		} );
	}

	void RuntimeLayer::OnEvent( Event& rEvent )
	{
		if( rEvent.Type == EventType::Resize )
			OnWindowResize( ( RubyWindowResizeEvent& ) rEvent );

		m_RuntimeScene->OnEvent( rEvent );

		if( g_AluraCanvas )
		{
			g_AluraCanvas->HandleDrawerEvents( rEvent );
		}

		switch( rEvent.Type )
		{
			default: break;
			case EventType::SceneTravel:
			{
				HandleSceneTravel( ( SceneTravelEvent& ) rEvent );
			} break;
		}
	}

	void RuntimeLayer::OnWindowResize( RubyWindowResizeEvent& e )
	{
		const int width = e.GetWidth(), height = e.GetHeight();

		if( width == 0 && height == 0 )
			return;

		m_SceneRenderer->SetViewportSize( ( uint32_t ) width, ( uint32_t ) height );

		if( g_AluraCanvas )
			g_AluraCanvas->SetSize( { width, height } );
	}

}
