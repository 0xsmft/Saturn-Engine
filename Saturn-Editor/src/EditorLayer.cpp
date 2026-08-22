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
#include "EditorLayer.h"

#include <Saturn/Project/Project.h>

#include <Saturn/ImGui/ImGuiAuxiliary.h>
#include <Saturn/ImGui/TitleBar.h>
#include <Saturn/ImGui/PrefabViewer.h>
#include <Saturn/ImGui/EditorIcons.h>
#include <Saturn/ImGui/EditorEvents.h>
#include <Saturn/ImGui/ContentBrowserPanel/ContentBrowserThumbnailCache.h>
#include <Saturn/ImGui/UndoRedo/EntityUndoRedoActions.h>
#include <Saturn/ImGui/EditorAboutWindowContents.h>
#include <Saturn/ImGui/RuntimeCommandWindow.h>
#include <Saturn/ImGui/MemoryStatisticsWindow.h>
#include <Saturn/ImGui/ShaderViewerWindow.h>

#include <Saturn/Serialisation/YAML/SceneSerialiser.h>
#include <Saturn/Serialisation/YAML/ProjectSerialiser.h>
#include <Saturn/Serialisation/YAML/EngineSettingsSerialiser.h>
#include <Saturn/Serialisation/YAML/AssetManagerSerialiser.h>
#include <Saturn/Serialisation/YAML/AssetSerialisers.h>
#include <Saturn/Serialisation/AssetBundle.h>
#include <Saturn/Serialisation/EditorShaderBundle.h>

#include <Saturn/Vulkan/SceneRenderer.h>
#include <Saturn/Vulkan/ShaderBundle.h>
#include <Saturn/Vulkan/Renderer2D.h>
#include <Saturn/Vulkan/AluraRenderer.h>

#include <Saturn/Core/Maths.h>
#include <Saturn/Core/EngineSettings.h>
#include <Saturn/Core/Profiler.h>
#include <Saturn/Core/Process.h>
#include <Saturn/Core/VirtualFS.h>
#include <Saturn/Core/AuxiliaryEvents.h>
#include <Saturn/Core/Ruby/RubyWindow.h>
#include <Saturn/Core/Ruby/RubyAuxiliary.h>
#include <Saturn/Core/Renderer/RenderThread.h>
#include <Saturn/Core/EnvironmentVariables.h>
#include <Saturn/Core/Memory/SObjectAllocator.h>
#include <Saturn/Core/AABB/Ray.h>

#include <Saturn/Asset/AssetManager.h>
#include <Saturn/Asset/Prefab.h>

#include <Saturn/GameFramework/Core/GameModule.h>
#include <Saturn/GameFramework/Core/ClassMetadataHandler.h>

#include <Saturn/Audio/AudioSystem.h>
#include <Saturn/Audio/SoundGroup.h>

#include <Saturn/Physics/PhysicsDebugMeshes.h>

#if !defined(JPH_DEBUG_RENDERER)
#define JPH_DEBUG_RENDERER
#include <Saturn/Physics/PhysicsDebugRecorder.h>
#endif

#include <Saturn/AI/Navigation/NavBoundsEntity.h>
#include <Saturn/AI/BehaviourTree/AssetViewer/BehaviourTreeAssetViewer.h>

#include <Saturn/NodeEditor/UI/NodeEditor.h>
#include <Saturn/NodeEditor/SandboxNodeEditor/SandboxNodeEditorViewer.h>
#include <Saturn/NodeEditor/GlobalNodeEditorTaskCache.h>

#include <Saturn/Project/Premake.h>

#include <Saturn/Runtime/RuntimeEvents.h>

#include <Saturn/Alura/AluraCanvas.h>
#include <Saturn/Alura/AluraLayer.h>

#include <Saturn/RuntimeConsole/ConsoleCommandManager.h>

#include <Saturn/Online/OnlineAPI.h>
#if defined(SAT_WITH_STEAM)
#include <Saturn/Online/Steam/SteamOnlineSystemAPI.h>
#endif

#include <ImGuizmo/ImGuizmo.h>

#include <imspinner/imspinner.h>

#include <glm/gtc/type_ptr.hpp>

#include "Editor/TextEditors.h"
#include "Editor/ProjectZipper.h"

namespace Saturn {

	static constexpr inline bool operator==( const ImVec2& lhs, const ImVec2& rhs ) { return lhs.x == rhs.x && lhs.y == rhs.y; }
	static constexpr inline bool operator!=( const ImVec2& lhs, const ImVec2& rhs ) { return !( lhs == rhs ); }

	EditorLayer::EditorLayer()
		: m_EditorCamera( 45.0f, 1280.0f, 720.0f, 0.1f, 1000.0f ),
		m_SuspendedEditorCamera( 45.0f, 1280.0f, 720.0f, 0.1f, 1000.0f ),
		m_EditorScene( Ref<Scene>::Create() )
	{
		Scene::SetActiveScene( m_EditorScene.Get() );

		m_EditorCamera.SetActive( true );

		// Init Physics
		m_PhysicsFoundation.Init();

		// Editor Application should of loaded a project but if not assert.
		SAT_CORE_VERIFY( Project::GetActiveProject(), "No project was given." );

		VirtualFS::Get().MountBase( Project::GetActiveConfig().Name, Project::GetActiveProject()->GetRootDir() );

		m_AssetManager = Ref<AssetManager>::Create();

		Project::GetActiveProject()->CheckMissingAssetRefs();

		SClass::ProcessNewlyLoadedSClasses();

		m_GameModule = FSObjectAllocator::AllocateSObject<GameModule>();

		ClassMetadataHandler::Get().CreateLinkedClassList();
	}

