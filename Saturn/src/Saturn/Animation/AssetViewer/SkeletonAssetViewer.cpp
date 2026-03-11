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
#include "SkeletonAssetViewer.h"

#include "Saturn/Asset/AssetManager.h"

// TOOD: #FixSceneRendererIncludes
#include "Saturn/Vulkan/Renderer2D.h"
#include "Saturn/Vulkan/AluraRenderer.h"
#include "Saturn/Vulkan/SceneRenderer.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include "Saturn/ImGui/EditorIcons.h"

#include "Saturn/Core/Renderer/EditorCamera.h"

#include <imgui.h>
#include <ImGuizmo/ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>

namespace Saturn {

	SkeletonAssetViewer::SkeletonAssetViewer( AssetID id )
		: AssetViewer( id ), SubSceneRendererWindow()
	{
		m_AssetType = AssetType::Skeleton;

		Ref<SkeletonAsset> snd = AssetManager::Get()->GetAssetAs<SkeletonAsset>( m_AssetID );
		m_SkeletonAsset = snd;

		m_Open = true;
		m_Name = std::format( "{0}##{1}", m_SkeletonAsset->Name, std::to_string( m_SkeletonAsset->ID ) );

		m_BoneHierarchyPanel.Initialise( id );
	
		Initialise();
		SetViewportWindowID( m_AssetID );
		PickBestMesh();
	}

	SkeletonAssetViewer::~SkeletonAssetViewer()
	{
	}

