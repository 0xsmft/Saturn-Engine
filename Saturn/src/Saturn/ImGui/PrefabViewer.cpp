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
#include "PrefabViewer.h"

#include "Saturn/Core/Renderer/RenderThread.h"

#include "Saturn/Asset/AssetRegistry.h"
#include "Saturn/Vulkan/AluraRenderer.h"
#include "Saturn/Vulkan/Renderer2D.h"
#include "Saturn/Vulkan/SceneRenderer.h"

#include "ImGuiAuxiliary.h"

#include "EntitySelectionReason.h"
#include "EntitySelectionManager.h"

#include "ContentBrowserPanel/ContentBrowserThumbnailCache.h"

#include <ImGuizmo/ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>

namespace Saturn {

	static inline bool operator==( const ImVec2& lhs, const ImVec2& rhs ) { return lhs.x == rhs.x && lhs.y == rhs.y; }
	static inline bool operator!=( const ImVec2& lhs, const ImVec2& rhs ) { return !( lhs == rhs ); }

	PrefabViewer::PrefabViewer( AssetID id )
		: AssetViewer( id ), m_Camera( 45.0f, 1280.0f, 720.0f, 0.1f, 1000.0f ), m_SceneHierarchyPanel( Ref<SceneHierarchyPanel>::Create( "Prefab Hierarchy" ) )
	{
		m_SceneHierarchyPanel->SetCustomID( m_AssetID );
		m_SceneHierarchyPanel->OpenWindow();

		AddPrefab();

		m_SceneRenderer = Ref<SceneRenderer>::Create( SceneRendererFlag_RenderGrid );
		m_SceneRenderer->SetDynamicSky( 2.0f, 0.0f, 0.0f );
		m_SceneRenderer->SetCurrentScene( m_Prefab->GetScene().Get() );

		m_Camera.SetActive( true );
	}

	PrefabViewer::~PrefabViewer()
	{
		m_SceneRenderer = nullptr;
		m_SceneHierarchyPanel = nullptr;
		m_Prefab = nullptr;
	}

	void PrefabViewer::SetupDockspace()
	{
		ImGuiID dockID = ImGui::GetID( "PrefabViewerDckspc" );
		ImGui::DockBuilderRemoveNode( dockID );

		ImGui::DockBuilderAddNode( dockID, ImGuiDockNodeFlags_DockSpace );
		ImGui::DockBuilderSetNodeSize( dockID, ImGui::GetCurrentWindow()->Size );

		ImGuiID DockLeftID = ImGui::DockBuilderSplitNode( dockID, ImGuiDir_Left, 0.25f, nullptr, &dockID );
		ImGuiID DockDownID = ImGui::DockBuilderSplitNode( dockID, ImGuiDir_Down, 0.5f, nullptr, &DockLeftID );

		ImGui::DockBuilderDockWindow( "viewport", DockLeftID );
//		ImGui::DockBuilderDockWindow( m_SceneHierarchyPanel->GetName().c_str(), DockLeftID );

		ImGui::DockBuilderFinish( dockID );
	}

	void PrefabViewer::ResetDockspace()
	{

	}

	void PrefabViewer::OnImGuiRender()
	{
		// Root Window.
		ImGui::Begin( m_Name.c_str(), &m_Open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar );

		// Create custom dockspace.
		const ImGuiID dockID = ImGui::GetID( "PrefabViewerDckspc" );
		ImGui::DockSpace( dockID, ImVec2( 0.0f, 0.0f ), ImGuiDockNodeFlags_None );

		if( ImGui::BeginMenuBar() )
		{
			if( ImGui::BeginMenu( "Window" ) )
			{
				if( ImGui::MenuItem( "Reset Dock space" ) ) 
				{
					ResetDockspace();
				}

				if( ImGui::MenuItem( "Show or Hide Prefab Hierarchy" ) )
				{
					m_SceneHierarchyPanel->ShowOrHide();
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		//////////////////////////////////////////////////////////////////////////

		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );

		// Viewport
		const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;

		const std::string Name = "Viewport##" + std::to_string( m_AssetID );

		ImGuiWindowClass windowClass; 
		windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_AutoHideTabBar;

		ImGui::SetNextWindowClass( &windowClass );
		ImGui::Begin( Name.c_str(), 0, flags );

//		ImGui::PushID( static_cast< int >( m_AssetID ) );

		if( m_ViewportSize != ImGui::GetContentRegionAvail() )
		{
			m_ViewportSize = ImGui::GetContentRegionAvail();

			m_SceneRenderer->SetViewportSize( ( uint32_t ) m_ViewportSize.x, ( uint32_t ) m_ViewportSize.y );
			m_Camera.SetViewportSize( ( uint32_t ) m_ViewportSize.x, ( uint32_t ) m_ViewportSize.y );
		}

		Auxiliary::Image( m_SceneRenderer->CompositeImage(), m_ViewportSize, { 0, 1 }, { 1, 0 } );

//		ImGui::PopID();

		const ImVec2 minBound = ImGui::GetWindowPos();
		const ImVec2 maxBound = { minBound.x + m_ViewportSize.x, minBound.y + m_ViewportSize.y };

		m_ViewportFocused = ImGui::IsWindowFocused();
		m_MouseOverViewport = ImGui::IsWindowHovered();

		m_AllowCameraEvents = ImGui::IsMouseHoveringRect( minBound, maxBound ) && m_ViewportFocused || m_StartedRightClickInViewport;

		ImGui::End(); // Viewport

		ImGui::PopStyleVar(); // ImGuiStyleVar_WindowPadding

		// Scene Hierarchy panel
		if( m_SceneHierarchyPanel->IsOpen() )
			m_SceneHierarchyPanel->OnImGuiRender();

		ImGui::End(); // Root window

		if( m_Open == false )
		{
			PrefabSerialiser ps;
			ps.Serialise( m_Prefab );
		}
	}

	void PrefabViewer::OnUpdate( Timestep ts )
	{
		m_Camera.SetActive( m_AllowCameraEvents );
		m_Camera.OnUpdate( ts );

		// Update Scene for rendering (on main thread).
		m_Prefab->GetScene()->OnRenderEditor( &m_Camera, m_Camera.ViewMatrix(), m_SceneRenderer, ts );

		RenderThread::Get().Queue( [=]()
			{
				m_SceneRenderer->RenderScene();
			} );

		if( Input::Get().MouseButtonPressed( RubyMouseButton_Right ) && !m_StartedRightClickInViewport && m_ViewportFocused && m_MouseOverViewport )
			m_StartedRightClickInViewport = true;

		if( !Input::Get().MouseButtonPressed( RubyMouseButton_Right ) )
			m_StartedRightClickInViewport = false;
	}

	void PrefabViewer::OnEvent( Event& rEvent )
	{
		if( m_MouseOverViewport && m_AllowCameraEvents )
			m_Camera.OnEvent( rEvent );
	}

	void PrefabViewer::AddPrefab()
	{
		Ref<Prefab> prefab = AssetManager::Get()->GetAssetAs<Prefab>( m_AssetID );

		m_SceneHierarchyPanel->SetContext( prefab->GetScene() );

		m_Prefab = prefab;

		m_Open = true;
		m_Name = std::format( "{0}##PrefabViewer", m_Prefab->Name );
	}

}
