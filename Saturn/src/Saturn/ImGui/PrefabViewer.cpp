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

#include "ImGuiAuxiliary.h"

#include "Saturn/ImGui/EditorEvents.h"

#include "Saturn/Core/Renderer/RenderThread.h"

#include "Saturn/Asset/AssetRegistry.h"
#include "Saturn/Vulkan/AluraRenderer.h"
#include "Saturn/Vulkan/Renderer2D.h"
#include "Saturn/Vulkan/SceneRenderer.h"

#include "EntitySelectionReason.h"
#include "EntitySelectionManager.h"

#include "Saturn/Core/AABB/Ray.h"

#include <ImGuizmo/ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>

namespace Saturn {

	static inline bool operator==( const ImVec2& lhs, const ImVec2& rhs ) { return lhs.x == rhs.x && lhs.y == rhs.y; }
	static inline bool operator!=( const ImVec2& lhs, const ImVec2& rhs ) { return !( lhs == rhs ); }

	PrefabViewer::PrefabViewer( AssetID id )
		: AssetViewer( id ), m_Camera( 45.0f, 1280.0f, 720.0f, 0.1f, 1000.0f )
	{
		std::string name = std::format( "Prefab Hierarchy##{0}", ( uint64_t ) m_AssetID );
		m_SceneHierarchyPanel = Ref<SceneHierarchyPanel>::Create( name );
		m_SceneHierarchyPanel->SetCustomID( m_AssetID );
		m_SceneHierarchyPanel->OpenWindow();
		m_SceneHierarchyPanel->SetIsPrefabScene( true );

		AddPrefab();

		m_SceneRenderer = Ref<SceneRenderer>::Create( SceneRendererFlag_RenderGrid_DEPRECATED );
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

	void PrefabViewer::OnKeyPressed( RubyKeyEvent& rEvent )
	{
		switch( rEvent.GetKeycode() )
		{
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

			default:
				break;
		}
	}

	glm::vec2 PrefabViewer::ConvertMouseToViewportNDC()
	{
		auto [mx, my] = ImGui::GetMousePos();
		const auto& viewportBounds = m_ViewportBounds;

		mx -= m_ViewportBounds.Min.x;
		my -= m_ViewportBounds.Min.y;

		return { ( mx / m_ViewportSize.x ) * 2.0f - 1.0f, ( ( my / m_ViewportSize.y ) * 2.0f - 1.0f ) * -1.0f };
	}

	std::pair<glm::vec3, glm::vec3> PrefabViewer::RayCast( float mx, float my )
	{
		const glm::vec4 mouseClipPos = { mx, my, -1.0f, 1.0f };

		const auto inverseProj = glm::inverse( m_Camera.ProjectionMatrix() );
		const auto inverseView = glm::inverse( glm::mat3( m_Camera.ViewMatrix() ) );

		const glm::vec4 ray = inverseProj * mouseClipPos;
		const glm::vec3 rayPos = m_Camera.GetPosition();
		const glm::vec3 rayDir = inverseView * glm::vec3( ray );

		return { rayPos, rayDir };
	}

	bool PrefabViewer::OnMousePressed( RubyMouseEvent& rEvent )
	{
		if( !m_MouseOverViewport || rEvent.GetButton() != ( int ) RubyMouseButton_Left || ImGuizmo::IsOver() )
			return false;

		auto activeScene = m_Prefab->GetScene();

		const auto viewportMouse = ConvertMouseToViewportNDC();
		if( viewportMouse.x > -1.0f && viewportMouse.x < 1.0f && viewportMouse.y > -1.0f && viewportMouse.y < 1.0f )
		{
			bool hitAny = false;
			const auto [origin, dir] = RayCast( viewportMouse.x, viewportMouse.y );

			const auto staticMeshes = activeScene->GetAllEntitiesWith<StaticMeshComponent>();
			for( const auto& rEntity : staticMeshes )
			{
				const auto& comp = rEntity->GetComponent<StaticMeshComponent>();
				if( !comp.Mesh )
					continue;

				auto& rSubmeshes = comp.Mesh->Submeshes();
				for( uint32_t i = 0; i < rSubmeshes.size(); ++i )
				{
					const auto& rSubmesh = rSubmeshes[ i ];
					const glm::mat4 transform = activeScene->GetWorldSpaceTransform( rEntity ).GetTransform() * rSubmesh.Transform;

					const Ray ray = { .Origin = glm::inverse( transform ) * glm::vec4( origin, 1.0f ), .Direction = glm::inverse( glm::mat3( transform ) ) * dir };

					float t;
					const bool hit = ray.IntersectsAABB( rSubmesh.BoundingBox, t );
					if( hit )
					{
						hitAny = hit;

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
								EntitySelectionManager::Get()->Select( rEntity );
								EntitySelectionManager::Get()->SetSelectionReason( ESR_Other );

								break;
							}
						}
					}
				}
			}

			if( !hitAny && EntitySelectionManager::Get()->GetSelectionCount( activeScene.Get() ) )
			{
				EntitySelectionManager::Get()->ClearSelection( activeScene.Get(), true );
			}
		}

