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
#include "EditorLayer.h"

#include <Saturn/Project/Project.h>

#include <Saturn/ImGui/ImGuiAuxiliary.h>
#include <Saturn/Vulkan/SceneRenderer.h>
#include <Saturn/ImGui/TitleBar.h>
#include <Saturn/ImGui/MaterialAssetViewer/MaterialAssetViewer.h>
#include <Saturn/ImGui/PrefabViewer.h>
#include <Saturn/ImGui/Panel/Panel.h>
#include <Saturn/ImGui/Panel/PanelManager.h>
#include <Saturn/ImGui/EditorIcons.h>

#include <Saturn/Serialisation/SceneSerialiser.h>
#include <Saturn/Serialisation/ProjectSerialiser.h>
#include <Saturn/Serialisation/EngineSettingsSerialiser.h>
#include <Saturn/Serialisation/AssetRegistrySerialiser.h>
#include <Saturn/Serialisation/AssetSerialisers.h>
#include <Saturn/Serialisation/AssetBundle.h>

#include <Saturn/Physics/PhysicsFoundation.h>

#include <Saturn/Vulkan/ShaderBundle.h>
#include <Saturn/Vulkan/Renderer2D.h>
#include <Saturn/Vulkan/VulkanImageAux.h>

#include <Saturn/Core/EnvironmentVariables.h>

#include <ImGuizmo/ImGuizmo.h>

#include <imspinner/imspinner.h>

#include <Saturn/Core/Math.h>
#include <Saturn/Core/StringAuxiliary.h>
#include <Saturn/Core/EngineSettings.h>
#include <Saturn/Core/OptickProfiler.h>

#include <Saturn/Asset/AssetRegistry.h>
#include <Saturn/Asset/AssetManager.h>
#include <Saturn/Asset/Prefab.h>

#include <Saturn/GameFramework/Core/GameModule.h>
#include <Saturn/GameFramework/Core/ClassMetadataHandler.h>

#include <Saturn/Core/Renderer/RenderThread.h>
#include <Saturn/Core/VirtualFS.h>

#include <Saturn/Audio/AudioSystem.h>

#include <Saturn/Premake/Premake.h>
#include <Saturn/Core/Process.h>

#include <Saturn/Audio/SoundGroup.h>

#include <Saturn/Core/Ruby/RubyWindow.h>
#include <Saturn/Core/Ruby/RubyAuxiliary.h>

#include <glm/gtc/type_ptr.hpp>

namespace Saturn {

	static constexpr inline bool operator==( const ImVec2& lhs, const ImVec2& rhs ) { return lhs.x == rhs.x && lhs.y == rhs.y; }
	static constexpr inline bool operator!=( const ImVec2& lhs, const ImVec2& rhs ) { return !( lhs == rhs ); }

	EditorLayer::EditorLayer() 
		: m_EditorCamera( 45.0f, 1280.0f, 720.0f, 0.1f, 1000.0f ), 
		m_FallbackCamera( 45.0f, 1280.0f, 720.0f, 0.1f, 1000.0f ), 
		m_EditorScene( Ref<Scene>::Create() )
	{
		Scene::SetActiveScene( m_EditorScene.Get() );

		m_EditorCamera.SetActive( true );

		// Init Physics
		PhysicsFoundation* pPhysicsFoundation = new PhysicsFoundation();
		pPhysicsFoundation->Init();
		
		// Editor Application (EditorApplication.cpp) should of loaded a project but if not assert.
		SAT_CORE_ASSERT( Project::GetActiveProject(), "No project was given." );
		
		VirtualFS::Get().MountBase( Project::GetActiveConfig().Name, Project::GetActiveProject()->GetRootDir() );

		m_AssetManager = Ref<AssetManager>::Create();

		Project::GetActiveProject()->CheckMissingAssetRefs();

		m_GameModule = new GameModule();
	}

	void EditorLayer::OnAttach()
	{
		m_CheckerboardTexture = Ref< Texture2D >::Create( "content/textures/editor/checkerboard.tga", AddressingMode::Repeat );

		m_StartRuntimeTexture = Ref< Texture2D >::Create( "content/textures/editor/Play.png", AddressingMode::ClampToEdge );
		m_EndRuntimeTexture = Ref< Texture2D >::Create( "content/textures/editor/Stop.png", AddressingMode::ClampToEdge );

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
		
		// Create Panel Manager.
		m_PanelManager = Ref<PanelManager>::Create();

		m_PanelManager->AddPanel( Ref<SceneHierarchyPanel>::Create() );
		m_PanelManager->AddPanel( Ref<ContentBrowserPanel>::Create() );

		m_TitleBar = Ref<TitleBar>::Create();

		Ref<SceneHierarchyPanel> hierarchyPanel = m_PanelManager->GetPanel<SceneHierarchyPanel>();
		hierarchyPanel->SetContext( m_EditorScene );
		hierarchyPanel->SetSelectionChangedCallback( SAT_BIND_EVENT_FN( SelectionChanged ) );
		hierarchyPanel->OpenWindow();

		Ref<ContentBrowserPanel> contentBrowserPanel = m_PanelManager->GetPanel<ContentBrowserPanel>();
		contentBrowserPanel->OpenWindow();

		// Setup content browser panel at project dir.
		auto& rUserSettings = EngineSettings::Get();
		contentBrowserPanel->ResetPath( Project::GetActiveProject()->GetRootDir() );

		m_TitleBar->AddMenuBarFunction( SAT_BIND_EVENT_FN( DrawTitlebarOptions ) );
		m_TitleBar->AddOnExitFunction( SAT_BIND_EVENT_FN( OnTitlebarExit ) );

		// Now open the startup scene
		OpenFile( Project::GetActiveProject()->GetConfig().StartupSceneID );

		std::string title = std::format( "{0} - Saturn", Project::GetActiveConfig().Name );
		Application::Get().GetWindow()->ChangeTitle( title );
	}

	void EditorLayer::OnDetach()
	{
		EditorIcons::Clear();
		m_CheckerboardTexture = nullptr;
		m_PointLightTexture = nullptr;
	}

	EditorLayer::~EditorLayer()
	{
		AssetViewer::Terminate();

		m_TitleBar = nullptr;
	
		m_PanelManager = nullptr;

		Application::Get().PrimarySceneRenderer().SetCurrentScene( nullptr );
		
		if( m_RuntimeScene ) 
		{	
			m_RuntimeScene->OnRuntimeEnd();
			m_RuntimeScene = nullptr;
		}

		m_EditorScene = nullptr;
		m_AssetManager = nullptr;

		VirtualFS::Get().UnmountBase( Project::GetActiveConfig().Name );

		delete m_GameModule;
		m_GameModule = nullptr;
	}