	void EditorLayer::OnAttach()
	{
		// oooo lazily loading such an important system oooo
		AudioSystem::Get();

		m_SelectionManager = std::make_unique<EntitySelectionManager>();
		m_GlobalUndoRedoGroup = Ref<GlobalUndoRedoGroup>::Create();

		ConsoleCommandManager::Get();
		ConsoleCommandManager::Get().RegisterEngineDefaultCommands();

		constexpr TextureLoadFlags DEFAULT_TEXTURE_LOAD_FLAGS_NOT_FLIPPED = TextureLoadFlags_LoadOnMainThread;
		constexpr TextureLoadFlags DEFAULT_TEXTURE_LOAD_FLAGS_FLIPPED = TextureLoadFlags ( ( uint8_t ) TextureLoadFlags_LoadOnMainThread | ( uint8_t ) TextureLoadFlags_FlipVertically );

		m_CheckerboardTexture = Ref< Texture2D >::Create( "content/textures/editor/checkerboard.tga", AddressingMode::Repeat, DEFAULT_TEXTURE_LOAD_FLAGS_FLIPPED );

		m_StartRuntimeTexture = Ref< Texture2D >::Create( "content/textures/editor/Play.png", AddressingMode::ClampToEdge, DEFAULT_TEXTURE_LOAD_FLAGS_FLIPPED );
		m_EndRuntimeTexture = Ref< Texture2D >::Create( "content/textures/editor/Stop.png", AddressingMode::ClampToEdge, DEFAULT_TEXTURE_LOAD_FLAGS_FLIPPED );
		m_PauseRuntimeTexture = Ref< Texture2D >::Create( "content/textures/editor/Pause.png", AddressingMode::ClampToEdge, DEFAULT_TEXTURE_LOAD_FLAGS_FLIPPED );

		m_TranslationTexture = Ref< Texture2D >::Create( "content/textures/editor/Move.png", AddressingMode::ClampToEdge, DEFAULT_TEXTURE_LOAD_FLAGS_FLIPPED );
		m_RotationTexture = Ref< Texture2D >::Create( "content/textures/editor/Rotate.png", AddressingMode::ClampToEdge, DEFAULT_TEXTURE_LOAD_FLAGS_FLIPPED );
		m_ScaleTexture = Ref< Texture2D >::Create( "content/textures/editor/Scale.png", AddressingMode::ClampToEdge, DEFAULT_TEXTURE_LOAD_FLAGS_FLIPPED );
		m_SyncTexture = Ref< Texture2D >::Create( "content/textures/editor/Sync.png", AddressingMode::ClampToEdge, DEFAULT_TEXTURE_LOAD_FLAGS_FLIPPED );
		m_PointLightTexture = Ref< Texture2D >::Create( "content/textures/editor/Billboard_PointLight.png", AddressingMode::ClampToEdge, DEFAULT_TEXTURE_LOAD_FLAGS_NOT_FLIPPED );
		m_ExclamationTexture = Ref< Texture2D >::Create( "content/textures/editor/Exclamation.png", AddressingMode::ClampToEdge, DEFAULT_TEXTURE_LOAD_FLAGS_FLIPPED );
		m_StartErrorRuntimeTexture = Ref< Texture2D >::Create( "content/textures/editor/Play-Error.png", AddressingMode::ClampToEdge, DEFAULT_TEXTURE_LOAD_FLAGS_FLIPPED );

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
		EditorIcons::AddIcon( m_StartErrorRuntimeTexture );

		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/Billboard_Audio.png", AddressingMode::Repeat, DEFAULT_TEXTURE_LOAD_FLAGS_NOT_FLIPPED ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/Billboard_AudioLooping.png", AddressingMode::Repeat, DEFAULT_TEXTURE_LOAD_FLAGS_NOT_FLIPPED ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/Billboard_AudioMuted.png", AddressingMode::Repeat, DEFAULT_TEXTURE_LOAD_FLAGS_NOT_FLIPPED ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/Billboard_AudioListen.png", AddressingMode::Repeat, DEFAULT_TEXTURE_LOAD_FLAGS_NOT_FLIPPED ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/Billboard_Circle.png", AddressingMode::Repeat, DEFAULT_TEXTURE_LOAD_FLAGS_NOT_FLIPPED ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/Inspect.png", AddressingMode::Repeat, DEFAULT_TEXTURE_LOAD_FLAGS_FLIPPED ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/NoIcon.png", AddressingMode::Repeat, DEFAULT_TEXTURE_LOAD_FLAGS_FLIPPED ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/Error.png", AddressingMode::Repeat, DEFAULT_TEXTURE_LOAD_FLAGS_FLIPPED ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/Error_Small.png", AddressingMode::Repeat, DEFAULT_TEXTURE_LOAD_FLAGS_FLIPPED ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/Bin.png", AddressingMode::Repeat, DEFAULT_TEXTURE_LOAD_FLAGS_FLIPPED ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/Exclamation_Small.png", AddressingMode::Repeat, DEFAULT_TEXTURE_LOAD_FLAGS_FLIPPED ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/Information_Small.png", AddressingMode::Repeat, DEFAULT_TEXTURE_LOAD_FLAGS_FLIPPED ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/Settings.png", AddressingMode::Repeat, DEFAULT_TEXTURE_LOAD_FLAGS_FLIPPED ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/EditIcon.png", AddressingMode::Repeat, DEFAULT_TEXTURE_LOAD_FLAGS_FLIPPED ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/Billboard_AIAgent.png", AddressingMode::Repeat, DEFAULT_TEXTURE_LOAD_FLAGS_NOT_FLIPPED ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/FastForward.png", AddressingMode::Repeat, DEFAULT_TEXTURE_LOAD_FLAGS_NOT_FLIPPED ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/NextMultiMedia.png", AddressingMode::Repeat, DEFAULT_TEXTURE_LOAD_FLAGS_NOT_FLIPPED ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/Visible.png", AddressingMode::Repeat, DEFAULT_TEXTURE_LOAD_FLAGS_NOT_FLIPPED ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/Hidden.png", AddressingMode::Repeat, DEFAULT_TEXTURE_LOAD_FLAGS_NOT_FLIPPED ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/SearchFolder.png", AddressingMode::Repeat ) );
		EditorIcons::AddIcon( Ref<Texture2D>::Create( "content/textures/editor/Compile.png", AddressingMode::Repeat ) );

		// Create Panel Manager.
		m_ImGuiWindowManager = Ref<ImGuiWindowManager>::Create();

		Ref<SceneHierarchyPanel> hierarchyPanel = m_ImGuiWindowManager->AddWindow<SceneHierarchyPanel>();
		hierarchyPanel->SetHideFlags( ImGuiHideWindowFlags::Hide );
		hierarchyPanel->SetContext( m_EditorScene );
		hierarchyPanel->OpenWindow();

		Ref<ContentBrowserPanel> contentBrowserPanel = m_ImGuiWindowManager->AddWindow<ContentBrowserPanel>();
		contentBrowserPanel->SetHideFlags( ImGuiHideWindowFlags::Hide );
		contentBrowserPanel->OpenWindow();

		// Setup content browser panel at project dir.
		contentBrowserPanel->ResetPath( Project::GetActiveProject()->GetRootDir() );

		// Command window
		Ref<RuntimeCommandWindow> rtConsoleWindow = m_ImGuiWindowManager->AddWindow<RuntimeCommandWindow>();
		rtConsoleWindow->SetHideFlags( ImGuiHideWindowFlags::Hide );

		// Mem stats window
		Ref<MemoryStatisticsWindow> memStatWindow = m_ImGuiWindowManager->AddWindow<MemoryStatisticsWindow>();
		memStatWindow->SetHideFlags( ImGuiHideWindowFlags::Hide );

		m_TitleBar.AddMenuBarFunction( SAT_BIND_EVENT_FN( DrawTitlebarOptions ) );
		m_TitleBar.AddOnExitFunction( SAT_BIND_EVENT_FN( OnTitlebarExit ) );

		//////////////////////////////////////////////////////////////////////////
		// Load shader bundle (if possible)
		const std::filesystem::path editorShaderBundlePath = Application::Get()->GetAppDataFolder() / "EditorShaderBundle.ssb";

		if( std::filesystem::exists( editorShaderBundlePath ) )
		{
			if( EditorShaderBundle::ReadBundle() )
			{
				SAT_CORE_INFO( "Read engine shader bundle!" );
			}
			else
			{
				SAT_CORE_WARN( "Failed to read editor shader bundle! Engine will compile them." );
			}
		}
		else
			SAT_CORE_WARN( "No editor shader bundle exists! Engine will compile them." );

		//////////////////////////////////////////////////////////////////////////
		// Scene loading and Scene Renderer
		SceneRendererSpecification spec{ 
			.Width = 0u, 
			.Height = 0u, 
			.AOTechnique = AOTechnique::GTAO, 
			.Flags = SceneRendererFlag_MasterInstance,
			.TargetScene = m_EditorScene };

		m_SceneRenderer = Ref<SceneRenderer>::Create( spec );

		// Now open the startup scene
		const auto& rConfig = Project::GetActiveProject()->GetConfig();

		// Try load the editor startup scene, if thats zero load the game startup scene.
		const auto startupID = rConfig.EditorStartupSceneID == 0 ? rConfig.StartupSceneID : rConfig.EditorStartupSceneID;
		OpenFile( startupID );

		// Create camera preview scene renderer
		// NOTE: We have to create a Renderer2D due to us rendering this scene as an Editor Scene, not really ideal.
		spec.Width = spec.Height = 512u;
		spec.TargetScene = m_EditorScene.Get();
		spec.AOTechnique = AOTechnique::None;
		spec.Flags = SceneRendererFlag_NoFlags;
		m_CameraPreviewSceneRenderer = Ref<SceneRenderer>::Create( spec );

		// Force init debug meshes, would be better if we didn't lazy load :(
		// and this kinda beats the whole purpose of this class being lazy loaded.
		PhysicsDebugMeshes::Get();

		// Final step, set title
		const std::string title = std::format( "{0} - Saturn", Project::GetActiveConfig().Name );
		Application::Get()->GetWindow()->ChangeTitle( title );

		// Create online API but not init it yet.
		// Wait until runtime for that.
		m_OnlineAPI = OnlineAPI::CreateOnlineSystemAPI( Project::GetActiveProject()->GetOnlineAPIType() );
	}

	void EditorLayer::OnDetach()
	{
		EditorShaderBundle::TryPackageIfNeeded();

		PhysicsDebugMeshes::Get().Terminate();

		EditorIcons::Clear();
		m_CheckerboardTexture = nullptr;
		m_PointLightTexture = nullptr;
	}

	EditorLayer::~EditorLayer()
	{
		m_ImGuiWindowManager = nullptr;
		m_SandboxNodeEditorViewer = nullptr;

		delete g_AluraCanvas;
		g_AluraCanvas = nullptr;

		m_SceneRenderer->SetCurrentScene( nullptr );
		m_SceneRenderer = nullptr;

		m_CameraPreviewSceneRenderer->SetCurrentScene( nullptr );
		m_CameraPreviewSceneRenderer = nullptr;

		SingletonStorage::RemoveSingleton<EntitySelectionManager>( m_SelectionManager.get() );
		m_SelectionManager.reset();

		if( m_RuntimeScene )
		{
			PopAluraLayerImmediately();
			m_RuntimeScene->OnRuntimeEnd();
			m_RuntimeScene = nullptr;
		}

		m_EditorScene = nullptr;

		// Make sure we clear it before the game module is unloaded because the game may
		// have tasks that it defines, and if the game module is unloaded and we delete it
		// we will crash, because of course the address is invalid.
		GlobalNodeEditorTaskCache::Get().ClearAll();

		// Now terminate the audio system, before the asset man and the GameModule is unloaded.
		AudioSystem::Get().Terminate();

		m_AssetManager = nullptr;

		VirtualFS::Get().UnmountBase( Project::GetActiveConfig().Name );

		ClassMetadataHandler::Get().DestroyAndFreeAllSClasses();

		FSObjectAllocator::DeallocateSObject<GameModule>( m_GameModule );
		m_GameModule = nullptr;
	}

	void EditorLayer::OnUpdate( Timestep time )
	{
		SAT_PF_EVENT();

		if( Input::Get().MouseButtonPressed( RubyMouseButton_Right ) && m_MouseOverViewport ) 
		{
			// If we right click over the viewport but we are not focused, the we focus the window.
			if( !m_ViewportFocused )
			{
				ImGui::FocusWindow( GImGui->HoveredWindow );
			}

			if( !m_StartedRightClickInViewport && m_ViewportFocused )
			{
				m_StartedRightClickInViewport = true;
			}
		}

		if( !Input::Get().MouseButtonPressed( RubyMouseButton_Right ) )
			m_StartedRightClickInViewport = false;

		bool canSetCursorMode = m_MouseOverViewport;
		if( m_RuntimeScene )
		{
			if( m_RuntimeScene->GetRuntimeState() == RuntimeState::Suspended )
			{
				canSetCursorMode = m_AllowCameraEvents;
			}
			else
			{
				canSetCursorMode = !m_AluraLayer->AluraWantsControl();
			}
		}
		else
		{
			canSetCursorMode = m_AllowCameraEvents;
		}

//		const bool canSetCursorMode = m_RuntimeScene == nullptr ? m_AllowCameraEvents : m_RuntimeScene->GetRuntimeState() == RuntimeState::Suspended && !m_AluraLayer->AluraWantsControl() ? m_AllowCameraEvents : m_MouseOverViewport;

		Input::Get().SetCanSetCursorMode( canSetCursorMode );

		///////////////////////////////

		m_AssetManager->Tick( time );

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
			m_SceneRenderer->PreRender();

			m_RuntimeScene->OnUpdate( time );

			// Online subsystem update...
			if( m_OnlineAPI )
				m_OnlineAPI->Tick();

			// Suspended only, paused would be in the control of the user, so we don't switch the
			// camera.
			if( m_RuntimeScene->GetRuntimeState() == RuntimeState::Suspended ) [[unlikely]]
			{
				m_SuspendedEditorCamera.SetActive( m_AllowCameraEvents );
				m_SuspendedEditorCamera.OnUpdate( time );

				m_RuntimeScene->OnRenderEditor( &m_SuspendedEditorCamera, m_SuspendedEditorCamera.ViewMatrix(), m_SceneRenderer, time );
			}
			else [[likely]]
			{
				m_RuntimeScene->OnRenderRuntime( time, m_SceneRenderer );
			}

			if( m_ShowCameraFrustum )
			{
				if( auto entity = m_RuntimeScene->GetMainCameraEntity().Access() )
				{
					const auto& cc = entity->GetComponent<CameraComponent>().Camera;

					auto renderer2D = m_SceneRenderer->GetRenderer2D();
					cc->RenderDebugFrustum( renderer2D );
				}
			}
		}
		else
		{
			// Update camera
			m_EditorCamera.SetActive( m_AllowCameraEvents );
			m_EditorCamera.OnUpdate( time );

			// Update scene
			m_EditorScene->OnUpdate( time );

			m_EditorScene->OnRenderEditor( &m_EditorCamera, m_EditorCamera.ViewMatrix(), m_SceneRenderer, time );

#if SAT_FEATURE_CAMERA_PREVIEW == 1
			if( m_ShouldRenderCameraPreview && m_pSelectedCamera )
			{
				SceneCamera* pSceneCamera = dynamic_cast< SceneCamera* >( m_pSelectedCamera );
				if( pSceneCamera )
				{
					const auto entity = m_EditorScene->FindEntityByHandle( m_SelectedCameraEntityID );

					const auto& rTc = entity->GetComponent<TransformComponent>();

					pSceneCamera->SetPosition( rTc.Position );
					pSceneCamera->SetRotation( rTc.GetRotationEuler() );

					pSceneCamera->SetActive( true );
					pSceneCamera->OnUpdate( time );
					m_EditorScene->OnRenderEditor( pSceneCamera, pSceneCamera->ViewMatrix(), m_CameraPreviewSceneRenderer, time );
				}
			}
#endif

			// Do not accumulate auto save time during playtime.
			if( const auto prj = Project::GetActiveProject(); prj->IsAutoSavesEnabled() && !m_RequestRuntime )
			{
				m_LastAutoSaveTime += time;
			
				if( m_LastAutoSaveTime >= prj->GetAutoSaveInterval() )
				{
					SaveFileAuto();

					m_LastAutoSaveTime = 0.0f;
				}
			}
		}

		if( m_ShowMeshAABB )
		{
			for( const auto& rEntity : m_SelectionManager->GetSelectionContexts( g_ActiveScene ) )
			{
				const glm::mat4 transform = g_ActiveScene->GetTransformRelativeToParent( rEntity );
				if( rEntity->HasComponent<StaticMeshComponent>() )
				{
					const auto& rMesh = rEntity->GetComponent<StaticMeshComponent>().Mesh;
					m_SceneRenderer->GetRenderer2D()->SubmitAABB( rMesh->GetBoundingBox(), transform, { 1.0F, 0.0F, 0.0F, 1.0F } );
				}
				else if( rEntity->HasComponent<SkeletalMeshComponent>() )
				{
					const auto& rMesh = rEntity->GetComponent<SkeletalMeshComponent>().Mesh;
					m_SceneRenderer->GetRenderer2D()->SubmitAABB( rMesh->GetBoundingBox(), transform, { 1.0F, 0.0F, 0.0F, 1.0F } );
				}
			}
		}

		// Render scenes in other asset viewers
		m_ImGuiWindowManager->OnUpdate( time );

		RenderThread::Get().Queue( [ = ]()
		{
			m_SceneRenderer->RenderScene();
#if SAT_FEATURE_CAMERA_PREVIEW == 1
			if( m_ShouldRenderCameraPreview )
				m_CameraPreviewSceneRenderer->RenderScene();
#endif
		} );
	}

	void EditorLayer::OnImGuiRender()
	{
		SAT_PF_EVENT();

		// Draw dockspace.
		ImGui::DockSpaceOverViewport();

		if( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) || ( ImGui::IsMouseClicked( ImGuiMouseButton_Right ) && !m_StartedRightClickInViewport ) )
		{
			if( !m_RuntimeScene )
			{
				ImGui::FocusWindow( GImGui->HoveredWindow );
				Input::Get().SetCursorMode( RubyCursorMode::Normal );
			}
		}

		if( !m_FullscreenViewport )
		{
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
			if( m_JobModalOpen.load() )     DrawBlockingActionModal();
			if( m_ShowDistBuildOptions )    DrawDistOptionsModal();
			if( m_ShowDeleteNavMeshCachePopup ) DrawDeleteNavMeshModal();
			if( m_ShowDebugMsgBoxWindow )   DrawDebugMsgBoxWindow();
			if( m_ShowEditorDebugWindow )   DrawEditorDebugWindow();
			if( m_ShowCBThumbnailDebug )    ContentBrowserThumbnailCache::Get().OnImGuiRender( &m_ShowCBThumbnailDebug );
			if( m_ShowUndoRedoDebug )       m_GlobalUndoRedoGroup->OnImGuiRender( &m_ShowUndoRedoDebug );
			if( m_ShowSetPremakePathModal ) DrawSetPremakePathModal();
			if( m_ShowInvalidRecentProjectPathModal ) DrawInvalidRecentProjectModal();
		}

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

			if( /*( m_MouseOverViewport || m_ViewportFocused ) &&*/ m_RuntimeScene )
			{
				m_RuntimeScene->OnEvent( rEvent );

				if( g_AluraCanvas )
				{
					g_AluraCanvas->HandleDrawerEvents( rEvent );
				}
			}

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

			case EventType::HotReloadRequested:
			{
				if( m_GameModule->HasModule() && !m_RequestRuntime )
				{
					HotReloadGame();
				}
			} break;

			case EventType::HotReloadComplete:
			{
				// Load the new compiled module.
				ClassMetadataHandler::Get().BeginHotReload();
				m_GameModule->BeginHotReload();

				m_EditorScene->AcknowledgeHotReload();

				ClassMetadataHandler::Get().AcknowledgeHotReload();
				m_GameModule->EndHotReloadAndSwap();

				PushNotification( "Hot reload complete" );
			} break;
			
			case EventType::SceneTravel:
			{
				HandleSceneTravel( ( SceneTravelEvent& ) rEvent );
			} break;

			case EventType::CBOpenFile:
			{
				const auto& event = ( CBOpenFileEvent& ) rEvent;
				HandleOpenFileCB( event.GetID() );
			} break;

			case EventType::SkylightEntityModified:
			{
				const SkylightEntityModifiedEvent& rSkylightEvent = ( SkylightEntityModifiedEvent& ) rEvent;
				const auto& rParams = rSkylightEvent.GetParams();

				m_SceneRenderer->SetDynamicSky( rParams.x, rParams.y, rParams.z );

				if( m_CameraPreviewSceneRenderer )
				{
					m_CameraPreviewSceneRenderer->SetDynamicSky( rParams.x, rParams.y, rParams.z );
				}
			} break;

			case EventType::EntitySelected:
			{
				const EntitySelectedEvent& rSelectionEvent = ( EntitySelectedEvent& ) rEvent;
				if( const auto entity = m_EditorScene->FindEntityByID( rSelectionEvent.GetID() ) )
				{
					if( const auto cc = entity->TryGetComponent<CameraComponent>(); cc )
					{
						m_ShouldRenderCameraPreview = true;
						cc->Camera->SetViewportSize( 400u, 225u );

						m_pSelectedCamera = cc->Camera.Get();
						m_SelectedCameraEntityID = entity->GetHandle();
					}
				}
			} break;

			case EventType::EntityDeselected:
			{
				const EntityDeselectedEvent& rSelectionEvent = ( EntityDeselectedEvent& ) rEvent;
				for( const auto& ID : rSelectionEvent.GetIDs() )
				{
					if( const auto entity = m_EditorScene->FindEntityByID( ID ) )
					{
						if( const auto cc = entity->TryGetComponent<CameraComponent>(); cc )
						{
							m_ShouldRenderCameraPreview = false;
							m_pSelectedCamera = nullptr;

							m_SelectedCameraEntityID = entity->GetHandle();
						}
					}
				}
			} break;

			case EventType::RqOpenIDE:
			{
				const RequestOpenIDEEvent& rIDEEvent = ( RequestOpenIDEEvent& ) rEvent;

#if defined(SAT_PLATFORM_WINDOWS) || defined(SAT_PLATFORM_MACOS)
				std::filesystem::path solutionPath = Project::GetActiveProject()->GetRootDir();
				solutionPath /= std::format( "{0}.sln", Project::GetActiveConfig().Name );

				Auxiliary::TextEditors::OpenOptions openOptions{ .TextFilePath = rIDEEvent.GetPath() };
				Auxiliary::TextEditors::OpenVisualStudioLatest( solutionPath, openOptions );
#endif
			} break;

			case EventType::SendEditorNotification:
			{
				const SendEditorNotificationEvent& rNotificationEvent = ( SendEditorNotificationEvent& ) rEvent;

				PushNotification( rNotificationEvent.GetText(), 10.0f );
			} break;

			case EventType::PrefabModified: 
			{
				const OnPrefabModifiedEvent& rPrefabModifiedEvent = ( OnPrefabModifiedEvent& ) rEvent;
				HandlePrefabModification( rPrefabModifiedEvent.GetPrefabID() );
			} break;

			// This will work for now....
			case EventType::RequestRemoveLayerReply:
			{
				m_AluraLayer.reset();
			} break;

			case EventType::NodeEditorDebugBreak:
			{
			} break;

			case EventType::SceneRendererOptionCommandEntered:
			{
				const RuntimeSceneRendererKeCommand& rKeEvent = ( RuntimeSceneRendererKeCommand& ) rEvent;
				m_SceneRenderer->HandleKeCommand( rKeEvent.GetKey(), rKeEvent.GetValue() );
			} break;
		}
	}

	void EditorLayer::SaveFileAs()
	{
		// TODO: Support Saving scene as!
		const auto res = Application::Get()->SaveFile( "Modern Saturn scene file (*.scene)|*.scene" );
		if( !res.empty() )
		{
			SceneSerialiser serialiser( m_EditorScene );
			serialiser.Serialise( res );

			m_EditorScene->SetAbsolutePath( res );
		}
	}

	void EditorLayer::SaveFile()
	{
		const auto fullPath = Project::GetActiveProject()->FilepathAbs( m_EditorScene->Path );
		if( fullPath.has_extension() && fullPath.has_filename() && std::filesystem::exists( fullPath ) )
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

		PushNotification( "AUTO SAVING, PLEASE WAIT", 7.5f );
	}

	void EditorLayer::RevertFile()
	{
		if( m_EditorScene->Path.empty() )
		{
			MessageBoxInfo info{ .Text = "You are attempting to revert to a scene that has no restore point! Please save and try again." };
			PushMessageBox( info );

			return;
		}

		OpenFile( m_EditorScene->ID );
	}

	void EditorLayer::MarkSceneAsStartupFromTitlebar()
	{
		if( m_EditorScene->Path.empty() )
		{
			MessageBoxInfo info{ .Text = "You are attempting a mark a scene as startup when the scene asset hasn't been created yet! Please save and try again." };
			PushMessageBox( info );
		
			return;
		}

		Project::GetActiveConfig().StartupSceneID = m_EditorScene->ID;

		ProjectSerialiser ps;
		ps.Serialise();
	}

	void EditorLayer::OpenFile( AssetID id )
	{
		Ref<SceneHierarchyPanel> hierarchyPanel = m_ImGuiWindowManager->GetPanel<SceneHierarchyPanel>();

		Ref<Scene> newScene = Ref<Scene>::Create();
		g_ActiveScene = newScene.Get();

		m_SelectionManager->ClearSelection( g_ActiveScene, true );
		hierarchyPanel->SetContext( nullptr );

		// Clear old notifications from the old scene.
		m_Notifications.clear();

		// load new scene.....
		const Ref<Asset> asset = id == 0 ? nullptr : m_AssetManager->FindAsset( id );

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

		m_SelectionManager->ClearSelection( g_ActiveScene, true );
		hierarchyPanel->SetContext( nullptr );

		// Let Alura release it's references to the AluraDrawers
		// before the scene cleanup.
		g_AluraCanvas->OnSceneChange();
		m_AluraLayer->RelinquishControl();

		m_RuntimeScene->OnRuntimeEnd();

		const Ref<Asset> asset = m_AssetManager->FindAsset( id );

		SceneSerialiser serialiser( newScene );
		serialiser.Deserialise( asset );

		m_RuntimeScene = newScene;

		m_RuntimeScene->Name = asset->Name;
		m_RuntimeScene->Path = asset->Path;
		m_RuntimeScene->ID = asset->ID;
		m_RuntimeScene->Type = asset->Type;

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

	void EditorLayer::NewFile()
	{
		Ref<SceneHierarchyPanel> hierarchyPanel = m_ImGuiWindowManager->GetPanel<SceneHierarchyPanel>();

		Ref<Scene> newScene = Ref<Scene>::Create();
		g_ActiveScene = newScene.Get();

		m_SelectionManager->ClearSelection( newScene.Get(), true );
		hierarchyPanel->SetContext( newScene );

		// Clear old notifications from the old scene.
		m_Notifications.clear();

		m_SceneRenderer->SetCurrentScene( newScene.Get() );

		m_EditorScene = g_ActiveScene;
	}

	void EditorLayer::SaveProject()
	{
		ProjectSerialiser ps( Project::GetActiveProject() );
		ps.Serialise();

		AssetManagerSerialiser ars;
		ars.Serialise();
	}

	void EditorLayer::ClearAllAutoSaves()
	{
		for( const auto& rEntry : std::filesystem::directory_iterator( Project::GetActiveProject()->GetFullCachePath() ) )
		{
			if( rEntry.is_directory() )
				continue;

			const auto& rPath = rEntry.path();

			if( rPath.has_extension() && rPath.extension() == ".autoscene" )
			{
				std::filesystem::remove( rPath );
				SAT_CORE_INFO( "Cleared auto saved file: {0}", rPath.stem().string() );
			}
		}
	}

	void EditorLayer::ClearAutoSavesForActiveScene()
	{
		// Editor scenes only have autosaves, Runtime scene does not.
		std::regex autosaveRegex( "^" + m_EditorScene->Name + R"(\.(\d+)\.autoscene$)" );
		for( const auto& rEntry : std::filesystem::directory_iterator( Project::GetActiveProject()->GetFullCachePath() ) )
		{
			if( rEntry.is_directory() )
				continue;

			const auto& rPath = rEntry.path();
			const auto& rFilename = rPath.filename().string();
			if( std::regex_match( rFilename, autosaveRegex ) )
			{
				std::filesystem::remove( rPath );
				SAT_CORE_INFO( "Cleared auto saved file: {0}", rPath.stem().string() );
			}
		}
	}

	void EditorLayer::PreInitRuntime()
	{
		m_ImGuiWindowManager->MarkAllWindowsAsReadOnly();
		m_ImGuiWindowManager->OnRuntimeStateChanged( RuntimeState::Starting, RuntimeState::NoState );

		m_RuntimeScene = Ref<Scene>::Create();
		Scene::SetActiveScene( m_RuntimeScene.Get() );

		m_EditorScene->CopyScene( m_RuntimeScene );

		Input::Get().SetCanSetCursorMode( true );

		// Create canvas
		AluraCanvasSpecification canvasSpecification{};
		canvasSpecification.Size = glm::vec2{ m_ViewportSize.x, m_ViewportSize.y };
		canvasSpecification.Position = glm::vec2{ 0.0f };
		canvasSpecification.MasterFontAssetID = Project::GetActiveProject()->GetDefaultFontAsset();

		if( g_AluraCanvas )
			delete g_AluraCanvas;

		g_AluraCanvas = new AluraCanvas( canvasSpecification );
		g_AluraCanvas->SetContext( m_SceneRenderer->GetAluraRenderer() );

		if( m_OnlineAPI )
			m_OnlineAPI->Initialise();

		PushAluraLayer();
	}

	void EditorLayer::PostInitRuntime()
	{
		m_LastRuntimeAttemptFailed = false;

		m_ImGuiWindowManager->GetPanel<SceneHierarchyPanel>()->SetContext( m_RuntimeScene );

		m_SceneRenderer->SetCurrentScene( m_RuntimeScene.Get() );

		m_EditorCamera.SetActive( false );

		const std::string title = std::format( "{0} (Running) - Saturn", Project::GetActiveConfig().Name );
		Application::Get()->GetWindow()->ChangeTitle( title );

		m_ImGuiWindowManager->OnRuntimeStateChanged( RuntimeState::Running, RuntimeState::Starting );
	}

	void EditorLayer::EndRuntime()
	{
		m_ImGuiWindowManager->OnRuntimeStateChanged( RuntimeState::Ending, g_ActiveScene->GetRuntimeState() );

		PopAluraLayer();

		// Destroy canvas now before the scene closes.
		delete g_AluraCanvas;
		g_AluraCanvas = nullptr;

		Ref<SceneHierarchyPanel> hierarchyPanel = m_ImGuiWindowManager->GetPanel<SceneHierarchyPanel>();

		m_RuntimeScene->OnRuntimeEnd();
		Scene::SetActiveScene( m_EditorScene.Get() );

		hierarchyPanel->SetContext( m_EditorScene );

		m_SuspendedEditorCamera.SetActive( false );
		m_RuntimeScene = nullptr;

		m_SceneRenderer->SetCurrentScene( m_EditorScene.Get() );

		if( m_OnlineAPI )
			m_OnlineAPI->Terminate();

		const std::string title = std::format( "{0} - Saturn", Project::GetActiveConfig().Name );
		Application::Get()->GetWindow()->ChangeTitle( title );

		m_ImGuiWindowManager->OnRuntimeStateChanged( RuntimeState::NoState, RuntimeState::Ending );
		m_ImGuiWindowManager->ResetReadOnlyState();
	}

	void EditorLayer::CleanupRuntimeWhenFailed( RuntimeState lastState /*=RuntimeState::Starting*/ )
	{
		// Runtime was rejected, clean up and restore state
		m_ImGuiWindowManager->OnRuntimeStateChanged( RuntimeState::NoState, lastState );

		g_ActiveScene = m_EditorScene.Get();
		m_RuntimeScene = nullptr;

		m_RequestRuntime = false;
		m_LastRuntimeAttemptFailed = true;

		PushNotification( "Runtime request blocked. No camera was found after BeginPlay was called!", 15.0f );
	}

	bool EditorLayer::OnKeyPressed( RubyKeyEvent& rEvent )
	{
		switch( rEvent.GetKeycode() )
		{
			case RubyKey_Delete:
			{
				Ref<SceneHierarchyPanel> schPanel = m_ImGuiWindowManager->GetPanel<SceneHierarchyPanel>();

				const bool windowFocused = m_SelectionManager->GetSelectionReason() & ESR_SceneHierarchyPanel ? schPanel->IsFocused() : ( m_MouseOverViewport || m_ViewportFocused );
				if( !m_RuntimeScene && windowFocused )
				{
					// Because of our ref system, the entity will be deleted when we clear the selections.
					// What we are really doing here is freeing it from the registry and removing the children.
					for( auto& rEntity : m_SelectionManager->GetSelectionContexts( g_ActiveScene ) )
					{
						bool canDeleteNow = true;

						// Special deletion cases:
						//  (a) NavBoundsEntity -> need to show popup to ask if the user want to delete the cache.
						//  (b) Currently selected camera -> invalidate information about the camera.
						if( rEntity->GetClass() == NavBoundsEntity::StaticClass() )
						{
							m_NavMeshEntityToDelete = rEntity->GetHandle();
							m_ShowDeleteNavMeshCachePopup = true;

							canDeleteNow = false;
						}
						// NOT an else if, because what if the user has a camera and a navbounds??
						// Yes, very rare case, but it will still cause a crash if it's not handled like this.
						if( m_SelectedCameraEntityID == rEntity->GetHandle() )
						{
							m_pSelectedCamera = nullptr;
							m_SelectedCameraEntityID = entt::null;
							m_ShouldRenderCameraPreview = false;
						}

						if( canDeleteNow )
						{
							g_ActiveScene->DeleteEntity( rEntity );
						}
					}

					// The entities will be freed here! (if we could delete it in the last pass)
					m_SelectionManager->ClearSelection( g_ActiveScene, true );

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

			case RubyKey_F11:
			{
				if( m_MouseOverViewport || m_ViewportFocused )
				{
					m_PendingFullscreenChange ^= 1;
				}
			} break;
		}

		if( Input::Get().KeyPressed( RubyKey_LeftCtrl ) || Input::Get().KeyPressed( RubyKey_RightCtrl ) && !m_RuntimeScene )
		{
			switch( rEvent.GetKeycode() )
			{
				case RubyKey_D:
				{
					const auto selections = m_SelectionManager->GetSelectionContexts( g_ActiveScene );
					if( selections.empty() )
						break;

					for( const auto& rEntity : selections )
					{
						g_ActiveScene->DuplicateEntity( rEntity );
					}

					g_ActiveScene->MarkDirty();

					if( selections.size() > 1 )
					{
						const std::string text = std::format( "Duplicated {0} entities", selections.size() );
						PushNotification( text, 3.0f );
					}
					else
					{
						PushNotification( "Duplicated 1 entity", 3.0f );
					}
				} break;

				case RubyKey_F:
				{
					auto selectedEntities = m_SelectionManager->GetSelectionContexts( g_ActiveScene );
					if( selectedEntities.size() )
					{
						glm::vec3 Positions = {};
						for( auto& rEntity : selectedEntities )
						{
							TransformComponent worldSpace = g_ActiveScene->GetWorldSpaceTransform( rEntity );
							Positions += worldSpace.Position;
						}

						Positions /= selectedEntities.size();

						m_EditorCamera.Focus( Positions );
					}
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
						PushNotification( undoName, 3.0f );
					}
				} break;

				case RubyKey_Y:
				{
					if( auto action = m_GlobalUndoRedoGroup->GlobalRedoRecent(); action )
					{
						const std::string redoName = std::format( "Redo {0}", action->GetName() );
						PushNotification( redoName, 3.0f );
					}
				} break;

				case RubyKey_W:
				{
					CloseEditorAndOpenPB();
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

					case RubyKey_Z:
					{
						PrepZipProject();
					} break;
				}
			}

			if( Input::Get().KeyPressed( RubyKey_LeftAlt ) && g_ActiveScene != m_RuntimeScene.Get() )
			{
				switch( rEvent.GetKeycode() )
				{
					default: break;

					case RubyKey_F5:
					{
						if( m_GameModule->HasModule() )
							HotReloadGame();
					} break;
				}
			}
		}

		if( Input::Get().KeyPressed( RubyKey_LeftAlt ) && g_ActiveScene != m_RuntimeScene.Get() )
		{
			if( Input::Get().KeyPressed( RubyKey_LeftShift ) )
			{
				switch( rEvent.GetKeycode() )
				{
					default: break;
					case RubyKey_S:
					{
						SaveProject();
					} break;

					case RubyKey_N:
					{
						if( g_ActiveScene->IsDirty() ) 
						{
							m_ShowSceneDirtyModal = true;
							m_EventAfterPopup = [ this ]() { OpenFile( 0 ); };
						}
						else
						{
							OpenFile( 0 );
						}
					} break;
				}
			}
			else
			{
				switch( rEvent.GetKeycode() )
				{
					case RubyKey_F4:
					{
						Application::Get()->Close();
					} break;

					default:
						break;
				}
			}
		}

		return true;
	}

	static bool RayIntersectsBillboard( const glm::vec3& rayOrigin, const glm::vec3& rayDirection, float rayDistance, const glm::vec3& billboardPos, float sideLength )
	{
		const float epsilon = 1e-6f;
		float half_S = sideLength * 0.5f;
		glm::vec3 worldUp( 0.0f, 1.0f, 0.0f );

		glm::vec3 vecToOrigin = rayOrigin - billboardPos;
		float vecLength = glm::length( vecToOrigin );
		if( vecLength < epsilon )
		{
			return false;
		}
		glm::vec3 N = vecToOrigin / vecLength;

		glm::vec3 crossUp = glm::cross( N, worldUp );
		float crossLength = glm::length( crossUp );
		if( crossLength < epsilon )
		{
			crossUp = glm::cross( N, glm::vec3( 1.0f, 0.0f, 0.0f ) );
			crossLength = glm::length( crossUp );
			if( crossLength < epsilon )
			{
				return false;
			}
		}
		glm::vec3 U = crossUp / crossLength;
		glm::vec3 V = glm::cross( N, U );

		float denom = glm::dot( rayDirection, N );
		if( std::fabs( denom ) < epsilon )
		{
			return false;
		}

		float t = glm::dot( billboardPos - rayOrigin, N ) / denom;
		if( t < 0.0f || t > rayDistance )
		{
			return false;
		}

		glm::vec3 h = rayOrigin + t * rayDirection;

		glm::vec3 offset = h - billboardPos;
		float localU = glm::dot( offset, U );
		float localV = glm::dot( offset, V );
		if( std::fabs( localU ) <= half_S && std::fabs( localV ) <= half_S )
		{
			return true;
		}
		return false;
	}
	
	bool EditorLayer::TrySelectEntityFromMouse( Mesh* mesh, SharedPtr<Entity> entity, const glm::vec3& rOrigin, const glm::vec3& rDirection )
	{
		bool hitAny = false;

		auto& rSubmeshes = mesh->Submeshes();
		for( uint32_t i = 0; i < rSubmeshes.size(); i++ )
		{
			const auto& rSubmesh = rSubmeshes[ i ];
			const glm::mat4 transform = g_ActiveScene->GetWorldSpaceTransform( entity ).GetTransform() * rSubmesh.Transform;

			const Ray ray = { .Origin = glm::inverse( transform ) * glm::vec4( rOrigin, 1.0f ), .Direction = glm::inverse( glm::mat3( transform ) ) * rDirection };

			float t;
			const bool hit = ray.IntersectsAABB( rSubmesh.BoundingBox, t );
			if( hit )
			{
				const auto& rIndices = mesh->Indices();
				const auto& rVertices = mesh->Vertices();

				for( const auto& rTri : rIndices )
				{
					const glm::vec3& rV0 = rVertices[ rTri.V1 ].Position;
					const glm::vec3& rV1 = rVertices[ rTri.V2 ].Position;
					const glm::vec3& rV2 = rVertices[ rTri.V3 ].Position;

					float t;
					if( ray.IntersectsTri( rV0, rV1, rV2, t ) )
					{
						hitAny = hit;

						m_SelectionManager->Select( entity );
						m_SelectionManager->SetSelectionReason( ESR_Viewport );

						break;
					}
				}
			}
		}

		return hitAny;
	}

	void EditorLayer::HandlePrefabModification( AssetID prefabID )
	{
		SAT_CORE_ASSERT( !g_ActiveScene->IsRuntimeActive(), "Remind me to check and queue a prefab modification if runtime is active" );

		g_ActiveScene->OnModifyPrefab( prefabID );
	}

	void EditorLayer::PushAluraLayer()
	{
		m_AluraLayer = std::make_shared<AluraLayer>();
		Application::Get()->PushLayer( m_AluraLayer.get() );
	}

	void EditorLayer::PopAluraLayer()
	{
		Application::Get()->DispatchEvent<OnRequestRemoveApplicationLayer>( m_AluraLayer.get() );
	}

	void EditorLayer::PopAluraLayerImmediately()
	{
		Application::Get()->PopLayer( m_AluraLayer.get() );
		m_AluraLayer.reset();
	}

	bool EditorLayer::OnMousePressed( RubyMouseEvent& rEvent )
	{
		if( 
			( m_RuntimeScene && m_RuntimeScene->IsRuntimeRunning() ) || 
			!m_MouseOverViewport || 
			rEvent.GetButton() != RubyMouseButton_Left || 
			ImGuizmo::IsOver() )
			return false;

		const auto viewportMouse = ConvertMouseToViewportNDC();
		if( viewportMouse.x > -1.0f && viewportMouse.x < 1.0f && viewportMouse.y > -1.0f && viewportMouse.y < 1.0f )
		{
			bool hitAny = false;
			const auto [origin, dir] = RayCast( viewportMouse.x, viewportMouse.y );

			const auto staticMeshes = g_ActiveScene->GetAllEntitiesWith<StaticMeshComponent>();
			for( const auto& rEntity : staticMeshes )
			{
				const auto& comp = rEntity->GetComponent<StaticMeshComponent>();
				if( !comp.Mesh )
					continue;

				hitAny |= TrySelectEntityFromMouse( ( Mesh* ) comp.Mesh.Get(), rEntity, origin, dir );
			}

			const auto skMeshes = g_ActiveScene->GetAllEntitiesWith<SkeletalMeshComponent>();
			for( const auto& rEntity : skMeshes )
			{
				const auto& comp = rEntity->GetComponent<SkeletalMeshComponent>();
				if( !comp.Mesh )
					continue;

				hitAny |= TrySelectEntityFromMouse( ( Mesh* ) comp.Mesh.Get(), rEntity, origin, dir );
			}

			const auto billboards = g_ActiveScene->GetAllEntitiesWith<BillboardComponent>();
			for( const auto& rEntity : billboards )
			{
				const TransformComponent& rTc = g_ActiveScene->GetWorldSpaceTransform( rEntity );

				// TODO: Not sure if we should the transforms scale for sideLength arg
				//	     because what if the mesh is scaled to 1024, but the billboard is still 1x1 (which it will)
				//		 so for now we can just always use the 1x1 billboard and nothing else.
				//		 So, until we have some sort of scaling that is independent from the transform, 
				//		 1x1 will have to be used.
				//																						  |
				//																						  |
				//																						  v
				if( RayIntersectsBillboard( origin, dir, std::numeric_limits<float>::max(), rTc.Position, 1.0f ) )
				{
					hitAny |= true;
					m_SelectionManager->Select( rEntity );
					m_SelectionManager->SetSelectionReason( ESR_Viewport );
				}
			}

			if( !hitAny && m_SelectionManager->GetSelectionCount( g_ActiveScene ) )
			{
				m_SelectionManager->ClearSelection( g_ActiveScene );
			}
		}

		return false;
	}

	void EditorLayer::HandleSceneTravel( SceneTravelEvent& rEvent )
	{
		const AssetID destinationID = rEvent.GetID();
		const Ref<Asset> sceneAsset = m_AssetManager->FindAsset( destinationID );

		if( !sceneAsset )
		{
			SAT_CORE_ERROR( "Failed to travel as ASSET/{0} is not a valid scene ID!", destinationID );
			return;
		}

		OpenFileInRuntime( destinationID );
	}

	void EditorLayer::HandleOpenFileCB( UUID newSceneID )
	{
		if( g_ActiveScene->GetRuntimeState() == RuntimeState::Running || g_ActiveScene->GetRuntimeState() == RuntimeState::Suspended )
		{
			EndRuntime();

			// Because the Runtime Scene self ended runtime, we must reset the mouse because normally we wouldn't
			// as to end runtime via the Editor requires the mouse to be in an unlocked state.
			// TODO: A better way to handle runtime, would be to create a OnRuntimeStart, OnRuntimeSuspend, OnRuntimeEnd events
			Input::Get().SetCursorMode( RubyCursorMode::Normal, true );
			m_RequestRuntime = false;
		}

		if( m_EditorScene->IsDirty() )
		{
			m_ShowSceneDirtyModal = true;
			m_EventAfterPopup = [ this, copyID = newSceneID ]() { OpenFile( copyID ); };
		}
		else
		{
			OpenFile( newSceneID );
		}
	}

	static bool s_OpenAssetFinderPopup = false;

	void EditorLayer::DrawProjectSettingsWindow()
	{
		static bool shouldSaveProject = false;

		ImGuiIO& rIO = ImGui::GetIO();

		ImGui::SetNextWindowPos( ImVec2( rIO.DisplaySize.x * 0.5f - 150.0f, rIO.DisplaySize.y * 0.5f - 150.0f ), ImGuiCond_Once );
		if( ImGui::Begin( "Project settings", &m_ShowUserSettings ) )
		{
			auto& userSettings = EngineSettings::Get();
			Ref<Project> ActiveProject = Project::GetActiveProject();

			auto& rConfig = ActiveProject->GetConfig();
			auto& startupSceneID = rConfig.StartupSceneID;
			Ref<Asset> startupSceneAsset = m_AssetManager->FindAsset( startupSceneID );

			const auto boldFont = rIO.Fonts->Fonts[ 1 ];
			ImGui::PushFont( boldFont );
			ImGui::Text( "Project Information" );
			ImGui::Separator();
			ImGui::PopFont();

			ImGui::BeginHorizontal( "##prj_infoname" );
			ImGui::Text( "Project Name: %s", rConfig.Name.c_str() );
			ImGui::EndHorizontal();

			ImGui::BeginHorizontal( "##prj_infover" );
			{
				ImGui::Text( "Project Version:" );

				ImGui::Spring();

				std::string temporaryVerStr = ActiveProject->GetDeveloperVersion();
				if( Auxiliary::InputText( "##prjdevver", &temporaryVerStr ) )
				{
					ActiveProject->SetDeveloperVersion( temporaryVerStr );
					shouldSaveProject = true;
				}
			}
			ImGui::EndHorizontal();

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
					shouldSaveProject = true;
				}

				inspectDisabledFlag.Pop();

				{
					Auxiliary::ScopedDisabledFlag disabledFlag( rConfig.StartupSceneID == 0 && !m_RequestRuntime );

					if( Auxiliary::ImageButton( EditorIcons::GetIcon( "SearchFolder" ), { 24.0f, 24.0f } ) )
					{
						Ref<Asset> target = m_AssetManager->FindAsset( rConfig.StartupSceneID );

						if( target )
						{
							Ref<ContentBrowserPanel> contentBrowserPanel = m_ImGuiWindowManager->GetPanel<ContentBrowserPanel>();
							contentBrowserPanel->BrowseToItem( target->Path, rConfig.StartupSceneID );
						}
					}

					if( ImGui::BeginItemTooltip() )
					{
						ImGui::Text( "Find in Content Browser" );
						ImGui::EndTooltip();
					}
				}
			}
			ImGui::EndHorizontal();

			ImGui::BeginHorizontal( "##prj_edstrtscene" );
			{
				auto edStartupScene = rConfig.EditorStartupSceneID;
				Ref<Asset> edStartupSceneAsset = m_AssetManager->FindAsset( edStartupScene );

				ImGui::Text( "Editor Startup Scene:" );
				edStartupScene == 0 ? ImGui::TextColored( ImVec4( 1.0f, 0.0f, 0.0f, 1.0f ), "None" ) : ImGui::Text( edStartupSceneAsset->Name.c_str() );

				ImGui::Spring();

				Auxiliary::DisabledFlag inspectDisabledFlag( m_RequestRuntime );

				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), { 24.0f, 24.0f } ) )
					s_OpenAssetFinderPopup = true;

				if( Auxiliary::DrawAssetFinder( AssetType::Scene, &s_OpenAssetFinderPopup, rConfig.EditorStartupSceneID ) )
				{
					shouldSaveProject = true;
				}

				inspectDisabledFlag.Pop();

				{
					Auxiliary::ScopedDisabledFlag disabledFlag( rConfig.EditorStartupSceneID == 0 && !m_RequestRuntime );

					if( Auxiliary::ImageButton( EditorIcons::GetIcon( "SearchFolder" ), { 24.0f, 24.0f } ) )
					{
						Ref<Asset> target = m_AssetManager->FindAsset( rConfig.EditorStartupSceneID );

						if( target )
						{
							Ref<ContentBrowserPanel> contentBrowserPanel = m_ImGuiWindowManager->GetPanel<ContentBrowserPanel>();
							contentBrowserPanel->BrowseToItem( target->Path, rConfig.EditorStartupSceneID );
						}
					}

					if( ImGui::BeginItemTooltip() )
					{
						ImGui::Text( "Find in Content Browser" );
						ImGui::EndTooltip();
					}
				}
			}
			ImGui::EndHorizontal();

			ImGui::BeginVertical( "##prj_defaults" );

			ImGui::BeginHorizontal( "##prj_defmatasset" );
			{
				auto defaultMaterialID = ActiveProject->GetDefaultMaterialAsset();

				ImGui::Text( "Default Material Asset:" );
				defaultMaterialID == 0 ? ImGui::TextColored( ImVec4( 1.0f, 0.0f, 0.0f, 1.0f ), "None" ) : ImGui::Text( "%" PRIu64, defaultMaterialID );

				ImGui::Spring();

				Auxiliary::DisabledFlag inspectDisabledFlag( m_RequestRuntime );

				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), { 24.0f, 24.0f } ) )
					s_OpenAssetFinderPopup = true;

				if( Auxiliary::DrawAssetFinder( AssetType::Material, &s_OpenAssetFinderPopup, defaultMaterialID ) )
				{
					ActiveProject->SetDefaultMaterialAsset( defaultMaterialID );
					shouldSaveProject = true;
				}

				inspectDisabledFlag.Pop();

				{
					Auxiliary::ScopedDisabledFlag disabledFlag( defaultMaterialID == 0 );

					if( Auxiliary::ImageButton( EditorIcons::GetIcon( "SearchFolder" ), { 24.0f, 24.0f } ) )
					{
						const Ref<Asset> target = m_AssetManager->FindAsset( defaultMaterialID );

						if( target )
						{
							Ref<ContentBrowserPanel> contentBrowserPanel = m_ImGuiWindowManager->GetPanel<ContentBrowserPanel>();
							contentBrowserPanel->BrowseToItem( target->Path, defaultMaterialID );
						}
					}

					if( ImGui::BeginItemTooltip() )
					{
						ImGui::Text( "Find in Content Browser" );
						ImGui::EndTooltip();
					}
				}
			}
			ImGui::EndHorizontal();

			ImGui::BeginHorizontal( "##prj_defphysmatasset" );
			{
				auto defaultMaterialID = ActiveProject->GetDefaultPhysicsMaterialAsset();

				ImGui::Text( "Default Physics Material Asset:" );
				defaultMaterialID == 0 ? ImGui::TextColored( ImVec4( 1.0f, 0.0f, 0.0f, 1.0f ), "None" ) : ImGui::Text( "%" PRIu64, defaultMaterialID );

				ImGui::Spring();

				Auxiliary::DisabledFlag inspectDisabledFlag( m_RequestRuntime );

				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), { 24.0f, 24.0f } ) )
					s_OpenAssetFinderPopup = true;

				if( Auxiliary::DrawAssetFinder( AssetType::PhysicsMaterial, &s_OpenAssetFinderPopup, defaultMaterialID ) )
				{
					ActiveProject->SetDefaultPhysicsMaterialAsset( defaultMaterialID );
					shouldSaveProject = true;
				}

				inspectDisabledFlag.Pop();

				{
					Auxiliary::ScopedDisabledFlag disabledFlag( defaultMaterialID == 0 );

					if( Auxiliary::ImageButton( EditorIcons::GetIcon( "SearchFolder" ), { 24.0f, 24.0f } ) )
					{
						const Ref<Asset> target = m_AssetManager->FindAsset( defaultMaterialID );

						if( target )
						{
							Ref<ContentBrowserPanel> contentBrowserPanel = m_ImGuiWindowManager->GetPanel<ContentBrowserPanel>();
							contentBrowserPanel->BrowseToItem( target->Path, defaultMaterialID );
						}
					}

					if( ImGui::BeginItemTooltip() )
					{
						ImGui::Text( "Find in Content Browser" );
						ImGui::EndTooltip();
					}
				}
			}
			ImGui::EndHorizontal();

			ImGui::BeginHorizontal( "##prj_defalurafont" );
			{
				auto defaultFontAsset = ActiveProject->GetDefaultFontAsset();

				ImGui::Text( "Default Font Asset:" );
				defaultFontAsset == 0 ? ImGui::TextColored( ImVec4( 1.0f, 0.0f, 0.0f, 1.0f ), "None" ) : ImGui::Text( "%" PRIu64, defaultFontAsset );

				ImGui::Spring();

				Auxiliary::DisabledFlag inspectDisabledFlag( m_RequestRuntime );

				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), { 24.0f, 24.0f } ) )
					s_OpenAssetFinderPopup = true;

				if( Auxiliary::DrawAssetFinder( AssetType::Font, &s_OpenAssetFinderPopup, defaultFontAsset ) )
				{
					ActiveProject->SetDefaultFontAsset( defaultFontAsset );
					shouldSaveProject = true;
				}

				inspectDisabledFlag.Pop();

				{
					Auxiliary::ScopedDisabledFlag disabledFlag( defaultFontAsset == 0 );

					if( Auxiliary::ImageButton( EditorIcons::GetIcon( "SearchFolder" ), { 24.0f, 24.0f } ) )
					{
						const Ref<Asset> target = m_AssetManager->FindAsset( defaultFontAsset );

						if( target )
						{
							Ref<ContentBrowserPanel> contentBrowserPanel = m_ImGuiWindowManager->GetPanel<ContentBrowserPanel>();
							contentBrowserPanel->BrowseToItem( target->Path, defaultFontAsset );
						}
					}

					if( ImGui::BeginItemTooltip() )
					{
						ImGui::Text( "Find in Content Browser" );
						ImGui::EndTooltip();
					}
				}
			}
			ImGui::EndHorizontal();

			ImGui::BeginHorizontal( "##prj_defphysreg" );
			{
				auto defaultPhysReg = ActiveProject->GetDefaultPhysRegAsset();

				ImGui::Text( "Default Physics Surface Registry Asset:" );
				defaultPhysReg == 0 ? ImGui::TextColored( ImVec4( 1.0f, 0.0f, 0.0f, 1.0f ), "None" ) : ImGui::Text( "%" PRIu64, defaultPhysReg );

				ImGui::Spring();

				Auxiliary::DisabledFlag inspectDisabledFlag( m_RequestRuntime );

				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), { 24.0f, 24.0f } ) )
					s_OpenAssetFinderPopup = true;

				if( Auxiliary::DrawAssetFinder( AssetType::PhysSurfaceRegistry, &s_OpenAssetFinderPopup, defaultPhysReg ) )
				{
					ActiveProject->SetDefaultPhysRegAsset( defaultPhysReg );
					shouldSaveProject = true;
				}

				inspectDisabledFlag.Pop();

				{
					Auxiliary::ScopedDisabledFlag disabledFlag( defaultPhysReg == 0 );

					if( Auxiliary::ImageButton( EditorIcons::GetIcon( "SearchFolder" ), { 24.0f, 24.0f } ) )
					{
						const Ref<Asset> target = m_AssetManager->FindAsset( defaultPhysReg );

						if( target )
						{
							Ref<ContentBrowserPanel> contentBrowserPanel = m_ImGuiWindowManager->GetPanel<ContentBrowserPanel>();
							contentBrowserPanel->BrowseToItem( target->Path, defaultPhysReg );
						}
					}

					if( ImGui::BeginItemTooltip() )
					{
						ImGui::Text( "Find in Content Browser" );
						ImGui::EndTooltip();
					}
				}
			}
			ImGui::EndHorizontal();

			ImGui::PushFont( boldFont );
			ImGui::Text( "Autosaves" );
			ImGui::Separator();
			ImGui::PopFont();

			//			ImGui::BeginHorizontal( "##prj_autosaves" );
			{
				bool enableAutoSaves = ActiveProject->IsAutoSavesEnabled();

				if( Auxiliary::DrawBoolControl( "Enable Auto Saves", enableAutoSaves ) )
				{
					ActiveProject->EnableAutoSaves( enableAutoSaves );
					shouldSaveProject = true;
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

					shouldSaveProject = true;
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
			ImGui::Text( "Asset Bundle" );
			ImGui::Separator();
			ImGui::PopFont();

			{
				auto abCompressionThreshold = ActiveProject->GetCompressionThresholdForAssetBundle();
				if( Auxiliary::DrawUInt64Control( "Compression Threshold (KiB)", abCompressionThreshold, 0, 10240, 210.0f ) )
				{
					ActiveProject->SetCompressionThresholdForAssetBundle( abCompressionThreshold );
				}

				if( ImGui::BeginItemTooltip() )
				{
					ImGui::Text( "Any file above %" PRIu64 "KiB will get compressed, anything below will not.", abCompressionThreshold );
					ImGui::EndTooltip();
				}
			}

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
					for( uint16_t i = 0; i < RubyKey_EnumSize; ++i )
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

							shouldSaveProject = true;
						}

						if( IsSelected )
							ImGui::SetItemDefaultFocus();

						ImGui::PopID();
					}

					for( int i = 0; i < 5; ++i )
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

							shouldSaveProject = true;
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
					shouldSaveProject = true;
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
						++count;
				}

				if( count >= 1 )
				{
					ab.Name += " ";
					ab.Name += std::to_string( count );
				}

				ActiveProject->AddActionBinding( ab );
				shouldSaveProject = true;
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

				// Disable removing a sound group if we are in runtime
				Auxiliary::DisabledFlag disabledFlag( m_RequestRuntime );

				if( ImGui::SmallButton( "-" ) )
				{
					rIt = ActiveProject->GetSoundGroups().erase( rIt );
					shouldSaveProject = true;
				}
				else
				{
					++rIt;
				}

				disabledFlag.Pop();

				ImGui::EndHorizontal();
			}

			// We have to push a new one because we still want developers to be able to modify the volume of a sound group but not adding/removing it.
			Auxiliary::DisabledFlag disabledFlagIfRuntimeForSndGrps( m_RequestRuntime );
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
						++count;
				}

				if( count >= 1 )
				{
					std::string newName = std::format( "{0} ({1})", group->GetName(), std::to_string( count ) );
					group->SetName( newName );
				}

				ActiveProject->AddSoundGroup( group );
				shouldSaveProject = true;
			}

			ImGui::PopID();
			disabledFlagIfRuntimeForSndGrps.Pop();

			ImGui::PushFont( boldFont );
			ImGui::Text( "Online" );
			ImGui::Separator();
			ImGui::PopFont();

			ImGui::BeginHorizontal( "##selectosystem" );

			ImGui::Text( "Online System SDK:" );
			ImGui::Spring();

			OnlineSystemAPIType selectedType = ActiveProject->GetOnlineAPIType();

			std::string sdkDisplayName = "Null";
			switch( selectedType )
			{
				case OnlineSystemAPIType::Steam:
				{
#if defined(SAT_WITH_STEAM)
					sdkDisplayName = "Steam";
#else
					sdkDisplayName = "Steam (SAT_WITH_STEAM define missing!)";
#endif
				} break;

				case OnlineSystemAPIType::Epic:
				{
#if defined(SAT_WITH_EPIC)
					sdkDisplayName = "Epic";
#else
					sdkDisplayName = "Epic (SAT_WITH_EPIC define missing!)";
#endif
				} break;

				case OnlineSystemAPIType::Null:
				default:
					break;
			}

			Auxiliary::DisabledFlag onlineSystemDisabledIf( m_RequestRuntime );
			
			const auto& rStyle = ImGui::GetStyle();
			const float comboWidth = glm::max(
				200.0f,
				ImGui::CalcTextSize( sdkDisplayName.c_str() ).x +
				rStyle.FramePadding.x * 2.0f +
				ImGui::GetFrameHeight() );

			ImGui::SetNextItemWidth( comboWidth );
			if( ImGui::BeginCombo( "##osystem", sdkDisplayName.data() ) )
			{
				bool selected = ( selectedType == OnlineSystemAPIType::Null );
				if( ImGui::Selectable( "None", selected ) )
				{
					ActiveProject->SetOnlineSystemAPI( OnlineSystemAPIType::Null );

					// If we weren't selected before this, we need to re-create the system.
					if( !selected )
					{
						m_OnlineAPI = OnlineAPI::CreateOnlineSystemAPI( OnlineSystemAPIType::Null );
						shouldSaveProject = true;
					}
				}

#if defined(SAT_WITH_STEAM)
				selected = ( selectedType == OnlineSystemAPIType::Steam );
				if( ImGui::Selectable( "Steam", selected ) )
				{
					ActiveProject->SetOnlineSystemAPI( OnlineSystemAPIType::Steam );

					// If we weren't selected before this, we need to re-create the system.
					if( !selected )
					{
						m_OnlineAPI = OnlineAPI::CreateOnlineSystemAPI( OnlineSystemAPIType::Steam );
						shouldSaveProject = true;
					}
				}
#endif

#if defined(SAT_WITH_EPIC)
				selected = ( selectedType == OnlineSystemAPIType::Epic );
				if( ImGui::Selectable( "Steam", selected ) )
				{
					ActiveProject->SetOnlineSystemAPI( OnlineSystemAPIType::Epic );

					// If we weren't selected before this, we need to re-create the system.
					if( !selected )
					{
						m_OnlineAPI = OnlineAPI::CreateOnlineSystemAPI( OnlineSystemAPIType::Epic );
						shouldSaveProject = true;
					}
				}
#endif
				ImGui::EndCombo();
			}

			ImGui::EndHorizontal();

			// Call to GetOnlineAPIType() is needed if the type has changed.
			switch( ActiveProject->GetOnlineAPIType() )
			{
				default:
				case OnlineSystemAPIType::Null:
					break;

#if defined(SAT_WITH_STEAM)
				case Saturn::OnlineSystemAPIType::Steam:
				{
					ImGui::Text( "Steam Settings" );
					ImGui::Separator();
				
					ImGui::BeginHorizontal( "##steamaihz" );

					ImGui::Text( "Steam App ID" );

					ImGui::Spring();

					uint32_t id = ActiveProject->GetOnlineAppID();
					if( ImGui::InputScalarN( "##SteamAppID", ImGuiDataType_U32, ( void* ) &id, 1 ) ) 
					{
						ActiveProject->SetOnlineAppID( id );

						shouldSaveProject = true;
					}

					ImGui::EndHorizontal();

					if( id == 480 )
					{
						ImGui::TextDisabled( "Note: Your application will NOT ship with an ID of 480. You must have your game approved by Valve before shipping!" );
					}
					else
					{
						ImGui::TextDisabled( "Note: If you don't have an application on Steamworks, you may use steam ID 480, which is Spacewar, a shared testing application for developers." );
					}

					// I assume that the following text below is true?
					ImGui::TextDisabled( "It is against Steam's TOS to use any random appid that is not your own and attempt to ship the application! Or to spoof an existing ID!" );
				} break;
#endif

#if defined(SAT_WITH_EPIC)
				case Saturn::OnlineSystemAPIType::Epic:
					break;	
#endif
			}

			onlineSystemDisabledIf.Pop();

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
					drawRow( "Temporary Path", ActiveProject->GetTempDir().string() );
					drawRow( "Binary Path", ActiveProject->GetBinDir().string() );
					drawRow( "Cache Path", ActiveProject->GetFullCachePath().string() );

					// The game module may not actually contain a loaded module if this project has ever built for Distribution or it does not contain any C++ classes.
					if( m_GameModule->HasModule() )
					{
						drawRow( "Module Path", m_GameModule->GetModulePath().string() );
						drawRow( "Module Timestamp", std::format( "X{0}", m_GameModule->GetTimestamp() ) );
					}
					else
					{
						drawRow( "Module Path", "<NULL>" );
						drawRow( "Module Timestamp", "X0000000000" );
					}
				}

				ImGui::EndTable();
			}
