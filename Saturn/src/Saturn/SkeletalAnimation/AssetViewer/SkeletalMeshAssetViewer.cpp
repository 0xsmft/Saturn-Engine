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
#include "SkeletalMeshAssetViewer.h"

#include "SkeletonAssetViewer.h"

#include "Saturn/Core/Renderer/RenderThread.h"

#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Vulkan/SceneRenderer.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include "Saturn/ImGui/EditorIcons.h"
#include "Saturn/ImGui/ImGuiWindowManager.h"

#include "Saturn/Scene/Components.h"

namespace Saturn {

	static inline bool operator==( const ImVec2& lhs, const ImVec2& rhs ) { return lhs.x == rhs.x && lhs.y == rhs.y; }
	static inline bool operator!=( const ImVec2& lhs, const ImVec2& rhs ) { return !( lhs == rhs ); }
	
	SkeletalMeshAssetViewer::SkeletalMeshAssetViewer( AssetID id )
		: AssetViewer( id ), m_Camera( 45.0f, 1280.0f, 720.0f, 0.1f, 1000.0f )
	{
		m_Camera.SetActive( true );

		m_Scene = Ref<Scene>::Create();

		m_SceneRenderer = Ref<SceneRenderer>::Create( SceneRendererFlag_RenderGrid );

		m_SceneRenderer->SetDynamicSky( 2.0f, 0.0f, 0.0f );
		m_SceneRenderer->SetCurrentScene( m_Scene.Get() );

		AddMesh();
	}

	SkeletalMeshAssetViewer::~SkeletalMeshAssetViewer()
	{
		m_Entity = nullptr;
		m_SceneRenderer = nullptr;
		m_Scene = nullptr;
	}