	void EditorLayer::OnUpdate( Timestep time )
	{
		SAT_PF_EVENT();

		if( Input::Get().MouseButtonPressed( RubyMouseButton::Right ) && !m_StartedRightClickInViewport && m_ViewportFocused && m_MouseOverViewport )
			m_StartedRightClickInViewport = true;

		if( !Input::Get().MouseButtonPressed( RubyMouseButton::Right ) )
			m_StartedRightClickInViewport = false;

		Input::Get().SetCanSetCursorMode( m_RuntimeScene == nullptr ? m_AllowCameraEvents : m_MouseOverViewport );

		///////////////////////////////

		Ref<SceneHierarchyPanel> hierarchyPanel = m_PanelManager->GetPanel<SceneHierarchyPanel>();

		if( m_RequestRuntime )
		{
			if( !m_RuntimeScene )
			{
				m_RuntimeScene = Ref<Scene>::Create();
				Scene::SetActiveScene( m_RuntimeScene.Get() );

				m_EditorScene->CopyScene( m_RuntimeScene );

				Input::Get().SetCanSetCursorMode( true );

				m_RuntimeScene->OnRuntimeStart();

				hierarchyPanel->SetContext( m_RuntimeScene );

				Application::Get().PrimarySceneRenderer().SetCurrentScene( m_RuntimeScene.Get() );

				m_EditorCamera.SetActive( false );
			}
		}
		else
		{
			if( m_RuntimeScene && m_RuntimeScene->RuntimeRunning )
			{
				m_RuntimeScene->OnRuntimeEnd();
				Scene::SetActiveScene( m_EditorScene.Get() );

				hierarchyPanel->SetContext( m_EditorScene );

				m_RuntimeScene = nullptr;

				Application::Get().PrimarySceneRenderer().SetCurrentScene( m_EditorScene.Get() );
			}
		}

		if( m_RuntimeScene ) 
		{
			// TEMP:
			Renderer2D::Get().PreRender();

			m_RuntimeScene->OnUpdate( time );
			m_RuntimeScene->OnRenderRuntime( time, Application::Get().PrimarySceneRenderer() );
		}
		else 
		{
			m_EditorCamera.SetActive( m_AllowCameraEvents );
			m_EditorCamera.OnUpdate( time );

			m_EditorScene->OnUpdate( time );
			m_EditorScene->OnRenderEditor( m_EditorCamera, time, Application::Get().PrimarySceneRenderer() );
		}

		// Render scenes in other asset viewers
		AssetViewer::Update( time );
	}

	void EditorLayer::OnImGuiRender()
	{
		SAT_PF_EVENT();

		// Draw dockspace.
		ImGuiIO& io = ImGui::GetIO();
		ImGuiViewport* pViewport = ImGui::GetWindowViewport();
		ImGui::DockSpaceOverViewport( pViewport );
		
		if( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) || ( ImGui::IsMouseClicked( ImGuiMouseButton_Right ) && !m_StartedRightClickInViewport ) )
		{
			if( !m_RuntimeScene )
			{
				ImGui::FocusWindow( GImGui->HoveredWindow );
				Input::Get().SetCursorMode( RubyCursorMode::Normal );
			}
		}

		m_TitleBar->Draw();
		m_PanelManager->DrawAllPanels();
		
		if( m_ShowImGuiDemoWindow )     ImGui::ShowDemoWindow( &m_ShowImGuiDemoWindow );
		if( m_ShowUserSettings )        DrawProjectSettingsWindow();
		if( m_OpenAssetRegistryDebug )  DrawAssetRegistryDebug();
		if( m_OpenLoadedAssetDebug )    DrawLoadedAssetsDebug();
		if( m_OpenEditorSettings )      DrawEditorSettings();
		if( m_ShowVFSDebug )            DrawVFSDebug();
		if( m_OpenAboutWindow )         DrawAboutWindow();
		if( m_MessageBoxes.size() )     HandleMessageBoxes();
		if( m_ShowSceneRendererWindow ) DrawSceneRendererWindow();
		if( m_ShowRendererWindow )		DrawRendererWindow();
		if( m_ShowMetadataDebug )       DrawMetadataDebug();
		if( m_ShowSceneDirtyModal )     DrawSceneDirtyPopup();

		AssetViewer::Draw();

		if( m_JobModalOpen )
		{
			ImGui::OpenPopup( "Blocking Action" );
		}

		if( ImGui::BeginPopupModal( "Blocking Action", &m_JobModalOpen, ImGuiWindowFlags_NoSavedSettings ) )
		{
			if( m_BlockingOperation->GetTitle().empty() )
				ImGui::Text( "Please wait for the operation to complete..." );
			else
				ImGui::Text( m_BlockingOperation->GetTitle().c_str() );

			ImGui::Separator();

			if( std::string status = m_BlockingOperation->GetStatus(); !status.empty() )
			{
				ImGui::Text( status.c_str() );
			}

			ImGui::Separator();

			ImGui::BeginHorizontal( "##ItemsH" );

			ImSpinner::SpinnerAng( "##OPERATION_SPINNER", 25.0f / 2.0f, 2.0f, ImSpinner::white, ImSpinner::half_white, 8.6F );

			ImGui::Spring();

			if( float percent = m_BlockingOperation->GetProgress(); percent >= 1.0f )
			{
				ImGui::ProgressBar( percent / 100 );
			}

			ImGui::EndHorizontal();
		
			if( m_BlockingOperation->Completed() )
			{
				m_JobModalOpen = false;
				m_BlockingOperation->Reset();
			}

			ImGui::EndPopup();
		}
		
		// Deprecated -- user will set mesh materials in Scene Hierarchy Panel
		//               user can change material data in material node editor.
		//				 So for now there is no way for a mesh to have the same material but have different properties.
		//DrawMaterials();
		
		DrawViewport();