	void SkeletonAssetViewer::OnImGuiRender()
	{
		ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;
		if( m_DisableWindowMovement )
			flags |= ImGuiWindowFlags_NoMove;

		if( ImGui::Begin( m_Name.c_str(), &m_Open, flags ) )
		{
			const bool mainWindowDocked = ImGui::IsWindowDocked();

			// Create custom dockspace.
			const ImGuiID dockID = ImGui::GetID( "SkMeshDckspc" );
			ImGui::DockSpace( dockID, ImVec2( 0.0f, 0.0f ), ImGuiDockNodeFlags_None );
			
			//////////////////////////////////////////////////////////////////////////

			if( ImGui::BeginMenuBar() )
			{
				if( ImGui::BeginMenu( "File" ) )
				{
					if( ImGui::MenuItem( "Close" ) )
					{
						m_Open = false;
					}

					ImGui::EndMenu();
				}

				if( ImGui::BeginMenu( "View" ) )
				{
					if( ImGui::MenuItem( "Compatible Meshes" ) )
					{
						m_ShowCompatibleMeshes ^= 1;
					}

					if( ImGui::MenuItem( "Bone Hierarchy Panel" ) )
					{
						m_BoneHierarchyPanel.ShowOrHide();
					}

					ImGui::EndMenu();
				}

				if( ImGui::BeginMenu( "Debug" ) )
				{
					if( ImGui::MenuItem( "Raw Viewer" ) )
					{
						m_ShowAdvRawViewer ^= 1;
					}

					ImGui::EndMenu();
				}

				ImGui::EndMenuBar();
			}

			//////////////////////////////////////////////////////////////////////////

			if( m_BoneHierarchyPanel.IsOpen() ) m_BoneHierarchyPanel.OnImGuiRender();

			//////////////////////////////////////////////////////////////////////////

			if( m_ShowCompatibleMeshes ) DrawCompatibleMeshes();
			if( m_ShowFinderModal ) DrawPickCompatibleMeshWindow();

			//////////////////////////////////////////////////////////////////////////

			if( m_ShowAdvRawViewer ) DrawAdvRawViewer();

			//////////////////////////////////////////////////////////////////////////

			RenderViewport( false );

			ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );

			const auto viewportPosition = ImGui::GetWindowPos();
			const auto viewportSize = GetViewportSize();

			if( auto* pSelectedNode = m_BoneHierarchyPanel.GetSelectedItem() )
			{
				if( pSelectedNode->pItem->Type == SkelItemType::AttachmentPoint )
				{
					SkelAttachmentPoint* pAttachmentPoint = dynamic_cast< SkelAttachmentPoint* >( pSelectedNode->pItem );
					if( pAttachmentPoint )
					{
						glm::mat4 offsetTransform = glm::mat4( 1.0f );
						glm::mat4 ts = glm::translate( glm::mat4( 1.0f ), pAttachmentPoint->pBoneJoint->GetRelativePosition() ) * glm::toMat4( pAttachmentPoint->pBoneJoint->GetRelativeRotation() ) * glm::scale( glm::mat4( 1.0f ), pAttachmentPoint->pBoneJoint->GetRelativeScale() );

						ts = pAttachmentPoint->pBoneJoint->GetBoneMatrixPreview( m_SkeletalMesh );

						ImGuizmo::SetOrthographic( false );
						ImGuizmo::SetDrawlist();
						ImGuizmo::SetRect( viewportPosition.x, viewportPosition.y, viewportSize.x, viewportSize.y );

						ImGuizmo::Manipulate(
							glm::value_ptr( m_Camera.ViewMatrix() ),
							glm::value_ptr( m_Camera.ProjectionMatrix() ),
							ImGuizmo::TRANSLATE,
							ImGuizmo::LOCAL,
							glm::value_ptr( ts ),
							glm::value_ptr( offsetTransform )
						);

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
							glm::vec3 translation;
							glm::vec3 rotation;
							glm::vec3 scale;
							Maths::DecomposeTransform( ts * offsetTransform, translation, rotation, scale );

							const glm::vec3 DeltaRotation = rotation - glm::eulerAngles( pAttachmentPoint->pBoneJoint->GetRelativeRotation() );

							pAttachmentPoint->pBoneJoint->SetRelativePosition( translation );
							pAttachmentPoint->pBoneJoint->SetRelativeRotation( glm::eulerAngles( pAttachmentPoint->pBoneJoint->GetRelativeRotation() ) += DeltaRotation );
							pAttachmentPoint->pBoneJoint->SetRelativeScale( scale );
						}
						else
						{
							/*
							if( ( !mainWindowDocked || !ImGui::IsWindowDocked() ) || m_DisableViewportMovement || m_DisableWindowMovement )
							{
								m_DisableViewportMovement = false;
								m_DisableWindowMovement = false;
							}
							*/
						}
					}
				}
				else
				{
					SkelBoneItem* pBoneItem = dynamic_cast< SkelBoneItem* >( pSelectedNode->pItem );
					if( pBoneItem )
					{
						if( m_SkeletalMesh )
						{
							const auto& rBoneTransform = m_SkeletalMesh->GetDefaultBoneTransforms()[ pBoneItem->BoneIndex ];

							glm::mat4 offsetTransform = glm::mat4( 1.0f );
							glm::mat4 ts = rBoneTransform;

							ImGuizmo::SetOrthographic( false );
							ImGuizmo::SetDrawlist();
							ImGuizmo::SetRect( viewportPosition.x, viewportPosition.y, viewportSize.x, viewportSize.y );

							ImGuizmo::Manipulate(
								glm::value_ptr( m_Camera.ViewMatrix() ),
								glm::value_ptr( m_Camera.ProjectionMatrix() ),
								ImGuizmo::TRANSLATE,
								ImGuizmo::LOCAL,
								glm::value_ptr( ts ),
								glm::value_ptr( offsetTransform )
							);
						}
					}
				}
			}

			ImGui::PopStyleVar();

