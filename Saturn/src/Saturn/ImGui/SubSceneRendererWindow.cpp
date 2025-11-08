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
#include "SubSceneRendererWindow.h"

#include "Saturn/Vulkan/Renderer2D.h"

#include "Saturn/Core/Renderer/RenderThread.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"

namespace Saturn {

	static inline bool operator==( const ImVec2& lhs, const ImVec2& rhs ) { return lhs.x == rhs.x && lhs.y == rhs.y; }
	static inline bool operator!=( const ImVec2& lhs, const ImVec2& rhs ) { return !( lhs == rhs ); }

	SubSceneRendererWindow::SubSceneRendererWindow()
		: m_Camera( 45.0f, 1280.0f, 720.0f, 0.1f, 1000.0f )
	{
	}

	void SubSceneRendererWindow::Initialise()
	{
		m_Camera.SetActive( true );
		m_Scene = Ref<Scene>::Create();

		m_SceneRenderer = Ref<SceneRenderer>::Create( SceneRendererFlag_RenderGrid );

		m_SceneRenderer->SetDynamicSky( 2.0f, 0.0f, 0.0f );
		m_SceneRenderer->SetCurrentScene( m_Scene.Get() );
	}

	SubSceneRendererWindow::~SubSceneRendererWindow()
	{
		m_SceneRenderer = nullptr;
		m_Scene = nullptr;
	}

	void SubSceneRendererWindow::RenderViewport()
	{
		if( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) || ( ImGui::IsMouseClicked( ImGuiMouseButton_Right ) && !m_StartedRightClickInViewport ) )
		{
			ImGui::FocusWindow( GImGui->HoveredWindow );
			Input::Get().SetCursorMode( RubyCursorMode::Normal );
		}

		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );

		// Viewport
		ImGuiWindowClass windowClassNoDock;
		windowClassNoDock.DockingAlwaysTabBar = false;
		windowClassNoDock.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_None;

		ImGui::SetNextWindowClass( &windowClassNoDock );
		const std::string vpName = std::format( "Viewport##{0}", std::to_string( m_WindowID ) );

		ImGui::Begin( vpName.c_str(), 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse );
		ImGui::SetWindowDock( ImGui::GetCurrentWindow(), ImGui::GetID( "AxDckspc" ), ImGuiCond_FirstUseEver );

		if( m_ViewportSize != ImGui::GetContentRegionAvail() )
		{
			m_ViewportSize = ImGui::GetContentRegionAvail();

			m_SceneRenderer->SetViewportSize( ( uint32_t ) m_ViewportSize.x, ( uint32_t ) m_ViewportSize.y );
			m_Camera.SetViewportSize( ( uint32_t ) m_ViewportSize.x, ( uint32_t ) m_ViewportSize.y );
		}

		Auxiliary::Image( m_SceneRenderer->CompositeImage(), m_ViewportSize, { 0, 1 }, { 1, 0 } );

		const ImVec2 minBound = ImGui::GetWindowPos();
		const ImVec2 maxBound = { minBound.x + m_ViewportSize.x, minBound.y + m_ViewportSize.y };

		m_ViewportFocused = ImGui::IsWindowFocused();
		m_MouseOverViewport = ImGui::IsWindowHovered();

		m_AllowCameraEvents = ImGui::IsMouseHoveringRect( minBound, maxBound ) && m_ViewportFocused || m_StartedRightClickInViewport;

		ImGui::End();
		ImGui::PopStyleVar(); // ImGuiStyleVar_WindowPadding
	}

	void SubSceneRendererWindow::OnUpdateRenderer( Timestep ts )
	{
		// Only true if we are awaiting a shutdown from closing our window.
		if( !m_SceneRenderer )
			return;

		m_Camera.SetActive( m_AllowCameraEvents );
		m_Camera.OnUpdate( ts );

		m_Scene->OnUpdateAnimators( ts );

		// Update Scene for rendering (on main thread).
		m_Scene->OnRenderEditor( &m_Camera, m_Camera.ViewMatrix(), m_SceneRenderer, ts );

		RenderThread::Get().Queue( [ = ]()
		{
			m_SceneRenderer->RenderScene();
		} );

		if( Input::Get().MouseButtonPressed( RubyMouseButton_Right ) && !m_StartedRightClickInViewport && m_ViewportFocused && m_MouseOverViewport )
			m_StartedRightClickInViewport = true;

		if( !Input::Get().MouseButtonPressed( RubyMouseButton_Right ) )
			m_StartedRightClickInViewport = false;
	}

	void SubSceneRendererWindow::OnCameraEvent( Event& rEvent )
	{
		if( m_MouseOverViewport && m_AllowCameraEvents )
			m_Camera.OnEvent( rEvent );
	}

}