		//CheckMissingEnv();
	}

	void EditorLayer::OnEvent( RubyEvent& rEvent )
	{
		// If the mouse is over the viewport allow for the scroll event to happen
		// The scroll event does not care if the camera is active or not.
		if( m_MouseOverViewport ) m_EditorCamera.OnEvent( rEvent );
		
		if( false )	m_FallbackCamera.OnEvent( rEvent );

		AssetViewer::ProcessEvent( rEvent );

		if( rEvent.Type == RubyEventType::KeyPressed ) OnKeyPressed( (RubyKeyEvent&)rEvent );
	}

	void EditorLayer::SaveFileAs()
	{
		// TODO: Support Saving scene as!
		auto res = Application::Get().SaveFile( "Saturn Scene file (*.scene, *.sc)\0*.scene; *.sc\0" );

		SceneSerialiser serialiser( m_EditorScene );
		serialiser.Serialise();
	}

	void EditorLayer::SaveFile()
	{
		auto fullPath = Project::GetActiveProject()->FilepathAbs( m_EditorScene->Path );

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

	void EditorLayer::OpenFile( AssetID id )
	{
		Ref<SceneHierarchyPanel> hierarchyPanel = m_PanelManager->GetPanel<SceneHierarchyPanel>();

		Ref<Scene> newScene = Ref<Scene>::Create();
		GActiveScene = newScene.Get();

		hierarchyPanel->ClearSelection();
		hierarchyPanel->SetContext( nullptr );

		Ref<Asset> asset = id == 0 ? nullptr : AssetManager::Get().FindAsset( id );
		
		if( id != 0 )
		{
			SceneSerialiser serialiser( newScene );
			serialiser.Deserialise( asset->Path );
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
	
		GActiveScene = m_EditorScene.Get();

		hierarchyPanel->SetContext( m_EditorScene );
		newScene = nullptr;

		Application::Get().PrimarySceneRenderer().SetCurrentScene( m_EditorScene.Get() );
	}

	void EditorLayer::SaveProject()
	{
		ProjectSerialiser ps( Project::GetActiveProject() );
		ps.Serialise( Project::GetActiveProject()->GetConfig().Path );

		AssetRegistrySerialiser ars;
		ars.Serialise( AssetManager::Get().GetAssetRegistry() );
	}

	void EditorLayer::SelectionChanged( Ref<Entity> e )
	{
	}

	void EditorLayer::ViewportSizeCallback( uint32_t Width, uint32_t Height )
	{
	}

	bool EditorLayer::OnKeyPressed( RubyKeyEvent& rEvent )
	{
		switch( rEvent.GetScancode() )
		{
			case RubyKey::Delete:
			{
				Ref<SceneHierarchyPanel> hierarchyPanel = m_PanelManager->GetPanel<SceneHierarchyPanel>();

				if( hierarchyPanel && !m_RuntimeScene )
				{
					// Because of our ref system, the entity will be deleted when we clear the selections.
					// What we are really doing here is freeing it from the registry.

					for( auto& rEntity : hierarchyPanel->GetSelectionContexts() )
					{
						GActiveScene->DeleteEntity( rEntity );
					}

					hierarchyPanel->ClearSelection();

					GActiveScene->MarkDirty();
				}
			} break;

			case RubyKey::Q:
				if( m_MouseOverViewport && !m_StartedRightClickInViewport )
					m_GizmoOperation = 0;
				break;
	
			case RubyKey::W:
				if( m_MouseOverViewport && !m_StartedRightClickInViewport )
					m_GizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
				break;

			case RubyKey::E:
				if( m_MouseOverViewport && !m_StartedRightClickInViewport )
					m_GizmoOperation = ImGuizmo::OPERATION::ROTATE;
				break;
			
			case RubyKey::R:
				if( m_MouseOverViewport && !m_StartedRightClickInViewport )
					m_GizmoOperation = ImGuizmo::OPERATION::SCALE;
				break;
		}

		if( Input::Get().KeyPressed( RubyKey::Ctrl ) && !m_RuntimeScene )
		{
			switch( rEvent.GetScancode() )
			{
				case RubyKey::D:
				{
					Ref<SceneHierarchyPanel> hierarchyPanel = m_PanelManager->GetPanel<SceneHierarchyPanel>();

					if( hierarchyPanel )
					{
						for( const auto& rEntity : hierarchyPanel->GetSelectionContexts() )
						{
							GActiveScene->DuplicateEntity( rEntity );
						}

						GActiveScene->MarkDirty();
					}

				} break;

				// TODO: Support more than one selection.
				case RubyKey::F:
				{
					Ref<SceneHierarchyPanel> hierarchyPanel = m_PanelManager->GetPanel<SceneHierarchyPanel>();

					if( hierarchyPanel )
					{
						auto& selectedEntities = hierarchyPanel->GetSelectionContexts();

						glm::vec3 Positions = {};
						for( auto& rEntity : selectedEntities )
						{
							TransformComponent worldSpace = GActiveScene->GetWorldSpaceTransform( rEntity );
							Positions += worldSpace.Position;
						}

						Positions /= selectedEntities.size();

						m_EditorCamera.Focus( Positions );
					}
				} break;

				case RubyKey::S:
				{
					SaveFile();
				} break;
			}

			if( Input::Get().KeyPressed( RubyKey::Shift ) )
			{
				switch( rEvent.GetScancode() )
				{
					case RubyKey::S:
					{
						SaveFileAs();
					} break;
				}
			}

#if defined(SAT_RELEASE)
			if( Input::Get().KeyPressed( RubyKey::Alt ) && m_RuntimeScene == nullptr )
			{
				switch( rEvent.GetScancode() )
				{
					case RubyKey::F5:
					{
						HotReloadGame();
					} break;
				}
			}
#endif
		}

		return true;
	}

	static bool s_OpenAssetFinderPopup = false;
	static AssetID s_AssetFinderID = 0;

	void EditorLayer::DrawProjectSettingsWindow()
	{
		static bool ShouldSaveProject = false;

		ImGuiIO& rIO = ImGui::GetIO();

		auto& userSettings = EngineSettings::Get();
		Ref<Project> ActiveProject = Project::GetActiveProject();

		auto& rConfig = ActiveProject->GetConfig();
		auto& startupScene = rConfig.StartupSceneID;

		Ref<Asset> asset = AssetManager::Get().FindAsset( startupScene );

		ImGui::SetNextWindowPos( ImVec2( rIO.DisplaySize.x * 0.5f - 150.0f, rIO.DisplaySize.y * 0.5f - 150.0f ), ImGuiCond_Once );

		if( ImGui::Begin( "Project settings", &m_ShowUserSettings ) ) 
		{
			auto boldFont = rIO.Fonts->Fonts[ 1 ];
			ImGui::PushFont( boldFont );
			ImGui::Text( "Project Defaults" );
			ImGui::Separator();
			ImGui::PopFont();

			if( s_OpenAssetFinderPopup )
				ImGui::OpenPopup( "AssetFinderPopup" );

			ImGui::BeginHorizontal( "##prj_strtscene" );
			{
				ImGui::Text( "Startup Scene:" );
				startupScene == 0 ? ImGui::TextColored( ImVec4( 1.0f, 0.0f, 0.0f, 1.0f ), "None" ) : ImGui::Text( asset->Name.c_str() );

				ImGui::Spring();

				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), { 24.0f, 24.0f } ) )
					s_OpenAssetFinderPopup = true;

				if( Auxiliary::DrawAssetFinder( AssetType::Scene, &s_OpenAssetFinderPopup, rConfig.StartupSceneID ) )
				{
					ShouldSaveProject = true;
				}

				{
					Auxiliary::ScopedDisabledFlag disabledFlag( rConfig.StartupSceneID == 0 );

					if( Auxiliary::ImageButton( EditorIcons::GetIcon( "NoIcon" ), { 24.0f, 24.0f } ) )
					{
						Ref<Asset> target = AssetManager::Get().FindAsset( rConfig.StartupSceneID );

						if( target )
						{
							Ref<ContentBrowserPanel> contentBrowserPanel = m_PanelManager->GetPanel<ContentBrowserPanel>();
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

				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), { 24.0f, 24.0f } ) )
					s_OpenAssetFinderPopup = true;

				if( Auxiliary::DrawAssetFinder( AssetType::Material, &s_OpenAssetFinderPopup, defaultMaterialID ) )
				{
					ActiveProject->SetDefaultMaterialAsset( defaultMaterialID );
					ShouldSaveProject = true;
				}

				{
					Auxiliary::ScopedDisabledFlag disabledFlag( defaultMaterialID == 0 );

					if( Auxiliary::ImageButton( EditorIcons::GetIcon( "NoIcon" ), { 24.0f, 24.0f } ) )
					{
						Ref<Asset> target = AssetManager::Get().FindAsset( defaultMaterialID );

						if( target )
						{
							Ref<ContentBrowserPanel> contentBrowserPanel = m_PanelManager->GetPanel<ContentBrowserPanel>();
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

				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), { 24.0f, 24.0f } ) )
					s_OpenAssetFinderPopup = true;

				if( Auxiliary::DrawAssetFinder( AssetType::PhysicsMaterial, &s_OpenAssetFinderPopup, defaultMaterialID ) )
				{
					ActiveProject->SetDefaultPhysicsMaterialAsset( defaultMaterialID );
					ShouldSaveProject = true;
				}

				{
					Auxiliary::ScopedDisabledFlag disabledFlag( defaultMaterialID == 0 );

					if( Auxiliary::ImageButton( EditorIcons::GetIcon( "NoIcon" ), { 24.0f, 24.0f } ) )
					{
						Ref<Asset> target = AssetManager::Get().FindAsset( defaultMaterialID );

						if( target )
						{
							Ref<ContentBrowserPanel> contentBrowserPanel = m_PanelManager->GetPanel<ContentBrowserPanel>();
							contentBrowserPanel->BrowseToItem( target->Path, defaultMaterialID );
						}
					}
				}
			}
			ImGui::EndHorizontal();

			ImGui::EndVertical();

			ImGui::PushFont( boldFont );
			ImGui::Text( "Action Bindings" );
			ImGui::Separator();

			ImGui::PopFont();

			for( auto rIt = ActiveProject->GetActionBindings().begin(); rIt != ActiveProject->GetActionBindings().end(); )
			{
				auto& rBinding = *( rIt );

				char buffer[ 256 ];
				memset( buffer, 0, 256 );
				memcpy( buffer, rBinding.Name.data(), rBinding.Name.length() );

				std::string id = "##" + std::to_string( rBinding.ID );

				ImGui::SetNextItemWidth( 130.0f );
				if( ImGui::InputText( id.data(), buffer, 256 ) )
				{
					rBinding.Name = std::string( buffer );
				}

				ImGui::SameLine(); // HACK, There seems to bug with the ImGui Layout as the InputText works fine when it's not in a Horizontal layout. (Update) Seems to be with certain IDs/labels

				ImGui::BeginHorizontal( rBinding.Name.data() );

				ImGui::SetNextItemWidth( 130.0f );
				if( ImGui::BeginCombo( "##KEYLIST", rBinding.ActionName.data() ) )
				{
					for( int i = 0; i < RubyKey::EnumSize; i++ )
					{
						const auto& result = RubyKeyToString( ( RubyKey ) i );

						// This is here because of how we do our loop, some keys will be empty because the values to do not match up.
						if( result.empty() )
							continue;

						bool IsSelected = ( rBinding.ActionName == result );

						ImGui::PushID( i );

						ImGui::SetNextItemWidth( 130.0f );
						if( ImGui::Selectable( result.data(), IsSelected ) )
						{
							if( rBinding.Type == ActionBindingType::Mouse )
								rBinding.MouseButton = RubyMouseButton::Unknown;

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

						bool IsSelected = ( rBinding.ActionName == result );

						ImGui::PushID( i );

						ImGui::SetNextItemWidth( 130.0f );
						if( ImGui::Selectable( result.data(), IsSelected ) )
						{
							if( rBinding.Type == ActionBindingType::Key )
								rBinding.Key = RubyKey::UnknownKey;

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
				ActionBinding ab;
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

			ImGui::PushFont( boldFont );
			ImGui::Text( "Audio Groups" );
			ImGui::Separator();
			ImGui::PopFont();

			for( auto rIt = ActiveProject->GetSoundGroups().begin(); rIt != ActiveProject->GetSoundGroups().end(); )
			{
				auto& rSoundGroup = *( rIt );

				char buffer[ 256 ];
				memset( buffer, 0, 256 );
				memcpy( buffer, rSoundGroup->GetName().data(), rSoundGroup->GetName().length() );

				// TODO: Change to unique ID
				std::string id = "##entergrpname";

				ImGui::SetNextItemWidth( 130.0f );
				if( ImGui::InputText( id.data(), buffer, 256 ) )
				{
					rSoundGroup->SetName( std::string( buffer ) );
				}

				ImGui::SameLine(); // HACK, There seems to bug with the ImGui Layout as the InputText works fine when it's not in a Horizontal layout. (Update) Seems to be with certain IDs/labels

				ImGui::BeginHorizontal( rSoundGroup->GetName().data() );

				if( ImGui::SmallButton( "-" ) )
				{
					rIt = ActiveProject->GetSoundGroups().erase( rIt );
					ShouldSaveProject = true;
				}
				else
				{
					++rIt;
				}

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

			ImGui::BeginVertical( "##PRJDBGV" );

			auto drawDebugText = []( const std::string& id, const std::string& key, const std::string& value )
				{
					ImGui::BeginHorizontal( id.c_str() );
					ImGui::Text( "%s", key.c_str() );
					ImGui::Text( " : " );
					ImGui::Text( "%s", value.c_str() );
					ImGui::EndHorizontal();
				};

			drawDebugText( "##PRJD1", "Root Path", ActiveProject->GetRootDir().string() );
			drawDebugText( "##PRJD2", ".sproject Path", ActiveProject->GetConfig().Path.string() );
			drawDebugText( "##PRJD3", "Assets Path", ActiveProject->GetFullAssetPath().string() );
			drawDebugText( "##PRJD4", "Premake filename", ActiveProject->GetPremakeFile().string() );
			drawDebugText( "##PRJD5", "Temp Path", ActiveProject->GetTempDir().string() );
			drawDebugText( "##PRJD6", "Bin Path", ActiveProject->GetBinDir().string() );
			drawDebugText( "##PRJD7", "Cache Path", ActiveProject->GetFullCachePath().string() );

			drawDebugText( "##PRJD8", "Module Path", GameModule::Get().GetModulePath().string() );
			drawDebugText( "##PRJD9", "Module Timestamp", GameModule::Get().GetTimestamp() );

			ImGui::EndVertical();
#endif
		}

		ImGui::End();

		if( ShouldSaveProject && !m_ShowUserSettings )
		{
			ProjectSerialiser ps;
			ps.Serialise( Project::GetActiveProject()->GetRootDir().string() );
		}
	}

	void EditorLayer::HotReloadGame()
	{
#if defined(SAT_RELEASE)
		SAT_CORE_INFO( "Begin hot reload" );

		SaveFile();
		SaveProject();

		ClassMetadataHandler::Get().BeginHotReload();

		m_GameModule->BeginHotReload();
		Project::GetActiveProject()->Build( ConfigKind::Release, "/HOTRELOAD" );
		m_GameModule->EndHotReload();

		m_EditorScene->AcknowledgeHotReload();
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
			if( ImGui::BeginTable( "##FileTable", 5, TableFlags, ImVec2( ImGui::GetWindowSize().x, ImGui::GetWindowSize().y ) ) )
			{
				ImGui::TableSetupColumn( "Asset Name" );
				ImGui::TableSetupColumn( "ID" );
				ImGui::TableSetupColumn( "Type" );
				ImGui::TableSetupColumn( "Path" );
				ImGui::TableSetupColumn( "Version" );

				ImGui::TableHeadersRow();

				for( auto&& [id, asset] : AssetManager::Get().GetCombinedAssetMap() )
				{
					if( !Filter.PassFilter( asset->GetName().c_str() ) )
						continue;

					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex( 0 );
					ImGui::Selectable( asset->GetName().c_str(), false );

					ImGui::TableSetColumnIndex( 1 );
					ImGui::Text( "%llu", id );

					ImGui::TableSetColumnIndex( 2 );
					ImGui::Text( AssetTypeToString( asset->GetAssetType() ).data(), false );

					ImGui::TableSetColumnIndex( 3 );
					ImGui::Text( asset->Path.string().c_str() );

					ImGui::TableSetColumnIndex( 4 );
					ImGui::Text( "%i", asset->Version );

					if( asset->Version != SAT_CURRENT_VERSION )
					{
						ImGui::SameLine();
						ImGui::Text( "(Version does not match)" );
					}
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

			ImGuiTableFlags TableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX | ImGuiTableFlags_NoBordersInBody;
			if( ImGui::BeginTable( "##FileTable", 3, TableFlags, ImVec2( ImGui::GetWindowSize().x, ImGui::GetWindowSize().y * 0.85f ) ) )
			{
				ImGui::TableSetupColumn( "Asset Name" );
				ImGui::TableSetupColumn( "ID" );
				ImGui::TableSetupColumn( "Type" );

				ImGui::TableHeadersRow();

				for( auto&& [id, asset] : AssetManager::Get().GetCombinedLoadedAssetMap() )
				{
					if( !Filter.PassFilter( asset->GetName().c_str() ) )
						continue;

					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex( 0 );
					ImGui::Selectable( asset->GetName().c_str(), false );

					ImGui::TableSetColumnIndex( 1 );
					ImGui::Text( "%llu", id );

					ImGui::TableSetColumnIndex( 2 );
					ImGui::Text( AssetTypeToString( asset->GetAssetType() ).data(), false );
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
			auto boldFont = rIO.Fonts->Fonts[ 1 ];
			auto italicsFont = rIO.Fonts->Fonts[ 2 ];

			ImGui::PushFont( boldFont );
			ImGui::Text( "Saturn Editor Settings" );
			ImGui::PopFont();

			ImGui::PushStyleColor( ImGuiCol_Text, ImVec4{ 0.7f, 0.7f, 0.7f, 0.7f } );
			ImGui::PushFont( italicsFont );
			ImGui::Text( "Saturn Engine Version: %s", SAT_CURRENT_VERSION_STRING );
			ImGui::PopFont();
			ImGui::PopStyleColor();

			ImGui::PushStyleColor( ImGuiCol_Separator, ImVec4{ 0.7f, 0.7f, 0.7f, 0.7f } );
			ImGui::Separator();
			ImGui::PopStyleColor();

			ImGui::End();
		}
	}

	void EditorLayer::DrawMaterials()
	{
		ImGui::Begin( "Materials" );

		Ref<SceneHierarchyPanel> hierarchyPanel = m_PanelManager->GetPanel<SceneHierarchyPanel>();

		if( hierarchyPanel->GetSelectionContexts().size() > 0 )
		{
			Ref<Entity> rSelection = hierarchyPanel->GetSelectionContext();

			if( rSelection->HasComponent<StaticMeshComponent>() )
			{
				if( auto& mesh = rSelection->GetComponent<StaticMeshComponent>().Mesh )
				{
					ImGui::TextDisabled( "%llx", rSelection->GetComponent<IdComponent>().ID );

					ImGui::Separator();

					for( auto& rMaterial : mesh->GetMaterialAssets() )
					{
						DrawMaterialHeader( rMaterial );
					}
				}
			}
		}

		ImGui::End();
	}

	void EditorLayer::DrawMaterialHeader( Ref<MaterialAsset>& rMaterial )
	{
		auto drawItemValue = [&]( const char* name, const char* property )
			{
				ImGui::Text( name );

				ImGui::Separator();

				float v = rMaterial->Get< float >( property );

				ImGui::PushID( name );

				ImGui::DragFloat( "##drgflt", &v, 0.01f, 0.0f, 10000.0f );

				ImGui::PopID();

				if( v != rMaterial->Get<float>( property ) )
					rMaterial->Set( property, v );
			};

		auto displayItemMap = [&]( const char* property )
			{
				Ref< Texture2D > v = rMaterial->GetResource( property );

				if( v && v->GetDescriptorSet() )
					ImGui::Image( v->GetDescriptorSet(), ImVec2( 100, 100 ) );
				else
					ImGui::Image( m_CheckerboardTexture->GetDescriptorSet(), ImVec2( 100, 100 ) );
			};

		if( ImGui::CollapsingHeader( rMaterial->GetName().c_str() ) )
		{
			ImGui::PushID( static_cast< int >( rMaterial->GetAssetID() ) );
			ImGui::Text( "Asset ID: %llu", ( uint64_t ) rMaterial->GetAssetID() );

			ImGui::Separator();

			UUID id = rMaterial->GetAssetID();
			Auxiliary::DrawAssetDragDropTarget<MaterialAsset>( "Change asset", rMaterial->GetName().c_str(), id,
				[rMaterial]( Ref<MaterialAsset> asset ) mutable
				{
					rMaterial->SetMaterial( asset->GetMaterial() );
				} );

			ImGui::Separator();

			ImGui::Text( "Albedo" );

			ImGui::Separator();

			displayItemMap( "u_AlbedoTexture" );

			ImGui::SameLine();

			if( ImGui::Button( "...##opentexture", ImVec2( 50, 20 ) ) )
			{
				std::string file = Application::Get().OpenFile( "Texture File (*.png *.tga)\0*.tga; *.png\0" );

				if( !file.empty() )
				{
					rMaterial->SetResource( "u_AlbedoTexture", Ref<Texture2D>::Create( file, AddressingMode::Repeat ) );
				}
			}

			glm::vec3 color = rMaterial->Get<glm::vec3>( "u_Materials.AlbedoColor" );

			bool changed = ImGui::ColorEdit3( "##Albedo Color", glm::value_ptr( color ), ImGuiColorEditFlags_NoInputs );

			if( changed )
				rMaterial->Set<glm::vec3>( "u_Materials.AlbedoColor", color );

			drawItemValue( "Emissive", "u_Materials.Emissive" );

			ImGui::Text( "Normal" );

			ImGui::Separator();

			bool UseNormalMap = rMaterial->Get< float >( "u_Materials.UseNormalMap" );

			if( UseNormalMap )
				displayItemMap( "u_NormalTexture" );

			if( ImGui::Checkbox( "Use Normal Map", &UseNormalMap ) )
				rMaterial->Set( "u_Materials.UseNormalMap", UseNormalMap ? 1.0f : 0.0f );

			// Roughness Value
			drawItemValue( "Roughness", "u_Materials.Roughness" );

			// Roughness map
			displayItemMap( "u_RoughnessTexture" );

			// Metalness value
			drawItemValue( "Metalness", "u_Materials.Metalness" );

			// Metalness map
			displayItemMap( "u_MetallicTexture" );

			ImGui::PopID();
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
			if( ImGui::MenuItem( "Save Scene", "Ctrl+S" ) )          SaveFile();
			if( ImGui::MenuItem( "Save Scene As", "Ctrl+Shift+S" ) ) SaveFileAs();

			if( ImGui::MenuItem( "Save Project" ) )                  SaveProject();
			if( ImGui::MenuItem( "Close Project" ) )                 CloseEditorAndOpenPB();
			if( ImGui::MenuItem( "Exit", "Alt+F4" ) )                Application::Get().Close();

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

			if( ImGui::MenuItem( "Upgrade assets" ) ) Project::GetActiveProject()->UpgradeAssets();

			ImGui::SeparatorText( "Building and Distribution" );

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

			if( ImGui::MenuItem( "Setup Project for Distribution" ) )
			{
				if( !m_BlockingOperation )
					m_BlockingOperation = Ref<JobProgress>::Create();

				JobSystem::Get().AddJob( [this]()
					{
						m_JobModalOpen = true;
						m_BlockingOperation->SetStatus( "Initializing..." );

						SaveFile();
						SaveProject();

						Project::GetActiveProject()->PrepForDist();
						
						m_BlockingOperation->SetStatus( "Building Shader bundle..." );
						if( !BuildShaderBundle() )
							return;

						if( auto result = AssetBundle::BundleAssets( m_BlockingOperation ); result != AssetBundleResult::Success )
						{
							Application::Get().GetWindow()->FlashAttention();

							auto resultStr = AssetBundleResultToString( result );

							MessageBoxInfo msgBox
							{
								.Title = "Error",
								.Text = std::format( "Asset bundle failed to build error was: {0}", resultStr ),
								.Buttons = MessageBoxButtons_Ok
							};

							PushMessageBox( msgBox );
						}
						else
						{
							MessageBoxInfo msgBox
							{
								.Title = "Asset bundle successfully built",
								.Text = "Asset Bundle successfully built. You may now compile the game in the \"Dist\" configuration.\nYou can do this in your IDE. Or go to Project->Distribute project in the title bar.",
								.Buttons = MessageBoxButtons_Ok,
								.Type = MessageBoxType::Information
							};

							PushMessageBox( msgBox );
						}
					} );
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

#if defined( SAT_DEBUG )
			ImGui::SeparatorText( "DEBUG" );

			if( ImGui::MenuItem( "DEBUG: Read Asset Bundle" ) )
			{
				Application::Get().GetSpecification().Flags |= ApplicationFlag_UseVFS;
				auto res = AssetBundle::ReadBundle();
			}

			if( ImGui::MenuItem( "DEBUG: Build Asset Bundle (no shaders)" ) )
			{
				JobSystem::Get().AddJob( [this]()
					{
						m_JobModalOpen = true;

						if( auto result = AssetBundle::BundleAssets( m_BlockingOperation ); result != AssetBundleResult::Success )
						{
							SAT_CORE_ASSERT( false );
						}
					} );
			}
#endif

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
						Project::GetActiveProject()->Rebuild( ConfigKind::Dist );

						m_BlockingOperation->SetProgress( 50.0f );

						m_BlockingOperation->SetStatus( "Copying for Distribution" );
						Project::GetActiveProject()->Distribute( ConfigKind::Dist );
					} );
			}

			if( ImGui::BeginItemTooltip() )
			{
				ImGui::Text( "Attempts to compile the project and fully setup the project for Distribution.\nMake sure you have prepare the project before attempting to distribute the project." );
				ImGui::EndTooltip();
			}

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

			ImGui::SeparatorText( "Demo Window" );
			if( ImGui::MenuItem( "Show demo window", "" ) )           m_ShowImGuiDemoWindow    ^= 1;

			ImGui::SeparatorText( "Virtual Filesystem (VFS)" );
			if( ImGui::MenuItem( "Virtual Filesystem Debug", "" ) )   m_ShowVFSDebug           ^= 1;

			ImGui::EndMenu();
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
				ImGui::Text( "PhysX: Version 4.1.1" );

				Auxiliary::EndTreeNode();
			}

			ImGui::End();
		}
	}

	void EditorLayer::DrawSceneRendererWindow()
	{
		if( ImGui::Begin( "Scene Renderer", &m_ShowSceneRendererWindow ) )
		{
			Application::Get().PrimarySceneRenderer().ImGuiRender();

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
			ClassMetadataHandler::Get().EachTreeNode( 
				[&]( const auto& rMetadata ) 
				{
					if( Auxiliary::TreeNode( rMetadata.Name.c_str(), false ) )
					{
						ImGui::Text( "Parent Class: %s", rMetadata.ParentClassName.c_str() );
						ImGui::Text( "Generated Source path: %s", rMetadata.GeneratedSourcePath.string().c_str() );
						ImGui::Text( "Header path: %s", rMetadata.HeaderPath.string().c_str() );

						Auxiliary::EndTreeNode();
					}
				} );
		}

		ImGui::End();
	}

	void EditorLayer::DrawSceneDirtyPopup()
	{
		ImGui::SetNextWindowPos( ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2( 0.5f, 0.5f ) );
		if( ImGui::BeginPopupModal( "SceneDirtyPopup", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings ) )
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

	void EditorLayer::DrawViewport()
	{
		// Viewport Image & Drag and drop handling
		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 0 ) );

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove;

		ImGui::Begin( "Viewport", 0, flags );

		if( m_ViewportSize != ImGui::GetContentRegionAvail() )
		{
			m_ViewportSize = ImGui::GetContentRegionAvail();

			Application::Get().PrimarySceneRenderer().SetViewportSize( ( uint32_t ) m_ViewportSize.x, ( uint32_t ) m_ViewportSize.y );
			Renderer2D::Get().SetViewportSize( ( uint32_t ) m_ViewportSize.x, ( uint32_t ) m_ViewportSize.y );
			m_EditorCamera.SetViewportSize( ( uint32_t ) m_ViewportSize.x, ( uint32_t ) m_ViewportSize.y );
			m_FallbackCamera.SetViewportSize( ( uint32_t ) m_ViewportSize.x, ( uint32_t ) m_ViewportSize.y );
		}

		ImGui::PushID( "VIEWPORT_IMAGE" );

		// In the editor we only should flip the image UV, we don't have to flip anything else.
		Auxiliary::Image( Application::Get().PrimarySceneRenderer().CompositeImage(), m_ViewportSize, { 0, 1 }, { 1, 0 } );

		if( ImGui::BeginDragDropTarget() )
		{
			if( auto payload = ImGui::AcceptDragDropPayload( "CONTENT_BROWSER_ITEM_SCENE" ) )
			{
				const wchar_t* path = ( const wchar_t* ) payload->Data;
				
				std::filesystem::path p = path;
				Ref<Asset> asset = AssetManager::Get().FindAsset( p );

				if( asset ) OpenFile( asset->ID );
			}

			if( auto payload = ImGui::AcceptDragDropPayload( "CONTENT_BROWSER_ITEM_PREFAB" ) )
			{
				const wchar_t* path = ( const wchar_t* ) payload->Data;

				std::filesystem::path p = path;

				Ref<Asset> asset = AssetManager::Get().FindAsset( p );
				// Make sure to load the prefab.
				Ref<Prefab> prefabAsset = AssetManager::Get().GetAssetAs<Prefab>( asset->GetAssetID() );

				m_EditorScene->CreatePrefab( prefabAsset );
				m_EditorScene->MarkDirty();
			}

			if( auto payload = ImGui::AcceptDragDropPayload( "CONTENT_BROWSER_ITEM_MODEL" ) )
			{
				const wchar_t* path = ( const wchar_t* ) payload->Data;

				std::filesystem::path p = path;

				Ref<Asset> asset = AssetManager::Get().FindAsset( p );
				Ref<StaticMesh> meshAsset = AssetManager::Get().GetAssetAs<StaticMesh>( asset->GetAssetID() );

				Ref<Entity> entity = Ref<Entity>::Create();
				entity->SetName( asset->Name );

				auto& rMeshComponent = entity->AddComponent<StaticMeshComponent>();
				rMeshComponent.Mesh = meshAsset;
				rMeshComponent.MaterialRegistry = Ref<MaterialRegistry>::Create( meshAsset );

				m_EditorScene->MarkDirty();
			}

			ImGui::EndDragDropTarget();
		}

		ImGui::PopID();

		// Viewport Gizmo controls
		Viewport_GizmoControl();

		// Viewport Runtime controls
		Viewport_RTControls();

		// Viewport Runtime settings controls
		Viewport_RTSettings();

		//// Render the real gizmo

		ImVec2 minBound = ImGui::GetWindowPos();
		ImVec2 maxBound = { minBound.x + m_ViewportSize.x, minBound.y + m_ViewportSize.y };

		m_ViewportFocused = ImGui::IsWindowFocused();
		m_MouseOverViewport = ImGui::IsWindowHovered();
		m_AllowCameraEvents = ImGui::IsMouseHoveringRect( minBound, maxBound ) && m_ViewportFocused || m_StartedRightClickInViewport;

		Ref<SceneHierarchyPanel> hierarchyPanel = m_PanelManager->GetPanel<SceneHierarchyPanel>();
		std::vector<Ref<Entity>>& selectedEntities = hierarchyPanel->GetSelectionContexts();

		// Calc center of transform.
		glm::vec3 Positions = {};
		glm::quat Rotations = {};
		glm::vec3 Scales = {};

		for( const auto& rEntity : selectedEntities )
		{
			TransformComponent worldSpace = GActiveScene->GetWorldSpaceTransform( rEntity );
			Positions += worldSpace.Position;
			Rotations += worldSpace.GetRotation();
			Scales += worldSpace.Scale; 
		}

		Positions /= selectedEntities.size();
		Rotations /= selectedEntities.size();
		Scales /= selectedEntities.size();

		glm::mat4 centerPoint = glm::translate( glm::mat4( 1.0f ), Positions ) * glm::toMat4( Rotations ) * glm::scale( glm::mat4( 1.0f ), Scales );
		glm::mat4 offsetTransform( 1.0f );

		///////////////////

		if( selectedEntities.size() && m_GizmoOperation != 0 )
		{
			ImGuizmo::SetOrthographic( false );
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect( minBound.x, minBound.y, m_ViewportSize.x, m_ViewportSize.y );

			const glm::mat4 Projection = m_EditorCamera.ProjectionMatrix();
			const glm::mat4 View = m_EditorCamera.ViewMatrix();

			ImGuizmo::Manipulate( glm::value_ptr( View ), glm::value_ptr( Projection ), ( ImGuizmo::OPERATION ) m_GizmoOperation, ImGuizmo::LOCAL, glm::value_ptr( centerPoint ), glm::value_ptr( offsetTransform ) );

			if( ImGuizmo::IsUsing() )
			{
				for( Ref<Entity>& entity : selectedEntities )
				{
					glm::mat4 transform = GActiveScene->GetTransformRelativeToParent( entity );
					auto& tc = entity->GetComponent<TransformComponent>();

					glm::vec3 translation;
					glm::vec3 rotation;
					glm::vec3 scale;

					Math::DecomposeTransform( transform * offsetTransform, translation, rotation, scale );

					glm::vec3 DeltaRotation = rotation - tc.GetRotationEuler();

					tc.Position = translation;
					tc.SetRotation( tc.GetRotationEuler() += DeltaRotation );
					tc.Scale = scale;

					// TODO: It would be nice if ImGuizmo provided a way for us to know when we stopped using instead of us marking the scene dirty every time we move.
					m_EditorScene->MarkDirty();
				}
			}
		}

		ImGui::PopStyleVar();
		ImGui::End();
	}

	void EditorLayer::Viewport_GizmoControl()
	{
		ImVec2 minBound = ImGui::GetWindowPos();
		ImVec2 maxBound = { minBound.x + m_ViewportSize.x, minBound.y + m_ViewportSize.y };

		// Viewport Gizmo toolbar
		ImGui::PushID( "VP_GIZMO" );

		constexpr float windowHeight = 32.0f;
		constexpr float icons = 3.0f;
		constexpr float neededSpace = 48.0f * icons - 10.0f;

		// For 4 icons
		//const float windowWidth = 166.0f;

		// For 3 icons
		// Formula is 24 * x - 10.0f (for item spacing)
		// Where x is number of icons
		constexpr float windowWidth = neededSpace - 10.0f;

		ImGui::SetNextWindowPos( ImVec2( minBound.x + 5.0f, minBound.y + 5.0f ) );
		ImGui::SetNextWindowSize( ImVec2( windowWidth, windowHeight ) );
		ImGui::Begin( "ViewportGizmoCrtl##viewport_tools", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings );

		ImGui::BeginVertical( "##v_gizmoV", { windowWidth, ImGui::GetContentRegionAvail().y } );
		ImGui::BeginHorizontal( "##v_gizmoH", { windowWidth, ImGui::GetContentRegionAvail().y } );

		ImGui::PushStyleColor( ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f } );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 5.0f * 2.0f, 0 ) );

		if( Auxiliary::ImageButton( m_TranslationTexture, { 24.0f, 24.0f } ) ) m_GizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
		if( Auxiliary::ImageButton( m_RotationTexture, { 24.0f, 24.0f } ) ) m_GizmoOperation = ImGuizmo::OPERATION::ROTATE;
		if( Auxiliary::ImageButton( m_ScaleTexture, { 24.0f, 24.0f } ) ) m_GizmoOperation = ImGuizmo::OPERATION::SCALE;

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
		ImVec2 minBound = ImGui::GetWindowPos();
		ImVec2 maxBound = { minBound.x + m_ViewportSize.x, minBound.y + m_ViewportSize.y };

		constexpr float windowHeight = 32.0f;
		constexpr float icons = 1.0f;
		constexpr float neededSpace = 48.0f * icons - 10.0f;
		constexpr float windowWidth = neededSpace - 10.0f;

		float runtimeCenterX = minBound.x + m_ViewportSize.x * 0.5f - windowWidth * 0.5f;

		// Runtime Controls
		ImGui::SetNextWindowPos( ImVec2( runtimeCenterX, minBound.y + 5.0f ) );
		ImGui::SetNextWindowSize( ImVec2( windowWidth, windowHeight ) );

		ImGui::Begin( "ViewportCenterRt##viewport_center_rt", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings );

		ImGui::BeginVertical( "##centerRTv", { windowWidth, ImGui::GetContentRegionAvail().y } );
		ImGui::BeginHorizontal( "##centerRTh", { windowWidth, ImGui::GetContentRegionAvail().y } );

		ImGui::PushStyleColor( ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f } );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 5.0f * 2.0f, 0 ) );

		Ref<Texture2D> texture = m_RequestRuntime ? m_EndRuntimeTexture : m_StartRuntimeTexture;

		if( Auxiliary::ImageButton( texture, ImVec2( 24.0f, 24.0f ) ) ) 
			m_RequestRuntime ^= 1;

		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		ImGui::Spring();
		ImGui::EndHorizontal();
		ImGui::Spring();
		ImGui::EndVertical();

		ImGui::End();
	}

	void EditorLayer::Viewport_RTSettings()
	{
		ImVec2 minBound = ImGui::GetWindowPos();
		ImVec2 maxBound = { minBound.x + m_ViewportSize.x, minBound.y + m_ViewportSize.y };

		constexpr float windowHeight = 32.0f;
		constexpr float icons = 1.0f;
		constexpr float neededSpace = 48.0f * icons - 10.0f;
		constexpr float windowWidth = neededSpace - 10.0f;

		float runtimeRightX = minBound.x + m_ViewportSize.x - neededSpace - 2.5f;

		// Runtime Controls
		ImGui::SetNextWindowPos( ImVec2( runtimeRightX, minBound.y + 5.0f ) );
		ImGui::SetNextWindowSize( ImVec2( windowWidth, windowHeight ) );

		ImGui::Begin( "ViewportRightRT##viewport_right_rt", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings );

		ImGui::BeginVertical( "##rightRTv", { windowWidth, ImGui::GetContentRegionAvail().y } );
		ImGui::BeginHorizontal( "##rightRTh", { windowWidth, ImGui::GetContentRegionAvail().y } );

		ImGui::PushStyleColor( ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f } );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 5.0f * 2.0f, 0 ) );

		{
#if defined(SAT_RELEASE)
		Auxiliary::ScopedDisabledFlag disabledFlag( m_RuntimeScene != nullptr );
#else
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
		if( m_EditorScene->IsDirty() )
		{
			m_ShowSceneDirtyModal = true;
			ImGui::OpenPopup( "SceneDirtyPopup" );

			Application::Get().GetWindow()->FlashAttention();
		}

		// Accept exit request if scene is not dirty.
		return !m_EditorScene->IsDirty();
	}

	void EditorLayer::DrawMessageBox( const MessageBoxInfo& rInfo )
	{
		ImGui::SetNextWindowPos( ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2( 0.5f, 0.5f ) );
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

			if( ( rInfo.Buttons & ( uint32_t )MessageBoxButtons_Ok ) != 0 )
			{
				if( ImGui::Button( "OK" ) )
				{
					ImGui::CloseCurrentPopup();
					PopMessageBox();
				}
			}

			if( ( rInfo.Buttons & ( uint32_t ) MessageBoxButtons_Cancel ) != 0 )
			{
				ImGui::Spring();

				if( ImGui::Button( "Cancel" ) )
				{
					ImGui::CloseCurrentPopup();
					PopMessageBox();
				}
			}
			
			if( ( rInfo.Buttons & ( uint32_t ) MessageBoxButtons_Exit ) != 0 )
			{
				ImGui::Spring();

				if( ImGui::Button( "Exit" ) )
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
			if( ImGui::BeginPopupModal( "Missing Environment Variable", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings ) )
			{
				ImGui::Text( "The environment variable SATURN_PREMAKE_PATH is not set." );
				ImGui::Text( "This is required in order to build projects." );

				ImGui::Separator();

				static std::string path = "";

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

						Auxiliary::SetEnvironmentVariable( "SATURN_PREMAKE_PATH", path.c_str() );

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
		// Texture Pass shader is only ever loaded in Dist and we are not on Dist at this point.
		Ref<Shader> TexturePass = ShaderLibrary::Get().FindOrLoad( "TexturePass", "content/shaders/TexturePass.glsl" );

		auto shaderRes = ShaderBundle::BundleShaders();
		bool built = shaderRes == ShaderBundleResult::Success;

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

	void EditorLayer::ShowOrHideContentBrowserPanel()
	{
		Ref<ContentBrowserPanel> contentBrowserPanel = m_PanelManager->GetPanel<ContentBrowserPanel>();
		contentBrowserPanel->ShowOrHide();
	}

	void EditorLayer::ShowOrHideSceneHierarchyPanel()
	{
		Ref<SceneHierarchyPanel> sceneHierarchyPanel = m_PanelManager->GetPanel<SceneHierarchyPanel>();
		sceneHierarchyPanel->ShowOrHide();
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

}