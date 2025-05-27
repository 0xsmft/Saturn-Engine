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
#include "RuntimeLayer.h"

#include "Saturn/Project/Project.h"

#include "Saturn/Core/VirtualFS.h"
#include "Saturn/Core/ErrorDialog.h"
#include "Saturn/Core/Ruby/RubyWindow.h"

#include "Saturn/Serialisation/SceneSerialiser.h"
#include "Saturn/Serialisation/ProjectSerialiser.h"
#include "Saturn/Serialisation/EngineSettingsSerialiser.h"
#include "Saturn/Serialisation/AssetManagerSerialiser.h"
#include "Saturn/Serialisation/AssetSerialisers.h"
#include "Saturn/Serialisation/AssetBundle.h"

#include "Saturn/GameFramework/Core/GameModule.h"

#include "Saturn/NodeEditor/GlobalNodesList.h"

#include "Saturn/Vulkan/SceneRenderer.h"
#include "Saturn/Vulkan/Renderer2D.h"

#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Asset/Prefab.h"

namespace Saturn {

	RuntimeLayer::RuntimeLayer()
		: m_RuntimeScene( Ref<Scene>::Create() )
	{
		Scene::SetActiveScene( m_RuntimeScene.Get() );

		// Init Physics
		m_PhysicsFoundation.Init();

		VirtualFS::Get().MountBase( Project::GetActiveConfig().Name, Project::GetActiveProject()->GetRootDir() );

		// Load Asset bundle.
		if( auto result = AssetBundle::ReadBundle(); result != AssetBundleResult::Success )
		{
			std::string errMsg = std::format( "Asset Bundle could not be read. Error code: {0}", result );

			SAT_CORE_VERIFY( false, errMsg );
		}

		GlobalNodesList::RegisterAll();
		
		// "Load" the Game Module
		m_GameModule = new GameModule();

		OpenFile( Project::GetActiveProject()->GetConfig().StartupSceneID );

		Application::Get().GetWindow()->Show();

		m_RuntimeScene->OnRuntimeStart();
	}

	RuntimeLayer::~RuntimeLayer()
	{
		m_RuntimeScene->OnRuntimeEnd();
		m_RuntimeScene = nullptr;

		delete m_GameModule;
	}

	void RuntimeLayer::OpenFile( AssetID id )
	{
		Ref<Asset> asset = AssetManager::Get().FindAsset( id );

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
		m_RuntimeScene->Flags = asset->Flags;

		Scene::SetActiveScene( m_RuntimeScene.Get() );

		newScene = nullptr;

		Application::Get().PrimarySceneRenderer().SetCurrentScene( m_RuntimeScene.Get() );
	}

	void RuntimeLayer::OnUpdate( Timestep time )
	{
		m_RuntimeScene->OnUpdate( time );
		m_RuntimeScene->OnRenderRuntime( time, Application::Get().PrimarySceneRenderer() );
	}

	void RuntimeLayer::OnEvent( RubyEvent& rEvent )
	{
		if( rEvent.Type == RubyEventType::Resize )
			OnWindowResize( ( RubyWindowResizeEvent& ) rEvent );

		m_RuntimeScene->OnEvent( rEvent );
	}

	bool RuntimeLayer::OnWindowResize( RubyWindowResizeEvent& e )
	{
		int width = e.GetWidth(), height = e.GetHeight();

		if( width == 0 && height == 0 )
			return false;

		Application::Get().PrimarySceneRenderer().SetViewportSize( ( uint32_t ) width, ( uint32_t ) height );
		Renderer2D::Get().SetViewportSize( ( uint32_t ) width, ( uint32_t ) height );

		return true;
	}
}