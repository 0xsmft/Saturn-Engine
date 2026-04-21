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
#include "EditorViewport.h"

#include "EntitySelectionManager.h"

#include "ImGuiAuxiliary.h"

#include "Saturn/Core/App.h"
#include "Saturn/Core/Ruby/RubyWindow.h"
#include "Saturn/Core/Renderer/RenderThread.h"

#include "Saturn/Project/Project.h"

#include "Saturn/Vulkan/Renderer2D.h"
#include "Saturn/Vulkan/AluraRenderer.h"

#include "Saturn/ImGui/EditorIcons.h"

#include "SharedGlobals.h"
#include "Saturn/Alura/AluraCanvas.h"

#include <ImGuizmo/ImGuizmo.h>

#include <imgui.h>
#include <imgui_internal.h>

namespace Saturn {

	static constexpr inline bool operator==( const ImVec2& lhs, const ImVec2& rhs ) { return lhs.x == rhs.x && lhs.y == rhs.y; }
	static constexpr inline bool operator!=( const ImVec2& lhs, const ImVec2& rhs ) { return !( lhs == rhs ); }

	EditorViewport::EditorViewport( ViewportFlags flags )
		: m_ViewportFlags( flags ), m_EditorCamera( 45.0f, 1280.0f, 720.0f, 0.1f, 1000.0f )
	{
		m_EditorCamera.SetActive( true );
	}

	EditorViewport::~EditorViewport()
	{
	}

	void EditorViewport::Initialise( 
		SceneRendererFlags sceneRendererFlags, 
		Ref<Scene> scene, 
		const std::string& rName,
		UUID ID,
		bool* pRequestRuntimeVal /*= nullptr*/, bool* pLastRuntimeAttemptFailedVal /*= nullptr*/ )
	{
		m_StartRuntimeTexture = EditorIcons::GetIcon( "Play" );
		m_EndRuntimeTexture = EditorIcons::GetIcon( "Stop" );
		m_PauseRuntimeTexture = EditorIcons::GetIcon( "Pause" );
		m_StartErrorRuntimeTexture = EditorIcons::GetIcon( "Play-Error" );

		m_TranslationTexture = EditorIcons::GetIcon( "Move" );
		m_RotationTexture = EditorIcons::GetIcon( "Rotate" );
		m_ScaleTexture = EditorIcons::GetIcon( "Scale" );

		m_ViewportName = rName;
		m_ViewportID = ID;

		m_SceneRenderer = Ref<SceneRenderer>::Create( sceneRendererFlags );
		m_SceneRenderer->SetDynamicSky( 2.0f, 0.0f, 0.0f );
		m_SceneRenderer->SetCurrentScene( scene.Get() );

		m_Scene = scene;

		if( IsViewportFlagSet( VP_DisplayRuntimeControl ) )
		{
			m_pRequestRuntime = pRequestRuntimeVal;
			m_pLastRuntimeAttemptFailed = pLastRuntimeAttemptFailedVal;

			SAT_CORE_ASSERT( m_pRequestRuntime, "No value passed into m_pRequestRuntime!" );
			SAT_CORE_ASSERT( m_pLastRuntimeAttemptFailed, "No value passed into m_pRequestRuntime!" );
		}
	}

	void EditorViewport::OnUpdate( Timestep ts )
	{
		if( Input::Get().MouseButtonPressed( RubyMouseButton_Right ) && !m_StartedRightClickInViewport && m_ViewportFocused && m_MouseOverViewport )
			m_StartedRightClickInViewport = true;

		if( !Input::Get().MouseButtonPressed( RubyMouseButton_Right ) )
			m_StartedRightClickInViewport = false;

		// Only true if we are awaiting a shutdown from closing our window.
		if( !m_SceneRenderer )
			return;

		m_EditorCamera.SetActive( m_AllowCameraEvents );
		m_EditorCamera.OnUpdate( ts );

		m_Scene->OnUpdateAnimators( ts );

		// Update Scene for rendering (on main thread).
		m_Scene->OnRenderEditor( &m_EditorCamera, m_EditorCamera.ViewMatrix(), m_SceneRenderer, ts );

		RenderThread::Get().Queue( [ = ]()
		{
			m_SceneRenderer->RenderScene();
		} );
	}

