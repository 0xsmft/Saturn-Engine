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
#include "EditorLayer.h"

#include <Saturn/Project/Project.h>

#include <Saturn/ImGui/ImGuiAuxiliary.h>
#include <Saturn/ImGui/TitleBar.h>
#include <Saturn/ImGui/MaterialAssetViewer/MaterialAssetViewer.h>
#include <Saturn/ImGui/PrefabViewer.h>
#include <Saturn/ImGui/EditorIcons.h>
#include <Saturn/ImGui/EditorEvents.h>
#include <Saturn/ImGui/ContentBrowserPanel/ContentBrowserThumbnailCache.h>
#include <Saturn/ImGui/UndoRedo/EntityUndoRedoActions.h>

#include <Saturn/Serialisation/YAML/SceneSerialiser.h>
#include <Saturn/Serialisation/YAML/ProjectSerialiser.h>
#include <Saturn/Serialisation/YAML/EngineSettingsSerialiser.h>
#include <Saturn/Serialisation/YAML/AssetManagerSerialiser.h>
#include <Saturn/Serialisation/YAML/AssetSerialisers.h>
#include <Saturn/Serialisation/AssetBundle.h>

#include <Saturn/Vulkan/SceneRenderer.h>
#include <Saturn/Vulkan/ShaderBundle.h>
#include <Saturn/Vulkan/Renderer2D.h>
#include <Saturn/Vulkan/VulkanImageAux.h>
#include <Saturn/Vulkan/DefaultMeshes.h>

#include <Saturn/Core/Maths.h>
#include <Saturn/Core/StringAuxiliary.h>
#include <Saturn/Core/EngineSettings.h>
#include <Saturn/Core/OptickProfiler.h>
#include <Saturn/Core/Ruby/RubyWindow.h>
#include <Saturn/Core/Ruby/RubyAuxiliary.h>
#include <Saturn/Core/Process.h>
#include <Saturn/Core/Renderer/RenderThread.h>
#include <Saturn/Core/VirtualFS.h>
#include <Saturn/Core/EnvironmentVariables.h>

#include <Saturn/Asset/AssetRegistry.h>
#include <Saturn/Asset/AssetManager.h>
#include <Saturn/Asset/Prefab.h>

#include <Saturn/GameFramework/Core/GameModule.h>
#include <Saturn/GameFramework/Core/ClassMetadataHandler.h>

#include <Saturn/Audio/AudioSystem.h>
#include <Saturn/Audio/SoundGroup.h>

#include <Saturn/AI/Navigation/NavBoundsEntity.h>

#include <Saturn/Project/Premake.h>

#include <Saturn/Runtime/RuntimeEvents.h>

#include <ImGuizmo/ImGuizmo.h>

#include <imspinner/imspinner.h>

#include <glm/gtc/type_ptr.hpp>

namespace Saturn {

	static constexpr inline bool operator==( const ImVec2& lhs, const ImVec2& rhs ) { return lhs.x == rhs.x && lhs.y == rhs.y; }
	static constexpr inline bool operator!=( const ImVec2& lhs, const ImVec2& rhs ) { return !( lhs == rhs ); }

	EditorLayer::EditorLayer() 
		: m_EditorCamera( 45.0f, 1280.0f, 720.0f, 0.1f, 1000.0f ), 
		m_SuspendedEditorCamera( 45.0f, 1280.0f, 720.0f, 0.1f, 1000.0f ), 
		m_EditorScene( Ref<Scene>::Create() )
	{
#if defined( SAT_PROFILER_ENABLE )
		tracy::StartupProfiler();
#endif
		Scene::SetActiveScene( m_EditorScene.Get() );

		m_EditorCamera.SetActive( true );

		// Init Physics
		m_PhysicsFoundation.Init();

		// Editor Application should of loaded a project but if not assert.
		SAT_CORE_ASSERT( Project::GetActiveProject(), "No project was given." );
		
		VirtualFS::Get().MountBase( Project::GetActiveConfig().Name, Project::GetActiveProject()->GetRootDir() );

		m_AssetManager = Ref<AssetManager>::Create();

		Project::GetActiveProject()->CheckMissingAssetRefs();

		m_GameModule = new GameModule();
	}

	void EditorLayer::OnAttach()
	{
		m_SelectionManager = std::make_unique<EntitySelectionManager>();
		m_GlobalUndoRedoGroup = Ref<GlobalUndoRedoGroup>::Create();

		m_CheckerboardTexture = Ref< Texture2D >::Create( "content/textures/editor/checkerboard.tga", AddressingMode::Repeat );

		m_StartRuntimeTexture = Ref< Texture2D >::Create( "content/textures/editor/Play.png", AddressingMode::ClampToEdge );
		m_EndRuntimeTexture = Ref< Texture2D >::Create( "content/textures/editor/Stop.png", AddressingMode::ClampToEdge );
		m_PauseRuntimeTexture = Ref< Texture2D >::Create( "content/textures/editor/Pause.png", AddressingMode::ClampToEdge );

		m_TranslationTexture = Ref< Texture2D >::Create( "content/textures/editor/Move.png", AddressingMode::ClampToEdge );
		m_RotationTexture = Ref< Texture2D >::Create( "content/textures/editor/Rotate.png", AddressingMode::ClampToEdge );
		m_ScaleTexture = Ref< Texture2D >::Create( "content/textures/editor/Scale.png", AddressingMode::ClampToEdge );
		m_SyncTexture = Ref< Texture2D >::Create( "content/textures/editor/Sync.png", AddressingMode::ClampToEdge );
		m_PointLightTexture = Ref< Texture2D >::Create( "content/textures/editor/Billboard_PointLight.png", AddressingMode::ClampToEdge, false );
		m_ExclamationTexture = Ref< Texture2D >::Create( "content/textures/editor/Exclamation.png", AddressingMode::ClampToEdge );

		// Add all of our icons to the editor icons list so that we have use this anywhere else in the engine/editor.
		EditorIcons::AddIcon( m_CheckerboardTexture );
		EditorIcons::AddIcon( m_StartRuntimeTexture );
		EditorIcons::AddIcon( m_EndRuntimeTexture );
		EditorIcons::AddIcon( m_TranslationTexture );
		EditorIcons::AddIcon( m_RotationTexture );
		EditorIcons::AddIcon( m_ScaleTexture );
		EditorIcons::AddIcon( m_SyncTexture );
		EditorIcons::AddIcon( m_PointLightTexture );
		EditorIcons::AddIcon( m_ExclamationTexture );

		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/Billboard_Audio.png", AddressingMode::Repeat, false ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/Billboard_AudioLooping.png", AddressingMode::Repeat, false ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/Billboard_AudioMuted.png", AddressingMode::Repeat, false ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/Billboard_AudioListen.png", AddressingMode::Repeat, false ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/Inspect.png", AddressingMode::Repeat, true ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/NoIcon.png", AddressingMode::Repeat, true ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/Error.png", AddressingMode::Repeat, true ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/Error_Small.png", AddressingMode::Repeat, true ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/Bin.png", AddressingMode::Repeat, true ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/Exclamation_Small.png", AddressingMode::Repeat, true ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/Information_Small.png", AddressingMode::Repeat, true ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/Settings.png", AddressingMode::Repeat, true ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/EditIcon.png", AddressingMode::Repeat, false ) );
		
		// Create Panel Manager.
		m_ImGuiWindowManager = Ref<ImGuiWindowManager>::Create();

		m_ImGuiWindowManager->AddWindow<SceneHierarchyPanel>();
		m_ImGuiWindowManager->AddWindow<ContentBrowserPanel>();

		Ref<SceneHierarchyPanel> hierarchyPanel = m_ImGuiWindowManager->GetPanel<SceneHierarchyPanel>();
		hierarchyPanel->SetContext( m_EditorScene );
		hierarchyPanel->OpenWindow();

		Ref<ContentBrowserPanel> contentBrowserPanel = m_ImGuiWindowManager->GetPanel<ContentBrowserPanel>();
		contentBrowserPanel->OpenWindow();

		// Setup content browser panel at project dir.
		contentBrowserPanel->ResetPath( Project::GetActiveProject()->GetRootDir() );

		m_TitleBar.AddMenuBarFunction( SAT_BIND_EVENT_FN( DrawTitlebarOptions ) );
		m_TitleBar.AddOnExitFunction( SAT_BIND_EVENT_FN( OnTitlebarExit ) );

		//////////////////////////////////////////////////////////////////////////
		// Scene loading and Scene Renderer
		m_SceneRenderer = Ref<SceneRenderer>::Create( SceneRendererFlag_MasterInstance | SceneRendererFlag_RenderGrid );

		m_SceneRenderer->SetCurrentScene( m_EditorScene.Get() );

		// Now open the startup scene
		OpenFile( Project::GetActiveProject()->GetConfig().StartupSceneID );

		const std::string title = std::format( "{0} - Saturn", Project::GetActiveConfig().Name );
		Application::Get().GetWindow()->ChangeTitle( title );

		if( !Project::GetActiveProject()->HasThumbnail() )
		{
			EditorNotification notification{ .Text = "Generating Project Thumbnail", .Lifetime = 5.0f };
			PushNotification( notification );

			JobSystem::Get().AddJob( [this]()
			{
				std::this_thread::sleep_for( std::chrono::seconds( 2 ) );

				RenderThread::Get().Queue( [this]()
				{
					m_SceneRenderer->Screenshot( Project::GetActiveProject()->GetThumbnailPath(), glm::vec2( 156.0f, 128.0f ) );
				} );
			} );
		}
	}

	void EditorLayer::OnDetach()
	{
		EditorIcons::Clear();
		m_CheckerboardTexture = nullptr;
		m_PointLightTexture = nullptr;
	}

	EditorLayer::~EditorLayer()
	{
		m_ImGuiWindowManager = nullptr;

		m_SceneRenderer->SetCurrentScene( nullptr );
		m_SceneRenderer = nullptr;
		
		m_SelectionManager.reset();

		if( m_RuntimeScene ) 
		{	
			m_RuntimeScene->OnRuntimeEnd();
			m_RuntimeScene = nullptr;
		}

		m_EditorScene = nullptr;
		m_AssetManager = nullptr;

		VirtualFS::Get().UnmountBase( Project::GetActiveConfig().Name );

#if defined( SAT_PROFILER_ENABLE )
		tracy::ShutdownProfiler();
#endif

		delete m_GameModule;
		m_GameModule = nullptr;
	}

	void EditorLayer::OnUpdate( Timestep time )
	{
		SAT_PF_EVENT();

		if( Input::Get().MouseButtonPressed( RubyMouseButton_Right ) && !m_StartedRightClickInViewport && m_ViewportFocused && m_MouseOverViewport )
			m_StartedRightClickInViewport = true;

		if( !Input::Get().MouseButtonPressed( RubyMouseButton_Right ) )
			m_StartedRightClickInViewport = false;

		const bool canSetCursorMode = m_RuntimeScene == nullptr ? m_AllowCameraEvents : m_RuntimeScene->GetRuntimeState() == RuntimeState::Suspended ? m_AllowCameraEvents : m_MouseOverViewport;

		Input::Get().SetCanSetCursorMode( canSetCursorMode );

		///////////////////////////////

		// Start runtime if needed
		if( m_RequestRuntime )
		{
			if( !m_RuntimeScene )
			{
				PreInitRuntime();

				if( m_RuntimeScene->OnRuntimeStart() )
				{
					PostInitRuntime();
				}
				else
				{
					CleanupRuntimeWhenFailed();
				}
			}
			else if( !m_RuntimeScene->IsRuntimeActive() ) 
			{
				// Because the Runtime Scene self ended runtime, we must reset the mouse because normally we wouldn't
				// as to end runtime via the Editor requires the mouse to be in an unlocked state.
				// TODO: A better way to handle runtime, would be to create a OnRuntimeStart, OnRuntimeSuspend, OnRuntimeEnd events
				Input::Get().SetCursorMode( RubyCursorMode::Normal, true );
				m_RequestRuntime = false;

				EndRuntime();
			}
		}
		else
		{
			if( m_RuntimeScene && m_RuntimeScene->IsRuntimeActive() )
			{
				EndRuntime();
			}
		}

		if( m_RuntimeScene ) 
		{
			Renderer2D::Get().PreRender();

			m_RuntimeScene->OnUpdate( time );

			if( m_RuntimeScene->GetRuntimeState() == RuntimeState::Suspended ) [[unlikely]]
			{
				m_SuspendedEditorCamera.SetActive( m_AllowCameraEvents );
				m_SuspendedEditorCamera.OnUpdate( time );

				m_RuntimeScene->OnRenderEditor( m_SuspendedEditorCamera, time, *m_SceneRenderer );
			}
			else [[likely]]
			{
				m_RuntimeScene->OnRenderRuntime( time, *m_SceneRenderer );
			}

			if( m_ShowCameraFrustum )
			{
				if( auto entity = m_RuntimeScene->GetMainCameraEntity().Access() )
				{
					const auto& cc = entity->GetComponent<CameraComponent>().Camera;
					cc.RenderDebugFrustum();
				}
			}
		}
		else 
		{
			m_EditorCamera.SetActive( m_AllowCameraEvents );
			m_EditorCamera.OnUpdate( time );

			m_EditorScene->OnUpdate( time );
			m_EditorScene->OnRenderEditor( m_EditorCamera, time, *m_SceneRenderer );

			m_LastAutoSaveTime += time;

			if( const auto prj = Project::GetActiveProject(); prj->IsAutoSavesEnabled() && m_LastAutoSaveTime >= prj->GetAutoSaveInterval() )
			{
				SaveFileAuto();

				m_LastAutoSaveTime = 0.0f;
			}
		}

		if( m_ShowMeshAABB )
		{
			for( const auto& rEntity : EntitySelectionManager::Get().GetSelectionContexts() )
			{
				const glm::mat4 transform = g_ActiveScene->GetTransformRelativeToParent( rEntity );
				if( rEntity->HasComponent<StaticMeshComponent>() )
				{
					const auto& rMesh = rEntity->GetComponent<StaticMeshComponent>().Mesh;
					Renderer2D::Get().SubmitAABB( rMesh->GetBoundingBox(), transform, { 1.0F, 0.0F, 0.0F, 1.0F } );
				}
			}
		}

		// Render scenes in other asset viewers
		m_ImGuiWindowManager->OnUpdate( time );

		RenderThread::Get().Queue( [ = ]() { m_SceneRenderer->RenderScene(); } );
	}

