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
#include "StaticMeshAssetViewer.h"

#include "Saturn/Core/Renderer/RenderThread.h"

#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Vulkan/SceneRenderer.h"

#include "ImGuiAuxiliary.h"
#include "EditorIcons.h"

#include "Saturn/Scene/Components.h"

#include "Saturn/Physics/PhysicsFoundation.h"

namespace Saturn {

	static inline bool operator==( const ImVec2& lhs, const ImVec2& rhs ) { return lhs.x == rhs.x && lhs.y == rhs.y; }
	static inline bool operator!=( const ImVec2& lhs, const ImVec2& rhs ) { return !( lhs == rhs ); }

	StaticMeshAssetViewer::StaticMeshAssetViewer( AssetID id )
		: AssetViewer( id ), m_Camera( 45.0f, 1280.0f, 720.0f, 0.1f, 1000.0f )
	{
		m_Camera.SetActive( true );

		m_Scene = Ref<Scene>::Create();

		SceneRendererFlags flags = SceneRendererFlag_RenderGrid;
		m_SceneRenderer = Ref<SceneRenderer>::Create( flags );

		m_SceneRenderer->SetDynamicSky( 2.0f, 0.0f, 0.0f );
		m_SceneRenderer->SetCurrentScene( m_Scene.Get() );

		AddMesh();
	}

	StaticMeshAssetViewer::~StaticMeshAssetViewer()
	{
		m_SceneRenderer = nullptr;
		m_Scene = nullptr;
	}