#endif
		}

		ImGui::End();

		// Only save project if the window has been closed.
		if( shouldSaveProject && !m_ShowUserSettings )
		{
			ProjectSerialiser ps;
			ps.Serialise();

			shouldSaveProject = false;
		}
	}

	void EditorLayer::ProjectSettings_DrawSoundGroupEdit( Ref<SoundGroup>& rSoundGroup )
	{
		char buffer[ 256 ];
		std::memset( buffer, 0, 256 );
		std::memcpy( buffer, rSoundGroup->GetName().data(), rSoundGroup->GetName().length() );

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

	void EditorLayer::PrepZipProject()
	{
		// Ask the user to select a folder where the zip archive will be made.
		const auto& rOutPath = Application::Get()->OpenFolder();
		if( rOutPath.empty() )
			return;

		if( !m_BlockingOperation )
			m_BlockingOperation = Ref<JobProgress>::Create();

		SaveProject();
		SaveFile();

		JobSystem::Get().QueueJob(
			[ this, rOutPath ]()
		{
			m_JobModalOpen.store( true );
			m_BlockingOperation->SetTitle( "Zipping project" );
			m_BlockingOperation->SetStatus( "Zipping... please wait" );

			ProjectZipper::ZipActiveProject( rOutPath );
	
			m_BlockingOperation->OnComplete();
		} );
	}

	void EditorLayer::QueuePremakeJob()
	{
		JobSystem::Get().QueueJob( [ this ]()
		{
			if( !Project::GetActiveProject()->HasPremakeFile() )
				Project::GetActiveProject()->CreatePremakeFile();

			if( Premake::Launch( Project::GetActiveProject()->GetRootDir(), L"premake5.lua", PREFERED_PREMAKE_ACTION_FOR_OS ) )
			{
				PushNotification( "Generated project files." );
			}
			else
			{
				PushNotification( "Failed to generated project files!" );
			}
		} );
	}

	void EditorLayer::HotReloadGame()
	{
		PushNotification( "Waiting for JobSystem to start Hot-Reload..." );
		SAT_CORE_INFO( "[HotReload] Begin hot reload" );

		SaveFile();
		SaveProject();

		if( !m_BlockingOperation )
			m_BlockingOperation = Ref<JobProgress>::Create();

		JobSystem::Get().QueueJob( 
			[ this ]()
		{
			m_JobModalOpen.store( true );
			m_BlockingOperation->SetTitle( "Building Project" );
			m_BlockingOperation->SetStatus( "Hot-Reloading" );

			// Attempt to build with HOTRELOAD switch.
			const auto status = Project::GetActiveProject()->Build( Application::GetCurrentConfigKind(), "/HOTRELOAD" );

			SAT_CORE_INFO( "[HotReload] SaturnBuildTool exited with code {0}", ( uint8_t ) status );

			switch( status )
			{
				case SaturnBuildToolExitCodes::Success:
				{
					// It's not safe to be messing with our SClasses on a job system thread.
					// So we'll do it next frame on the main thread.
					Application::Get()->DispatchEvent<Event>( EventType::HotReloadComplete, EC_Editor );
				} break;

				default:
				case SaturnBuildToolExitCodes::Failure:
				{
					MessageBoxInfo info{ .Text = "Hot-reload compilation failed!" };
					PushMessageBox( info );
				} break;

				case SaturnBuildToolExitCodes::NothingTodo:
				{
					PushNotification( "Nothing to Hot-Reload! Project is up to date." );
				} break;
			}

			m_BlockingOperation->OnComplete();
		} );
	}

	void EditorLayer::DrawAssetRegistryDebug()
	{
		if( ImGui::Begin( "Asset Manager", &m_OpenAssetRegistryDebug, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse ) )
		{
			static ImGuiTextFilter Filter;

			ImGui::Text( "Search" );
			ImGui::SameLine();
			Filter.Draw( "##search" );

			if( ImGui::BeginTable( "##FileTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX | ImGuiTableFlags_NoBordersInBody, ImVec2( ImGui::GetWindowSize().x, ImGui::GetWindowSize().y ) ) )
			{
				ImGui::TableSetupColumn( "Asset Name" );
				ImGui::TableSetupColumn( "ID" );
				ImGui::TableSetupColumn( "Type" );
				ImGui::TableSetupColumn( "Path" );
				ImGui::TableSetupColumn( "Version" );
				ImGui::TableSetupColumn( "Find Asset", ImGuiTableColumnFlags_NoHeaderLabel );

				ImGui::TableHeadersRow();

				for( auto&& [id, asset] : m_AssetManager->GetAssetMap() )
				{
					if( !Filter.PassFilter( asset->Name.c_str() ) || !Filter.PassFilter( std::to_string( asset->ID ).c_str() ) )
						continue;

					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex( 0 );
					ImGui::Selectable( asset->Name.c_str(), false );

					ImGui::TableSetColumnIndex( 1 );
					ImGui::Text( "%" PRIu64, id );

					ImGui::TableSetColumnIndex( 2 );
					ImGui::Text( AssetTypeToString( asset->Type ).data(), false );

					ImGui::TableSetColumnIndex( 3 );
					ImGui::Text( asset->Path.string().c_str() );

					ImGui::TableSetColumnIndex( 4 );
					ImGui::Text( "%i", asset->Version );

					if( asset->Version < AssetVersion::Latest )
					{
						ImGui::SameLine();
						ImGui::Text( "(Version is older than latest)" );
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
		}

		ImGui::End();
	}

	void EditorLayer::DrawLoadedAssetsDebug()
	{
		if( ImGui::Begin( "Loaded Assets", &m_OpenLoadedAssetDebug ) )
		{
			static ImGuiTextFilter Filter;

			ImGui::Text( "Search for assets..." );
			ImGui::SameLine();
			Filter.Draw( "##search" );

			if( ImGui::BeginTable( "##FileTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX | ImGuiTableFlags_NoBordersInBody, ImVec2( ImGui::GetWindowSize().x, ImGui::GetWindowSize().y * 0.85f ) ) )
			{
				ImGui::TableSetupColumn( "Asset Name" );
				ImGui::TableSetupColumn( "ID" );
				ImGui::TableSetupColumn( "Type" );
				ImGui::TableSetupColumn( "Ref Count" );
				ImGui::TableSetupColumn( "Find Asset", ImGuiTableColumnFlags_NoHeaderLabel );

				ImGui::TableHeadersRow();

				for( const auto& [id, asset] : m_AssetManager->GetLoadedAssetMap() )
				{
					if( !Filter.PassFilter( asset->Name.c_str() ) )
						continue;

					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex( 0 );
					ImGui::Selectable( asset->Name.c_str(), false );

					ImGui::TableSetColumnIndex( 1 );
					ImGui::Text( "%" PRIu64, id );

					ImGui::TableSetColumnIndex( 2 );
					ImGui::Text( AssetTypeToString( asset->Type ).data() );

					ImGui::TableSetColumnIndex( 3 );
					ImGui::Text( "%" PRIu32, asset->GetRefCount() );

					ImGui::TableSetColumnIndex( 4 );
					ImGui::PushID( ( int ) id );
					if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), { ImGui::TableGetHeaderRowHeight(), ImGui::TableGetHeaderRowHeight() } ) )
					{
						Ref<ContentBrowserPanel> contentBrowserPanel = m_ImGuiWindowManager->GetPanel<ContentBrowserPanel>();

						contentBrowserPanel->BrowseToItem( asset->Path, id );
					}
					ImGui::PopID();
				}

				ImGui::EndTable();
			}
		}

		ImGui::End();
	}

	void EditorLayer::DrawEditorSettings()
	{
		bool shouldSaveEngSettings = false;

		ImGui::SetNextWindowSize( ImVec2( 750.0f, 750.0f ), ImGuiCond_Appearing );
		if( ImGui::Begin( "Editor Settings", &m_OpenEditorSettings ) )
		{
			auto& rIO = ImGui::GetIO();
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

			auto& rEngineSettings = EngineSettings::Get();

			ImGui::PushFont( boldFont );
			ImGui::Text( "Customisation" );
			ImGui::PopFont();

			ImGui::Text( "Font" );
			ImGui::SameLine();

			const char* pPreviewValue = nullptr;

			const EditorFont fontBeforeSelection = rEngineSettings.GetEditorFont();
			switch( fontBeforeSelection )
			{
				case EditorFont::NotoSans:
				{
					pPreviewValue = "Noto Sans";
				} break;

				case EditorFont::Atkinson:
				{
					pPreviewValue = "Atkinson";
				} break;

				default:
					pPreviewValue = "UNKNOWN FONT";
					break;
			}

			ImGui::SetNextItemWidth( 130.0f );
			if( ImGui::BeginCombo( "##selectFont", pPreviewValue ) )
			{
				if( ImGui::Selectable( "Noto Sans", rEngineSettings.GetEditorFont() == EditorFont::NotoSans ) )
				{
					if( fontBeforeSelection != EditorFont::NotoSans )
					{
						rEngineSettings.SetEditorFont( EditorFont::NotoSans );
						m_FontChanged = true;
					}
				}

				if( ImGui::Selectable( "Atkinson Hyperlegible Next", rEngineSettings.GetEditorFont() == EditorFont::Atkinson ) )
				{
					if( fontBeforeSelection != EditorFont::Atkinson )
					{
						rEngineSettings.SetEditorFont( EditorFont::Atkinson );
						m_FontChanged = true;
					}
				}

				ImGui::EndCombo();
			}

			if( m_FontChanged )
			{
				const char* pWarningText = "An Editor restart is required for the changes to apply!";

				const ImVec2 padding = ImGui::GetStyle().FramePadding;
				const ImVec2 textPosition = ImGui::GetCursorScreenPos();
				const ImVec2 textSize = ImGui::CalcTextSize( pWarningText );

				const ImVec2 min = ImVec2( textPosition.x - padding.x, textPosition.y - padding.y );
				const ImVec2 max = ImVec2( textPosition.x + padding.x + textSize.x, textPosition.y + padding.y + textSize.y );

				ImGui::GetWindowDrawList()->AddRectFilled( min, max,
					IM_COL32( 200, 30, 60, 255 ), 2.0f, ImDrawFlags_RoundCornersAll );

				ImGui::TextUnformatted( pWarningText );
			}

			ImGui::Separator();

			ImGui::Text( "Recent Projects" );
			for( const auto& rPath : rEngineSettings.GetAllRecentProjects() )
			{
				ImGui::BulletText( rPath.string().c_str() );
			}

			ImGui::BeginHorizontal( "##eng_startprj" );
			{
				ImGui::Text( "Startup Project: %s", rEngineSettings.StartupProject.string().c_str() );

				ImGui::Spring();

				Auxiliary::DisabledFlag inspectDisabledFlag( m_RequestRuntime );

				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), { 24.0f, 24.0f } ) )
				{
					const auto filePath = Application::Get()->OpenFile( "Saturn Project file (*.sproject)|sproject" );
					if( !filePath.empty() )
					{
						rEngineSettings.StartupProject = filePath;
						shouldSaveEngSettings = true;
					}
				}

				inspectDisabledFlag.Pop();

				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "SearchFolder" ), { 24.0f, 24.0f } ) )
				{
					Application::Get()->OpenNativeFileExplorer( rEngineSettings.StartupProject, true );
				}

				if( ImGui::BeginItemTooltip() )
				{
					ImGui::Text( "Find in Native Explorer" );
					ImGui::EndTooltip();
				}

				ImGui::SetNextItemWidth( 130.0f );
				if( ImGui::BeginCombo( "##recentprojects", rEngineSettings.StartupProjectName.data() ) )
				{
					for( const auto& rProject : rEngineSettings.GetAllRecentProjects() )
					{
						const bool selected = rEngineSettings.StartupProject == rProject;
						if( ImGui::Selectable( rProject.string().data(), selected ) )
						{
							rEngineSettings.StartupProject = rProject;
							rEngineSettings.StartupProjectName = rProject.stem().string();
						}
					}

					ImGui::EndCombo();
				}
			}
			ImGui::EndHorizontal();

			if( shouldSaveEngSettings )
			{
				EngineSettingsSerialiser ess;
				ess.Serialise();
			}
		}

		ImGui::End();
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

			if( ImGui::MenuItem( "New Scene", "Alt+Shift+N" ) )		 NewFile();
			if( ImGui::MenuItem( "Save Scene", "Ctrl+S" ) )          SaveFile();
			if( ImGui::MenuItem( "Save Scene As", "Ctrl+Shift+S" ) ) SaveFileAs();
			if( ImGui::MenuItem( "Mark Scene as startup"  ) )        MarkSceneAsStartupFromTitlebar();
			if( ImGui::MenuItem( "Revert" ) )						 RevertFile();

			ImGui::Separator();

			if( ImGui::MenuItem( "Save Project", "Alt+Shift+S" ) )   SaveProject();
			if( ImGui::MenuItem( "Zip Project", "Alt+Shift+Z" ) )    PrepZipProject();
			
			if( ImGui::BeginMenu( "Open Recent Projects" ) ) 
			{
				for( const auto& rRecentProject : EngineSettings::Get().GetAllRecentProjects() ) 
				{
					const std::string& rFilepathStr = rRecentProject.string();
					if( ImGui::MenuItem( rFilepathStr.c_str() ) )
					{
						// has_extension check just in case for whatever reason the path is invalid.
						if( std::filesystem::exists( rRecentProject ) && rRecentProject.has_extension() )
						{
							SaveProject();

							if( m_EditorScene->IsDirty() )
							{
								Application::Get()->GetWindow()->FlashAttention();

								m_ShowSceneDirtyModal = true;
								m_EventAfterPopup = [ & ]() { CloseEditorAndOpenNewProj( rRecentProject ); };
							}
							else
							{
								CloseEditorAndOpenNewProj( rRecentProject );
							}
						}
						else
						{
							m_InvalidRecentProjectPath = rRecentProject;
							m_ShowInvalidRecentProjectPathModal = true;
						}

						break;
					}
				}

				ImGui::EndMenu();
			}
			
			if( ImGui::MenuItem( "Close Project", "Ctrl+W" ) )       CloseEditorAndOpenPB();

			disabledIfRuntime.Pop();

			ImGui::Separator();
			
			if( ImGui::MenuItem( "Exit", "Alt+F4" ) )                if( OnTitlebarExit() ) Application::Get()->Close();

			ImGui::EndMenu();
		}

		if( ImGui::BeginMenu( "Edit" ) )
		{
			{
				Auxiliary::ScopedDisabledFlag disabledIfRuntime( m_RequestRuntime );

				{
					Auxiliary::ScopedDisabledFlag disabledIfNoUndo( m_GlobalUndoRedoGroup->IsUndoActionsEmpty() );
					if( ImGui::MenuItem( "Undo", "Ctrl+Z" ) )           m_GlobalUndoRedoGroup->GlobalUndoRecent();
				}

				{
					Auxiliary::ScopedDisabledFlag disabledIfNoRedo( m_GlobalUndoRedoGroup->IsRedoActionsEmpty() );
					if( ImGui::MenuItem( "Redo", "Ctrl+Y" ) )           m_GlobalUndoRedoGroup->GlobalRedoRecent();
				}

				{
					Auxiliary::ScopedDisabledFlag disabledIfNoActions( !m_GlobalUndoRedoGroup->HasAnyActions() );
					if( ImGui::MenuItem( "Clear all action history" ) ) m_GlobalUndoRedoGroup->ClearAll();
				}
			}

			ImGui::EndMenu();
		}

		if( ImGui::BeginMenu( "Project" ) )
		{
			if( ImGui::BeginMenu( "Open Project in" ) )
			{
#if defined(SAT_PLATFORM_WINDOWS) || defined(SAT_PLAFORM_MACOS)
				if( ImGui::MenuItem( "Visual Studio Latest" ) )
				{
					std::filesystem::path solutionPath = Project::GetActiveProject()->GetRootDir();
					solutionPath /= std::format( "{0}.sln", Project::GetActiveConfig().Name );

					Auxiliary::TextEditors::OpenVisualStudioLatest( solutionPath );
				}
#endif

				ImGui::EndMenu();
			}

			ImGui::SeparatorText( "Settings" );

			if( ImGui::MenuItem( "Project settings" ) ) m_ShowUserSettings ^= 1;

			ImGui::SeparatorText( "Compatibility" );

			{
				Auxiliary::ScopedDisabledFlag disabled( m_RequestRuntime );
				if( ImGui::MenuItem( "Upgrade assets" ) ) Project::GetActiveProject()->UpgradeAssets();
			}

			ImGui::SeparatorText( "Development" );
			{
				Auxiliary::ScopedDisabledFlag disabled( m_RequestRuntime );

				if( ImGui::MenuItem( "Recreate project files" ) )
				{
					if( !( m_HasPremakePath = Auxiliary::HasEnvironmentVariable( "SATURN_PREMAKE_PATH" ) ) )
					{
						m_PendingPremakeJobAfterPathIsSet = m_ShowSetPremakePathModal = true;
					}
					else
					{
						QueuePremakeJob();
					}
				}

				if( ImGui::BeginItemTooltip() )
				{
					ImGui::Text( "Uses premake5 to regenerate the project files.\nEnvironment variable \"SATURN_PREMAKE_PATH\" must be set." );
					ImGui::EndTooltip();
				}

				{
					Auxiliary::ScopedDisabledFlag disabledIfNoMod( !m_GameModule->HasModule() );
					if( ImGui::MenuItem( "Hot Reload Game" ) )
					{
						HotReloadGame();
					}

					if( ImGui::BeginItemTooltip() )
					{
						if( m_RequestRuntime )
						{
							ImGui::Text( "Cannot hot-reload during runtime." );
						}
						else if( !m_GameModule->HasModule() )
						{
							ImGui::Text( "No module exists to be hot-reloaded." );
						}
						else
						{
							ImGui::Text( "Hot-reload the game. (Alt+F5)" );
						}

						ImGui::EndTooltip();
					}
				}
			}

			ImGui::SeparatorText( "Distribution" );
			{
				if( ImGui::MenuItem( "Build Bundles" ) )
				{
					if( ValidateProjectDefaults() )
					{
						m_ShowDistBuildOptions ^= 1;
					}
				}

				if( ImGui::BeginItemTooltip() )
				{
					ImGui::Text( "Distribution requires a Shader Bundle and an Asset Bundle in order for the runtime to build." );
					ImGui::EndTooltip();
				}
				
				if( ImGui::MenuItem( "Compile project as distribution target" ) )
				{
					m_HasPremakePath = Auxiliary::HasEnvironmentVariable( "SATURN_PREMAKE_PATH" );

					if( !m_BlockingOperation )
						m_BlockingOperation = Ref<JobProgress>::Create();

					JobSystem::Get().QueueJob( [ this ]()
					{
						m_JobModalOpen.store( true );
						m_BlockingOperation->SetTitle( "Distributing Project" );

						m_BlockingOperation->SetStatus( "Building project" );
						if( Project::GetActiveProject()->Rebuild( ApplicationConfigKind::Dist ) == SaturnBuildToolExitCodes::Failure ) 
						{
							MessageBoxInfo msgBox = { .Title = "Error##MsgBox", .Text = "Failed to compile for Dist, aborting..." };
							PushMessageBox( msgBox );

							m_JobModalOpen.store( false );
							m_BlockingOperation->OnComplete();
						}

						m_BlockingOperation->SetProgress( 50.0f );

						m_BlockingOperation->SetStatus( "Copying for Distribution" );
						Project::GetActiveProject()->Distribute( ApplicationConfigKind::Dist );

						m_BlockingOperation->SetProgress( 100.0f );
						m_BlockingOperation->OnComplete();
					} );
				}

				if( ImGui::BeginItemTooltip() )
				{
					ImGui::Text( "Attempts to compile the project and fully setup the project for Distribution." );
					ImGui::EndTooltip();
				}
			}

#if defined( SAT_DEBUG )
			ImGui::SeparatorText( "DEBUG" );
			if( ImGui::MenuItem( "DEBUG: Build Asset Bundle (no shaders)" ) )
			{
				CreateAssetBundleJob();
			}
#endif

			ImGui::EndMenu();
		}

		if( ImGui::BeginMenu( "View" ) )
		{
			ImGui::SeparatorText( "Windows" );
			if( ImGui::MenuItem( "Project settings" ) )           m_ShowUserSettings ^= 1;
			if( ImGui::MenuItem( "Editor Settings" ) )            m_OpenEditorSettings ^= 1;
			if( ImGui::MenuItem( "Scene Renderer" ) )             m_ShowSceneRendererWindow ^= 1;
			if( ImGui::MenuItem( "Renderer (Vulkan Info)" ) )     m_ShowRendererWindow ^= 1;
			if( ImGui::MenuItem( "Content Browser Panel" ) )      ShowOrHideContentBrowserPanel();
			if( ImGui::MenuItem( "Scene Hierarchy Panel" ) )      ShowOrHideSceneHierarchyPanel();
			if( ImGui::MenuItem( "Runtime Command Window" ) )     ShowOrHideRTCmdWindow();
			if( ImGui::MenuItem( "Memory Statistics" ) )		  ShowOrHideMemStatsWindow();

			ImGui::SeparatorText( "Asset Manager" );
			if( ImGui::MenuItem( "Asset Registry Debug" ) )       m_OpenAssetRegistryDebug ^= 1;
			if( ImGui::MenuItem( "Loaded Assets Debug" ) )        m_OpenLoadedAssetDebug ^= 1;
			if( ImGui::MenuItem( "Asset Dependencies" ) )         m_ShowAssetDependencies ^= 1;

			ImGui::SeparatorText( "SClass" );
			if( ImGui::MenuItem( "Metadata Debug" ) )             m_ShowMetadataDebug ^= 1;

			ImGui::SeparatorText( "Demo Window" );
			if( ImGui::MenuItem( "Show demo window" ) )           m_ShowImGuiDemoWindow ^= 1;

			ImGui::SeparatorText( "Virtual Filesystem (VFS)" );
			if( ImGui::MenuItem( "Virtual Filesystem Debug" ) )   m_ShowVFSDebug ^= 1;

			ImGui::SeparatorText( "Scene Renderer" );
			if( ImGui::MenuItem( "Render Mesh AABB" ) )           m_ShowMeshAABB ^= 1;
			if( ImGui::MenuItem( "Show Camera Frustum" ) )        m_ShowCameraFrustum ^= 1;

			if( ImGui::BeginMenu( "Scene Visualisation Options" ) )
			{
				auto& rVisualisationOptions = g_ActiveScene->GetVisualisationOptions();

				if( ImGui::Checkbox( "Show Grid", &rVisualisationOptions.ShowGrid ) )
					g_ActiveScene->MarkDirty();

				if( ImGui::Checkbox( "Show Grid (Runtime)", &rVisualisationOptions.ShowGridOnRuntime ) )
					g_ActiveScene->MarkDirty();

				if( ImGui::BeginMenu( "Physics Colliders Options" ) )
				{
					bool showNone = rVisualisationOptions.PhysColliderOptions == PhysicsColliderVisualisationOptions::Disabled;
					if( ImGui::Checkbox( "No Visualisation", &showNone ) )
					{
						if( showNone )
							rVisualisationOptions.PhysColliderOptions = PhysicsColliderVisualisationOptions::Disabled;
						else
							rVisualisationOptions.PhysColliderOptions = PhysicsColliderVisualisationOptions::SelectedOnly;

						g_ActiveScene->MarkDirty();
					}

					bool showAll = rVisualisationOptions.PhysColliderOptions == PhysicsColliderVisualisationOptions::All;
					if( ImGui::Checkbox( "All", &showAll ) )
					{
						if( showAll )
							rVisualisationOptions.PhysColliderOptions = PhysicsColliderVisualisationOptions::All;
						else
							rVisualisationOptions.PhysColliderOptions = PhysicsColliderVisualisationOptions::Disabled;

						g_ActiveScene->MarkDirty();
					}

					bool showSelected = rVisualisationOptions.PhysColliderOptions == PhysicsColliderVisualisationOptions::SelectedOnly;
					if( ImGui::Checkbox( "Selected Only", &showSelected ) )
					{
						if( showSelected )
							rVisualisationOptions.PhysColliderOptions = PhysicsColliderVisualisationOptions::SelectedOnly;
						else
							rVisualisationOptions.PhysColliderOptions = PhysicsColliderVisualisationOptions::Disabled;

						g_ActiveScene->MarkDirty();
					}

					ImGui::EndMenu();
				}

				if( ImGui::BeginMenu( "AI Options" ) )
				{
					bool showBTInfo = rVisualisationOptions.AIVisualisationOptions & AIVisualisationOptions_BehaviourTreeInfo;
					if( ImGui::Checkbox( "Behaviour Tree Info", &showBTInfo ) )
					{
						if( showBTInfo )
							rVisualisationOptions.AIVisualisationOptions |= AIVisualisationOptions_BehaviourTreeInfo;
						else
							rVisualisationOptions.AIVisualisationOptions &= ~AIVisualisationOptions_BehaviourTreeInfo;

						g_ActiveScene->MarkDirty();
					}

					bool navPathsInfo = rVisualisationOptions.AIVisualisationOptions & AIVisualisationOptions_NavPaths;
					if( ImGui::Checkbox( "Nav Paths", &navPathsInfo ) )
					{
						if( navPathsInfo )
							rVisualisationOptions.AIVisualisationOptions |= AIVisualisationOptions_NavPaths;
						else
							rVisualisationOptions.AIVisualisationOptions &= ~AIVisualisationOptions_NavPaths;

						g_ActiveScene->MarkDirty();
					}

					ImGui::EndMenu();
				}

				if( ImGui::BeginMenu( "Skeleton Options" ) )
				{
					bool showLines = rVisualisationOptions.SkeletonVisualisationOptions & SkeletonVisualisationOptions_BoneLines;
					if( ImGui::Checkbox( "Lines", &showLines ) )
					{
						if( showLines )
							rVisualisationOptions.SkeletonVisualisationOptions |= SkeletonVisualisationOptions_BoneLines;
						else
							rVisualisationOptions.SkeletonVisualisationOptions &= ~SkeletonVisualisationOptions_BoneLines;

						g_ActiveScene->MarkDirty();
					}

					bool showNames = rVisualisationOptions.SkeletonVisualisationOptions & SkeletonVisualisationOptions_Names;
					if( ImGui::Checkbox( "Names", &showNames ) )
					{
						if( showNames )
							rVisualisationOptions.SkeletonVisualisationOptions |= SkeletonVisualisationOptions_Names;
						else
							rVisualisationOptions.SkeletonVisualisationOptions &= ~SkeletonVisualisationOptions_Names;

						g_ActiveScene->MarkDirty();
					}

					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}

			ImGui::SeparatorText( "Content Browser" );
			if( ImGui::MenuItem( "Show Thumbnail Cache" ) )       m_ShowCBThumbnailDebug ^= 1;

			ImGui::SeparatorText( "Undo Redo" );
			if( ImGui::MenuItem( "Show Undo Redo Stack" ) )       m_ShowUndoRedoDebug ^= 1;

			{
				Auxiliary::ScopedDisabledFlag disabled( m_RequestRuntime );

				ImGui::SeparatorText( "Auto Saves" );
				if( ImGui::MenuItem( "Clear all auto saves" ) )           ClearAllAutoSaves();
				if( ImGui::MenuItem( "Clear all for the active scene" ) )  ClearAutoSavesForActiveScene();
			}

			ImGui::SeparatorText( "Physics" );
			if( ImGui::MenuItem( "Open Jolt debug viewer" ) )       PhysicsDebugRecorder::OpenRecordedFile();
			if( ImGui::MenuItem( "Open debug viewer folder" ) )
			{
				std::filesystem::path outPath = Project::GetActiveProject()->GetFullCachePath();
				outPath /= "PerUser";
				Application::Get()->OpenNativeFileExplorer( outPath );
			}

			ImGui::SeparatorText( "Debug" );
			if( ImGui::MenuItem( "DEBUG: Open sandbox node editor" ) )
			{
				if( !m_SandboxNodeEditorViewer )
					m_SandboxNodeEditorViewer = Ref<SandboxNodeEditorViewer>::Create();

				m_SandboxNodeEditorViewer->ForceOpenWindow();

				m_ImGuiWindowManager->AddWindow( m_SandboxNodeEditorViewer, "SndboxVwr" );
			}

			if( ImGui::MenuItem( "DEBUG: Open message box & notification test" ) )  m_ShowDebugMsgBoxWindow ^= 1;
			if( ImGui::MenuItem( "DEBUG: Open editor debug window" ) )				m_ShowEditorDebugWindow ^= 1;
			if( ImGui::MenuItem( "DEBUG: Simulate Read Only state" ) )				m_ImGuiWindowManager->MarkAllWindowsAsReadOnly();
			if( ImGui::MenuItem( "DEBUG: Reset Read Only state" ) )					m_ImGuiWindowManager->ResetReadOnlyState();
			if( ImGui::MenuItem( "DEBUG: Mark scene as dirty" ) )					m_EditorScene->MarkDirty();

			ImGui::EndMenu();
		}

		if( m_RequestRuntime )
		{
			if( ImGui::BeginMenu( "Runtime" ) )
			{
				const auto runtimeState = m_RuntimeScene->GetRuntimeState();

				// Play
				{
					Auxiliary::ScopedDisabledFlag disabled( runtimeState != RuntimeState::Suspended );
					if( ImGui::MenuItem( "Play" ) ) m_RuntimeScene->ResumeRuntime();

					if( ImGui::BeginItemTooltip() )
					{
						ImGui::Text( "Cannot start a new runtime while the scene is already in runtime." );
#if defined(SAT_DEBUG)
						ImGui::Text( "%s", m_RequestRuntime ? "RUNTIME RUNNING" : "RUNTIME NOT RUNNING" );
#endif
						ImGui::EndTooltip();
					}
				}

				// Stop
				if( ImGui::MenuItem( "Stop", "F5" ) ) m_RequestRuntime = false;

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

		if( ImGui::BeginMenu( "Help" ) )
		{
			if( ImGui::MenuItem( "About" ) )        m_OpenAboutWindow ^= 1;
			ImGui::EndMenu();
		}

		// Draw Project name text and box.
		ImGui::SeparatorEx( ImGuiSeparatorFlags_Vertical );

#if defined(SAT_DEBUG) || 1
		const std::string prjName = Project::GetActiveConfig().Name;
		const auto devVer = Project::GetActiveProject()->GetDeveloperVersion();

		std::string text = prjName;
		if( !devVer.empty() )
			text = std::format( "{0}-{1}", prjName, devVer );
#else
		std::string text = Project::GetActiveConfig().Name;
#endif

		text += std::format( " | {0}", g_ActiveScene->Name.empty() ? "<New Scene>" : g_ActiveScene->Name );

		const ImVec2 textSize = ImGui::CalcTextSize( text.c_str() );
		ImDrawList* pDrawList = ImGui::GetWindowDrawList();
		const float frameHeight = ImGui::GetFrameHeight();

		const ImVec2 min = ImGui::GetWindowPos() + ImGui::GetCursorPos();
		const ImVec2 max = min + ImVec2( textSize.x + frameHeight, frameHeight );
		const ImRect buttonRect( min, max );

		const ImU32 color = ImGui::GetColorU32( ImGuiCol_ButtonHovered );
		pDrawList->AddRectFilled( buttonRect.Min, buttonRect.Max, color );

		// Text centered in the rect.
		const float textY = min.y + ( frameHeight - textSize.y ) * 0.5f;
		pDrawList->AddText( ImVec2( min.x + frameHeight * 0.5f, textY ), IM_COL32( 255, 255, 255, 255 ), text.c_str() );

		ImGui::Dummy( buttonRect.GetSize() );
		if( ImGui::BeginItemTooltip() )
		{
			const std::string pathStr = Project::GetActiveConfig().Path.string();
			ImGui::Text( pathStr.c_str() );
			ImGui::EndTooltip();
		}
	}

	void EditorLayer::DrawAboutWindow()
	{
		if( ImGui::Begin( "About", &m_OpenAboutWindow ) )
		{
			EditorAboutWindowContents::DrawContents();

			ImGui::End();
		}
	}

	void EditorLayer::DrawSceneRendererWindow()
	{
		if( ImGui::Begin( "Scene Renderer", &m_ShowSceneRendererWindow ) )
		{
			m_SceneRenderer->ImGuiRender();

			if( Auxiliary::TreeNode( "Alura" ) )
			{
				if( g_AluraCanvas )
				{
					ImGui::Text( "AluraCanvas::FrameTime: %.3f ms", g_AluraCanvas->GetFrameTiming() );
				}
				else
				{
					ImGui::TextDisabled( "No Alura Canvas exists!" );
				}

				Auxiliary::EndTreeNode();
			}

			if( Auxiliary::TreeNode( "Editor Camera" ) )
			{
				ImGui::Text( "Width %i, Height %i, Ratio %i", m_EditorCamera.GetViewportWidth(), m_EditorCamera.GetViewportHeight(), m_EditorCamera.GetAspectRatio() );

				const auto& rPosition = m_EditorCamera.GetPosition();
				ImGui::Text( "Position X: %f, Y: %f, Z: %f", rPosition.x, rPosition.y, rPosition.z );

				Auxiliary::EndTreeNode();
			}

			if( Auxiliary::TreeNode( "Engine Shaders", false ) )
			{
				ImGui::BeginHorizontal( "##edbundleopt" );

				if( ImGui::Button( "Force package engine shaders" ) )
				{
					if( EditorShaderBundle::BundleShaders() )
					{
						PushNotification( "Packaged editor shader bundle!" );
					}
					else
					{
						PushNotification( "Failed to package editor shader bundle!" );
					}
				}

				ImGui::Spring();

				if( ImGui::Button( "Delete ShaderBundle" ) )
				{
					const std::filesystem::path shaderBundleFilePath = Application::Get()->GetAppDataFolder() / "EditorShaderBundle.ssb";
					if( std::filesystem::exists( shaderBundleFilePath ) )
					{
						std::filesystem::remove( shaderBundleFilePath );
					}
				}

				ImGui::EndHorizontal();

				ImGui::BeginTable( "##engShaders", 4 );
				ImGui::TableSetupColumn( "##name" );
				ImGui::TableSetupColumn( "##recomp" );
				ImGui::TableSetupColumn( "##view" );
				ImGui::TableSetupColumn( "##edit" );

				for( auto& [name, shader] : ShaderLibrary::Get().GetShaders() )
				{
					ImGui::TableNextColumn();
					ImGui::AlignTextToFramePadding();
					ImGui::Text( name.c_str() );

					if( ImGui::BeginItemTooltip() )
					{
						ImGui::Text( "Hash: %" PRIu64, shader->GetShaderHash() );
						ImGui::EndTooltip();
					}

					ImGui::TableNextColumn();

					ImGui::PushID( ( int ) shader->GetShaderHash() );

					if( ImGui::Button( "Recompile" ) )
					{
						if( !shader->TryRecompile() )
						{
							Application::Get()->GetWindow()->FlashAttention();

							MessageBoxInfo msgBox = { .Title = "Error", .Text = std::format( "Shader '{0}' failed to recompile. Defaulting back to last successful build. Check debug console for more information!", shader->GetName() ) };
							PushMessageBox( msgBox );
						}
						else
						{
							PushNotification( "Hot reloaded shader!" );
						}
					}

					ImGui::TableNextColumn();

					if( ImGui::Button( "Show in Explorer" ) )
					{
						std::filesystem::path absPath = std::filesystem::absolute( shader->GetFilepath() );
						Application::Get()->OpenNativeFileExplorer( absPath, true );
					}

					ImGui::TableNextColumn();

					if( ImGui::Button( "Edit/View" ) )
					{
						std::filesystem::path absPath = std::filesystem::absolute( shader->GetFilepath() );
						
						auto wind = Ref<ShaderViewerWindow>::Create( absPath );
						wind->OpenWindow();
						m_ImGuiWindowManager->AddWindow( wind, wind->GetWindowName() );
					}

					ImGui::PopID();
				}

				ImGui::EndTable();
				Auxiliary::EndTreeNode();
			}
		}

		ImGui::End();
	}

	void EditorLayer::DrawRendererWindow()
	{
		if( ImGui::Begin( "Renderer", &m_ShowRendererWindow ) )
		{
			ImGui::Text( "Frame Time: %.2f ms", Application::Get()->Time().Milliseconds() );

			for( const auto& devices : VulkanContext::Get()->GetPhysicalDeviceProperties() )
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
			static ImGuiTextFilter s_SearchFilter;

			ImGuiIO& rIO = ImGui::GetIO();

			const auto italicsFont = rIO.Fonts->Fonts[ 2 ];
			ImGui::PushFont( italicsFont );
			ImGui::TextDisabled( "Showing all SClasses" );
			ImGui::PopFont();

			s_SearchFilter.DrawWithHint( "##classfinder", "Search for classes via name or via their hash.", 436.0f );

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
					ClassMetadataHandler::Get().EveryClass(
						[ & ]( const SClass* pClass )
					{
						// TODO: Fix this shit conversion.
						if( s_SearchFilter.PassFilter( std::to_string( pClass->GetHash() ).c_str() ) || 
							s_SearchFilter.PassFilter( pClass->GetName().c_str() ) )
						{
							ImGui::TableNextRow();

							ImGui::TableSetColumnIndex( 0 );
							ImGui::Text( "%s", pClass->GetName().c_str() );

							ImGui::TableSetColumnIndex( 1 );
							ImGui::Text( "%" PRIu64, pClass->GetSize() );

							ImGui::TableSetColumnIndex( 2 );
							ImGui::Text( "%" PRIu64, pClass->GetAlignment() );

							ImGui::TableSetColumnIndex( 3 );
							ImGui::Text( "%" PRIu64, pClass->GetHash() );

							ImGui::TableSetColumnIndex( 4 );
							ImGui::Text( "%i", pClass->GetPropertyCount() );

							ImGui::TableSetColumnIndex( 5 );
							ImGui::Text( "%s", pClass->GetHeaderPath().string().c_str() );
						}
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
				for( const auto& [assetID, rDependency] : m_AssetManager->GetAssetDependencies() )
				{
					if( Auxiliary::TreeNode( std::to_string( assetID ), false ) )
					{
						for( const MemoryAssetDependencyBase* pBase : rDependency )
						{
							ImGui::Text( "Address 0x%p", ( void* ) pBase );
						}

						Auxiliary::EndTreeNode();
					}
				}

				Auxiliary::EndTreeNode();
			}

			if( Auxiliary::TreeNode( "Asset Dependencies", true ) )
			{
				for( const auto& [assetID, rDependencies] : m_AssetManager->GetPureAssetDependencies() )
				{
					const Ref<Asset> asset = m_AssetManager->FindAsset( assetID );
					if( Auxiliary::TreeNode( asset->Name, false ) )
					{
						for( const AssetID id : rDependencies )
						{
							const Ref<Asset> dependency = m_AssetManager->FindAsset( id );
							if( dependency )
							{
								ImGui::Text( dependency->Name.data() );

								if( ImGui::BeginItemTooltip() )
								{
									ImGui::Text( "Asset" );
									ImGui::Separator();

									ImGui::Text( "%s", dependency->Path.string().c_str() );
									ImGui::Text( "Asset: %" PRIu64, dependency->ID );
									ImGui::Text( "Asset Name: %s", dependency->Name.c_str() );
									ImGui::Text( "Asset Version: %" PRIu64, dependency->Version );

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

				if( m_EventAfterPopup )
					m_EventAfterPopup();
			}

			ImGui::Spring();

			if( ImGui::Button( "Close without saving" ) )
			{
				m_ShowSceneDirtyModal = false;
				ImGui::CloseCurrentPopup();

				if( m_EventAfterPopup )
					m_EventAfterPopup();
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

		if( ImGui::BeginPopupModal( "Blocking Action", nullptr, ImGuiWindowFlags_NoSavedSettings ) )
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

			ImSpinner::SpinnerAng( "##OPERATION_SPINNER", 25.0f * 0.5F, 2.0f, ImSpinner::white, ImSpinner::half_white, 8.6F );

			ImGui::Spring();

			if( const float percent = m_BlockingOperation->GetProgress(); percent >= 1.0f )
			{
				ImGui::ProgressBar( percent / 100.0f );
			}

			ImGui::EndHorizontal();

			if( m_BlockingOperation->Completed() )
			{
				m_JobModalOpen.store( false );
				m_BlockingOperation->Reset();
			}

			ImGui::EndPopup();
		}
	}

	void EditorLayer::DrawDistOptionsModal()
	{
		ImGui::OpenPopup( "Specify build options" );

		if( ImGui::BeginPopupModal( "Specify build options", &m_ShowDistBuildOptions, ImGuiWindowFlags_NoSavedSettings ) )
		{
			ImGui::Text( "What would you like to do?" );

			Auxiliary::DrawBoolControl( "Build Shader Bundle", m_ShouldBuildShaderBundle );
			Auxiliary::DrawBoolControl( "Build Asset Bundle", m_ShouldBuildAssetBundle );

			ImGui::Separator();

			ImGui::BeginHorizontal( "##SboOptions" );

			if( ImGui::Button( "Build" ) )
			{
				if( !m_BlockingOperation )
					m_BlockingOperation = Ref<JobProgress>::Create();

				m_JobModalOpen.store( true );
				m_BlockingOperation->SetStatus( "Initialising..." );

				SaveFile();
				SaveProject();

				if( m_ShouldBuildShaderBundle )
					CreateShaderBundleJob();

				// TODO: Think of a better way for this... checking the sizes of the message boxes is not a good thing.
				if( m_MessageBoxes.size() == 0 && m_ShouldBuildAssetBundle )
				{
					CreateAssetBundleJob();
				}

				m_ShowDistBuildOptions = false;
				ImGui::CloseCurrentPopup();
			}

			if( ImGui::Button( "Cancel" ) )
			{
				m_ShowDistBuildOptions = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndHorizontal();

			ImGui::EndPopup();
		}
	}

	void EditorLayer::DrawDeleteNavMeshModal()
	{
		ImGui::OpenPopup( "Purge Navigation Cache" );

		if( ImGui::BeginPopupModal( "Purge Navigation Cache", &m_ShowDeleteNavMeshCachePopup, ImGuiWindowFlags_NoSavedSettings ) )
		{
			ImGui::Text( "Would you like to also delete the Navigation Cache associated with the NavBounds entity?" );

			ImGui::Separator();

			ImGui::BeginHorizontal( "##pncOptions" );

			if( ImGui::Button( "Yes, delete now" ) )
			{
				SharedPtr<Entity> ent = g_ActiveScene->FindEntityByHandle( m_NavMeshEntityToDelete );
				if( ent )
				{
					const std::string filename = std::format( "NavMesh{0}.{1}.srnc", g_ActiveScene->Name, ( uint64_t ) ent->GetUUID() );

					// Rare case, if the user deletes the file manually the deletes the entity this would crash here.
					const std::filesystem::path path = Project::GetActiveProject()->GetFullCachePath() / filename;
					if( std::filesystem::exists( path ) )
					{
						std::filesystem::remove( path );
					}
				}

				g_ActiveScene->DeleteEntity( ent );

				m_NavMeshEntityToDelete = entt::null;
				m_ShowDeleteNavMeshCachePopup = false;
				ImGui::CloseCurrentPopup();
			}

			if( ImGui::Button( "No, I'll do it later" ) )
			{
				{
					SharedPtr<Entity> ent = g_ActiveScene->FindEntityByHandle( m_NavMeshEntityToDelete );
					g_ActiveScene->DeleteEntity( ent );
				}

				m_NavMeshEntityToDelete = entt::null;
				m_ShowDeleteNavMeshCachePopup = false;
				ImGui::CloseCurrentPopup();
			}

			if( ImGui::Button( "Cancel" ) )
			{
				m_NavMeshEntityToDelete = entt::null;
				m_ShowDeleteNavMeshCachePopup = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndHorizontal();

			ImGui::EndPopup();
		}
	}

	void EditorLayer::DrawDebugMsgBoxWindow()
	{
		if( ImGui::Begin( "Debug message box & notification window", &m_ShowDebugMsgBoxWindow ) )
		{
			if( ImGui::Button( "Show debug message box" ) )
			{
				MessageBoxInfo info{ "Dummy msg box", "This is a test" };
				PushMessageBox( info );
			}

			if( ImGui::Button( "Add dummy notification" ) )
			{
				const std::string name = std::format( "This is a test: idx {0}", m_Notifications.size() );
				PushNotification( name );
			}
		}

		ImGui::End();
	}

#define SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( var ) ImGui::Text( #var " %d", var )

	void EditorLayer::DrawEditorDebugWindow()
	{
		if( ImGui::Begin( "Editor debug", &m_ShowEditorDebugWindow ) )
		{
			ImGui::SeparatorText( "Internal state" );

			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_AllowCameraEvents );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_StartedRightClickInViewport );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_ViewportFocused );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_MouseOverViewport );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_OpenEditorSettings );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_ShowImGuiDemoWindow );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_ShowVFSDebug );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_HasPremakePath );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_OpenAssetRegistryDebug );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_OpenLoadedAssetDebug );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_OpenAboutWindow );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_ShowMetadataDebug );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_ShowAssetDependencies );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_ShowRendererWindow );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_ShowSceneRendererWindow );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_ShowSceneDirtyModal );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_ShowUserSettings );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_RequestRuntime );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_ShowCameraFrustum );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_ShowMeshAABB );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_ShowCBThumbnailDebug );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_ShowUndoRedoDebug );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_ShowOperation );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_ShowDistBuildOptions );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_ShouldBuildShaderBundle );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_ShouldBuildAssetBundle );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_WasGizmoUsed );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_LastRuntimeAttemptFailed );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_FullscreenViewport );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_PendingFullscreenChange );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_ShouldRenderCameraPreview );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_DisableViewportMovement );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_ShowDeleteNavMeshCachePopup );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_DebugBreakAlreadyHandled );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_FontChanged );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_ShowRuntimeConsoleWindow );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_ShowDebugMsgBoxWindow );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_ShowEditorDebugWindow );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_ShowMemStatsWindow );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_ShowSetPremakePathModal );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_PendingPremakeJobAfterPathIsSet );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( m_ShowInvalidRecentProjectPathModal );
			SAT_ED_DBG_ADD_TEXT_FOR_INTRL_BOOL_STATE( Input::Get().CanSetCursorMode() );

			ImGui::Text( "m_LastAutoSaveTime %.2f seconds", m_LastAutoSaveTime );
			ImGui::Text( "m_AutoSaveCount %u", m_AutoSaveCount );
		}

		ImGui::End();
	}

	void EditorLayer::DrawSetPremakePathModal()
	{
		static std::filesystem::path s_PremakePath;

		ImGui::OpenPopup( "Premake path not set" );

		if( ImGui::BeginPopupModal( "Premake path not set", nullptr, ImGuiWindowFlags_NoSavedSettings ) )
		{
			ImGui::Text( "The environment variable SAT_PREMAKE_PATH is not set. Saturn needs to know the path of premake in order to build projects." );
			ImGui::Separator();

			const auto pathStr = s_PremakePath.string();
			ImGui::InputText( "##path", ( char* ) pathStr.c_str(), pathStr.size(), ImGuiInputTextFlags_ReadOnly );
			ImGui::SameLine();
			if( ImGui::Button( "..." ) )
			{
#if defined(SAT_PLATFORM_WINDOWS)
				const auto path = Application::Get()->OpenFile( "Application|*.exe" );
#else
				const auto path = Application::Get()->OpenFile( "Application|*" );
#endif
				s_PremakePath = path;
			}

			ImGui::Separator();

			ImGui::BeginHorizontal( "##opthz" );

			{
				// TODO: Check for existence after "..." is pressed.
				Auxiliary::ScopedDisabledFlag disabledIf( s_PremakePath.empty() || !std::filesystem::exists( s_PremakePath ) );

				if( ImGui::Button( "Set" ) )
				{
					Auxiliary::SetEnvironmentVariable( "SATURN_PREMAKE_PATH", s_PremakePath.string() );

					if( m_PendingPremakeJobAfterPathIsSet )
					{
						QueuePremakeJob();
						m_PendingPremakeJobAfterPathIsSet = false;
					}

					m_ShowSetPremakePathModal = false;
					s_PremakePath = L"";
					ImGui::CloseCurrentPopup();
				}
			}

			if( ImGui::Button( "Cancel" ) )
			{
				m_ShowSetPremakePathModal = false;
				s_PremakePath = L"";
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndHorizontal();

			ImGui::EndPopup();
		}
	}

	void EditorLayer::DrawInvalidRecentProjectModal()
	{
		ImGui::OpenPopup( "Recent Project Path Invalid##invalidrp" );

		if( ImGui::BeginPopupModal( "Recent Project Path Invalid##invalidrp", &m_ShowInvalidRecentProjectPathModal, ImGuiWindowFlags_NoSavedSettings ) )
		{
			const auto recentPrjStr = m_InvalidRecentProjectPath.string();
			ImGui::Text( "The path %s is an invalid startup project path or the path is does not exist.", recentPrjStr.c_str() );
			ImGui::Text( "A recent project path must contain the full path to the .sproject file." );

			ImGui::BeginHorizontal( "##optionhz" );

			if( ImGui::Button( "Okay" ) )
			{
				m_InvalidRecentProjectPath.clear();
				m_ShowInvalidRecentProjectPathModal = false;
				ImGui::CloseCurrentPopup();
			}

			if( ImGui::Button( "Remove from recent projects" ) )
			{
				auto& rRecentProjects = EngineSettings::Get().GetAllRecentProjects();

				std::erase_if( rRecentProjects, 
					[ this ]( const auto& rCandidate ) -> bool
				{
					return rCandidate == m_InvalidRecentProjectPath;
				});

				EngineSettingsSerialiser ess;
				ess.Serialise();

				m_ShowInvalidRecentProjectPathModal = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndHorizontal();

			ImGui::EndPopup();
		}
	}

	void EditorLayer::DrawViewport()
	{
		// Viewport Image & Drag and drop handling
		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );

		if( m_PendingFullscreenChange )
		{
			m_FullscreenViewport ^= 1;

			if( m_FullscreenViewport )
			{
				m_PreVPFullscreenSize = m_ViewportSize;
				m_PreVPFullscreenPosition = m_ViewportBounds.Min;
				m_PreVPDockedNodeID = ImGui::GetWindowDockID();

				const ImVec2 size = ImVec2( ( float ) Application::Get()->GetWindow()->GetWidth(), ( float ) Application::Get()->GetWindow()->GetHeight() );

				const auto windowPosition = Application::Get()->GetWindow()->GetPosition();
				const ImVec2 viewportPos = ImVec2( ( float ) windowPosition.x, ( float ) windowPosition.y );

				ImGui::SetNextWindowDockID( 0, ImGuiCond_Always );
				ImGui::SetNextWindowPos( ImVec2( ( float ) windowPosition.x, ( float ) windowPosition.y ) );
				ImGui::SetNextWindowSize( size );
			}
			else
			{
				ImGui::SetNextWindowDockID( m_PreVPDockedNodeID, ImGuiCond_Always );
				ImGui::SetNextWindowPos( m_PreVPFullscreenPosition );
				ImGui::SetNextWindowSize( m_PreVPFullscreenSize );
			}

			m_PendingFullscreenChange = false;
		}

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
		if( m_FullscreenViewport )
			flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings;

		if( m_DisableViewportMovement )
			flags |= ImGuiWindowFlags_NoMove;

		ImGui::Begin( "Viewport", nullptr, flags );

		if( m_ViewportSize != ImGui::GetContentRegionAvail() )
		{
			m_ViewportSize = ImGui::GetContentRegionAvail();

			m_SceneRenderer->SetViewportSize( ( uint32_t ) m_ViewportSize.x, ( uint32_t ) m_ViewportSize.y );
			m_EditorCamera.SetViewportSize( ( uint32_t ) m_ViewportSize.x, ( uint32_t ) m_ViewportSize.y );
			m_SuspendedEditorCamera.SetViewportSize( ( uint32_t ) m_ViewportSize.x, ( uint32_t ) m_ViewportSize.y );

			if( g_AluraCanvas )
				g_AluraCanvas->SetSize( glm::vec2{ m_ViewportSize.x, m_ViewportSize.y } );
		}

		ImGui::PushID( "VIEWPORT_IMAGE" );

		// In the editor we only should flip the image UV, we don't have to flip anything else.
		Auxiliary::Image( m_SceneRenderer->CompositeImage(), m_ViewportSize, { 0, 1 }, { 1, 0 } );

		if( ImGui::BeginDragDropTarget() )
		{
			if( auto payload = ImGui::AcceptDragDropPayload( "CONTENT_BROWSER_ITEM_MULTI_NDT" ) )
			{
				m_SelectionManager->ClearSelection( m_EditorScene.Get(), false );
				m_SelectionManager->EnableMultiSelection();

				const auto contentBrowserPanel = m_ImGuiWindowManager->GetPanel<ContentBrowserPanel>();
				for( const auto& rItem : contentBrowserPanel->GetSelectedItems() )
				{
					switch( rItem->GetAsset()->Type )
					{
						case AssetType::Prefab:
						{
							DndImportPrefab( rItem->GetAsset(), DndFlags_Select );
						} break;

						case AssetType::StaticMesh:
						{
							DndImportStaticMesh( rItem->GetAsset(), DndFlags_Select );
						} break;

						case AssetType::SkeletalMesh:
						{
							DndImportSkeletalMesh( rItem->GetAsset(), DndFlags_Select );
						} break;

						case AssetType::Sound:
						{
							DndImportSound( rItem->GetAsset(), DndFlags_Select );
						} break;

						default:
							break;
					}
				}
			}

			if( auto payload = ImGui::AcceptDragDropPayload( "CONTENT_BROWSER_ITEM_SCENE" ) )
			{
				const UUID* pUUID = ( const UUID* ) payload->Data;
				HandleOpenFileCB( *pUUID );
			}

			if( auto payload = ImGui::AcceptDragDropPayload( "CONTENT_BROWSER_ITEM_PREFAB" ) )
			{
				const UUID* pUUID = ( const UUID* ) payload->Data;

				Ref<Asset> asset = m_AssetManager->FindAsset( *pUUID );

				DndImportPrefab( asset, DndFlags_Select );
			}

			if( auto payload = ImGui::AcceptDragDropPayload( "CONTENT_BROWSER_ITEM_MODEL" ) )
			{
				const UUID* pUUID = ( const UUID* ) payload->Data;

				Ref<Asset> asset = m_AssetManager->FindAsset( *pUUID );
				DndImportStaticMesh( asset, DndFlags_Select );
			}

			if( auto payload = ImGui::AcceptDragDropPayload( "CONTENT_BROWSER_ITEM_SKMODEL" ) )
			{
				const UUID* pUUID = ( const UUID* ) payload->Data;

				Ref<Asset> asset = m_AssetManager->FindAsset( *pUUID );

				DndImportSkeletalMesh( asset, DndFlags_Select );
			}

			if( auto payload = ImGui::AcceptDragDropPayload( "CONTENT_BROWSER_ITEM_SND" ) )
			{
				const UUID* pUUID = ( const UUID* ) payload->Data;

				Ref<Asset> asset = m_AssetManager->FindAsset( *pUUID );
				DndImportSound( asset, DndFlags_Select );
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

		if( m_ShouldRenderCameraPreview )
			Viewport_CameraPreview();

		ImGui::PopStyleVar();

		ImGui::End();
	}

	void EditorLayer::Viewport_GizmoControl()
	{
		if( g_ActiveScene->IsRuntimeRunning() || g_ActiveScene->IsPaused() )
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

	void EditorLayer::Viewport_CameraPreview()
	{
		const ImVec2 minBound = ImGui::GetWindowPos();
		const ImVec2 maxBound = { minBound.x + m_ViewportSize.x, minBound.y + m_ViewportSize.y };

		constexpr ImVec2 size = ImVec2( 400.0f, 225.0f );
		ImGui::SetNextWindowPos( ImVec2( maxBound.x - size.x - 10.0f, maxBound.y - size.y - 10.0f ) );

		ImGui::SetNextWindowSize( size, ImGuiCond_FirstUseEver );
		ImGui::Begin( "CameraPreview##vp_camerapreview", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing );

		const std::string text = "Camera Preview";
		const ImVec2 textSize = ImGui::CalcTextSize( text.data() );

		ImGui::SetCursorPosX( ( size.x - textSize.x ) * 0.5f );
		ImGui::Text( text.data() );

		Auxiliary::Image( m_CameraPreviewSceneRenderer->CompositeImage(), ImGui::GetContentRegionAvail(), { 0, 1 }, { 1, 0 } );
		ImGui::End();
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
				Application::Get()->GetWindow()->ChangeTitle( title );
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
			Application::Get()->GetWindow()->ChangeTitle( title );
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
		if( g_ActiveScene->IsRuntimeRunning() || g_ActiveScene->IsPaused() )
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
			Auxiliary::ScopedDisabledFlag disabledIfRuntime( m_RequestRuntime || !m_GameModule->HasModule() );
		
			if( Auxiliary::ImageButton( m_SyncTexture, ImVec2( 24.0f, 24.0f ) ) )
				HotReloadGame();
		}

		if( ImGui::BeginItemTooltip() )
		{
			ImGui::Text( "Hot Reload (Alt+F5)" );

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

		if( g_AluraCanvas )
			g_AluraCanvas->SetPosition( { minBound.x, minBound.y } );

		const ImVec2 maxBound = { minBound.x + m_ViewportSize.x, minBound.y + m_ViewportSize.y };

		m_ViewportFocused   = ImGui::IsWindowFocused();
		m_MouseOverViewport = ImGui::IsWindowHovered();
		m_AllowCameraEvents = ImGui::IsMouseHoveringRect( minBound, maxBound ) && m_ViewportFocused || m_StartedRightClickInViewport;
		m_ViewportBounds    = ImRect( minBound, maxBound );

		std::vector<SharedPtr<Entity>> selectedEntities = m_SelectionManager->GetSelectionContexts( g_ActiveScene );

		// Calc center of transform.
		glm::vec3 Positions = {};
		glm::quat Rotations = {};
		glm::vec3 Scales = {};

		for( const auto& rEntity : selectedEntities )
		{
			TransformComponent worldSpace = g_ActiveScene->GetWorldSpaceTransform( rEntity );
			Positions += worldSpace.Position;
			Rotations += worldSpace.GetRotation();
			Scales += worldSpace.Scale;
		}

		Positions /= selectedEntities.size();
		Rotations /= static_cast< float >( selectedEntities.size() );
		Scales /= selectedEntities.size();

		glm::mat4 centerPoint = glm::translate( glm::mat4( 1.0f ), Positions ) * glm::toMat4( Rotations ) * glm::scale( glm::mat4( 1.0f ), Scales );

		///////////////////

		if( selectedEntities.size() && m_GizmoOperation != 0 )
		{
			ImGuizmo::SetOrthographic( false );
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect( minBound.x, minBound.y, m_ViewportSize.x, m_ViewportSize.y );

			const glm::mat4 Projection = m_SceneRenderer->GetRendererCamera().pCamera->ProjectionMatrix();
			const glm::mat4 View = m_SceneRenderer->GetRendererCamera().ViewMatrix;

			ImGuizmo::Manipulate( glm::value_ptr( View ), glm::value_ptr( Projection ), ( ImGuizmo::OPERATION ) m_GizmoOperation, ImGuizmo::LOCAL, glm::value_ptr( centerPoint ), nullptr );

			if( !ImGui::IsWindowDocked() && ImGuizmo::IsOver() )
			{
				m_DisableViewportMovement = true;
			}
			else if( m_DisableViewportMovement && !ImGuizmo::IsOver() )
			{
				m_DisableViewportMovement = false;
			}

			if( ImGuizmo::IsUsing() )
			{
				if( selectedEntities.size() == 1 )
				{
					SharedPtr<Entity> entity = selectedEntities[ 0 ];
					auto& tc = entity->GetComponent<TransformComponent>();

					// Convert world-space to parent-local space:
					//
					// Right, imagine we have:
					// 
					// Parent at [0, 10, 10] (world)
					// Child at [0, 12, 12] (world)
					// 
					// then after the following code below it would become:
					// centerPoint * inverse( parent ) = [0, 2, 2]
					// 
					// as the inverse acts as a subtraction here.
					//
					if( SharedPtr<Entity> parent = entity->TryGetParent() )
					{
						// But, make sure we get the parent's world space if that parent has a parent and so on.
						const glm::mat4 parentTransform = g_ActiveScene->GetWorldSpaceTransform( parent );
						centerPoint = glm::inverse( parentTransform ) * centerPoint;
					}

					// Store original transform for undo/redo
					if( !m_WasGizmoUsed )
					{
						m_GizmoOrignalTransforms[ entity->GetHandle() ] = tc.GetTransform();
					}

					glm::vec3 translation;
					glm::vec3 rotation;
					glm::vec3 scale;
					Maths::DecomposeTransform( centerPoint, translation, rotation, scale );

					switch( m_GizmoOperation )
					{
						case ImGuizmo::TRANSLATE:
						{
							tc.Position = translation;
						} break;

						case ImGuizmo::ROTATE:
						{
							glm::vec3 rotationEuler = tc.GetRotationEuler();

							// Normalise the angle to [-180 to 180],
							// this stops us from adding the rotation over and over again,
							// and avoid us having rotational values of 600 degrees when we
							// convert this to degrees.
							rotationEuler.x = fmodf( rotationEuler.x + glm::pi<float>(), glm::two_pi<float>() ) - glm::pi<float>();
							rotationEuler.y = fmodf( rotationEuler.y + glm::pi<float>(), glm::two_pi<float>() ) - glm::pi<float>();
							rotationEuler.z = fmodf( rotationEuler.z + glm::pi<float>(), glm::two_pi<float>() ) - glm::pi<float>();

							glm::vec3 delta = rotation - rotationEuler;

							if( fabs( delta.x ) < 0.001F ) delta.x = 0.0F;
							if( fabs( delta.y ) < 0.001F ) delta.y = 0.0F;
							if( fabs( delta.z ) < 0.001F ) delta.z = 0.0F;

							tc.SetRotation( tc.GetRotationEuler() += delta );
						} break;

						case ImGuizmo::SCALE:
						{
							tc.Scale = scale;
						} break;
					}
				}
				else
				{
					for( SharedPtr<Entity>& rEntity : selectedEntities )
					{

					}
				}

				m_WasGizmoUsed = true;
			}
			else if( m_WasGizmoUsed ) // Stopped using
			{
				m_EditorScene->MarkDirty();

				if( !ImGui::IsWindowDocked() || m_DisableViewportMovement )
				{
					m_DisableViewportMovement = false;
				}

				/*
				for( const auto& [handle, transform] : m_GizmoOrignalTransforms )
				{
					const auto& [newPosition, newRotation, newScale] = m_GizmoModifiedTransforms[ handle ];

					SharedPtr<Entity> entity = m_EditorScene->FindEntityByHandle( handle );
					TransformComponent& tc = entity->GetComponent<TransformComponent>();

					glm::mat4 newTransform = glm::translate( glm::mat4( 1.0f ), newPosition )
						* glm::toMat4( glm::quat( newRotation ) )
						* glm::scale( glm::mat4( 1.0f ), newScale );

					Ref<UndoRedoActionModifyTransformation> action = Ref<UndoRedoActionModifyTransformation>::Create( entity, transform, newTransform );
					m_GlobalUndoRedoGroup->AddAction( action, ( uint64_t ) entity->GetHandle() );

					if( entity->HasComponent<NavigationMeshSpecificationComponent>() )
					{
						SharedPtr<NavBoundsEntity> bounds = entity.As<NavBoundsEntity>();
						bounds->GatherGeometryAndBuild();
					}
				}
				*/

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
		std::filesystem::path WorkingDir = SaturnDir / "Saturn-ProjectBrowser";

		const std::string binaryFolderName = std::format( "{0}-{1}-x86_64", Application::GetCurrentConfigName(), Application::GetCurrentPlatformBinaryName() );

		SaturnDir /= L"bin";
		SaturnDir /= binaryFolderName;
		SaturnDir /= L"Saturn-ProjectBrowser";

#if defined( SAT_PLATFORM_WINDOWS )
		SaturnDir /= L"Saturn-ProjectBrowser.exe";
#else
		SaturnDir /= L"Saturn-ProjectBrowser";
#endif
		DetachedProcess dp( SaturnDir.wstring(), WorkingDir );
		Application::Get()->Close();
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
			m_EventAfterPopup = []() { Application::Get()->Close(); };

			ImGui::OpenPopup( "SceneDirtyPopup" );

			Application::Get()->GetWindow()->FlashAttention();
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

			if( ( rInfo.Buttons & ( uint32_t ) MessageBoxButtons_Ok ) != 0 )
			{
				if( buttonIndex > 0 ) { ImGui::Spring(); ++buttonIndex; }

				if( ImGui::Button( "OK" ) )
				{
					ImGui::CloseCurrentPopup();
					PopMessageBox();
				}
			}

			if( ( rInfo.Buttons & ( uint32_t ) MessageBoxButtons_Cancel ) != 0 )
			{
				if( buttonIndex > 0 ) { ImGui::Spring(); ++buttonIndex; }

				if( ImGui::Button( "Cancel" ) )
				{
					ImGui::CloseCurrentPopup();
					PopMessageBox();
				}
			}

			if( ( rInfo.Buttons & ( uint32_t ) MessageBoxButtons_Exit ) != 0 )
			{
				if( buttonIndex > 0 ) { ImGui::Spring(); ++buttonIndex; }

				if( ImGui::Button( "Exit" ) )
				{
					ImGui::CloseCurrentPopup();
					PopMessageBox();
				}
			}

			if( ( rInfo.Buttons & ( uint32_t ) MessageBoxButtons_Yes ) != 0 )
			{
				if( buttonIndex > 0 ) { ImGui::Spring(); ++buttonIndex; }

				if( ImGui::Button( "Yes" ) )
				{
					ImGui::CloseCurrentPopup();
					PopMessageBox();
				}
			}

			if( ( rInfo.Buttons & ( uint32_t ) MessageBoxButtons_No ) != 0 )
			{
				if( buttonIndex > 0 ) { ImGui::Spring(); ++buttonIndex; }

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
#if defined(SAT_PLATFORM_WINDOWS)
					path = Application::Get()->OpenFile( "Application|*.exe" );
#elif defined(SAT_PLATFORM_LINUX)
					path = Application::Get()->OpenFile( "Application|*" );
#endif
				}

				if( !path.empty() )
				{
					if( ImGui::Button( "Set" ) )
					{
						Auxiliary::SetEnvironmentVariable( "SATURN_PREMAKE_PATH", path.string().c_str() );

						ImGui::CloseCurrentPopup();
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

		Application::Get()->GetWindow()->FlashAttention();

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
		const auto defaultFontID = ActiveProject->GetDefaultFontAsset();

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

		if( defaultFontID == 0 )
		{
			MessageBoxInfo msgBox
			{
				.Title = "Warning",
				.Text = "No font default font asset was selected in the Project Defaults, you can still build only if this project does not contain any UI!, if the project does use Alura and no font is selected the Distribution application will verify.",
				.Buttons = MessageBoxButtons_Ok
			};

			PushMessageBox( msgBox );
		}

#if defined(SAT_WITH_STEAM)
		if( ActiveProject->GetOnlineAppID() == 480 )
		{
			MessageBoxInfo msgBox
			{
				.Title = "Warning",
				.Text = "You are unable to ship an application with a Steam App ID of 480. (Spacewar)",
				.Buttons = MessageBoxButtons_Ok
			};

			PushMessageBox( msgBox );
		}
#endif

		return result;
	}

	void EditorLayer::CreateShaderBundleJob()
	{
		JobSystem::Get().QueueJob( [ this ]()
		{
			m_BlockingOperation->SetStatus( "Building Shader bundle..." );
			BuildShaderBundle();
		} );
	}

	void EditorLayer::CreateAssetBundleJob()
	{
		JobSystem::Get().QueueJob( [ this ]()
		{
			if( const auto result = AssetBundle::BundleAssets( m_BlockingOperation ); result != AssetBundleResult::Success )
			{
				Application::Get()->GetWindow()->FlashAttention();

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
		m_ImGuiWindowManager->GetPanel<ContentBrowserPanel>()->ShowOrHide( ImGuiHideWindowFlags::Hide );
	}

	void EditorLayer::ShowOrHideSceneHierarchyPanel()
	{
		m_ImGuiWindowManager->GetPanel<SceneHierarchyPanel>()->ShowOrHide( ImGuiHideWindowFlags::Hide );
	}

	void EditorLayer::ShowOrHideRTCmdWindow() 
	{
		m_ImGuiWindowManager->GetPanel<RuntimeCommandWindow>()->ShowOrHide( ImGuiHideWindowFlags::Hide );
	}

	void EditorLayer::ShowOrHideMemStatsWindow()
	{
		m_ImGuiWindowManager->GetPanel<MemoryStatisticsWindow>()->ShowOrHide( ImGuiHideWindowFlags::Hide );
	}

	glm::vec2 EditorLayer::ConvertMouseToViewportNDC()
	{
		auto [mx, my] = ImGui::GetMousePos();
		const auto& viewportBounds = m_ViewportBounds;

		mx -= m_ViewportBounds.Min.x;
		my -= m_ViewportBounds.Min.y;

		return { ( mx / m_ViewportSize.x ) * 2.0f - 1.0f, ( ( my / m_ViewportSize.y ) * 2.0f - 1.0f ) * -1.0f };
	}

	std::pair<glm::vec3, glm::vec3> EditorLayer::RayCast( float mx, float my )
	{
		const glm::vec4 mouseClipPos = { mx, my, -1.0f, 1.0f };

		glm::mat4 inverseProj{};
		glm::mat3 inverseView{};
		glm::vec3 rayPos{};

		// Use suspended editor camera if we are suspended.
		if( ( m_RuntimeScene && m_RuntimeScene->IsPausedOrSuspended() ) )
		{
			inverseProj = glm::inverse( m_SuspendedEditorCamera.ProjectionMatrix() );
			inverseView = glm::inverse( glm::mat3( m_SuspendedEditorCamera.ViewMatrix() ) );
			rayPos = m_SuspendedEditorCamera.GetPosition();
		}
		else
		{
			inverseProj = glm::inverse( m_EditorCamera.ProjectionMatrix() );
			inverseView = glm::inverse( glm::mat3( m_EditorCamera.ViewMatrix() ) );
			rayPos = m_EditorCamera.GetPosition();
		}

		const glm::vec4 ray = inverseProj * mouseClipPos;
		const glm::vec3 rayDir = inverseView * glm::vec3( ray );

		return { rayPos, rayDir };
	}

	void EditorLayer::DndImportPrefab( Ref<Asset> asset, DragNDropAssetFlags flags /*= DndFlags_ClearSelection*/ )
	{
		Ref<Prefab> prefabAsset = m_AssetManager->GetAssetAs<Prefab>( asset->ID );

		CreateEntityParameters createEntityParameters{};
		createEntityParameters.Tag = asset->Name;

		auto entity = m_EditorScene->CreatePrefab( prefabAsset, createEntityParameters );
		m_EditorScene->MarkDirty();

		PlaceEntityRelativeToMousePos( entity );

		if( ( flags & DndFlags_ClearSelection ) )
			m_SelectionManager->ClearSelection( m_EditorScene.Get(), true );

		if( ( flags & DndFlags_Select ) )
			m_SelectionManager->Select( entity );
	}

	void EditorLayer::DndImportStaticMesh( Ref<Asset> asset, DragNDropAssetFlags flags /*= DndFlags_ClearSelection*/ )
	{
		Ref<StaticMesh> meshAsset = m_AssetManager->GetAssetAs<StaticMesh>( asset->ID );

		SharedPtr<Entity> entity = m_EditorScene->CreateEntity( asset->Name );

		auto& rMeshComponent = entity->AddComponent<StaticMeshComponent>();
		rMeshComponent.Mesh = meshAsset;
		rMeshComponent.MaterialRegistry = Ref<MaterialRegistry>::Create( meshAsset );

		PlaceEntityRelativeToMousePos( entity );

		m_EditorScene->MarkDirty();

		if( ( flags & DndFlags_ClearSelection ) )
			m_SelectionManager->ClearSelection( m_EditorScene.Get(), true );

		if( ( flags & DndFlags_Select ) )
			m_SelectionManager->Select( entity );
	}

	void EditorLayer::DndImportSkeletalMesh( Ref<Asset> asset, DragNDropAssetFlags flags /*= DndFlags_ClearSelection*/ )
	{
		Ref<SkeletalMesh> meshAsset = m_AssetManager->GetAssetAs<SkeletalMesh>( asset->ID );

		SharedPtr<Entity> entity = m_EditorScene->CreateEntity( asset->Name );

		auto& rMeshComponent = entity->AddComponent<SkeletalMeshComponent>();
		rMeshComponent.Mesh = meshAsset;
		rMeshComponent.MaterialRegistry = Ref<MaterialRegistry>::Create( meshAsset );

		PlaceEntityRelativeToMousePos( entity );

		m_EditorScene->MarkDirty();

		if( ( flags & DndFlags_ClearSelection ) )
			m_SelectionManager->ClearSelection( m_EditorScene.Get(), true );

		if( ( flags & DndFlags_Select ) )
			m_SelectionManager->Select( entity );
	}

	void EditorLayer::DndImportSound( Ref<Asset> asset, DragNDropAssetFlags flags /*= DndFlags_ClearSelection*/ )
	{
		SharedPtr<Entity> entity = m_EditorScene->CreateEntity( asset->Name );

		auto& rAudioPlayerComponent = entity->AddComponent<AudioPlayerComponent>();
		rAudioPlayerComponent.SpecAssetID = asset->ID;

		PlaceEntityRelativeToMousePos( entity );

		m_EditorScene->MarkDirty();

		if( ( flags & DndFlags_ClearSelection ) )
			m_SelectionManager->ClearSelection( m_EditorScene.Get(), true );

		if( ( flags & DndFlags_Select ) )
			m_SelectionManager->Select( entity );
	}

	void EditorLayer::PlaceEntityRelativeToMousePos( SharedPtr<Entity> entity )
	{
		// TODO: We will want to do a raycast so the Z axis is relative to where the mouse hits
		//		 for example, if we have an object that is +10 meters away, with this current method it will
		//		 not place it there and will only place from where cameras clip is.
		//		 If we did a raycast we could detect where the mouse was, shoot a ray, and see what it hits, if it
		//		 hits something get the Z coord and set it, if not we use the cameras clip.
		//
		const auto viewportMouse = ConvertMouseToViewportNDC();
		if( viewportMouse.x > -1.0f && viewportMouse.x < 1.0f && viewportMouse.y > -1.0f && viewportMouse.y < 1.0f )
		{
			const glm::vec4 rayClip = glm::vec4( viewportMouse.x, viewportMouse.y, -1.0f, 1.0f );
			glm::vec4 rayEye = glm::inverse( m_EditorCamera.m_Projection ) * rayClip;
			rayEye = glm::vec4( rayEye.x, rayEye.y, -1.0f, 0.0f );

			const glm::vec3 rayWorld = glm::normalize(
				glm::vec3( glm::inverse( m_EditorCamera.m_ViewMatrix ) * rayEye )
			);

			const glm::vec3 rayOrigin = m_EditorCamera.GetPosition();
			entity->SetPosition( rayOrigin + rayWorld * 10.0f );
		}
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

	void EditorLayer::PushNotification( const std::string& rName, float lifetime /*= 5.0f */ )
	{
		m_Notifications.emplace_back( rName, lifetime );
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

		const std::string windowID = std::format( "##EDITOR_NOFITICATION/{0}", ( uint64_t ) rInfo.ID );
		ImGui::Begin( windowID.c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDocking );

		ImGui::BeginHorizontal( ( int ) rInfo.ID );

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
				++rIt;
			}
		}
	}

	void EditorLayer::CloseEditorAndOpenNewProj( const std::filesystem::path& rProjectPath )
	{
		std::filesystem::path args = Auxiliary::GetEnvironmentVariableWs( L"SATURN_DIR" );
		std::filesystem::path workingDir = args / "Saturn-Editor";

		const std::string binaryFolderName = std::format( "{0}-{1}-x86_64", Application::GetCurrentConfigName(), Application::GetCurrentPlatformBinaryName() );

		args /= L"bin";
		args /= binaryFolderName;
		args /= L"Saturn-Editor";

#if defined( SAT_PLATFORM_WINDOWS )
		args /= L"Saturn-Editor.exe";
#elif defined( SAT_PLATFORM_MACOS )
		args /= L"Saturn-Editor.app";
#else
		args /= L"Saturn-Editor";
#endif

		args += std::format( " {}", rProjectPath.string() );

		DetachedProcess dp( args.wstring(), workingDir );
		Application::Get()->Close();
	}

}