	void SkeletalMeshAssetViewer::OnImGuiRender()
	{
		// Root Window.
		ImGuiWindowFlags RootWindowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse;

		ImGui::SetNextWindowPos( ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Once );
		ImGui::SetNextWindowSize( ImVec2( 350.0f, 350.0f ), ImGuiCond_FirstUseEver );

		ImGui::Begin( m_Mesh->Name.c_str(), &m_Open, RootWindowFlags );

		// Create custom dockspace.
		const ImGuiID dockID = ImGui::GetID( "SkMeshDckspc" );
		ImGui::DockSpace( dockID, ImVec2( 0.0f, 0.0f ), ImGuiDockNodeFlags_None );

		//////////////////////////////////////////////////////////////////////////

		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 0 ) );

		if( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) || ( ImGui::IsMouseClicked( ImGuiMouseButton_Right ) && !m_StartedRightClickInViewport ) )
		{
			ImGui::FocusWindow( GImGui->HoveredWindow );
			Input::Get().SetCursorMode( RubyCursorMode::Normal );
		}

		// Viewport
		ImGuiWindowClass windowClassNoDock;
		windowClassNoDock.DockingAlwaysTabBar = false;
		windowClassNoDock.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_None;

		const std::string Name = "##" + std::to_string( m_AssetID );
		ImGui::SetNextWindowClass( &windowClassNoDock );
		ImGui::Begin( Name.c_str(), 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings );
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

		const ImVec2 minBound = ImGui::GetWindowPos();
		const ImVec2 maxBound = { minBound.x + m_ViewportSize.x, minBound.y + m_ViewportSize.y };

		m_ViewportFocused = ImGui::IsWindowFocused();
		m_MouseOverViewport = ImGui::IsWindowHovered();

		m_AllowCameraEvents = ImGui::IsMouseHoveringRect( minBound, maxBound ) && m_ViewportFocused || m_StartedRightClickInViewport;

		ImGui::End(); // Viewport

		ImGui::Begin( "Sidebar" );

		if( Auxiliary::TreeNode( "Materials" ) )
		{
			int i = 0;
			for( auto& rMaterial : m_Mesh->GetMaterialAssets() )
			{
				ImGui::PushID( i );

				if( ImGui::TreeNodeEx( rMaterial->Name.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding ) )
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

					if( Auxiliary::DrawAssetFinder( AssetType::Material, &open, m_AssetFinderOut, 0 ) )
					{
						Ref<MaterialAsset> newAsset = AssetManager::Get().GetAssetAs<MaterialAsset>( m_AssetFinderOut );
						rMaterial->SetMaterial( newAsset->GetMaterial() );

						// Update Pure Dependencies & Update ADN Dependencies
						AssetManager::Get().UnregisterAssetDependency( m_AssetID, rMaterial->ID );

						m_Mesh->GetMaterialRegistry()->SetMaterial( i, m_AssetFinderOut );

						AssetManager::Get().RegisterAssetDependency( m_AssetID, m_AssetFinderOut );
					}

					Auxiliary::EndTreeNode();
				}

				ImGui::PopID();

				i++;
			}

			Auxiliary::EndTreeNode();
		}

		if( Auxiliary::TreeNode( "Preview Animation" ) )
		{
			ImGui::TextDisabled( "%s", m_PreviewAnimation == nullptr ? "<NULL>" : m_PreviewAnimation->Name.c_str() );

			bool open = false;

			if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), ImVec2( 24.0f, 24.0f ) ) )
			{
				open = true;
			}

			if( Auxiliary::DrawAssetFinder( AssetType::SkeletalAnimation, &open, m_AssetFinderOut, 0 ) )
			{
				Ref<SkeletalAnimationAsset> newAsset = AssetManager::Get().GetAssetAs<SkeletalAnimationAsset>( m_AssetFinderOut );

				m_PreviewAnimation = newAsset;

				m_Entity->GetComponent<SkeletalMeshComponent>().LocalAnimator.InitAnimation( m_AssetFinderOut, m_Mesh );
			}

			ImGui::Separator();

			if( ImGui::Button( "Reset" ) )
			{
				m_PreviewAnimation = nullptr;
				m_Entity->GetComponent<SkeletalMeshComponent>().LocalAnimator.Clear();
			}

			Auxiliary::EndTreeNode();
		}

		ImGui::End();

		ImGui::SetNextWindowClass( &windowClassNoDock );
		ImGui::Begin( "##Toolbar" );

		ImGui::BeginHorizontal( "##tbv" );

		if( ImGui::Button( "Save", ImVec2( 50.0f, 50.0f ) ) )
		{
			SkeletalMeshAssetSerialiser sma;
			sma.Serialise( m_Mesh );
		}

		ImGui::Spring();

		if( ImGui::Button( "Open Skeleton", ImVec2( 50.0f, 50.0f ) ) )
		{
			const AssetID skeletonID = m_Mesh->GetSkeletonAsset()->ID;
			const Ref<Asset> asset = AssetManager::Get().FindAsset( skeletonID );
			if( asset )
			{
				const std::string windowName = std::format( "{0}##{1}", asset->Name, ( uint64_t ) skeletonID );
				ImGuiWindowManager::Get().OpenOrShowWindow<SkeletonAssetViewer>( windowName, skeletonID );

				ImGui::SetWindowFocus( windowName.c_str() );
			}
		}

		ImGui::EndHorizontal();

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

	void SkeletalMeshAssetViewer::OnUpdate( Timestep ts )
	{
		// Only true if we are awaiting a shutdown from closing our window.
		if( !m_SceneRenderer )
			return;

		m_Camera.SetActive( m_AllowCameraEvents );
		m_Camera.OnUpdate( ts );

		m_Scene->OnUpdateAnimators( ts );

		// Update Scene for rendering (on main thread).
		m_Scene->OnRenderEditor( m_Camera, ts, *m_SceneRenderer );

		RenderThread::Get().Queue( [ = ]()
		{
			m_SceneRenderer->RenderScene();
		} );

		if( Input::Get().MouseButtonPressed( RubyMouseButton_Right ) && !m_StartedRightClickInViewport && m_ViewportFocused && m_MouseOverViewport )
			m_StartedRightClickInViewport = true;

		if( !Input::Get().MouseButtonPressed( RubyMouseButton_Right ) )
			m_StartedRightClickInViewport = false;
	}

	void SkeletalMeshAssetViewer::OnEvent( Event& rEvent )
	{
		if( m_MouseOverViewport && m_AllowCameraEvents )
			m_Camera.OnEvent( rEvent );
	}

	void SkeletalMeshAssetViewer::AddMesh()
	{
		Ref<SkeletalMesh> mesh = AssetManager::Get().GetAssetAs<SkeletalMesh>( m_AssetID );

		m_Mesh = mesh;

		m_Open = true;

		m_Entity = m_Scene->CreateEntity( "InternalViewerEntity" );
		m_Entity->AddComponent<SkeletalMeshComponent>().Mesh = mesh;
	}

}