	void EditorLayer::OnImGuiRender()
	{
		SAT_PF_EVENT();

		// Draw dockspace.
		ImGui::DockSpaceOverViewport( ImGui::GetWindowViewport() );
		
		if( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) || ( ImGui::IsMouseClicked( ImGuiMouseButton_Right ) && !m_StartedRightClickInViewport ) )
		{
			if( !m_RuntimeScene )
			{
				ImGui::FocusWindow( GImGui->HoveredWindow );
				Input::Get().SetCursorMode( RubyCursorMode::Normal );
			}
		}

		m_TitleBar.OnImGuiRender();

		m_ImGuiWindowManager->DrawAll();
		
		if( m_ShowImGuiDemoWindow )     ImGui::ShowDemoWindow( &m_ShowImGuiDemoWindow );
		if( m_ShowUserSettings )        DrawProjectSettingsWindow();
		if( m_OpenAssetRegistryDebug )  DrawAssetRegistryDebug();
		if( m_OpenLoadedAssetDebug )    DrawLoadedAssetsDebug();
		if( m_OpenEditorSettings )      DrawEditorSettings();
		if( m_ShowVFSDebug )            DrawVFSDebug();
		if( m_OpenAboutWindow )         DrawAboutWindow();
		if( m_MessageBoxes.size() )     HandleMessageBoxes();
		if( m_Notifications.size() )    DrawNotifications();
		if( m_ShowSceneRendererWindow ) DrawSceneRendererWindow();
		if( m_ShowRendererWindow )		DrawRendererWindow();
		if( m_ShowMetadataDebug )       DrawMetadataDebug();
		if( m_ShowAssetDependencies )   DrawAssetDependencies();
		if( m_ShowSceneDirtyModal )     DrawSceneDirtyPopup();
		if( m_JobModalOpen )            DrawBlockingActionModal();
		if( m_ShowCBThumbnailDebug )    ContentBrowserThumbnailCache::Get().OnImGuiRender( &m_ShowCBThumbnailDebug );
		if( m_ShowUndoRedoDebug )       GlobalUndoRedoGroup::Get().OnImGuiRender( &m_ShowUndoRedoDebug );
		
