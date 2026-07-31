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

#include "Saturn/Vulkan/SceneRenderer.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include "Saturn/ImGui/EditorIcons.h"
#include "Saturn/ImGui/EditorEvents.h"

#include <imgui.h>
#include <ImGuizmo/ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>

namespace Saturn {

	SkeletonAssetViewer::SkeletonAssetViewer( AssetID id )
		: AssetViewer( id )
	{
		m_AssetType = AssetType::Skeleton;

		Ref<SkeletonAsset> snd = AssetManager::Get()->GetAssetAs<SkeletonAsset>( m_AssetID );
		m_SkeletonAsset = snd;

		m_Open = true;
		m_Name = std::format( "{0}##{1}", m_SkeletonAsset->Name, std::to_string( m_SkeletonAsset->ID ) );

		m_BoneHierarchyPanel.Initialise( id );

		// Make sure this panel has a unique name.
		m_BoneHierarchyPanel.AppendToName( std::format( "##{}1", ( uint64_t ) m_AssetID ) );

		m_Scene = Ref<Scene>::Create();
		m_Viewport = std::make_unique<EditorViewport>( VP_DefaultSub );
		
		const std::string vpName = std::format( "Viewport##{0}", ( uint64_t ) m_AssetID );
		m_Viewport->Initialise( SceneRendererFlag_NoAlura, m_Scene, vpName, m_AssetID );

		PickBestMesh();
	}