			End();
		}

		ImGui::End();

		if( m_Open == false )
		{
			m_SkeletonAsset->PortToNewestVersion();
			SkeletonAssetSerialiser sas;
			sas.Serialise( m_SkeletonAsset );
		}
	}

	void SkeletonAssetViewer::DrawAdvRawViewer()
	{
		if( ImGui::Begin( "Skeleton Raw Data Viewer", &m_ShowAdvRawViewer ) ) 
		{
			if( Auxiliary::TreeNode( "Bone Joints", false ) )
			{
				auto& rBoneJoints = m_SkeletonAsset->GetBoneJoints();
				for( auto itr = rBoneJoints.begin(); itr != rBoneJoints.end(); )
				{
					auto& rJoint = *itr;

					ImGui::BeginHorizontal( &rJoint );
					ImGui::TextDisabled( "%s", rJoint.GetName().c_str() );
					if( ImGui::SmallButton( "-" ) )
					{
						itr = rBoneJoints.erase( itr );
					}
					else
					{
						++itr;
					}
					ImGui::EndHorizontal();
				}

				Auxiliary::EndTreeNode();
			}
		}
		ImGui::End();
	}

	void SkeletonAssetViewer::DrawCompatibleMeshes()
	{
#if !defined(SAT_DIST)
		// Compatibility Information
		ImGui::Begin( "Compatible Meshes" );
		for( const auto& rMeshID : m_SkeletonAsset->GetCompatibleMeshes() )
		{
			ImGui::BeginHorizontal( ( int ) rMeshID );

			ImGui::Text( "%llu", rMeshID );
			if( ImGui::SmallButton( "-" ) )
			{
				m_SkeletonAsset->MarkAsUncompatibleMesh( rMeshID );

				// Not great having this here, but will be fine for the time being.
				ImGui::EndHorizontal();
				break;
			}

			ImGui::EndHorizontal();
		}

		if( ImGui::SmallButton( "+" ) )
		{
			m_ShowFinderModal = true;
			ImGui::OpenPopup( "PickMesh##CompSk" );
		}
		ImGui::End();
#endif
	}

	void SkeletonAssetViewer::DrawPickCompatibleMeshWindow()
	{
#if !defined(SAT_DIST)
		bool* pOpen = nullptr;
		if( m_SkeletalMesh )
		{
			pOpen = &m_ShowFinderModal;
		}
		else
			ImGui::OpenPopup( "PickMesh##CompSk" );

		if( ImGui::BeginPopupModal( "PickMesh##CompSk", pOpen, ImGuiWindowFlags_NoSavedSettings ) )
		{
			ImGui::Text( "Please pick the mesh that you'd like to mark as compatible with this skeleton." );

			ImGui::BeginHorizontal( "##meshinfohz" );

			std::string inputTextData = m_TemporaryCompatibleMeshID == 0 ? "No Mesh" : std::to_string( m_TemporaryCompatibleMeshID );
			Auxiliary::InputText( "##name", &inputTextData, ImGuiInputTextFlags_ReadOnly );

			bool open = false;
			if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), ImVec2( 24.0f, 24.0f ) ) )
			{
				open ^= 1;
			}
			Auxiliary::DrawAssetFinder( AssetType::SkeletalMesh, &open, m_TemporaryCompatibleMeshID );

			ImGui::EndHorizontal();

			ImGui::Separator();
			ImGui::BeginHorizontal( "##options" );

			if( ImGui::Button( "Confirm" ) )
			{
				m_SkeletonAsset->AddCompatibleMesh( m_TemporaryCompatibleMeshID );

				if( !m_SkeletalMesh )
				{
					PickBestMesh();
				}

				m_TemporaryCompatibleMeshID = 0;
				m_ShowFinderModal = false;
				ImGui::CloseCurrentPopup();
			}

			Auxiliary::DisabledFlag disabledIf( !m_SkeletalMesh );

			if( ImGui::Button( "Cancel" ) )
			{
				m_TemporaryCompatibleMeshID = 0;
				m_ShowFinderModal = false;
				ImGui::CloseCurrentPopup();
			}

			disabledIf.Pop();

			ImGui::EndHorizontal();

			ImGui::EndPopup();
		}
#endif
	}

	void SkeletonAssetViewer::OnUpdate( Timestep ts )
	{
		OnUpdateRenderer( ts );
	}

	void SkeletonAssetViewer::OnEvent( Event& rEvent )
	{
		OnCameraEvent( rEvent );
	}

	void SkeletonAssetViewer::PickBestMesh()
	{
#if !defined(SAT_DIST)
		for( const auto& rMeshID : m_SkeletonAsset->GetCompatibleMeshes() )
		{
			// Pick the first one
			auto& skComp = m_Scene->CreateEntity()->AddComponent<SkeletalMeshComponent>();
			skComp.Mesh = AssetManager::Get()->GetAssetAs<SkeletalMesh>( rMeshID );
			skComp.MaterialRegistry = Ref<MaterialRegistry>::Create();
			skComp.LocalAnimator = Ref<Animator>::Create();

			m_SkeletalMesh = skComp.Mesh;
			break;
		}

		m_ShowFinderModal = ( m_SkeletalMesh == nullptr );
#endif
	}

}