		DrawViewport();
	}

	void EditorLayer::OnEvent( Event& rEvent )
	{
		// Cameras and ImGui Windows should only receive Ruby events and/or Editor events 
		// not Runtime Events or Scene 
		if( rEvent.HasCategory( EC_Ruby ) || rEvent.HasCategory( EC_Editor ) )
		{
			// If the mouse is over the viewport allow for the scroll event to happen
			// The scroll event does not care if the camera is active or not.
			if( m_MouseOverViewport )
			{
				m_EditorCamera.OnEvent( rEvent );
		
				if( m_RequestRuntime )
					m_SuspendedEditorCamera.OnEvent( rEvent );
			}

			if( m_RuntimeScene )
				m_RuntimeScene->OnEvent( rEvent );
	
			m_ImGuiWindowManager->ProcessEvent( rEvent );
		}

		switch( rEvent.Type )
		{
			default: break;

			case EventType::KeyPressed:
			{
				OnKeyPressed( ( RubyKeyEvent& ) rEvent );
			} break;

			case EventType::MousePressed:
			{
				OnMousePressed( ( RubyMouseEvent& ) rEvent );
			} break;

			case EventType::SceneTravel:
			{
				HandleSceneTravel( ( SceneTravelEvent& ) rEvent );
			} break;

			case EventType::SkylightEntityModified:
			{
				const SkylightEntityModifiedEvent& rSkylightEvent = ( SkylightEntityModifiedEvent& ) rEvent;
				const auto& rParams = rSkylightEvent.GetParams();

				m_SceneRenderer->SetDynamicSky( rParams.x, rParams.y, rParams.z );
			} break;
		}
	}

	void EditorLayer::SaveFileAs()
	{
		// TODO: Support Saving scene as!
		const auto res = Application::Get().SaveFile( "Saturn Scene file (*.scene, *.sc)\0*.scene; *.sc\0" );

		SceneSerialiser serialiser( m_EditorScene );
		serialiser.Serialise( res );
	}

	void EditorLayer::SaveFile()
	{
		const auto fullPath = Project::GetActiveProject()->FilepathAbs( m_EditorScene->Path );
		if( std::filesystem::exists( fullPath ) )
		{
			SceneSerialiser ss( m_EditorScene );
			ss.Serialise();
		}
		else
		{
			SaveFileAs();
		}
	}

	void EditorLayer::SaveFileAuto()
	{
		m_AutoSaveCount = ( m_AutoSaveCount + 1 ) % 2;

		std::filesystem::path overridePath = "Cache";
		overridePath /= std::format( "{0}.{1}.autoscene", m_EditorScene->Name, m_AutoSaveCount );

		SceneSerialiser ss( m_EditorScene );
		ss.Serialise( overridePath, true );

		EditorNotification notification{ .Text = "AUTO SAVING, PLEASE WAIT", .Lifetime = 5.0f };
		PushNotification( notification );
	}

	void EditorLayer::OpenFile( AssetID id )
	{
		Ref<SceneHierarchyPanel> hierarchyPanel = m_ImGuiWindowManager->GetPanel<SceneHierarchyPanel>();

		Ref<Scene> newScene = Ref<Scene>::Create();
		g_ActiveScene = newScene.Get();

		EntitySelectionManager::Get().ClearSelection();
		hierarchyPanel->SetContext( nullptr );

		const Ref<Asset> asset = id == 0 ? nullptr : AssetManager::Get().FindAsset( id );
		
		if( asset )
		{
			SceneSerialiser serialiser( newScene );
			serialiser.Deserialise( asset );
		}

		m_EditorScene = newScene;

		if( asset )
		{
			m_EditorScene->Name = asset->Name;
			m_EditorScene->Path = asset->Path;
			m_EditorScene->ID = asset->ID;
			m_EditorScene->Type = asset->Type;
			m_EditorScene->Flags = asset->Flags;
		}
	
		g_ActiveScene = m_EditorScene.Get();

		hierarchyPanel->SetContext( m_EditorScene );
		newScene = nullptr;

		m_SceneRenderer->SetCurrentScene( m_EditorScene.Get() );
	}

	void EditorLayer::OpenFileInRuntime( AssetID id )
	{
		Ref<SceneHierarchyPanel> hierarchyPanel = m_ImGuiWindowManager->GetPanel<SceneHierarchyPanel>();

		Ref<Scene> newScene = Ref<Scene>::Create();
		g_ActiveScene = newScene.Get();

		EntitySelectionManager::Get().ClearSelection();
		hierarchyPanel->SetContext( nullptr );

		m_RuntimeScene->OnRuntimeEnd();

		const Ref<Asset> asset = AssetManager::Get().FindAsset( id );

		SceneSerialiser serialiser( newScene );
		serialiser.Deserialise( asset );

		m_RuntimeScene = newScene;

		m_RuntimeScene->Name = asset->Name;
		m_RuntimeScene->Path = asset->Path;
		m_RuntimeScene->ID = asset->ID;
		m_RuntimeScene->Type = asset->Type;
		m_RuntimeScene->Flags = asset->Flags;

		g_ActiveScene = m_RuntimeScene.Get();

		hierarchyPanel->SetContext( m_RuntimeScene );
		newScene = nullptr;

		m_SceneRenderer->SetCurrentScene( m_RuntimeScene.Get() );

		// If we fail to start runtime, terminate it for good.
		if( !m_RuntimeScene->OnRuntimeStart() )
		{
			hierarchyPanel->SetContext( m_EditorScene );
			CleanupRuntimeWhenFailed( RuntimeState::Running );
		}
	}

	void EditorLayer::SaveProject()
	{
		ProjectSerialiser ps( Project::GetActiveProject() );
		ps.Serialise( Project::GetActiveProject()->GetConfig().Path );

		AssetManagerSerialiser ars;
		ars.Serialise();
	}

	void EditorLayer::PreInitRuntime()
	{
		m_ImGuiWindowManager->OnRuntimeStateChanged( RuntimeState::Starting, RuntimeState::NoState );

		m_RuntimeScene = Ref<Scene>::Create();
		Scene::SetActiveScene( m_RuntimeScene.Get() );

		m_EditorScene->CopyScene( m_RuntimeScene );

		Input::Get().SetCanSetCursorMode( true );
	}

	void EditorLayer::PostInitRuntime()
	{
		m_LastRuntimeAttemptFailed = false;

		m_ImGuiWindowManager->GetPanel<SceneHierarchyPanel>()->SetContext( m_RuntimeScene );

		m_SceneRenderer->SetCurrentScene( m_RuntimeScene.Get() );

		m_EditorCamera.SetActive( false );

		const std::string title = std::format( "{0} (Running) - Saturn", Project::GetActiveConfig().Name );
		Application::Get().GetWindow()->ChangeTitle( title );

		m_ImGuiWindowManager->OnRuntimeStateChanged( RuntimeState::Running, RuntimeState::Starting );
	}

	void EditorLayer::EndRuntime()
	{
		m_ImGuiWindowManager->OnRuntimeStateChanged( RuntimeState::Ending, g_ActiveScene->GetRuntimeState() );

		Ref<SceneHierarchyPanel> hierarchyPanel = m_ImGuiWindowManager->GetPanel<SceneHierarchyPanel>();

		m_RuntimeScene->OnRuntimeEnd();
		Scene::SetActiveScene( m_EditorScene.Get() );

		hierarchyPanel->SetContext( m_EditorScene );

		m_SuspendedEditorCamera.SetActive( false );
		m_RuntimeScene = nullptr;

		m_SceneRenderer->SetCurrentScene( m_EditorScene.Get() );

		const std::string title = std::format( "{0} - Saturn", Project::GetActiveConfig().Name );
		Application::Get().GetWindow()->ChangeTitle( title );

		m_ImGuiWindowManager->OnRuntimeStateChanged( RuntimeState::NoState, RuntimeState::Ending );
	}

	void EditorLayer::CleanupRuntimeWhenFailed( RuntimeState lastState /*=RuntimeState::Starting*/ )
	{
		// Runtime was rejected, clean up and restore state
		m_ImGuiWindowManager->OnRuntimeStateChanged( RuntimeState::NoState, lastState );

		g_ActiveScene = m_EditorScene.Get();
		m_RuntimeScene = nullptr;

		m_RequestRuntime = false;
		m_LastRuntimeAttemptFailed = true;

		EditorNotification notification{ .Text = "Runtime request blocked. No camera was found after BeginPlay was called!", .Lifetime = 15.0f };
		PushNotification( notification );
	}

	bool EditorLayer::OnKeyPressed( RubyKeyEvent& rEvent )
	{
		switch( rEvent.GetKeycode() )
		{
			case RubyKey_Delete:
			{
				if( !m_RuntimeScene )
				{
					// Because of our ref system, the entity will be deleted when we clear the selections.
					// What we are really doing here is freeing it from the registry and removing the children.
					for( auto& rEntity : EntitySelectionManager::Get().GetSelectionContexts() )
					{
						GlobalUndoRedoGroup::Get().RemoveIfActionHasIdentifier( (uint64_t)rEntity->GetHandle() );
						
						g_ActiveScene->DeleteEntity( rEntity );
					}

					// The entities will be freed here!
					EntitySelectionManager::Get().ClearSelection();

					g_ActiveScene->MarkDirty();
				}
			} break;

			// We will never add Undo/Redo support to these as it's faster to just use the single shortcut key than do Control+Z/Y
			case RubyKey_Q:
				if( m_MouseOverViewport && !m_StartedRightClickInViewport )
					m_GizmoOperation = 0;
				break;

			case RubyKey_W:
				if( m_MouseOverViewport && !m_StartedRightClickInViewport )
					m_GizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
				break;

			case RubyKey_E:
				if( m_MouseOverViewport && !m_StartedRightClickInViewport )
					m_GizmoOperation = ImGuizmo::OPERATION::ROTATE;
				break;

			case RubyKey_R:
				if( m_MouseOverViewport && !m_StartedRightClickInViewport )
					m_GizmoOperation = ImGuizmo::OPERATION::SCALE;
				break;

			case RubyKey_F5:
			{
				if( m_MouseOverViewport || m_ViewportFocused )
					m_RequestRuntime ^= 1;
			} break;
		}

		if( Input::Get().KeyPressed( RubyKey_LeftCtrl ) && !m_RuntimeScene )
		{
			switch( rEvent.GetKeycode() )
			{
				case RubyKey_D:
				{					
					for( const auto& rEntity : EntitySelectionManager::Get().GetSelectionContexts() )
					{
						g_ActiveScene->DuplicateEntity( rEntity );
					}

					g_ActiveScene->MarkDirty();
				} break;

				// TODO: Support more than one selection.
				case RubyKey_F:
				{
					auto& rSelectedEntities = EntitySelectionManager::Get().GetSelectionContexts();

					glm::vec3 Positions = {};
					for( auto& rEntity : rSelectedEntities )
					{
						TransformComponent worldSpace = g_ActiveScene->GetWorldSpaceTransform( rEntity );
						Positions += worldSpace.Position;
					}

					Positions /= rSelectedEntities.size();

					m_EditorCamera.Focus( Positions );
				} break;

				case RubyKey_S:
				{
					SaveFile();
				} break;

				case RubyKey_Z:
				{
					if( auto action = m_GlobalUndoRedoGroup->GlobalUndoRecent(); action )
					{
						const std::string undoName = std::format( "Undo {0}", action->GetName() );
						EditorNotification notification{ .Text = undoName, .Lifetime = 3.0f };
						PushNotification( notification );
					}
				} break;
			
				case RubyKey_Y:
				{
					if( auto action = m_GlobalUndoRedoGroup->GlobalRedoRecent(); action )
					{
						const std::string redoName = std::format( "Redo {0}", action->GetName() );
						EditorNotification notification{ .Text = redoName, .Lifetime = 3.0f };
						PushNotification( notification );
					}
				} break;
			}

			if( Input::Get().KeyPressed( RubyKey_LeftShift ) || Input::Get().KeyPressed( RubyKey_RightShift ) )
			{
				switch( rEvent.GetKeycode() )
				{
					case RubyKey_S:
					{
						SaveFileAs();
					} break;
				}
			}

#if defined(SAT_RELEASE)
			if( Input::Get().KeyPressed( RubyKey_LeftAlt ) && g_ActiveScene != m_RuntimeScene.Get() )
			{
				switch( rEvent.GetKeycode() )
				{
					case RubyKey_F5:
					{
						HotReloadGame();
					} break;
				}
			}
#endif
		}

		return true;
	}

	struct Ray
	{
		glm::vec3 Origin;
		glm::vec3 Direction;

		inline bool IntersectsAABB( const AABB& rBB, float& t ) const
		{
			glm::vec3 dirfrac{};
			// r.dir is unit direction vector of ray
			dirfrac.x = 1.0f / Direction.x;
			dirfrac.y = 1.0f / Direction.y;
			dirfrac.z = 1.0f / Direction.z;
			// lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
			// r.org is origin of ray
			const glm::vec3& lb = rBB.Min;
			const glm::vec3& rt = rBB.Max;
			const float t1 = ( lb.x - Origin.x ) * dirfrac.x;
			const float t2 = ( rt.x - Origin.x ) * dirfrac.x;
			const float t3 = ( lb.y - Origin.y ) * dirfrac.y;
			const float t4 = ( rt.y - Origin.y ) * dirfrac.y;
			const float t5 = ( lb.z - Origin.z ) * dirfrac.z;
			const float t6 = ( rt.z - Origin.z ) * dirfrac.z;

			const float tmin = glm::max( glm::max( glm::min( t1, t2 ), glm::min( t3, t4 ) ), glm::min( t5, t6 ) );
			const float tmax = glm::min( glm::min( glm::max( t1, t2 ), glm::max( t3, t4 ) ), glm::max( t5, t6 ) );

			// if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us
			if( tmax < 0 )
			{
				t = tmax;
				return false;
			}

			// if tmin > tmax, ray doesn't intersect AABB
			if( tmin > tmax )
			{
				t = tmax;
				return false;
			}

			t = tmin;
			return true;
		}

		bool IntersectsTri( const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, float& t ) const
		{
			const glm::vec3 E1 = b - a;
			const glm::vec3 E2 = c - a;
			const glm::vec3 N = cross( E1, E2 );
			const float det = -glm::dot( Direction, N );
			const float invdet = 1.f / det;
			const glm::vec3 AO = Origin - a;
			const glm::vec3 DAO = glm::cross( AO, Direction );
			const float u = glm::dot( E2, DAO ) * invdet;
			const float v = -glm::dot( E1, DAO ) * invdet;
			t = glm::dot( AO, N ) * invdet;
			return ( det >= 1e-6f && t >= 0.0f && u >= 0.0f && v >= 0.0f && ( u + v ) <= 1.0f );
		}
	};

	bool EditorLayer::OnMousePressed( RubyMouseEvent& rEvent )
	{
		if( m_RuntimeScene || !m_MouseOverViewport || rEvent.GetButton() != (int)RubyMouseButton_Left || ImGuizmo::IsOver() )
			return false;

		const auto viewportMouse = ConvertMouseToViewportNDC();
		if( viewportMouse.x > -1.0f && viewportMouse.x < 1.0f && viewportMouse.y > -1.0f && viewportMouse.y < 1.0f )
		{
			const auto [origin, dir] = RayCast( viewportMouse.x, viewportMouse.y );

			const auto staticMeshes = g_ActiveScene->GetAllEntitiesWith<StaticMeshComponent>();
			for( const auto& rEntity : staticMeshes )
			{
				const auto& comp = rEntity->GetComponent<StaticMeshComponent>();
				if( !comp.Mesh ) 
					continue;

				auto& rSubmeshes = comp.Mesh->Submeshes();
				for( uint32_t i = 0; i < rSubmeshes.size(); i++ )
				{
					const auto& rSubmesh = rSubmeshes[ i ];
					const glm::mat4 transform = g_ActiveScene->GetWorldSpaceTransform( rEntity ).GetTransform() * rSubmesh.Transform;

					const Ray ray = { .Origin = glm::inverse( transform ) * glm::vec4( origin, 1.0f ), .Direction = glm::inverse( glm::mat3( transform ) ) * dir };

					float t;
					const bool hit = ray.IntersectsAABB( rSubmesh.BoundingBox, t );
					if( hit )
					{
						const auto& rIndices = comp.Mesh->Indices();
						const auto& rVertices = comp.Mesh->Vertices();

						for( const auto& rTri : rIndices )
						{
							const glm::vec3& rV0 = rVertices[ rTri.V1 ].Position;
							const glm::vec3& rV1 = rVertices[ rTri.V2 ].Position;
							const glm::vec3& rV2 = rVertices[ rTri.V3 ].Position;

							float t;
							if( ray.IntersectsTri( rV0, rV1, rV2, t ) )
							{
								auto hierarchyPanel = m_ImGuiWindowManager->GetPanel<SceneHierarchyPanel>();
								hierarchyPanel->SetSelected( rEntity );

								break;
							}
						}
					}
				}
			}
		}

		return false;
	}

	void EditorLayer::HandleSceneTravel( SceneTravelEvent& rEvent )
	{
		const AssetID destinationID = rEvent.GetID();
		const Ref<Asset> sceneAsset = AssetManager::Get().FindAsset( destinationID );
		
		if( !sceneAsset )
		{
			SAT_CORE_ERROR( "Failed to travel as ASSET/{0} is not a valid scene ID!", destinationID );
			return;
		}

		OpenFileInRuntime( destinationID );
	}

	static bool s_OpenAssetFinderPopup = false;

	void EditorLayer::DrawProjectSettingsWindow()
	{
		static bool ShouldSaveProject = false;

		ImGuiIO& rIO = ImGui::GetIO();

		auto& userSettings = EngineSettings::Get();
		Ref<Project> ActiveProject = Project::GetActiveProject();

		auto& rConfig = ActiveProject->GetConfig();
		auto& startupSceneID = rConfig.StartupSceneID;
		Ref<Asset> startupSceneAsset = AssetManager::Get().FindAsset( startupSceneID );

		ImGui::SetNextWindowPos( ImVec2( rIO.DisplaySize.x * 0.5f - 150.0f, rIO.DisplaySize.y * 0.5f - 150.0f ), ImGuiCond_Once );

		if( ImGui::Begin( "Project settings", &m_ShowUserSettings ) ) 
		{
			const auto boldFont = rIO.Fonts->Fonts[ 1 ];
			ImGui::PushFont( boldFont );
			ImGui::Text( "Project Defaults" );
			ImGui::Separator();
			ImGui::PopFont();

			if( s_OpenAssetFinderPopup )
				ImGui::OpenPopup( "AssetFinderPopup" );

			ImGui::BeginHorizontal( "##prj_strtscene" );
			{
				ImGui::Text( "Startup Scene:" );
				startupSceneID == 0 ? ImGui::TextColored( ImVec4( 1.0f, 0.0f, 0.0f, 1.0f ), "None" ) : ImGui::Text( startupSceneAsset->Name.c_str() );

				ImGui::Spring();

				Auxiliary::DisabledFlag inspectDisabledFlag( m_RequestRuntime );

				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), { 24.0f, 24.0f } ) )
					s_OpenAssetFinderPopup = true;

				if( Auxiliary::DrawAssetFinder( AssetType::Scene, &s_OpenAssetFinderPopup, rConfig.StartupSceneID ) )
				{
					ShouldSaveProject = true;
				}

				inspectDisabledFlag.Pop();

				{
					Auxiliary::ScopedDisabledFlag disabledFlag( rConfig.StartupSceneID == 0 && !m_RequestRuntime );

					if( Auxiliary::ImageButton( EditorIcons::GetIcon( "NoIcon" ), { 24.0f, 24.0f } ) )
					{
						Ref<Asset> target = AssetManager::Get().FindAsset( rConfig.StartupSceneID );

						if( target )
						{
							Ref<ContentBrowserPanel> contentBrowserPanel = m_ImGuiWindowManager->GetPanel<ContentBrowserPanel>();
							contentBrowserPanel->BrowseToItem( target->Path, rConfig.StartupSceneID );
						}
					}
				}
			}
			ImGui::EndHorizontal();

			ImGui::BeginVertical( "##prj_defaults" );

			ImGui::BeginHorizontal( "##prj_defmatasset" );
			{
				auto defaultMaterialID = ActiveProject->GetDefaultMaterialAsset();

				ImGui::Text( "Default Material Asset:" );
				defaultMaterialID == 0 ? ImGui::TextColored( ImVec4( 1.0f, 0.0f, 0.0f, 1.0f ), "None" ) : ImGui::Text( "%llu", defaultMaterialID );

				ImGui::Spring();

				Auxiliary::DisabledFlag inspectDisabledFlag( m_RequestRuntime );

				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), { 24.0f, 24.0f } ) )
					s_OpenAssetFinderPopup = true;

				if( Auxiliary::DrawAssetFinder( AssetType::Material, &s_OpenAssetFinderPopup, defaultMaterialID ) )
				{
					ActiveProject->SetDefaultMaterialAsset( defaultMaterialID );
					ShouldSaveProject = true;
				}

				inspectDisabledFlag.Pop();

				{
					Auxiliary::ScopedDisabledFlag disabledFlag( defaultMaterialID == 0 );

					if( Auxiliary::ImageButton( EditorIcons::GetIcon( "NoIcon" ), { 24.0f, 24.0f } ) )
					{
						const Ref<Asset> target = AssetManager::Get().FindAsset( defaultMaterialID );

						if( target )
						{
							Ref<ContentBrowserPanel> contentBrowserPanel = m_ImGuiWindowManager->GetPanel<ContentBrowserPanel>();
							contentBrowserPanel->BrowseToItem( target->Path, defaultMaterialID );
						}
					}
				} 
			}
			ImGui::EndHorizontal();

			ImGui::BeginHorizontal( "##prj_defphysmatasset" );
			{
				auto defaultMaterialID = ActiveProject->GetDefaultPhysicsMaterialAsset();

				ImGui::Text( "Default Physics Material Asset:" );
				defaultMaterialID == 0 ? ImGui::TextColored( ImVec4( 1.0f, 0.0f, 0.0f, 1.0f ), "None" ) : ImGui::Text( "%llu", defaultMaterialID );

				ImGui::Spring();

				Auxiliary::DisabledFlag inspectDisabledFlag( m_RequestRuntime );

				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), { 24.0f, 24.0f } ) )
					s_OpenAssetFinderPopup = true;

				if( Auxiliary::DrawAssetFinder( AssetType::PhysicsMaterial, &s_OpenAssetFinderPopup, defaultMaterialID ) )
				{
					ActiveProject->SetDefaultPhysicsMaterialAsset( defaultMaterialID );
					ShouldSaveProject = true;
				}

				inspectDisabledFlag.Pop();

				{
					Auxiliary::ScopedDisabledFlag disabledFlag( defaultMaterialID == 0 );

					if( Auxiliary::ImageButton( EditorIcons::GetIcon( "NoIcon" ), { 24.0f, 24.0f } ) )
					{
						const Ref<Asset> target = AssetManager::Get().FindAsset( defaultMaterialID );

						if( target )
						{
							Ref<ContentBrowserPanel> contentBrowserPanel = m_ImGuiWindowManager->GetPanel<ContentBrowserPanel>();
							contentBrowserPanel->BrowseToItem( target->Path, defaultMaterialID );
						}
					}
				}
			}
			ImGui::EndHorizontal();

//			ImGui::BeginHorizontal( "##prj_autosaves" );
			{
				bool enableAutoSaves = ActiveProject->IsAutoSavesEnabled();
			
				if( Auxiliary::DrawBoolControl( "Enable Auto Saves", enableAutoSaves ) ) 
				{
					ActiveProject->EnableAutoSaves( enableAutoSaves );
					ShouldSaveProject = true;
				}

				Auxiliary::DisabledFlag disabledIfNoAutoSaves( !ActiveProject->IsAutoSavesEnabled() );

				ImGui::BeginHorizontal( "##prj_autosaves_interval" );

				float intervalSeconds = ActiveProject->GetAutoSaveInterval();
				std::string unit;
				float displayValue = intervalSeconds;

				if( intervalSeconds < 60.0f )
				{
					unit = "seconds";
				}
				else if( intervalSeconds < 3600.0f ) 
				{
					unit = "minutes";
					displayValue /= 60.0f;
				}
				else 
				{
					unit = "hours";
					displayValue /= 3600.0f;
				}

				if( Auxiliary::DrawFloatControl( "Auto Save Interval", displayValue ) )
				{
					if( unit == "seconds" )
					{
						intervalSeconds = displayValue;
					}
					else if( unit == "minutes" )
					{
						intervalSeconds = displayValue * 60.0f;
					}
					else
					{
						// Anything larger, display as hours
						intervalSeconds = displayValue * 3600.0f;
					}

					ShouldSaveProject = true;
					ActiveProject->SetAutoSaveInterval( intervalSeconds );

					// Reset timer because this value is incremented even when auto saves are disabled.
					m_LastAutoSaveTime = 0.0f;
				}

				ImGui::Spring();
				ImGui::Text( unit.c_str() );

				ImGui::EndHorizontal();

				disabledIfNoAutoSaves.Pop();
			}