	SkeletonAssetViewer::~SkeletonAssetViewer()
	{
		m_MeshEntity = nullptr;
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

			m_Viewport->Draw( false );

			const auto viewportPosition = ImGui::GetWindowPos();
			const auto viewportSize = m_Viewport->GetSize();

			if( auto* pSelectedNode = m_BoneHierarchyPanel.GetSelectedItem() )
			{
				if( pSelectedNode->pItem->Type == SkelItemType::AttachmentPoint )
				{
					SkelAttachmentPoint* pAttachmentPoint = dynamic_cast< SkelAttachmentPoint* >( pSelectedNode->pItem );
					if( pAttachmentPoint )
					{
						auto* pBoneJoint = pAttachmentPoint->pBoneJoint;

						// Get world space transform.
						glm::mat4 ts = glm::translate( glm::mat4( 1.0f ), pBoneJoint->GetRelativePosition() )
							* glm::toMat4( pBoneJoint->GetRelativeRotation() );

						// Horrid copy.
						auto boneTransform = m_SkeletalMesh->GetDefaultBoneTransforms().at( pBoneJoint->GetBoneIndex() );

						ImGuizmo::SetOrthographic( false );
						ImGuizmo::SetDrawlist();
						ImGuizmo::SetRect( viewportPosition.x, viewportPosition.y, viewportSize.x, viewportSize.y );

						ImGuizmo::Manipulate(
							glm::value_ptr( m_Viewport->GetCamera().ViewMatrix() ),
							glm::value_ptr( m_Viewport->GetCamera().ProjectionMatrix() ),
							( ImGuizmo::OPERATION ) m_Viewport->GetGizmoOperation(),
							ImGuizmo::LOCAL,
							glm::value_ptr( ts ),
							glm::value_ptr( boneTransform )
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
							m_Viewport->DisableViewportMovement( true );
						}
						// Outcome 3
						else if( ( !mainWindowDocked && !ImGui::IsWindowDocked() ) && ImGuizmo::IsOver() )
						{
							// Only disable viewport, no need to disable main window...
							m_Viewport->DisableViewportMovement( true );
						}
						// Outcome 4
						else if( ( m_Viewport->IsViewportMovementDisabled() || m_DisableWindowMovement ) && !ImGuizmo::IsOver() )
						{
							m_Viewport->DisableViewportMovement( false );
							m_DisableWindowMovement = false;
						}

						if( ImGuizmo::IsUsing() )
						{
							glm::vec3 translation;
							glm::vec3 rotation;
							glm::vec3 scale;
							Maths::DecomposeTransform( ts, translation, rotation, scale );

							switch( m_Viewport->GetGizmoOperation() )
							{
								case ImGuizmo::OPERATION::TRANSLATE:
								{
									pBoneJoint->SetRelativePosition( translation );
								} break;

								case ImGuizmo::OPERATION::ROTATE:
								{
									const glm::vec3 DeltaRotation = rotation - glm::eulerAngles( pBoneJoint->GetRelativeRotation() );

									pBoneJoint->SetRelativeRotation( glm::eulerAngles( pBoneJoint->GetRelativeRotation() ) += DeltaRotation );
								} break;

								case ImGuizmo::OPERATION::SCALE:
								{
									pBoneJoint->SetRelativeScale( scale );
								} break;

								default:
									break;
							}
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
						const auto& rBoneTransform = m_SkeletalMesh->GetDefaultBoneTransforms()[ pBoneItem->BoneIndex ];

						glm::mat4 offsetTransform = glm::mat4( 1.0f );
						glm::mat4 ts = rBoneTransform;

						ImGuizmo::SetOrthographic( false );
						ImGuizmo::SetDrawlist();
						ImGuizmo::SetRect( viewportPosition.x, viewportPosition.y, viewportSize.x, viewportSize.y );

						ImGuizmo::Manipulate(
							glm::value_ptr( m_Viewport->GetCamera().ViewMatrix() ),
							glm::value_ptr( m_Viewport->GetCamera().ProjectionMatrix() ),
							ImGuizmo::TRANSLATE,
							ImGuizmo::LOCAL,
							glm::value_ptr( ts ),
							glm::value_ptr( offsetTransform )
						);
					}
				}
			}

			m_Viewport->EndDrawing();
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
				Auxiliary::ScopedDisabledFlag disabledIfReadOnly( m_IsReadOnly );

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
		if( ImGui::Begin( "Compatible Meshes", &m_ShowCompatibleMeshes ) ) 
		{
			Auxiliary::ScopedDisabledFlag disabledIfReadOnly( m_IsReadOnly );

			for( const auto& rMeshID : m_SkeletonAsset->GetCompatibleMeshes() )
			{
				ImGui::BeginHorizontal( ( int ) rMeshID );

				ImGui::Text( "%" PRIu64, rMeshID );
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
		m_Viewport->OnUpdate( ts );
	}

	void SkeletonAssetViewer::OnEvent( Event& rEvent )
	{
		if( ( rEvent.Category & EC_Ruby ) != 0 )
			m_Viewport->OnEvent( rEvent );

		if( ( rEvent.Category & EC_Editor ) != 0 )
		{
			switch( rEvent.Type )
			{
				case EventType::BoneHierarchyPanel_AddPreviewMesh:
				{
					OnPreviewMeshAdded& rAddedEvent = ( OnPreviewMeshAdded& ) rEvent;

					// Check if the in-coming event is our asset, because multiple
					// assets viewers may be open at once.
					if( rAddedEvent.GetSkeletonAssetID() == m_SkeletalMesh->GetSkeletonAsset()->ID )
					{
						const std::string entityName = rAddedEvent.GetPreviewName() + rAddedEvent.GetBoneJointName();

						SharedPtr<Entity> entity = m_Scene->CreateEntity( entityName );
						entity->AttachToBone( m_MeshEntity, rAddedEvent.GetBoneJointName() );
						entity->AddComponent<StaticMeshComponent>();
					}
				} break;

				case EventType::BoneHierarchyPanel_PreviewMeshStructurallyModified: 
				{
					OnPreviewMeshMeshChange& rMeshChanged = ( OnPreviewMeshMeshChange& ) rEvent;

					// Check if the in-coming event is our asset, because multiple
					// assets viewers may be open at once.
					if( rMeshChanged.GetSkeletonAssetID() == m_SkeletalMesh->GetSkeletonAsset()->ID )
					{
						const std::string entityName = rMeshChanged.GetPreviewName() + rMeshChanged.GetBoneJointName();

						if( auto entity = m_Scene->FindEntityByTag( entityName ) )
						{
							auto& mc = entity->GetComponent<StaticMeshComponent>();
							mc.Mesh = AssetManager::Get()->GetAssetAs<StaticMesh>( rMeshChanged.GetMeshID() );
							mc.MaterialRegistry = Ref<MaterialRegistry>::Create( mc.Mesh );
						}
					}
				} break;

				case EventType::BoneHierarchyPanel_RemovePreviewMesh:
				{
					OnPreviewMeshRemoved& rPreviewMeshRemoved = ( OnPreviewMeshRemoved& ) rEvent;

					// Check if the in-coming event is our asset, because multiple
					// assets viewers may be open at once.
					if( rPreviewMeshRemoved.GetSkeletonAssetID() == m_SkeletalMesh->GetSkeletonAsset()->ID )
					{
						const std::string entityName = rPreviewMeshRemoved.GetPreviewName() + rPreviewMeshRemoved.GetBoneJointName();
						
						if( auto entity = m_Scene->FindEntityByTag( entityName ) )
						{
							m_Scene->DeleteEntity( entity, true );
						}
					}
				} break;

				case EventType::BoneHierarchyPanel_PreviewMeshRenamed:
				{
					OnPreviewMeshRenamed& rPreviewMeshRenamed = ( OnPreviewMeshRenamed& ) rEvent;

					// Check if the in-coming event is our asset, because multiple
					// assets viewers may be open at once.
					if( rPreviewMeshRenamed.GetSkeletonAssetID() == m_SkeletalMesh->GetSkeletonAsset()->ID )
					{
						const std::string entityName = rPreviewMeshRenamed.GetOldName() + rPreviewMeshRenamed.GetBoneJointName();

						if( auto entity = m_Scene->FindEntityByTag( entityName ) )
						{
							// Rename to the new name.
							entity->SetName( rPreviewMeshRenamed.GetNewName() + rPreviewMeshRenamed.GetBoneJointName() );
						}
					}
				} break;

				default:
					break;
			}
		}
	}

	void SkeletonAssetViewer::PickBestMesh()
	{
#if !defined(SAT_DIST)
		for( const auto& rMeshID : m_SkeletonAsset->GetCompatibleMeshes() )
		{
			m_MeshEntity = m_Scene->CreateEntity();

			// Pick the first one
			auto& skComp = m_MeshEntity->AddComponent<SkeletalMeshComponent>();
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