	void EditorViewport::Draw()
	{
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
		if( m_FullscreenViewport )
			flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings;

		if( m_DisableViewportMovement )
			flags |= ImGuiWindowFlags_NoMove;

		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );
		if( ImGui::Begin( m_ViewportName.data(), nullptr, flags ) )
		{
			ImVec2 vpSize = ImVec2( m_ViewportSize.x, m_ViewportSize.y );
			if( vpSize != ImGui::GetContentRegionAvail() )
			{
				vpSize = ImGui::GetContentRegionAvail();

				// Resize our scene renderer.
				m_SceneRenderer->SetViewportSize( ( uint32_t ) vpSize.x, ( uint32_t ) vpSize.y );

				m_EditorCamera.SetViewportSize( ( uint32_t ) vpSize.x, ( uint32_t ) vpSize.y );

//				m_SuspendedEditorCamera.SetViewportSize( ( uint32_t ) m_ViewportSize.x, ( uint32_t ) m_ViewportSize.y );

				if( IsViewportFlagSet( VP_AluraCanvas ) && g_AluraCanvas )
					g_AluraCanvas->SetSize( glm::vec2{ m_ViewportSize.x, m_ViewportSize.y } );

				m_ViewportSize = glm::vec2( vpSize.x, vpSize.y );
			}


			// In the editor we only should flip the image UV, we don't have to flip anything else.
			Auxiliary::Image( m_SceneRenderer->CompositeImage(), ImVec2( m_ViewportSize.x, m_ViewportSize.y ), { 0, 1 }, { 1, 0 } );

			if( IsViewportFlagSet( VP_DisplayGizmoControl ) )
			{
				// Viewport Gizmo controls on the left
				Viewport_GizmoControl();
			}

			if( IsViewportFlagSet( VP_DisplayRuntimeControl ) )
			{
				// Viewport Runtime controls on the middle
				Viewport_RTControls();
			}

			// Yes, I know it says "Runtime settings" it used to be that...
			if( IsViewportFlagSet( VP_DisplayHotReloadControl ) )
			{
				// Viewport Runtime settings controls on the right
				Viewport_RTSettings();
			}

			if( IsViewportFlagSet( VP_DisplayGizmo ) )
			{
				Viewport_DrawGizmo();
			}
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void EditorViewport::OnEvent( Event& rEvent )
	{
		SAT_CORE_ASSERT( rEvent.Category & EC_Ruby );

		switch( rEvent.Type )
		{
			case EventType::KeyPressed:
			{
				OnKeyPressed( ( RubyKeyEvent& ) rEvent );
			} break;

			case EventType::MouseScroll:
			{
				if( m_MouseOverViewport && m_AllowCameraEvents )
					m_EditorCamera.OnEvent( rEvent );
			} break;

			default:
				break;
		}
	}

	bool EditorViewport::OnKeyPressed( RubyKeyEvent& rEvent )
	{
		switch( rEvent.GetKeycode() )
		{
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
				if( IsViewportFlagSet( VP_DisplayRuntimeControl ) )
				{
					if( m_MouseOverViewport || m_ViewportFocused )
						*m_pRequestRuntime ^= 1;
				}
			} break;

			case RubyKey_F11:
			{
				if( m_MouseOverViewport || m_ViewportFocused )
				{
					m_PendingFullscreenChange ^= 1;
				}
			} break;

			default:
				break;
		}

		return false;
	}

	bool EditorViewport::OnMousePressed( RubyMouseEvent& rEvent )
	{
		return false;
	}

	void EditorViewport::Viewport_DrawGizmo()
	{
#if !defined(SAT_DIST)
		const ImVec2 minBound = ImGui::GetWindowPos();
		const ImVec2 maxBound = { minBound.x + m_ViewportSize.x, minBound.y + m_ViewportSize.y };

		m_ViewportFocused = ImGui::IsWindowFocused();
		m_MouseOverViewport = ImGui::IsWindowHovered();
		m_AllowCameraEvents = ImGui::IsMouseHoveringRect( minBound, maxBound ) && m_ViewportFocused || m_StartedRightClickInViewport;

		m_ViewportBoundsMin = glm::vec2( minBound.x, minBound.y );
		m_ViewportBoundsMax = glm::vec2( maxBound.x, maxBound.y );

		std::vector<SharedPtr<Entity>> selectedEntities = EntitySelectionManager::Get()->GetSelectionContexts( m_Scene.Get() );

		// Calc center of transform.
		glm::vec3 Positions = {};
		glm::quat Rotations = {};
		glm::vec3 Scales = {};

		for( const auto& rEntity : selectedEntities )
		{
			TransformComponent worldSpace = m_Scene->GetWorldSpaceTransform( rEntity );
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
				for( SharedPtr<Entity>& rEntity : selectedEntities )
				{
					auto& tc = rEntity->GetComponent<TransformComponent>();

					// Set new transform
					glm::mat4 transform = m_Scene->GetTransformRelativeToParent( rEntity );

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

							// Normalise the angle to [-180 to 180]
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

				m_WasGizmoUsed = true;
			}
			else if( m_WasGizmoUsed ) // Stopped using
			{
				m_Scene->MarkDirty();

				if( !ImGui::IsWindowDocked() || m_DisableViewportMovement )
				{
					m_DisableViewportMovement = false;
				}

				m_WasGizmoUsed = false;
			}
		}
#endif
	}