	void StaticMeshAssetViewer::OnImGuiRender()
	{
		// Root Window.
		ImGuiWindowFlags RootWindowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse;
		
		ImGui::SetNextWindowPos( ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Once );
		ImGui::SetNextWindowSize( ImVec2( 350.0f, 350.0f ), ImGuiCond_FirstUseEver );

		ImGui::Begin( m_Mesh->Name.c_str(), &m_Open, RootWindowFlags );

		// Create custom dockspace.
		ImGuiID dockID = ImGui::GetID( "StaticMeshDckspc" );
		ImGui::DockSpace( dockID, ImVec2( 0.0f, 0.0f ), ImGuiDockNodeFlags_None );

		//////////////////////////////////////////////////////////////////////////

		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 0 ) );

		if( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) || ( ImGui::IsMouseClicked( ImGuiMouseButton_Right ) && !m_StartedRightClickInViewport ) )
		{
			ImGui::FocusWindow( GImGui->HoveredWindow );
			Input::Get().SetCursorMode( RubyCursorMode::Normal );
		}

		// Viewport
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
		std::string Name = "##" + std::to_string( m_AssetID );
		ImGui::Begin( Name.c_str(), 0, flags );
		ImGui::SetWindowDock( ImGui::GetCurrentWindow(), dockID, ImGuiCond_FirstUseEver );

		ImGui::PushID( static_cast< int >( m_AssetID ) );
		
		if( m_ViewportSize != ImGui::GetContentRegionAvail() )
		{
			m_ViewportSize = ImGui::GetContentRegionAvail();

			m_SceneRenderer->SetViewportSize( ( uint32_t ) m_ViewportSize.x, ( uint32_t ) m_ViewportSize.y );
			m_Camera.SetViewportSize( ( uint32_t ) m_ViewportSize.x, ( uint32_t ) m_ViewportSize.y );
		}

		Auxiliary::Image( m_SceneRenderer->CompositeImage(), m_ViewportSize, { 0, 1 }, { 1, 0 } );

		ImGui::PopID();

		ImVec2 minBound = ImGui::GetWindowPos();
		ImVec2 maxBound = { minBound.x + m_ViewportSize.x, minBound.y + m_ViewportSize.y };

		m_ViewportFocused = ImGui::IsWindowFocused();
		m_MouseOverViewport = ImGui::IsWindowHovered();

		m_AllowCameraEvents = ImGui::IsMouseHoveringRect( minBound, maxBound ) && m_ViewportFocused || m_StartedRightClickInViewport;

		ImGui::End(); // Viewport

		ImGui::Begin( "Sidebar" );

		if( Auxiliary::TreeNode( "Physics" ) )
		{
			ShapeType type = m_Mesh->GetAttachedShape();
			
			constexpr const char* pItems[] = { "None", "Box", "Sphere", "Capsule", "Convex Mesh", "Triangle Mesh" };
			static ShapeType SelectedEnum = type;
			static const char* Selected = pItems[ (int)SelectedEnum ];

			ImGui::Text( "Select Physics Shape Type:" );
			ImGui::SameLine();

			if( ImGui::BeginCombo( "##setshape", Selected ) )
			{
				for( int i = 0; i < IM_ARRAYSIZE( pItems ); i++ )
				{
					bool IsSelected = ( Selected == pItems[ i ] );

					if( ImGui::Selectable( pItems[ i ], IsSelected ) ) 
					{
						SelectedEnum = (ShapeType)i;
						Selected = pItems[ i ];

						m_Mesh->SetAttachedShape( SelectedEnum );
					}

					if( IsSelected )
					{
						ImGui::SetItemDefaultFocus();
					}
				}

				ImGui::EndCombo();
			}

			if( SelectedEnum == ShapeType::TriangleMesh || SelectedEnum == ShapeType::ConvexMesh )
			{
				if( ImGui::Button( "Generate Mesh Collider" ) )
				{
					bool Result = PhysicsFoundation::Get().GetCookingContext().CookMeshCollider( m_Mesh, SelectedEnum );

					// TODO: Show a dialog box of what failed.
				}
			}

			ImGui::Text( "Set Physics Material" );
			ImGui::SameLine();
			
			static AssetID id;
			static bool s_Open = true;

			if( ImGui::Button( "...##openmesh", ImVec2( 50.0f, 20.0f ) ) )
			{
				s_Open = !s_Open;
			}
			
			if( Auxiliary::DrawAssetFinder( AssetType::PhysicsMaterial, &s_Open, id, 0 ) ) 
			{
				m_Mesh->SetPhysicsMaterial( id );
			}

			Auxiliary::EndTreeNode();
		}

		static AssetID s_id;

		if( Auxiliary::TreeNode( "Materials" ) )
		{
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;

			int i = 0;
			for( auto& rMaterial : m_Mesh->GetMaterialAssets() )
			{
				ImGui::PushID( i );

				if( ImGui::TreeNodeEx( rMaterial->Name.c_str(), flags ) )
				{
					ImGui::BeginHorizontal( i );

					ImGui::TextDisabled( "%s", rMaterial->Name.empty() ? "<NULL>" : rMaterial->Name.c_str() );

					bool open = false;

					if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), ImVec2( 24.0f, 24.0f ) ) )
					{
						open = true;
					}

					ImGui::Spring();

					ImGui::EndHorizontal();

					if( Auxiliary::DrawAssetFinder( AssetType::Material, &open, s_id, 0 ) )
					{
						Ref<MaterialAsset> newAsset = AssetManager::Get().GetAssetAs<MaterialAsset>( s_id );
						rMaterial->SetMaterial( newAsset->GetMaterial() );

						// Update Pure Dependencies & Update ADN Dependencies
						AssetManager::Get().UnregisterAssetDependency( m_AssetID, rMaterial->ID );

						m_Mesh->GetMaterialRegistry()->SetMaterial( i, s_id );

						AssetManager::Get().RegisterAssetDependency( m_AssetID, s_id );
					}

					Auxiliary::EndTreeNode();
				}

				ImGui::PopID();

				i++;
			}

			Auxiliary::EndTreeNode();
		}

		ImGui::End();

		ImGui::Begin( "##Toolbar" );

		ImGui::BeginVertical( "##tbv" );

		if( ImGui::Button( "Save", ImVec2( 50.0f, 50.0f ) ) ) 
		{
			StaticMeshAssetSerialiser sma;
			sma.Serialise( m_Mesh );
		}

		ImGui::EndVertical();

		ImGui::End();

		ImGui::PopStyleVar(); // ImGuiStyleVar_WindowPadding
		ImGui::End();

		if( m_Open == false )
		{
			m_Open = false;

			RenderThread::Get().Queue( [ = ]()
			{
				m_SceneRenderer = nullptr;
			} );
		}
	}

	void StaticMeshAssetViewer::OnUpdate( Timestep ts )
	{
		// Only true if we are awaiting a shutdown from closing our window.
		if( !m_SceneRenderer )
			return;

		m_Camera.SetActive( m_AllowCameraEvents );
		m_Camera.OnUpdate( ts );

		// Update Scene for rendering (on main thread).
		m_Scene->OnRenderEditor( m_Camera, ts, *m_SceneRenderer );

		RenderThread::Get().Queue( [=]()
		{
			m_SceneRenderer->RenderScene();
		} );

		if( Input::Get().MouseButtonPressed( RubyMouseButton_Right ) && !m_StartedRightClickInViewport && m_ViewportFocused && m_MouseOverViewport )
			m_StartedRightClickInViewport = true;

		if( !Input::Get().MouseButtonPressed( RubyMouseButton_Right ) )
			m_StartedRightClickInViewport = false;
	}

	void StaticMeshAssetViewer::OnEvent( Event& rEvent )
	{
		if( m_MouseOverViewport && m_AllowCameraEvents )
			m_Camera.OnEvent( rEvent );
	}

	void StaticMeshAssetViewer::AddMesh()
	{
		Ref<StaticMesh> mesh = AssetManager::Get().GetAssetAs<StaticMesh>( m_AssetID );

		m_Mesh = mesh;

		m_Open = true;

		auto e = m_Scene->CreateEntity( "InternalViewerEntity" );
		e->AddComponent<StaticMeshComponent>().Mesh = mesh;
	}

}