//			ImGui::EndHorizontal();

			ImGui::EndVertical();

			ImGui::PushFont( boldFont );
			ImGui::Text( "Action Bindings" );
			ImGui::Separator();

			ImGui::PopFont();

			Auxiliary::DisabledFlag disabledFlagIfRuntime( m_RequestRuntime );

			for( auto rIt = ActiveProject->GetActionBindings().begin(); rIt != ActiveProject->GetActionBindings().end(); )
			{
				auto& rBinding = *( rIt );

				const std::string id = "##" + std::to_string( rBinding.ID );

				ImGui::SetNextItemWidth( 130.0f );
				Auxiliary::InputText( id.data(), &rBinding.Name );

				ImGui::SameLine(); // HACK, There seems to bug with the ImGui Layout as the InputText works fine when it's not in a Horizontal layout. (Update) Seems to be with certain IDs/labels

				ImGui::BeginHorizontal( rBinding.Name.data() );

				ImGui::SetNextItemWidth( 130.0f );
				if( ImGui::BeginCombo( "##KEYLIST", rBinding.ActionName.data() ) )
				{
					for( uint16_t i = 0; i < RubyKey_EnumSize; i++ )
					{
						const auto& result = RubyKeyToString( ( RubyKey ) i );

						// This is here because of how we do our loop, some keys will be empty because the values to do not match up.
						if( result.empty() )
							continue;

						const bool IsSelected = ( rBinding.ActionName == result );

						ImGui::PushID( i );

						ImGui::SetNextItemWidth( 130.0f );
						if( ImGui::Selectable( result.data(), IsSelected ) )
						{
							if( rBinding.Type == ActionBindingType::Mouse )
								rBinding.MouseButton = RubyMouseButton_Unknown;

							rBinding.Key = ( RubyKey ) i;
							rBinding.Type = ActionBindingType::Key;
							rBinding.ActionName = result;

							ShouldSaveProject = true;
						}

						if( IsSelected )
							ImGui::SetItemDefaultFocus();

						ImGui::PopID();
					}

					for( int i = 0; i < 5; i++ )
					{
						const auto& result = RubyMouseButtonToString( ( RubyMouseButton ) i );

						// This is here because of how we do our loop, some keys will be empty because the values to do not match up.
						if( result.empty() )
							continue;

						const bool IsSelected = ( rBinding.ActionName == result );

						ImGui::PushID( i );

						ImGui::SetNextItemWidth( 130.0f );
						if( ImGui::Selectable( result.data(), IsSelected ) )
						{
							if( rBinding.Type == ActionBindingType::Key )
								rBinding.Key = RubyKey_UnknownKey;

							rBinding.MouseButton = ( RubyMouseButton ) i;
							rBinding.Type = ActionBindingType::Mouse;
							rBinding.ActionName = result;

							ShouldSaveProject = true;
						}

						if( IsSelected )
							ImGui::SetItemDefaultFocus();

						ImGui::PopID();
					}

					ImGui::EndCombo();
				}

				if( ImGui::SmallButton( "-" ) )
				{
					rIt = ActiveProject->GetActionBindings().erase( rIt );
					ShouldSaveProject = true;
				}
				else
				{
					++rIt;
				}

				ImGui::EndHorizontal();
			}

			if( ImGui::SmallButton( "+" ) )
			{
				ActionBindingData ab;
				ab.Name = "Empty Binding";

				int count = 0;
				// Find all other actions bindings with the same name.
				for( const auto& bindings : ActiveProject->GetActionBindings() )
				{
					if( bindings.Name.contains( "Empty Binding" ) )
						count++;
				}

				if( count >= 1 )
				{
					ab.Name += " ";
					ab.Name += std::to_string( count );
				}

				ActiveProject->AddActionBinding( ab );
				ShouldSaveProject = true;
			}

			disabledFlagIfRuntime.Pop();

			ImGui::PushFont( boldFont );
			ImGui::Text( "Audio Groups" );
			ImGui::Separator();
			ImGui::PopFont();

			ProjectSettings_DrawSoundGroupEdit( AudioSystem::Get().GetMasterSoundGroup() );
			ImGui::EndHorizontal(); // EndHorizontal from ProjectSettings_DrawSoundGroupEdit

			for( auto rIt = ActiveProject->GetSoundGroups().begin(); rIt != ActiveProject->GetSoundGroups().end(); )
			{
				auto& rSoundGroup = *( rIt );

				ProjectSettings_DrawSoundGroupEdit( rSoundGroup );

				Auxiliary::DisabledFlag disabledFlag( m_RequestRuntime );

				if( ImGui::SmallButton( "-" ) )
				{
					rIt = ActiveProject->GetSoundGroups().erase( rIt );
					ShouldSaveProject = true;
				}
				else
				{
					++rIt;
				}

				disabledFlag.Pop();

				ImGui::EndHorizontal();
			}

			ImGui::PushID( "##nwSndGrp" );

			if( ImGui::SmallButton( "+" ) )
			{
				Ref<SoundGroup> group = Ref<SoundGroup>::Create( "New Sound Group" );
				group->Init( false );

				// Find all other sound groups with the same name.
				int count = 0;
				for( const auto& rGroups : ActiveProject->GetSoundGroups() )
				{
					if( rGroups->GetName().contains( "New Sound Group" ) )
						count++;
				}

				if( count >= 1 )
				{
					std::string newName = std::format( "{0} ({1})", group->GetName(), std::to_string( count ) );
					group->SetName( newName );
				}

				ActiveProject->AddSoundGroup( group );
				ShouldSaveProject = true;
			}

			ImGui::PopID();

			// This does not matter because the editor is not designed to run in Dist, however, right now I want to keep this in release builds.
#if !defined(SAT_DIST)
			ImGui::PushFont( boldFont );
			ImGui::Text( "Project Debug information" );
			ImGui::Separator();
			ImGui::PopFont();

			if( ImGui::BeginTable( "##DebugInfoPrj", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX | ImGuiTableFlags_NoBordersInBody ) )
			{
				ImGui::TableSetupColumn( "Key" );
				ImGui::TableSetupColumn( "Value" );

				ImGui::TableHeadersRow();

				{
					auto drawRow = []( const char* pKey, const std::string& value )
					{
						ImGui::TableNextRow();
						
						ImGui::TableSetColumnIndex( 0 );
						ImGui::Text( "%s", pKey );
						
						ImGui::TableSetColumnIndex( 1 );
						ImGui::Text( "%s", value.c_str() );
					};

					drawRow( "Root Path", ActiveProject->GetRootDir().string() );
					drawRow( ".sproject Path", ActiveProject->GetConfig().Path.string() );
					drawRow( "Asset Path", ActiveProject->GetFullAssetPath().string() );
					drawRow( "Premake filename", ActiveProject->GetPremakeFile().string() );
					drawRow( "Temporary Path", ActiveProject->GetTempDir().string());
					drawRow( "Binary Path", ActiveProject->GetBinDir().string() );
					drawRow( "Cache Path", ActiveProject->GetFullCachePath().string() );

					drawRow( "Module Path", GameModule::Get().GetModulePath().string() );
					drawRow( "Module Timestamp", std::format( "X{0}", GameModule::Get().GetTimestamp() ) );
				}

				ImGui::EndTable();
			}