	void EditorViewport::Viewport_GizmoControl()
	{
		if( g_ActiveScene->IsRuntimeRunning() || g_ActiveScene->IsPaused() )
			return;

		const ImVec2 minBound = ImGui::GetWindowPos();
		const ImVec2 maxBound = { minBound.x + m_ViewportSize.x, minBound.y + m_ViewportSize.y };

		// Viewport Gizmo toolbar
		ImGui::PushID( ( int ) m_ViewportID );

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

		ImGui::Begin( "ViewportGizmoCrtl", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings );

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

	void EditorViewport::Viewport_RTControls()
	{
		const ImVec2 minBound = ImGui::GetWindowPos();
		const ImVec2 maxBound = { minBound.x + m_ViewportSize.x, minBound.y + m_ViewportSize.y };

		constexpr float windowHeight = 32.0f;
		const float icons = *m_pRequestRuntime ? 3.0f : 1.0f; // 3 icons if runtime is running else, one icon (play button)
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

		if( *m_pRequestRuntime )
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

	void EditorViewport::Viewport_RTControls_Default()
	{
		const Ref<Texture2D> texture = *m_pLastRuntimeAttemptFailed ? m_StartErrorRuntimeTexture : m_StartRuntimeTexture;
		if( Auxiliary::ImageButton( texture, ImVec2( 24.0f, 24.0f ) ) )
		{
			*m_pRequestRuntime = true;
		}

		if( ImGui::BeginItemTooltip() )
		{
			ImGui::BeginHorizontal( "##centerRTtooltip" );

			ImGui::Text( *m_pLastRuntimeAttemptFailed ? "Runtime request blocked. No camera was found after BeginPlay was called!" : "Request runtime to start" );
			ImGui::Spring();
#if defined(SAT_DEBUG)
			ImGui::Text( "%s", *m_pRequestRuntime ? "RUNTIME RUNNING" : "RUNTIME NOT RUNNING" );
			ImGui::Spring();
#endif

			ImGui::EndHorizontal();

			ImGui::EndTooltip();
		}
	}

	void EditorViewport::Viewport_RTControls_Running()
	{
		const auto runtimeState = m_Scene->GetRuntimeState();

		// Draw play/resume button
		{
			Auxiliary::ScopedDisabledFlag disabled( runtimeState != RuntimeState::Suspended );

			if( Auxiliary::ImageButton( m_StartRuntimeTexture, ImVec2( 24.0f, 24.0f ) ) )
			{
				m_Scene->ResumeRuntime();

				const std::string title = std::format( "{0} (Running) - Saturn", Project::GetActiveConfig().Name );
				Application::Get()->GetWindow()->ChangeTitle( title );
			}

			if( ImGui::BeginItemTooltip() )
			{
				ImGui::Text( "Can not start a new runtime while the scene is already in runtime." );
#if defined(SAT_DEBUG)
				ImGui::Text( "%s", *m_pRequestRuntime ? "RUNTIME RUNNING" : "RUNTIME NOT RUNNING" );
#endif
				ImGui::EndTooltip();
			}
		}

		// Stop
		if( Auxiliary::ImageButton( m_EndRuntimeTexture, ImVec2( 24.0f, 24.0f ) ) )
		{
			*m_pRequestRuntime = false;
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
			m_Scene->SuspendRuntime();

			const std::string title = std::format( "{0} (RT Suspended) - Saturn", Project::GetActiveConfig().Name );
			Application::Get()->GetWindow()->ChangeTitle( title );
		}

		if( ImGui::BeginItemTooltip() )
		{
			ImGui::Text( "Suspend the runtime and allowing the user to use the Editor Camera" );
			ImGui::EndTooltip();
		}
	}

	void EditorViewport::Viewport_RTSettings()
	{

	}

}