		return false;
	}

	void PrefabViewer::DrawDirtyPopup()
	{
		ImGui::OpenPopup( "Prefab is dirty" );

		if( ImGui::BeginPopupModal( "Prefab is dirty", nullptr, ImGuiWindowFlags_NoSavedSettings ) )
		{
			ImGui::Text( "There is unsaved changes to this Prefab Asset, what would you like to do?" );
			ImGui::Separator();

			ImGui::BeginHorizontal( "##optionhzpd" );

			if( ImGui::Button( "Save" ) )
			{
				m_ShowDirtyPopup = m_Dirty = m_Open = false;

				m_Prefab->RebuildComponentCache();

				PrefabSerialiser ps;
				ps.Serialise( m_Prefab );

				// Fire the event.
				Application::Get()->DispatchEvent<OnPrefabModifiedEvent>( m_Prefab->ID );
			}

			if( ImGui::Button( "Discard changes" ) )
			{
				m_ShowDirtyPopup = m_Dirty = m_Open = false;
			}

			if( ImGui::Button( "Cancel" ) )
			{
				m_ShowDirtyPopup = false;
			}

			ImGui::EndHorizontal();

			ImGui::EndPopup();
		}
	}

	void PrefabViewer::OnImGuiRender()
	{
#if !defined(SAT_DIST)
		ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;
		if( m_DisableWindowMovement )
			flags |= ImGuiWindowFlags_NoMove;

		// Root Window.
		ImGui::Begin( m_Name.c_str(), &m_Open, flags );

		const bool mainWindowDocked = ImGui::IsWindowDocked();

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

			if( ImGui::BeginMenu( "Auxiliary" ) )
			{
				if( ImGui::BeginMenu( "Scene Visualisation Options" ) )
				{
					auto scene = m_Prefab->GetScene();

					auto& rVisualisationOptions = scene->GetVisualisationOptions();

					ImGui::Checkbox( "Show Grid", &rVisualisationOptions.ShowGrid );

					if( ImGui::BeginMenu( "Physics Colliders Options" ) )
					{
						bool showNone = rVisualisationOptions.PhysColliderOptions == PhysicsColliderVisualisationOptions::Disabled;
						if( ImGui::Checkbox( "No Visualisation", &showNone ) )
						{
							if( showNone )
								rVisualisationOptions.PhysColliderOptions = PhysicsColliderVisualisationOptions::Disabled;
							else // Default to selected only
								rVisualisationOptions.PhysColliderOptions = PhysicsColliderVisualisationOptions::SelectedOnly;
						}

						bool showAll = rVisualisationOptions.PhysColliderOptions == PhysicsColliderVisualisationOptions::All;
						if( ImGui::Checkbox( "All", &showAll ) )
						{
							if( showAll )
								rVisualisationOptions.PhysColliderOptions = PhysicsColliderVisualisationOptions::All;
							else
								rVisualisationOptions.PhysColliderOptions = PhysicsColliderVisualisationOptions::Disabled;
						}

						bool showSelected = rVisualisationOptions.PhysColliderOptions == PhysicsColliderVisualisationOptions::SelectedOnly;
						if( ImGui::Checkbox( "Selected Only", &showSelected ) )
						{
							if( showSelected )
								rVisualisationOptions.PhysColliderOptions = PhysicsColliderVisualisationOptions::SelectedOnly;
							else
								rVisualisationOptions.PhysColliderOptions = PhysicsColliderVisualisationOptions::Disabled;
						}

						ImGui::EndMenu();
					}

					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		if( m_ShowDirtyPopup ) DrawDirtyPopup();

		//////////////////////////////////////////////////////////////////////////

		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );

		// Viewport
		const ImGuiWindowFlags vpflags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;

		const std::string Name = "Viewport##" + std::to_string( m_AssetID );

		ImGuiWindowClass windowClass; 
		windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_AutoHideTabBar;

		ImGui::SetNextWindowClass( &windowClass );
		ImGui::Begin( Name.c_str(), 0, vpflags );

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
		m_ViewportBounds = ImRect( minBound, maxBound );
		m_AllowCameraEvents = ImGui::IsMouseHoveringRect( minBound, maxBound ) && m_ViewportFocused || m_StartedRightClickInViewport;

		const auto viewportPosition = ImGui::GetWindowPos();

		auto selectedEntities = EntitySelectionManager::Get()->GetSelectionContexts( m_Prefab->GetScene().Get() );

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

		if( selectedEntities.size() && m_GizmoOperation != 0 )
		{
			ImGuizmo::SetOrthographic( false );
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect( viewportPosition.x, viewportPosition.y, m_ViewportSize.x, m_ViewportSize.y );

			const glm::mat4 Projection = m_SceneRenderer->GetRendererCamera().pCamera->ProjectionMatrix();
			const glm::mat4 View = m_SceneRenderer->GetRendererCamera().ViewMatrix;

			ImGuizmo::Manipulate( 
				glm::value_ptr( View ), 
				glm::value_ptr( Projection ), 
				( ImGuizmo::OPERATION ) m_GizmoOperation, 
				ImGuizmo::LOCAL, 
				glm::value_ptr( centerPoint ), 
				nullptr );

			// Figure out what window needs it's movement disabled
			// Four possible options:
			//  1) The main window is not docked and the viewport is    -> freeze main window
			//  2) The main window is docked but the viewport isn't     -> freeze viewport window
			//  3) No windows are docked                                -> freeze viewport window
			//  4) All windows are docked								-> nothing to do
			// Outcome 1
			if( !mainWindowDocked && ImGui::IsWindowDocked() && ImGuizmo::IsOver() )
			{
				m_DisableWindowMovement = true;
			}
			// Outcome 2
			else if( !ImGui::IsWindowDocked() && ImGuizmo::IsOver() )
			{
				m_DisableViewportMovement = true;
			}
			// Outcome 3
			else if( ( !mainWindowDocked && !ImGui::IsWindowDocked() ) && ImGuizmo::IsOver() )
			{
				// Only disable viewport, no need to disable main window...
				m_DisableViewportMovement = true;
			}
			// Outcome 4
			else if( ( m_DisableViewportMovement || m_DisableWindowMovement ) && !ImGuizmo::IsOver() )
			{
				m_DisableViewportMovement = false;
				m_DisableWindowMovement = false;
			}

			if( ImGuizmo::IsUsing() )
			{
				for( SharedPtr<Entity>& rEntity : selectedEntities )
				{
					auto& tc = rEntity->GetComponent<TransformComponent>();

					// Set new transform
					glm::mat4 transform = g_ActiveScene->GetTransformRelativeToParent( rEntity );

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
			}
		}

		ImGui::End(); // Viewport

		ImGui::PopStyleVar(); // ImGuiStyleVar_WindowPadding

		// Scene Hierarchy panel
		if( m_SceneHierarchyPanel->IsOpen() )
			m_SceneHierarchyPanel->OnImGuiRender();

		ImGui::End(); // Root window

		if( m_Dirty && !m_Open )
		{
			m_Open = m_ShowDirtyPopup = true;
		}
#endif
	}

	void PrefabViewer::OnUpdate( Timestep ts )
	{
		m_Dirty = m_Prefab->GetScene()->IsDirty();

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

		switch( rEvent.Type )
		{
			case EventType::KeyPressed:
			{
				OnKeyPressed( ( RubyKeyEvent& ) rEvent );
			} break;

			case EventType::MousePressed:
			{
				OnMousePressed( ( RubyMouseEvent& ) rEvent );
			} break;

			default:
				break;
		}
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