#endif
		}

		ImGui::End();

		// Only save project if the window has been closed.
		if( ShouldSaveProject && !m_ShowUserSettings )
		{
			ProjectSerialiser ps;
			ps.Serialise( Project::GetActiveProject()->GetRootDir().string() );

			ShouldSaveProject = false;
		}
	}

	void EditorLayer::ProjectSettings_DrawSoundGroupEdit( Ref<SoundGroup>& rSoundGroup )
	{
		char buffer[ 256 ];
		memset( buffer, 0, 256 );
		memcpy( buffer, rSoundGroup->GetName().data(), rSoundGroup->GetName().length() );

		// TODO: Change to unique ID
		std::string id = "##entergrpname";

		ImGuiInputTextFlags inputTextFlags = m_RequestRuntime ? ImGuiInputTextFlags_ReadOnly : 0;

		ImGui::SetNextItemWidth( 130.0f );
		if( ImGui::InputText( id.data(), buffer, 256, inputTextFlags ) )
		{
			rSoundGroup->SetName( std::string( buffer ) );
		}

		ImGui::SameLine(); // HACK, There seems to bug with the ImGui Layout as the InputText works fine when it's not in a Horizontal layout. (Update) Seems to be with certain IDs/labels

		ImGui::BeginHorizontal( rSoundGroup->GetName().data() );

		// Volume & Pitch Control
		float volume = rSoundGroup->GetVolume();
		float pitch = rSoundGroup->GetPitch();

		ImGui::BeginHorizontal( "##vpSliders" );

		ImGui::Text( "Volume" );

		ImGui::SetNextItemWidth( 130.0f );
		if( ImGui::DragFloat( "##volumeMul", &volume, 0.1f, 0.0f, 100.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp ) )
		{
			rSoundGroup->SetVolume( volume );
		}

		ImGui::Spring();

		ImGui::Text( "Pitch" );

		ImGui::SetNextItemWidth( 130.0f );
		if( ImGui::DragFloat( "##pitchMul", &pitch, 0.1f, 0.0f, 100.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp ) )
		{
			rSoundGroup->SetPitch( pitch );
		}

		ImGui::Spring();

		// End vpSliders Horizontal
		ImGui::EndHorizontal();
		
		// Don't end main horizontal as we might need to still draw more.
	}

	void EditorLayer::HotReloadGame()
	{
#if defined(SAT_RELEASE)
		EditorNotification notification;
		notification.Text = "Attemping hot reload";
		notification.Lifetime = 5.0f;

		PushNotification( notification );

		SAT_CORE_INFO( "Begin hot reload" );

		SaveFile();
		SaveProject();

		m_GameModule->BeginHotReload();
		Project::GetActiveProject()->Build( ApplicationConfigKind::Release, "/HOTRELOAD" );
		m_GameModule->EndHotReload();

		m_EditorScene->AcknowledgeHotReload();

		notification.Text = "Hot reload complete";
		PushNotification( notification );
#endif
	}

	void EditorLayer::DrawAssetRegistryDebug()
	{
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
		if( ImGui::Begin( "Asset Manager", &m_OpenAssetRegistryDebug, flags ) )
		{
			static ImGuiTextFilter Filter;

			ImGui::Text( "Search" );
			ImGui::SameLine();
			Filter.Draw( "##search" );

			ImGuiTableFlags TableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX | ImGuiTableFlags_NoBordersInBody;
			if( ImGui::BeginTable( "##FileTable", 6, TableFlags, ImVec2( ImGui::GetWindowSize().x, ImGui::GetWindowSize().y ) ) )
			{
				ImGui::TableSetupColumn( "Asset Name" );
				ImGui::TableSetupColumn( "ID" );
				ImGui::TableSetupColumn( "Type" );
				ImGui::TableSetupColumn( "Path" );
				ImGui::TableSetupColumn( "Version" );
				ImGui::TableSetupColumn( "Find Asset", ImGuiTableColumnFlags_NoHeaderLabel );

				ImGui::TableHeadersRow();

				for( auto&& [id, asset] : AssetManager::Get().GetCombinedAssetMap() )
				{
					if( !Filter.PassFilter( asset->Name.c_str() ) )
						continue;

					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex( 0 );
					ImGui::Selectable( asset->Name.c_str(), false );

					ImGui::TableSetColumnIndex( 1 );
					ImGui::Text( "%llu", id );

					ImGui::TableSetColumnIndex( 2 );
					ImGui::Text( AssetTypeToString( asset->Type ).data(), false );

					ImGui::TableSetColumnIndex( 3 );
					ImGui::Text( asset->Path.string().c_str() );

					ImGui::TableSetColumnIndex( 4 );
					ImGui::Text( "%i", asset->Version );

					if( asset->Version != SAT_CURRENT_VERSION )
					{
						ImGui::SameLine();
						ImGui::Text( "(Version does not match)" );
					}

					ImGui::TableSetColumnIndex( 5 );
					ImGui::PushID( ( int ) id );
					if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), { 24.0f, ImGui::TableGetHeaderRowHeight() } ) )
					{
						Ref<ContentBrowserPanel> contentBrowserPanel = m_ImGuiWindowManager->GetPanel<ContentBrowserPanel>();

						contentBrowserPanel->BrowseToItem( asset->Path, id );
					}
					ImGui::PopID();
				}

				ImGui::EndTable();
			}

			ImGui::End();
		}
	}

	void EditorLayer::DrawLoadedAssetsDebug()
	{
		if( ImGui::Begin( "Loaded Assets", &m_OpenLoadedAssetDebug ) )
		{
			static ImGuiTextFilter Filter;

			ImGui::Text( "Search for assets..." );
			ImGui::SameLine();
			Filter.Draw( "##search" );

			if( ImGui::BeginTable( "##FileTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX | ImGuiTableFlags_NoBordersInBody, ImVec2( ImGui::GetWindowSize().x, ImGui::GetWindowSize().y * 0.85f ) ) )
			{
				ImGui::TableSetupColumn( "Asset Name" );
				ImGui::TableSetupColumn( "ID" );
				ImGui::TableSetupColumn( "Type" );
				ImGui::TableSetupColumn( "Find Asset", ImGuiTableColumnFlags_NoHeaderLabel );

				ImGui::TableHeadersRow();

				for( auto&& [id, asset] : AssetManager::Get().GetCombinedLoadedAssetMap() )
				{
					if( !Filter.PassFilter( asset->Name.c_str() ) )
						continue;

					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex( 0 );
					ImGui::Selectable( asset->Name.c_str(), false );

					ImGui::TableSetColumnIndex( 1 );
					ImGui::Text( "%llu", id );

					ImGui::TableSetColumnIndex( 2 );
					ImGui::Text( AssetTypeToString( asset->Type ).data(), false );

					ImGui::TableSetColumnIndex( 3 );
					ImGui::PushID( (int)id );
					if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), { ImGui::TableGetHeaderRowHeight(), ImGui::TableGetHeaderRowHeight() } ) )
					{
						Ref<ContentBrowserPanel> contentBrowserPanel = m_ImGuiWindowManager->GetPanel<ContentBrowserPanel>();

						contentBrowserPanel->BrowseToItem( asset->Path, id );
					}
					ImGui::PopID();
				}

				ImGui::EndTable();
			}

			ImGui::End();
		}
	}

	void EditorLayer::DrawEditorSettings()
	{
		auto& rIO = ImGui::GetIO();

		ImGui::SetNextWindowSize( ImVec2( 750.0f, 750.0f ), ImGuiCond_Appearing );
		if( ImGui::Begin( "Editor Settings", &m_OpenEditorSettings ) )
		{
			const auto boldFont = rIO.Fonts->Fonts[ 1 ];
			const auto italicsFont = rIO.Fonts->Fonts[ 2 ];

			ImGui::PushFont( boldFont );
			ImGui::Text( "Saturn Editor Settings" );
			ImGui::PopFont();

			ImGui::PushStyleColor( ImGuiCol_Text, ImVec4{ 0.7f, 0.7f, 0.7f, 0.7f } );
			ImGui::PushFont( italicsFont );
			ImGui::Text( "Saturn Engine Version: %s (%d)", SAT_CURRENT_VERSION_STRING, SAT_CURRENT_VERSION );
			ImGui::PopFont();
			ImGui::PopStyleColor();

			ImGui::PushStyleColor( ImGuiCol_Separator, ImVec4{ 0.7f, 0.7f, 0.7f, 0.7f } );
			ImGui::Separator();
			ImGui::PopStyleColor();

			ImGui::End();
		}
	}

	void EditorLayer::DrawVFSDebug()
	{
		VirtualFS& rVirtualFS = VirtualFS::Get();

		ImGui::Begin( "Virtual File system", &m_ShowVFSDebug );

		if( Auxiliary::TreeNode( "VFS Info", false ) )
		{
			ImGui::Text( "Mount Bases: %i", rVirtualFS.GetMountBases() );
			ImGui::Text( "Mounts: %i", rVirtualFS.GetMounts() );
		
			Auxiliary::EndTreeNode();
		}

		rVirtualFS.ImGuiRender();

		ImGui::End();
	}

	void EditorLayer::DrawTitlebarOptions()
	{
		if( ImGui::BeginMenu( "File" ) )
		{
			Auxiliary::DisabledFlag disabledIfRuntime( m_RequestRuntime );
				
			if( ImGui::MenuItem( "Save Scene", "Ctrl+S" ) )          SaveFile();
			if( ImGui::MenuItem( "Save Scene As", "Ctrl+Shift+S" ) ) SaveFileAs();

			if( ImGui::MenuItem( "Save Project" ) )                  SaveProject();
			if( ImGui::MenuItem( "Close Project" ) )                 CloseEditorAndOpenPB();

			disabledIfRuntime.Pop();

			if( ImGui::MenuItem( "Exit", "Alt+F4" ) )                if( OnTitlebarExit() ) Application::Get().Close();

			ImGui::EndMenu();
		}

		if( ImGui::BeginMenu( "Saturn" ) )
		{
			if( ImGui::MenuItem( "About" ) )        m_OpenAboutWindow ^= 1;
			
			ImGui::SeparatorText( "Windows" );

			if( ImGui::MenuItem( "Scene Renderer" ) )         m_ShowSceneRendererWindow ^= 1;
			if( ImGui::MenuItem( "Renderer (Vulkan Info)" ) ) m_ShowRendererWindow ^= 1;
			if( ImGui::MenuItem( "Content Browser Panel" ) )  ShowOrHideContentBrowserPanel();
			if( ImGui::MenuItem( "Scene Hierarchy Panel" ) )  ShowOrHideSceneHierarchyPanel();

			ImGui::EndMenu();
		}

		if( ImGui::BeginMenu( "Project" ) )
		{
			ImGui::SeparatorText( "Settings" );

			if( ImGui::MenuItem( "Project settings" ) ) m_ShowUserSettings ^= 1;

			ImGui::SeparatorText( "Compatibility" );

			{
				Auxiliary::ScopedDisabledFlag disabled( m_RequestRuntime );
				if( ImGui::MenuItem( "Upgrade assets" ) ) Project::GetActiveProject()->UpgradeAssets();
			}

			ImGui::SeparatorText( "Building and Distribution" );

			{
				Auxiliary::ScopedDisabledFlag disabled( m_RequestRuntime );

				if( ImGui::MenuItem( "Recreate project files" ) )
				{
					m_HasPremakePath = Auxiliary::HasEnvironmentVariable( "SATURN_PREMAKE_PATH" );

					JobSystem::Get().AddJob( []()
					{
						if( !Project::GetActiveProject()->HasPremakeFile() )
							Project::GetActiveProject()->CreatePremakeFile();

						Premake::Launch( Project::GetActiveProject()->GetRootDir().wstring() );
					} );
				}

				if( ImGui::BeginItemTooltip() )
				{
					ImGui::Text( "Uses Premake5 to regenerate the project files.\nEnvironment variable \"SATURN_PREMAKE_PATH\" must be set." );
					ImGui::EndTooltip();
				}

				if( ImGui::MenuItem( "Setup Project for Distribution & Build Asset Bundle" ) )
				{
					if( ValidateProjectDefaults() )
					{
						if( !m_BlockingOperation )
							m_BlockingOperation = Ref<JobProgress>::Create();

						CreateShaderBundleJob();

						// TODO: Think of a better way for this... checking the sizes of the message boxes is not a good thing.
						if( m_MessageBoxes.size() == 0 )
						{
							CreateAssetBundleJob();
						}
					}
				}

				if( ImGui::BeginItemTooltip() )
				{
					ImGui::Text( "Attempts to build the Shader Bundle and the Asset Bundle and copies important build files for distribution.\nYou must run this before clicking the \"Distribute project\" button." );
					ImGui::EndTooltip();
				}

				if( ImGui::MenuItem( "Build Shader Bundle" ) )
				{
					BuildShaderBundle();
				}

				if( ImGui::BeginItemTooltip() )
				{
					ImGui::Text( "Attempts to compile all shaders and bundles them all into one file.\nYou do not need to do this if your intent is to prepare the project for distribution as that option will build it for you.\nOnly build the Shader Bundle if there is a problem with your shaders." );
					ImGui::EndTooltip();
				}


				if( ImGui::MenuItem( "Distribute project" ) )
				{
					m_HasPremakePath = Auxiliary::HasEnvironmentVariable( "SATURN_PREMAKE_PATH" );

					if( !m_BlockingOperation )
						m_BlockingOperation = Ref<JobProgress>::Create();

					JobSystem::Get().AddJob( [this]()
						{
							m_JobModalOpen = true;
							m_BlockingOperation->SetTitle( "Distributing Project" );

							m_BlockingOperation->SetStatus( "Building project" );
							Project::GetActiveProject()->Rebuild( ApplicationConfigKind::Dist );

							m_BlockingOperation->SetProgress( 50.0f );

							m_BlockingOperation->SetStatus( "Copying for Distribution" );
							Project::GetActiveProject()->Distribute( ApplicationConfigKind::Dist );

							m_BlockingOperation->SetProgress( 100.0f );
							m_BlockingOperation->OnComplete();
						} );
				}

				if( ImGui::BeginItemTooltip() )
				{
					ImGui::Text( "Attempts to compile the project and fully setup the project for Distribution.\nMake sure you have prepare the project before attempting to distribute the project." );
					ImGui::EndTooltip();
				}
			}

#if defined( SAT_DEBUG )
			ImGui::SeparatorText( "DEBUG" );

			if( ImGui::MenuItem( "DEBUG: Read Asset Bundle" ) )
			{
				Application::Get().GetSpecification().Flags |= ApplicationFlag_UseVFS;
				auto res = AssetBundle::ReadBundle();
			}

			if( ImGui::MenuItem( "DEBUG: Build Asset Bundle (no shaders)" ) )
			{
				CreateAssetBundleJob();
			}
#endif

			ImGui::EndMenu();
		}

		if( ImGui::BeginMenu( "Settings" ) )
		{
			if( ImGui::MenuItem( "Project settings", "" ) )           m_ShowUserSettings       ^= 1;
			if( ImGui::MenuItem( "Editor Settings", "" ) )            m_OpenEditorSettings     ^= 1;

			ImGui::EndMenu();
		}

		if( ImGui::BeginMenu( "Auxiliary" ) )
		{
			ImGui::SeparatorText( "Asset Registry" );
			if( ImGui::MenuItem( "Asset Registry Debug", "" ) )       m_OpenAssetRegistryDebug ^= 1;
			if( ImGui::MenuItem( "Loaded Assets Debug", "" ) )        m_OpenLoadedAssetDebug   ^= 1;
			if( ImGui::MenuItem( "Metadata Debug", "" ) )             m_ShowMetadataDebug      ^= 1;
			if( ImGui::MenuItem( "Asset Dependencies", "" ) )         m_ShowAssetDependencies  ^= 1;

			ImGui::SeparatorText( "Demo Window" );
			if( ImGui::MenuItem( "Show demo window", "" ) )           m_ShowImGuiDemoWindow    ^= 1;

			ImGui::SeparatorText( "Virtual Filesystem (VFS)" );
			if( ImGui::MenuItem( "Virtual Filesystem Debug", "" ) )   m_ShowVFSDebug           ^= 1;

			ImGui::SeparatorText( "Scene Renderer" );
			if( ImGui::MenuItem( "Render Mesh AABB", "" ) )           m_ShowMeshAABB           ^= 1;
			if( ImGui::MenuItem( "Show Camera Frustum", "" ) )        m_ShowCameraFrustum      ^= 1;

			ImGui::SeparatorText( "Content Browser" );
			if( ImGui::MenuItem( "Show Thumbnail Cache", "" ) )       m_ShowCBThumbnailDebug   ^= 1;

			ImGui::SeparatorText( "Undo Redo" );
			if( ImGui::MenuItem( "Show Undo Redo Stack", "" ) )       m_ShowUndoRedoDebug      ^= 1;

			ImGui::EndMenu();
		}

		if( m_RequestRuntime )
		{
			if( ImGui::BeginMenu( "Runtime" ) )
			{
				auto runtimeState = m_RuntimeScene->GetRuntimeState();

				// Play
				{
					Auxiliary::ScopedDisabledFlag disabled( runtimeState != RuntimeState::Suspended );
					if( ImGui::MenuItem( "Play" ) ) m_RuntimeScene->ResumeRuntime();

					if( ImGui::BeginItemTooltip() )
					{
						ImGui::Text( "Can not start a new runtime while the scene is already in runtime." );
#if defined(SAT_DEBUG)
						ImGui::Text( "%s", m_RequestRuntime ? "RUNTIME RUNNING" : "RUNTIME NOT RUNNING" );
#endif
						ImGui::EndTooltip();
					}
				}

				// Stop
				if( ImGui::MenuItem( "Stop" ) ) m_RequestRuntime = false;

				if( ImGui::BeginItemTooltip() )
				{
					ImGui::Text( "Stop the active runtime" );
					ImGui::EndTooltip();
				}

				// Suspend
				{
					Auxiliary::ScopedDisabledFlag disabled( runtimeState == RuntimeState::Suspended );
					if( ImGui::MenuItem( "Suspend" ) ) m_RuntimeScene->SuspendRuntime();

					if( ImGui::BeginItemTooltip() )
					{
						ImGui::Text( "Suspend the runtime and allowing the user to use the Editor Camera" );
						ImGui::EndTooltip();
					}
				}

				ImGui::EndMenu();
			}
		}
	}

	void EditorLayer::DrawAboutWindow()
	{
		if( ImGui::Begin( "About", &m_OpenAboutWindow ) )
		{
			ImGui::Text( "Saturn Engine x64 %s (%s build)", Application::GetCurrentPlatformName(), Application::GetCurrentConfigName() );

			ImGui::Text( "Built on: %s %s (EditorLayer.cpp)", __DATE__, __TIME__ );

			ImGui::Text( "Saturn Engine Version: %s (Internal Number: %i)", SAT_CURRENT_VERSION_STRING, SAT_CURRENT_VERSION );

			ImGui::Separator();

			ImGui::Text( "All icons in the engine are provided by icons8 via https://icons8.com/\nUsing the Tanah Basah set (https://icons8.com/icons/authors/v03BjHji0KTr/tanah-basah)" );

			ImGui::Separator();

			if( Auxiliary::TreeNode( "Third Party libraries" ) )
			{
				ImGui::Text( "dear imgui: %s (%d)", IMGUI_VERSION, IMGUI_VERSION_NUM );
				ImGui::Text( "SPIRV-Cross" );
				ImGui::Text( "Tracy" );
				ImGui::Text( "yaml-cpp" );
				ImGui::Text( "zlib: Version 1.3.1, January 22nd, 2024" );
				ImGui::Text( "PhysX: Version 4.1.1, Copyright NVIDIA Corporation" );
				ImGui::Text( "Recast & Detour" );
				ImGui::Text( "glm" );
				ImGui::Text( "entt" );
				ImGui::Text( "vma" );
				ImGui::Text( "miniaudio" );

				Auxiliary::EndTreeNode();
			}

			ImGui::Separator();

			if( Auxiliary::TreeNode( "Past version numbers" ) )
			{
				ImGui::Text( "Saturn version 0.1.0 (%llu)", SAT_VERSION_A_0_1_0 );
				ImGui::Text( "Saturn version 0.1.1 (%llu)", SAT_VERSION_A_0_1_1 );
				ImGui::Text( "Saturn version 0.1.2 (%llu)", SAT_VERSION_A_0_1_2 );
				ImGui::Text( "Saturn version 0.1.3 (%llu)", SAT_VERSION_A_0_1_3 );
				ImGui::Text( "Saturn version 0.1.4 (%llu)", SAT_VERSION_A_0_1_4 );
				ImGui::Text( "Saturn version 0.2.0 (%llu)", SAT_VERSION_A_0_2_0 );
				ImGui::Text( "Saturn version 0.2.1 (%llu)", SAT_VERSION_A_0_2_1 );
				ImGui::Text( "Saturn version 0.2.2 (%llu)", SAT_VERSION_A_0_2_2 );

				Auxiliary::EndTreeNode();
			}

			ImGui::End();
		}
	}

	void EditorLayer::DrawSceneRendererWindow()
	{
		if( ImGui::Begin( "Scene Renderer", &m_ShowSceneRendererWindow ) )
		{
			m_SceneRenderer->ImGuiRender();

			if( Auxiliary::TreeNode( "Shaders", false ) )
			{
				ImGui::BeginVertical( "shadersV" );

				for( auto& [name, shader] : ShaderLibrary::Get().GetShaders() )
				{
					ImGui::Columns( 2 );
					ImGui::SetColumnWidth( 0, 125.0f );
					ImGui::PushMultiItemsWidths( 2, ImGui::CalcItemWidth() );

					ImGui::BeginHorizontal( name.c_str() );

					ImGui::Text( name.c_str() );

					ImGui::PopItemWidth();

					ImGui::NextColumn();

					if( ImGui::Button( "Recompile" ) )
					{
						if( !shader->TryRecompile() )
						{
							Application::Get().GetWindow()->FlashAttention();

							MessageBoxInfo msgBox = { .Title = "Error", .Text = std::format( "Shader '{0}' failed to recompile. Defaulting back to last successful build.", shader->GetName() ) };
							PushMessageBox( msgBox );
						}
					}

					ImGui::PopItemWidth();

					ImGui::Columns( 1 );

					ImGui::EndHorizontal();
				}

				ImGui::EndVertical();

				Auxiliary::EndTreeNode();
			}
		}

		ImGui::End();
	}

	void EditorLayer::DrawRendererWindow()
	{
		if( ImGui::Begin( "Renderer", &m_ShowRendererWindow ) )
		{
			ImGui::Text( "Frame Time: %.2f ms", Application::Get().Time().Milliseconds() );

			for( const auto& devices : VulkanContext::Get().GetPhysicalDeviceProperties() )
			{
				ImGui::Text( "Device Name: %s", devices.DeviceProps.deviceName );
				ImGui::Text( "API Version: %i", devices.DeviceProps.apiVersion );
				ImGui::Text( "Vendor ID: %i", devices.DeviceProps.vendorID );
				ImGui::Text( "Vulkan Version: 1.2.128" );
			}
		}

		ImGui::End();
	}

	void EditorLayer::DrawMetadataDebug()
	{
		if( ImGui::Begin( "Class Metadata Debug", &m_ShowMetadataDebug ) )
		{
			ImGuiIO& rIO = ImGui::GetIO();

			const auto italicsFont = rIO.Fonts->Fonts[ 2 ];
			ImGui::PushFont( italicsFont );
			ImGui::TextDisabled( "Showing all SClasses" );
			ImGui::PopFont();

			if( ImGui::BeginTable( "##DebugInfoClsM", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody ) )
			{
				ImGui::TableSetupColumn( "Name" );
				ImGui::TableSetupColumn( "Size" );
				ImGui::TableSetupColumn( "Align" );
				ImGui::TableSetupColumn( "Hash" );
				ImGui::TableSetupColumn( "Properties" );
				ImGui::TableSetupColumn( "Path" );

				ImGui::TableHeadersRow();

				{
					ClassMetadataHandler::Get().EachClassNode(
						[&]( const SClass* pClass )
					{
						ImGui::TableNextRow();

						ImGui::TableSetColumnIndex( 0 );
						ImGui::Text( "%s", pClass->GetName().c_str() );

						ImGui::TableSetColumnIndex( 1 );
						ImGui::Text( "%llu", pClass->GetSize() );

						ImGui::TableSetColumnIndex( 2 );
						ImGui::Text( "%llu", pClass->GetAlignment() );

						ImGui::TableSetColumnIndex( 3 );
						ImGui::Text( "%llu", pClass->GetHash() );

						ImGui::TableSetColumnIndex( 4 );
						ImGui::Text( "%i", pClass->GetPropertyCount() );

						ImGui::TableSetColumnIndex( 5 );
						ImGui::Text( "%s", pClass->GetHeaderPath().string().c_str() );
					} );
				}

				ImGui::EndTable();
			}

			ImGui::PushFont( italicsFont );
			ImGui::TextDisabled( "%i SClasses", ClassMetadataHandler::Get().GetNumberOfClasses() );
			ImGui::PopFont();
		}

		ImGui::End();
	}

	void EditorLayer::DrawAssetDependencies() 
	{
		ImGui::SetNextWindowPos( ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2( 0.5f, 0.5f ) );
		if( ImGui::Begin( "Asset Dependencies", &m_ShowAssetDependencies, ImGuiWindowFlags_NoSavedSettings ) )
		{
			if( Auxiliary::TreeNode( "Asset Dependencies (Memory)", false ) )
			{
				for( const auto& [assetID, rDependency] : AssetManager::Get().GetAssetDependencies() )
				{
					if( Auxiliary::TreeNode( std::to_string( assetID ), false ) )
					{
						for( const MemoryAssetDependencyBase* pBase : rDependency )
						{
							ImGui::Text( "ADB/Base" );
							ImGui::Text( "0x%p", ( void* ) pBase );
						}

						Auxiliary::EndTreeNode();
					}
				}

				Auxiliary::EndTreeNode();
			}

			if( Auxiliary::TreeNode( "Asset Dependencies", true ) )
			{
				for( const auto& [assetID, rDependencies] : AssetManager::Get().GetPureAssetDependencies() )
				{
					const Ref<Asset> asset = AssetManager::Get().FindAsset( assetID );
					if( Auxiliary::TreeNode( asset->Name, false ) )
					{
						for( const AssetID id : rDependencies )
						{
							const Ref<Asset> dependency = AssetManager::Get().FindAsset( id );
							if( dependency )
							{
								ImGui::Text( dependency->Name.data() );

								if( ImGui::BeginItemTooltip() )
								{
									ImGui::Text( "Asset" );
									ImGui::Separator();

									ImGui::Text( "%s", dependency->Path.string().c_str() );
									ImGui::Text( "Asset: %llu", dependency->ID );
									ImGui::Text( "Asset Name: %s", dependency->Name.c_str() );
									ImGui::Text( "Asset Version: %llu", dependency->Version );

									ImGui::EndTooltip();
								}
							}
							else
								ImGui::TextColored( ImVec4( 1.0F, 0.0F, 0.0F, 1.0F ), "<NULL>" );
						}

						Auxiliary::EndTreeNode();
					}
				}

				Auxiliary::EndTreeNode();
			}

			ImGui::End();
		}
	}

	void EditorLayer::DrawSceneDirtyPopup()
	{
		ImGui::SetNextWindowPos( ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2( 0.5f, 0.5f ) );
		if( ImGui::BeginPopupModal( "SceneDirtyPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings ) )
		{
			ImGui::Text( "You have unsaved changes to this scene. Would you like to save them?" );

			ImGui::Separator();

			ImGui::BeginHorizontal( "##SCENEDIRTHOZ" );
			
			if( ImGui::Button( "Save" ) ) 
			{
				SaveFile();

				m_ShowSceneDirtyModal = false;
				ImGui::CloseCurrentPopup();

				Application::Get().Close();
			}

			ImGui::Spring();

			if( ImGui::Button( "Close without saving" ) )
			{
				m_ShowSceneDirtyModal = false;
				ImGui::CloseCurrentPopup();

				Application::Get().Close();
			}

			ImGui::Spring();

			if( ImGui::Button( "Cancel" ) )
			{
				m_ShowSceneDirtyModal = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndHorizontal();

			ImGui::EndPopup();
		}

		ImGui::OpenPopup( "SceneDirtyPopup" );
	}

	void EditorLayer::DrawBlockingActionModal()
	{
		// Force the popup to be open
		ImGui::OpenPopup( "Blocking Action" );

		if( ImGui::BeginPopupModal( "Blocking Action", &m_JobModalOpen, ImGuiWindowFlags_NoSavedSettings ) )
		{
			if( m_BlockingOperation->GetTitle().empty() )
				ImGui::Text( "Please wait for the operation to complete..." );
			else
				ImGui::Text( m_BlockingOperation->GetTitle().c_str() );

			ImGui::Separator();

			if( const std::string status = m_BlockingOperation->GetStatus(); !status.empty() )
			{
				ImGui::Text( status.c_str() );
			}

			ImGui::Separator();

			ImGui::BeginHorizontal( "##ItemsH" );

			ImSpinner::SpinnerAng( "##OPERATION_SPINNER", 25.0f / 2.0f, 2.0f, ImSpinner::white, ImSpinner::half_white, 8.6F );

			ImGui::Spring();

			if( const float percent = m_BlockingOperation->GetProgress(); percent >= 1.0f )
			{
				ImGui::ProgressBar( percent / 100.0f );
			}

			ImGui::EndHorizontal();

			if( m_BlockingOperation->Completed() )
			{
				m_JobModalOpen = false;
				m_BlockingOperation->Reset();
			}

			ImGui::EndPopup();
		}
	}

	void EditorLayer::DrawViewport()
	{
		// Viewport Image & Drag and drop handling
		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );

		ImGui::Begin( "Viewport", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove );

		if( m_ViewportSize != ImGui::GetContentRegionAvail() )
		{
			m_ViewportSize = ImGui::GetContentRegionAvail();

			m_SceneRenderer->SetViewportSize( ( uint32_t ) m_ViewportSize.x, ( uint32_t ) m_ViewportSize.y );
			Renderer2D::Get().SetViewportSize( ( uint32_t ) m_ViewportSize.x, ( uint32_t ) m_ViewportSize.y );
			m_EditorCamera.SetViewportSize( ( uint32_t ) m_ViewportSize.x, ( uint32_t ) m_ViewportSize.y );
			m_SuspendedEditorCamera.SetViewportSize( ( uint32_t ) m_ViewportSize.x, ( uint32_t ) m_ViewportSize.y );
		}

		ImGui::PushID( "VIEWPORT_IMAGE" );

		// In the editor we only should flip the image UV, we don't have to flip anything else.
		Auxiliary::Image( m_SceneRenderer->CompositeImage(), m_ViewportSize, { 0, 1 }, { 1, 0 } );

		if( ImGui::BeginDragDropTarget() )
		{
			if( auto payload = ImGui::AcceptDragDropPayload( "CONTENT_BROWSER_ITEM_SCENE" ) )
			{
				const UUID* pUUID = ( const UUID* ) payload->Data;
				OpenFile( *pUUID );
			}

			if( auto payload = ImGui::AcceptDragDropPayload( "CONTENT_BROWSER_ITEM_PREFAB" ) )
			{
				const UUID* pUUID = ( const UUID* ) payload->Data;

				Ref<Asset> asset = AssetManager::Get().FindAsset( *pUUID );
				Ref<Prefab> prefabAsset = AssetManager::Get().GetAssetAs<Prefab>( asset->ID );

				m_EditorScene->CreatePrefab( prefabAsset );
				m_EditorScene->MarkDirty();
			}

			if( auto payload = ImGui::AcceptDragDropPayload( "CONTENT_BROWSER_ITEM_MODEL" ) )
			{
				const UUID* pUUID = ( const UUID* ) payload->Data;

				Ref<Asset> asset = AssetManager::Get().FindAsset( *pUUID );
				Ref<StaticMesh> meshAsset = AssetManager::Get().GetAssetAs<StaticMesh>( asset->ID );

				SharedPtr<Entity> entity = m_EditorScene->CreateEntity( asset->Name );

				auto& rMeshComponent = entity->AddComponent<StaticMeshComponent>();
				rMeshComponent.Mesh = meshAsset;
				rMeshComponent.MaterialRegistry = Ref<MaterialRegistry>::Create( meshAsset );

				m_EditorScene->MarkDirty();
			}

			ImGui::EndDragDropTarget();
		}

		ImGui::PopID();

		// Viewport Gizmo controls on the left
		Viewport_GizmoControl();

		// Viewport Runtime controls on the middle
		Viewport_RTControls();

		// Viewport Runtime settings controls on the right
		Viewport_RTSettings();

		//// Render the real gizmo
		Viewport_DrawGizmo();

		ImGui::PopStyleVar();
		ImGui::End();
	}

	void EditorLayer::Viewport_GizmoControl()
	{
		if( g_ActiveScene->IsRuntimeRunning() )
			return;

		const ImVec2 minBound = ImGui::GetWindowPos();
		const ImVec2 maxBound = { minBound.x + m_ViewportSize.x, minBound.y + m_ViewportSize.y };

		// Viewport Gizmo toolbar
		ImGui::PushID( "VP_GIZMO" );

		constexpr float windowHeight = 32.0f;
		constexpr float icons = 3.0f;
		constexpr float neededSpace = 48.0f * icons - 10.0f;

		// For 4 icons
		//const float windowWidth = 166.0f;

		// For 3 icons
		// Formula is 24 * n - 10.0f (for item spacing)
		// Where n is number of icons
		constexpr float windowWidth = neededSpace - 10.0f;

		ImGui::SetNextWindowPos( ImVec2( minBound.x + 5.0f, minBound.y + 5.0f ) );
		ImGui::SetNextWindowSize( ImVec2( windowWidth, windowHeight ) );
		ImGui::Begin( "ViewportGizmoCrtl##viewport_tools", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings );

		ImGui::BeginVertical( "##v_gizmoV", { windowWidth, ImGui::GetContentRegionAvail().y } );
		ImGui::BeginHorizontal( "##v_gizmoH", { windowWidth, ImGui::GetContentRegionAvail().y } );

		ImGui::PushStyleColor( ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f } );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 5.0f * 2.0f, 0 ) );

		auto showTooltip = []( const char* pText )
		{
			if( ImGui::BeginItemTooltip() ) 
			{
				ImGui::Text( pText );
				ImGui::EndTooltip();
			}
		};

		if( Auxiliary::ImageButton( m_TranslationTexture, { 24.0f, 24.0f } ) ) m_GizmoOperation = ImGuizmo::OPERATION::TRANSLATE;

		showTooltip( "Translate (W)" );

		if( Auxiliary::ImageButton( m_RotationTexture, { 24.0f, 24.0f } ) ) m_GizmoOperation = ImGuizmo::OPERATION::ROTATE;

		showTooltip( "Rotate (E)" );

		if( Auxiliary::ImageButton( m_ScaleTexture, { 24.0f, 24.0f } ) ) m_GizmoOperation = ImGuizmo::OPERATION::SCALE;

		showTooltip( "Scale (R)" );

		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		ImGui::Spring();
		ImGui::EndHorizontal();
		ImGui::Spring();
		ImGui::EndVertical();

		ImGui::End();

		ImGui::PopID();
	}

	void EditorLayer::Viewport_RTControls()
	{
		const ImVec2 minBound = ImGui::GetWindowPos();
		const ImVec2 maxBound = { minBound.x + m_ViewportSize.x, minBound.y + m_ViewportSize.y };

		constexpr float windowHeight = 32.0f;
		const float icons = m_RequestRuntime ? 3.0f : 1.0f; // 3 icons if runtime is running else, one icon (play button)
		const float neededSpace = 48.0f * icons - 10.0f;
		const float windowWidth = neededSpace - 10.0f;

		const float runtimeCenterX = minBound.x + m_ViewportSize.x * 0.5f - windowWidth * 0.5f;

		// Runtime Controls
		ImGui::SetNextWindowPos( ImVec2( runtimeCenterX, minBound.y + 5.0f ) );
		ImGui::SetNextWindowSize( ImVec2( windowWidth, windowHeight ) );

		ImGui::Begin( "ViewportCenterRt##viewport_center_rt", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings );

		ImGui::BeginVertical( "##centerRTv", { windowWidth, ImGui::GetContentRegionAvail().y } );
		ImGui::BeginHorizontal( "##centerRTh", { windowWidth, ImGui::GetContentRegionAvail().y } );

		ImGui::PushStyleColor( ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f } );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 5.0f * 2.0f, 0 ) );

		if( m_RequestRuntime )
			Viewport_RTControls_Running();
		else
			Viewport_RTControls_Default();

		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		ImGui::Spring();
		ImGui::EndHorizontal();
		ImGui::Spring();
		ImGui::EndVertical();

		ImGui::End();
	}

	void EditorLayer::Viewport_RTControls_Default()
	{
		const Ref<Texture2D> texture = m_LastRuntimeAttemptFailed ? m_StartErrorRuntimeTexture : m_StartRuntimeTexture;
		if( Auxiliary::ImageButton( texture, ImVec2( 24.0f, 24.0f ) ) )
		{
			m_RequestRuntime = true;
		}

		if( ImGui::BeginItemTooltip() )
		{
			ImGui::BeginHorizontal( "##centerRTtooltip" );

			ImGui::Text( m_LastRuntimeAttemptFailed ? "Runtime request blocked. No camera was found after BeginPlay was called!" : "Request runtime to start" );
			ImGui::Spring();
#if defined(SAT_DEBUG)
			ImGui::Text( "%s", m_RequestRuntime ? "RUNTIME RUNNING" : "RUNTIME NOT RUNNING" );
			ImGui::Spring();
#endif

			ImGui::EndHorizontal();

			ImGui::EndTooltip();
		}
	}

	void EditorLayer::Viewport_RTControls_Running()
	{
		const auto runtimeState = m_RuntimeScene->GetRuntimeState();

		// Draw play/resume button
		{
			Auxiliary::ScopedDisabledFlag disabled( runtimeState != RuntimeState::Suspended );

			if( Auxiliary::ImageButton( m_StartRuntimeTexture, ImVec2( 24.0f, 24.0f ) ) )
			{
				m_RuntimeScene->ResumeRuntime();

				const std::string title = std::format( "{0} (Running) - Saturn", Project::GetActiveConfig().Name );
				Application::Get().GetWindow()->ChangeTitle( title );
			}

			if( ImGui::BeginItemTooltip() )
			{
				ImGui::Text( "Can not start a new runtime while the scene is already in runtime." );
#if defined(SAT_DEBUG)
				ImGui::Text( "%s", m_RequestRuntime ? "RUNTIME RUNNING" : "RUNTIME NOT RUNNING" );
#endif
				ImGui::EndTooltip();
			}
		}

		// Stop
		if( Auxiliary::ImageButton( m_EndRuntimeTexture, ImVec2( 24.0f, 24.0f ) ) )
		{
			m_RequestRuntime = false;
		}

		if( ImGui::BeginItemTooltip() )
		{
			ImGui::Text( "Stop the active runtime" );
			ImGui::EndTooltip();
		}

		// Suspend
		Auxiliary::ScopedDisabledFlag disabledFlag( runtimeState == RuntimeState::Suspended );

		if( Auxiliary::ImageButton( m_PauseRuntimeTexture, ImVec2( 24.0f, 24.0f ) ) )
		{
			m_RuntimeScene->SuspendRuntime();

			const std::string title = std::format( "{0} (RT Suspended) - Saturn", Project::GetActiveConfig().Name );
			Application::Get().GetWindow()->ChangeTitle( title );
		}

		if( ImGui::BeginItemTooltip() )
		{
			ImGui::Text( "Suspend the runtime and allowing the user to use the Editor Camera" );
			ImGui::EndTooltip();
		}
	}

	void EditorLayer::Viewport_RTSettings()
	{
		// Only show the hot reload settings when no runtime is active
		// So don't even show it while suspended.
		if( g_ActiveScene->IsRuntimeRunning() )
			return;

		const ImVec2 minBound = ImGui::GetWindowPos();
		const ImVec2 maxBound = { minBound.x + m_ViewportSize.x, minBound.y + m_ViewportSize.y };

		constexpr float windowHeight = 32.0f;
		constexpr float icons = 1.0f;
		constexpr float neededSpace = 48.0f * icons - 10.0f;
		constexpr float windowWidth = neededSpace - 10.0f;

		const float runtimeRightX = minBound.x + m_ViewportSize.x - neededSpace - 2.5f;

		// Hot reload Controls
		ImGui::SetNextWindowPos( ImVec2( runtimeRightX, minBound.y + 5.0f ) );
		ImGui::SetNextWindowSize( ImVec2( windowWidth, windowHeight ) );

		ImGui::Begin( "ViewportRightRT##viewport_right_rt", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings );

		ImGui::BeginVertical( "##rightRTv", { windowWidth, ImGui::GetContentRegionAvail().y } );
		ImGui::BeginHorizontal( "##rightRTh", { windowWidth, ImGui::GetContentRegionAvail().y } );

		ImGui::PushStyleColor( ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f } );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 5.0f * 2.0f, 0 ) );

		{
#if !defined(SAT_RELEASE)
		Auxiliary::ScopedDisabledFlag disabledFlag( true );
#endif
		if( Auxiliary::ImageButton( m_SyncTexture, ImVec2( 24.0f, 24.0f ) ) )
			HotReloadGame();
		}

		if( ImGui::BeginItemTooltip() )
		{
#if defined(SAT_RELEASE)
			ImGui::Text( "Hot Reload (Alt+F5)" );
#else
			ImGui::Text( "Hot Reload is only available when using the \"Release\" build configuration (Alt+F5)" );
#endif

			ImGui::EndTooltip();
		}

		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		ImGui::Spring();
		ImGui::EndHorizontal();
		ImGui::Spring();
		ImGui::EndVertical();

		ImGui::End();
	}

	void EditorLayer::Viewport_DrawGizmo()
	{
		const ImVec2 minBound = ImGui::GetWindowPos();
		m_SceneRenderer->SetViewportPosition( minBound.x, minBound.y );

		const ImVec2 maxBound = { minBound.x + m_ViewportSize.x, minBound.y + m_ViewportSize.y };

		m_ViewportFocused = ImGui::IsWindowFocused();
		m_MouseOverViewport = ImGui::IsWindowHovered();
		m_AllowCameraEvents = ImGui::IsMouseHoveringRect( minBound, maxBound ) && m_ViewportFocused || m_StartedRightClickInViewport;
		m_ViewportBounds = ImRect( minBound, maxBound );

		std::vector<SharedPtr<Entity>>& rSelectedEntities = EntitySelectionManager::Get().GetSelectionContexts();

		// Calc center of transform.
		glm::vec3 Positions = {};
		glm::quat Rotations = {};
		glm::vec3 Scales = {};

		for( const auto& rEntity : rSelectedEntities )
		{
			TransformComponent worldSpace = g_ActiveScene->GetWorldSpaceTransform( rEntity );
			Positions += worldSpace.Position;
			Rotations += worldSpace.GetRotation();
			Scales += worldSpace.Scale;
		}

		Positions /= rSelectedEntities.size();
		Rotations /= static_cast< float >( rSelectedEntities.size() );
		Scales /= rSelectedEntities.size();

		glm::mat4 centerPoint = glm::translate( glm::mat4( 1.0f ), Positions ) * glm::toMat4( Rotations ) * glm::scale( glm::mat4( 1.0f ), Scales );
		glm::mat4 offsetTransform( 1.0f );

		///////////////////

		if( rSelectedEntities.size() && m_GizmoOperation != 0 )
		{
			ImGuizmo::SetOrthographic( false );
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect( minBound.x, minBound.y, m_ViewportSize.x, m_ViewportSize.y );

			const glm::mat4 Projection = m_EditorCamera.ProjectionMatrix();
			const glm::mat4 View = m_EditorCamera.ViewMatrix();

			ImGuizmo::Manipulate( glm::value_ptr( View ), glm::value_ptr( Projection ), ( ImGuizmo::OPERATION ) m_GizmoOperation, ImGuizmo::LOCAL, glm::value_ptr( centerPoint ), glm::value_ptr( offsetTransform ) );

			if( ImGuizmo::IsUsing() )
			{
				for( SharedPtr<Entity>& rEntity : rSelectedEntities )
				{
					auto& tc = rEntity->GetComponent<TransformComponent>();

					// Store original transform for undo/redo
					if( !m_WasGizmoUsed )
					{
						m_GizmoOrignalTransforms[ rEntity->GetHandle() ] = tc.GetTransform();
					}

					// Set new transform
					glm::mat4 transform = g_ActiveScene->GetTransformRelativeToParent( rEntity );

					glm::vec3 translation;
					glm::vec3 rotation;
					glm::vec3 scale;
					Maths::DecomposeTransform( transform * offsetTransform, translation, rotation, scale );

					glm::vec3 DeltaRotation = rotation - tc.GetRotationEuler();

					tc.Position = translation;
					tc.SetRotation( tc.GetRotationEuler() += DeltaRotation );
					tc.Scale = scale;

					// Store modified transform for undo/redo
					m_GizmoModifiedTransforms[ rEntity->GetHandle() ] = std::make_tuple( tc.Position, tc.GetRotationEuler(), tc.Scale );
				}

				m_WasGizmoUsed = true;
			}
			else if( m_WasGizmoUsed ) // Stopped using
			{
				m_EditorScene->MarkDirty();

				for( const auto& [handle, transform] : m_GizmoOrignalTransforms )
				{
					const auto& [newPosition, newRotation, newScale] = m_GizmoModifiedTransforms[ handle ];

					SharedPtr<Entity> entity = m_EditorScene->FindEntityByHandle( handle );
					TransformComponent& tc = entity->GetComponent<TransformComponent>();

					glm::mat4 newTransform = glm::translate( glm::mat4( 1.0f ), newPosition )
						* glm::toMat4( glm::quat( newRotation ) )
						* glm::scale( glm::mat4( 1.0f ), newScale );

					Ref<UndoRedoActionModifyTransformation> action = Ref<UndoRedoActionModifyTransformation>::Create( entity, transform, newTransform );
					GlobalUndoRedoGroup::Get().AddAction( action, ( uint64_t ) entity->GetHandle() );

					if( entity->HasComponent<NavigationMeshSpecificationComponent>() )
					{
						SharedPtr<NavBoundsEntity> bounds = entity.As<NavBoundsEntity>();
						bounds->GatherGeometryAndBuild();
					}
				}

				m_GizmoOrignalTransforms.clear();
				m_GizmoModifiedTransforms.clear();

				m_WasGizmoUsed = false;
			}
		}
	}

	void EditorLayer::CloseEditorAndOpenPB()
	{
		SaveFile();
		SaveProject();

		std::filesystem::path SaturnDir = Auxiliary::GetEnvironmentVariableWs( L"SATURN_DIR" );
		std::filesystem::path WorkingDir = SaturnDir / "ProjectBrowser";

		// TODO: Allow for other platforms
#if defined( SAT_DEBUG )
		SaturnDir /= L"bin";
		SaturnDir /= L"Debug-windows-x86_64";
		SaturnDir /= L"ProjectBrowser";
		SaturnDir /= L"ProjectBrowser.exe";
#else
		SaturnDir /= L"bin";
		SaturnDir /= L"Release-windows-x86_64";
		SaturnDir /= L"ProjectBrowser";
		SaturnDir /= L"ProjectBrowser.exe";
#endif
		DeatchedProcess dp( SaturnDir.wstring(), WorkingDir );
		Application::Get().Close();
	}

	bool EditorLayer::OnTitlebarExit()
	{
		/*
		* Disabled until retries.
		if( m_RequestRuntime && m_RuntimeScene )
		{
			MessageBoxInfo msgBox
			{
				.Title = "Stop Runtime?",
				.Text = "Do you want to stop the runtime?",
				.Buttons = MessageBoxButtons_Yes | MessageBoxButtons_No,
				.Type = MessageBoxType::InformationNoIcon
			};

			PushMessageBox( msgBox );
		
			return false;
		}
		*/

		if( m_EditorScene->IsDirty() )
		{
			m_ShowSceneDirtyModal = true;
			ImGui::OpenPopup( "SceneDirtyPopup" );

			Application::Get().GetWindow()->FlashAttention();
		}

		// Otherwise, accept exit request if scene is not dirty.
		return !m_EditorScene->IsDirty();
	}

	void EditorLayer::DrawMessageBox( const MessageBoxInfo& rInfo )
	{
		ImGui::SetNextWindowPos( ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2( 0.5f, 0.5f ) );
		if( ImGui::BeginPopupModal( rInfo.Title.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings ) )
		{
			ImGui::BeginHorizontal( "##MsgBoxH" );

			switch( rInfo.Type )
			{
				// TODO: Create info texture.
				case MessageBoxType::Information:
				case MessageBoxType::Warning:
				{
					Auxiliary::Image( m_ExclamationTexture, ImVec2( 72, 72 ) );
				} break;

				case MessageBoxType::Error:
				{
					Auxiliary::Image( EditorIcons::GetIcon( "Error" ), ImVec2( 72, 72 ) );
				} break;

				case MessageBoxType::InformationNoIcon: break;
			}

			ImGui::Text( rInfo.Text.c_str() );

			ImGui::EndHorizontal();

			ImGui::BeginHorizontal( "##MsgBoxOpts" );
			int buttonIndex = 0;

			if( ( rInfo.Buttons & ( uint32_t )MessageBoxButtons_Ok ) != 0 )
			{
				if( buttonIndex > 0 ) { ImGui::Spring(); buttonIndex++; }

				if( ImGui::Button( "OK" ) )
				{
					ImGui::CloseCurrentPopup();
					PopMessageBox();
				}
			}

			if( ( rInfo.Buttons & ( uint32_t ) MessageBoxButtons_Cancel ) != 0 )
			{
				if( buttonIndex > 0 ) { ImGui::Spring(); buttonIndex++; }

				if( ImGui::Button( "Cancel" ) )
				{
					ImGui::CloseCurrentPopup();
					PopMessageBox();
				}
			}
			
			if( ( rInfo.Buttons & ( uint32_t ) MessageBoxButtons_Exit ) != 0 )
			{
				if( buttonIndex > 0 ) { ImGui::Spring(); buttonIndex++; }

				if( ImGui::Button( "Exit" ) )
				{
					ImGui::CloseCurrentPopup();
					PopMessageBox();
				}
			}

			if( ( rInfo.Buttons & ( uint32_t ) MessageBoxButtons_Yes ) != 0 )
			{
				if( buttonIndex > 0 ) { ImGui::Spring(); buttonIndex++; }

				if( ImGui::Button( "Yes" ) )
				{
					ImGui::CloseCurrentPopup();
					PopMessageBox();
				}
			}

			if( ( rInfo.Buttons & ( uint32_t ) MessageBoxButtons_No ) != 0 )
			{
				if( buttonIndex > 0 ) { ImGui::Spring(); buttonIndex++; }

				if( ImGui::Button( "No" ) )
				{
					ImGui::CloseCurrentPopup();
					PopMessageBox();
				}
			}

			// TODO: Handle retries.
			
			ImGui::EndHorizontal();

			ImGui::EndPopup();
		}

		ImGui::OpenPopup( rInfo.Title.c_str() );
	}

	void EditorLayer::CheckMissingEnv()
	{
		if( !m_HasPremakePath )
		{
			if( ImGui::BeginPopupModal( "Missing Environment Variable", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings ) )
			{
				ImGui::Text( "The environment variable SATURN_PREMAKE_PATH is not set." );
				ImGui::Text( "This is required in order to build projects." );

				ImGui::Separator();

				static std::filesystem::path path = "";

				ImGui::InputText( "##path", ( char* ) path.c_str(), 1024, ImGuiInputTextFlags_ReadOnly );
				ImGui::SameLine();
				if( ImGui::Button( "..." ) )
				{
					path = Application::Get().OpenFile( ".exe\0*.exe;\0" );
				}

				if( !path.empty() )
				{
					if( ImGui::Button( "Close" ) )
					{
						ImGui::CloseCurrentPopup();

						Auxiliary::SetEnvironmentVariable( "SATURN_PREMAKE_PATH", path.string().c_str() );

						m_HasPremakePath = true;
					}
				}

				ImGui::EndPopup();
			}

			ImGui::OpenPopup( "Missing Environment Variable" );
		}
	}

	bool EditorLayer::BuildShaderBundle()
	{
		// Make sure we include the Texture Pass shader.
		// Texture Pass shader is only ever loaded in Dist and we are not on Dist at this point, so load it now.
		Ref<Shader> TexturePass = ShaderLibrary::Get().FindOrLoad( "TexturePass", "content/shaders/TexturePass.glsl" );

		const auto shaderRes = ShaderBundle::BundleShaders();
		const bool built = shaderRes == ShaderBundleResult::Success;

		if( !built )
		{
			MessageBoxInfo msgBox
			{ 
				.Title = "Error", 
				.Text = std::format( "Shader bundle failed to build error was: {0}", ( int ) shaderRes ), 
				.Buttons = MessageBoxButtons_Ok 
			};
			
			PushMessageBox( msgBox );
		}

		Application::Get().GetWindow()->FlashAttention();

		ShaderLibrary::Get().Remove( TexturePass );
		TexturePass = nullptr;
	
		return built;
	}

	bool EditorLayer::ValidateProjectDefaults()
	{
		bool result = true;

		Ref<Project> ActiveProject = Project::GetActiveProject();
		auto& rConfig = ActiveProject->GetConfig();

		const auto& startupScene = rConfig.StartupSceneID;
		const auto defaultMaterialID = ActiveProject->GetDefaultMaterialAsset();
		const auto defaultPhysMaterialID = ActiveProject->GetDefaultPhysicsMaterialAsset();

		if( startupScene == 0 )
		{
			MessageBoxInfo msgBox
			{
				.Title = "Error",
				.Text = "In order to build the project for distribution you must specify a start-up Scene!\nGo to Project->Project Settings, and select a startup scene.",
				.Buttons = MessageBoxButtons_Ok
			};

			PushMessageBox( msgBox );

			result = false;
		}

		if( defaultMaterialID == 0 )
		{
			MessageBoxInfo msgBox
			{
				.Title = "Error",
				.Text = "In order to build the project for distribution you must specify a default Material Asset!\nGo to Project->Project Settings, and select a Material Asset.",
				.Buttons = MessageBoxButtons_Ok
			};

			PushMessageBox( msgBox );

			result = false;
		}

		if( defaultPhysMaterialID == 0 )
		{
			MessageBoxInfo msgBox
			{
				.Title = "Error",
				.Text = "In order to build the project for distribution you must specify a default Physics Material Asset!\nGo to Project->Project Settings, and select a Physics Material Asset.",
				.Buttons = MessageBoxButtons_Ok
			};

			PushMessageBox( msgBox );

			result = false;
		}

		return result;
	}

	void EditorLayer::CreateShaderBundleJob()
	{
		JobSystem::Get().AddJob( [ this ]()
			{
				m_JobModalOpen = true;
				m_BlockingOperation->SetStatus( "Initialising..." );

				SaveFile();
				SaveProject();

				Project::GetActiveProject()->PrepForDist();

				m_BlockingOperation->SetStatus( "Building Shader bundle..." );
				BuildShaderBundle();
			} );
	}

	void EditorLayer::CreateAssetBundleJob()
	{
		JobSystem::Get().AddJob( [ this ]()
			{
				if( const auto result = AssetBundle::BundleAssets( m_BlockingOperation ); result != AssetBundleResult::Success )
				{
					Application::Get().GetWindow()->FlashAttention();

					MessageBoxInfo msgBox
					{
						.Title = "Error",
						.Text = std::format( "Asset bundle failed to build error was: {0}", result ),
						.Buttons = MessageBoxButtons_Ok,
						.Type = MessageBoxType::Error
					};

					PushMessageBox( msgBox );
				}
				else
				{
					MessageBoxInfo msgBox
					{
						.Title = "Asset bundle successfully built",
						.Text = "Asset Bundle successfully built. You may now compile the game in the \"Dist\" configuration.\nYou can do this in your IDE or go to Project->Distribute project in the title bar.",
						.Buttons = MessageBoxButtons_Ok,
						.Type = MessageBoxType::Information
					};

					PushMessageBox( msgBox );
				}
			} );
	}

	void EditorLayer::ShowOrHideContentBrowserPanel()
	{
		m_ImGuiWindowManager->GetPanel<ContentBrowserPanel>()->ShowOrHide();
	}

	void EditorLayer::ShowOrHideSceneHierarchyPanel()
	{
		m_ImGuiWindowManager->GetPanel<SceneHierarchyPanel>()->ShowOrHide();
	}

	glm::vec2 EditorLayer::ConvertMouseToViewportNDC()
	{
		auto [mx, my] = ImGui::GetMousePos();
		const auto& viewportBounds = m_ViewportBounds;

		mx -= m_ViewportBounds.Min.x;
		my -= m_ViewportBounds.Min.y;

		return { ( mx / m_ViewportSize.x ) * 2.0f -1.0f, ( ( my / m_ViewportSize.y ) * 2.0f - 1.0f ) * -1.0f };
	}

	std::pair<glm::vec3, glm::vec3> EditorLayer::RayCast( float mx, float my )
	{
		const glm::vec4 mouseClipPos = { mx, my, -1.0f, 1.0f };

		const auto inverseProj = glm::inverse( m_EditorCamera.ProjectionMatrix() );
		const auto inverseView = glm::inverse( glm::mat3( m_EditorCamera.ViewMatrix() ) );

		const glm::vec4 ray = inverseProj * mouseClipPos;
		const glm::vec3 rayPos = m_EditorCamera.GetPosition();
		const glm::vec3 rayDir = inverseView * glm::vec3( ray );

		return { rayPos, rayDir };
	}

	void EditorLayer::PushMessageBox( MessageBoxInfo& rInfo )
	{
		if( !rInfo.Title.contains( "##MsgBox" ) )
			rInfo.Title = std::format( "{0}##MsgBox", rInfo.Title );

		m_MessageBoxes.push( rInfo );
	}

	void EditorLayer::PopMessageBox()
	{
		m_MessageBoxes.pop();
	}

	void EditorLayer::HandleMessageBoxes()
	{
		auto& rMessageBox = m_MessageBoxes.front();
		DrawMessageBox( rMessageBox );
	}

	void EditorLayer::PushNotification( EditorNotification& rInfo )
	{
		m_Notifications.push_back( rInfo );
	}

	void EditorLayer::PopNotification()
	{
		m_Notifications.pop_back();
	}

	float EditorLayer::DrawSingleNotification( EditorNotification& rInfo, float lastYOffset )
	{
		const auto dt = ImGui::GetIO().DeltaTime;

		// Position bottom-right corner
		const ImGuiViewport* pViewport = ImGui::GetMainViewport();
		const ImVec2 workPos = pViewport->WorkPos;
		const ImVec2 workSize = pViewport->WorkSize;

		// Animate window alpha
		// 0.5f is the duration
		rInfo.AnimationTime += dt;
		const float t = glm::clamp( rInfo.AnimationTime / 0.5f, 0.0f, 1.0f );

		const float easeAlpha = glm::clamp( 1.0f - glm::pow( 1.0f - t, 3.0f ), 0.0f, 1.0f );

		const ImVec2 windowPos = ImVec2( 
			workPos.x + workSize.x, 
			workPos.y + workSize.y - 48.0f - lastYOffset );

		const ImVec2 windowPivot = ImVec2( 1.0f, 1.0f );

		ImGui::SetNextWindowPos( windowPos, ImGuiCond_Always, windowPivot );
		ImGui::PushStyleVar( ImGuiStyleVar_Alpha, easeAlpha );

		const std::string windowID = std::format( "##EDITOR_NOFITICATION/{0}", (uint64_t)rInfo.ID );
		ImGui::Begin( windowID.c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDocking );

		ImGui::BeginHorizontal( rInfo.ID );

		switch( rInfo.NotificationType )
		{
			// TODO: Create info texture.
			case MessageBoxType::Information:
			case MessageBoxType::Warning: 
			{
				Auxiliary::Image( m_ExclamationTexture, ImVec2( 24.0f, 24.0f ) );
			} break;

			case MessageBoxType::Error:
			{
				Auxiliary::Image( EditorIcons::GetIcon( "Error" ), ImVec2( 24.0f, 24.0f ) );
			} break;
		}

		ImGui::Spring();

		ImGui::Text( "%s", rInfo.Text.c_str() );
		ImGui::Spring();
		ImGui::EndHorizontal();

		const float sizeY = ImGui::GetWindowSize().y;
		ImGui::End();
		ImGui::PopStyleVar();

		// Deduct lifetime
		rInfo.Lifetime -= dt;

		// YOffset
		return lastYOffset + sizeY + 10.0f;
	}

	void EditorLayer::DrawNotifications()
	{
		float yOffset = 0.0f;
		for( auto rIt = m_Notifications.begin(); rIt != m_Notifications.end(); )
		{
			auto& rNotification = *rIt;

			yOffset = DrawSingleNotification( rNotification, yOffset );
			
			if( rNotification.Lifetime <= 0.0f )
			{
				rIt = m_Notifications.erase( rIt );
			}
			else
			{
				rIt++;
			}
		}
	}

}
