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
#include "SkeletonBoneHierarchyPanel.h"

#include "Saturn/Core/App.h"
#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include "Saturn/ImGui/EditorEvents.h"
#include "Saturn/ImGui/EditorIcons.h"

#include <imgui.h>

namespace Saturn {
	
	SkeletonBoneHierarchyPanel::SkeletonBoneHierarchyPanel()
		: ImGuiWindow( "Skeletal Mesh Bone Hierarchy" )
	{
	}

	void SkeletonBoneHierarchyPanel::Initialise( AssetID id )
	{
		// If we already have an asset, make sure to clear the existing linked list.
		if( m_SkeletonAsset )
		{
			ClearTree();
		}

		m_SkeletonAsset = AssetManager::Get()->GetAssetAs<SkeletonAsset>( id );

		m_BoneTree.resize( m_SkeletonAsset->GetBoneNames().size() );

		// Create tree from the bones
		for( size_t i = 0; i < m_SkeletonAsset->GetBoneNames().size(); ++i )
		{
			SkelBoneItem* pBoneItem = new SkelBoneItem();
			pBoneItem->BoneIndex = ( uint32_t ) i;
			pBoneItem->Type = SkelItemType::Bone;

			m_BoneTree[ i ] = new SkelItemNode( pBoneItem );

			// Add bone joint if the bone has one.
			if( auto* pBone = m_SkeletonAsset->FindBoneJointViaBoneName( m_SkeletonAsset->GetBoneName( pBoneItem->BoneIndex ) ) )
			{
				SkelAttachmentPoint* pAttachmentPoint = new SkelAttachmentPoint();
				pAttachmentPoint->Type = SkelItemType::AttachmentPoint;
				pAttachmentPoint->pBoneJoint = pBone;

				SkelItemNode* pNode = new SkelItemNode( pAttachmentPoint );
				pNode->pParent = m_BoneTree[ i ];

				m_BoneTree[ i ]->Children.push_back( pNode );
			}
		}

		for( size_t i = 0; i < m_SkeletonAsset->GetBoneNames().size(); ++i )
		{
			const auto parentIndex = m_SkeletonAsset->GetParentIndex( ( uint32_t ) i );
			if( parentIndex == ~0u )
			{
				m_BoneTreeRoots.push_back( m_BoneTree[ i ] );
			}
			else
			{
				m_BoneTree[ parentIndex ]->Children.push_back( m_BoneTree[ i ] );
				m_BoneTree[ i ]->pParent = m_BoneTree[ parentIndex ];
			}
		}

		m_InspectorName = std::format( "Inspector##{}", ( uint64_t ) id );
	}

	void SkeletonBoneHierarchyPanel::ClearTree()
	{
		for( auto* pBoneRoots : m_BoneTreeRoots )
		{
			delete pBoneRoots;
		}

		m_BoneTreeRoots.clear();
		m_BoneTree.clear();
	}

	SkeletonBoneHierarchyPanel::~SkeletonBoneHierarchyPanel()
	{
		ClearTree();
	}

	void SkeletonBoneHierarchyPanel::AppendToName( const std::string& rName )
	{
		m_Name += rName;
	}

	void SkeletonBoneHierarchyPanel::OnImGuiRender()
	{
		if( ImGui::Begin( m_Name.c_str(), &m_Open ) )
		{
			for( auto* pBone : m_BoneTreeRoots )
			{
				DisplayBoneHierarchy( pBone );
			}

			if( ImGui::IsMouseDown( 0 ) && ImGui::IsWindowHovered() )
			{
				m_pSelectedBone = nullptr;
			}

			if( ImGui::BeginPopupContextWindow( 0, ImGuiPopupFlags_MouseButtonRight ) )
			{
				if( m_pSelectedBone )
				{
					Auxiliary::ScopedDisabledFlag disabledIfRo( m_IsReadOnly );

					switch( m_pSelectedBone->pItem->Type )
					{
						case SkelItemType::Bone:
						{
							DrawContextOptionsBone();
						} break;

						case SkelItemType::AttachmentPoint:
						{
							DrawContextOptionsAP();
						} break;

						case SkelItemType::AttachmentPoint_PreviewMesh:
						{
							DrawContextOptionsPreviewMesh();
						} break;

						default:
							break;
					}
				}

				ImGui::EndPopup();
			}

			DrawInspector();

			ImGui::End();
		}
	}

	void SkeletonBoneHierarchyPanel::DrawInspector()
	{
		if( ImGui::Begin( m_InspectorName.c_str(), nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse ) )
		{
			if( m_pSelectedBone )
			{
				switch( m_pSelectedBone->pItem->Type )
				{
					case SkelItemType::Bone:
						DrawInspectorForBone();
						break;

					case SkelItemType::AttachmentPoint:
						DrawInspectorForAP();
						break;

					case SkelItemType::AttachmentPoint_PreviewMesh:
						DrawInspectorForPreviewMesh();
						break;

					default:
						break;
				}

			}
		}
		ImGui::End();
	}

	void SkeletonBoneHierarchyPanel::DrawInspectorForBone()
	{
		SkelBoneItem* pBoneItem = dynamic_cast< SkelBoneItem* >( m_pSelectedBone->pItem );
		if( pBoneItem )
		{
			ImGui::BeginHorizontal( ( void* ) pBoneItem );
			ImGui::Text( "Bone Name: %s", m_SkeletonAsset->GetBoneName( pBoneItem->BoneIndex ).c_str() );
			ImGui::EndHorizontal();
		}
	}

	void SkeletonBoneHierarchyPanel::DrawInspectorForPreviewMesh()
	{
		SkelPreviewMesh* pPreviewMesh = dynamic_cast< SkelPreviewMesh* >( m_pSelectedBone->pItem );
		if( pPreviewMesh )
		{
			Auxiliary::ScopedDisabledFlag disabledIfRo( m_IsReadOnly );

			ImGui::BeginHorizontal( "##nameinput" );
			ImGui::Text( "Name" );
			if( Auxiliary::InputText( "##setname", &pPreviewMesh->Name, ImGuiInputTextFlags_EnterReturnsTrue ) ) 
			{
				OnRenameCommittedPreviewMesh( pPreviewMesh );
			}

			if( ImGui::IsItemDeactivatedAfterEdit() )
			{
				OnRenameCommittedPreviewMesh( pPreviewMesh );
			}

			ImGui::EndHorizontal();

			ImGui::Columns( 3 );
			// Arbitrary numbers...
			ImGui::SetColumnWidth( 0, 100.0f );
			ImGui::SetColumnWidth( 1, 300.0f );
			ImGui::SetColumnWidth( 2, 40.0f );
			ImGui::Text( "Mesh" );
			ImGui::NextColumn();
			ImGui::PushItemWidth( -1.0f );

			bool showFinder = false;
			if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), ImVec2( 24.0f, 24.0f ) ) )
			{
				showFinder = !showFinder;
			}

			ImGui::SameLine();

			auto assetID = pPreviewMesh->MeshID;
			ImGui::Text( "%" PRIu64, assetID );

			if( Auxiliary::DrawAssetFinder( AssetType::StaticMesh, &showFinder, assetID ) )
			{
				pPreviewMesh->MeshID = assetID;

				// Get the parent bone joint because the SkeletonAssetViewer needs it for it's
				// entity.
				SkelAttachmentPoint* pParentBoneJoint = dynamic_cast< SkelAttachmentPoint* >( m_pSelectedBone->pParent->pItem );
				if( pParentBoneJoint )
				{
					Application::Get()->DispatchEvent<OnPreviewMeshMeshChange>(
						m_SkeletonAsset->ID,
						pPreviewMesh->MeshID,
						pParentBoneJoint->pBoneJoint->GetName(),
						pPreviewMesh->Name );
				}
			}

			ImGui::PopItemWidth();
			ImGui::NextColumn();
		}
	}

	void SkeletonBoneHierarchyPanel::DrawInspectorForAP()
	{
		SkelAttachmentPoint* pAttachmentPoint = dynamic_cast< SkelAttachmentPoint* >( m_pSelectedBone->pItem );
		if( pAttachmentPoint )
		{
			BoneJoint* pBoneJoint = pAttachmentPoint->pBoneJoint;

			ImGui::Text( "Bone Name: %s", pBoneJoint->GetBoneName().c_str() );

			Auxiliary::ScopedDisabledFlag disabledIfRo( m_IsReadOnly );

			ImGui::BeginHorizontal( "##nameinput" );
			ImGui::Text( "Name" );
			Auxiliary::InputText( "##setname", &pBoneJoint->m_Name );
			ImGui::EndHorizontal();

			ImGui::Separator();

			Auxiliary::DrawVec3Control( "Position", pBoneJoint->m_Position );

			glm::vec3 rotationDeg = glm::degrees( glm::eulerAngles( pBoneJoint->m_Rotation ) );
			if( Auxiliary::DrawVec3Control( "Rotation", rotationDeg ) )
			{
				pBoneJoint->m_Rotation = glm::quat( glm::radians( rotationDeg ) );
			}

			Auxiliary::DrawVec3Control( "Scale", pBoneJoint->m_Scale, 1.0f );
		}
	}

	void SkeletonBoneHierarchyPanel::DrawContextOptionsBone()
	{
		ImGui::SeparatorText( "Bone Options" );
		if( ImGui::MenuItem( "Create new attachment point (socket)" ) )
		{
			// Get the bone node data.
			const SkelBoneItem* pBoneItem = dynamic_cast< const SkelBoneItem* >( m_pSelectedBone->pItem );
			if( pBoneItem )
			{
				// Find bone.
				const auto& rBoneInfo = m_SkeletonAsset->GetBoneName( pBoneItem->BoneIndex );

				auto& rBoneJoint = m_SkeletonAsset->AddNewBoneJoint( pBoneItem->BoneIndex, rBoneInfo, "New Attachment Point" );

				SkelAttachmentPoint* pAttachmentPoint = new SkelAttachmentPoint();
				pAttachmentPoint->Type = SkelItemType::AttachmentPoint;
				pAttachmentPoint->pBoneJoint = &rBoneJoint;

				SkelItemNode* pNode = new SkelItemNode();
				pNode->pItem = pAttachmentPoint;
				pNode->pParent = m_pSelectedBone;

				m_pSelectedBone->Children.push_back( pNode );
			}
		}
	}

	void SkeletonBoneHierarchyPanel::DrawContextOptionsAP()
	{
		ImGui::SeparatorText( "Attachment Point Options" );

		if( ImGui::MenuItem( "Create preview mesh" ) )
		{
			SkelAttachmentPoint* pAttachmentPoint = dynamic_cast< SkelAttachmentPoint* >( m_pSelectedBone->pItem );
			if( pAttachmentPoint )
			{
				SkelPreviewMesh* pPreviewMesh = new SkelPreviewMesh();
				pPreviewMesh->Type = SkelItemType::AttachmentPoint_PreviewMesh;
				pPreviewMesh->Name = "New Preview Mesh";
				pPreviewMesh->OldName = pPreviewMesh->Name;

				SkelItemNode* pNode = new SkelItemNode();
				pNode->pItem = pPreviewMesh;
				pNode->pParent = m_pSelectedBone;

				m_pSelectedBone->Children.push_back( pNode );

				Application::Get()->DispatchEvent<OnPreviewMeshAdded>(
					m_SkeletonAsset->ID,
					pAttachmentPoint->pBoneJoint->GetName(),
					pPreviewMesh->Name );
			}
		}
			
		if( ImGui::MenuItem( "Delete" ) )
		{
			SkelAttachmentPoint* pAttachmentPoint = dynamic_cast< SkelAttachmentPoint* >( m_pSelectedBone->pItem );
			if( pAttachmentPoint )
			{
				m_BoneTree.erase( std::remove( m_BoneTree.begin(), m_BoneTree.end(), m_pSelectedBone ), m_BoneTree.end() );

				m_pSelectedBone->pParent->Children.erase( 
					std::remove( 
						m_pSelectedBone->pParent->Children.begin(),
						m_pSelectedBone->pParent->Children.end(),
						m_pSelectedBone ), 
					m_pSelectedBone->pParent->Children.end()
				);

				auto* pOldSelected = m_pSelectedBone;
				m_pSelectedBone = m_pSelectedBone->pParent;

				delete pOldSelected;
			}
		}
	}

	void SkeletonBoneHierarchyPanel::DrawContextOptionsPreviewMesh()
	{
		ImGui::SeparatorText( "Preview Mesh Options" );

		if( ImGui::MenuItem( "Delete" ) )
		{
			SkelPreviewMesh* pPrewviewMesh = dynamic_cast< SkelPreviewMesh* >( m_pSelectedBone->pItem );
			if( pPrewviewMesh )
			{
				SkelAttachmentPoint* pParentBoneJoint = dynamic_cast< SkelAttachmentPoint* >( m_pSelectedBone->pParent->pItem );
				if( pParentBoneJoint )
				{
					// Differ event.
					Application::Get()->DispatchEvent<OnPreviewMeshRemoved>(
						m_SkeletonAsset->ID,
						pParentBoneJoint->pBoneJoint->GetName(),
						pPrewviewMesh->Name );
				}
				
				// Remove from bone tree.
				m_BoneTree.erase( std::remove( m_BoneTree.begin(), m_BoneTree.end(), m_pSelectedBone ), m_BoneTree.end() );

				// Remove from our parent's map of the children.
				m_pSelectedBone->pParent->Children.erase(
					std::remove(
						m_pSelectedBone->pParent->Children.begin(),
						m_pSelectedBone->pParent->Children.end(),
						m_pSelectedBone ),
					m_pSelectedBone->pParent->Children.end()
				);

				// Delete and set selected to parent.
				auto* pOldSelected = m_pSelectedBone;
				m_pSelectedBone = m_pSelectedBone->pParent;

				delete pOldSelected;
			}
		}
	}

	void SkeletonBoneHierarchyPanel::OnRenameCommittedPreviewMesh( SkelPreviewMesh* pPreviewMeshNode )
	{
		SkelAttachmentPoint* pParentBoneJoint = dynamic_cast< SkelAttachmentPoint* >( m_pSelectedBone->pParent->pItem );
		if( pParentBoneJoint )
		{
			Application::Get()->DispatchEvent<OnPreviewMeshRenamed>(
				m_SkeletonAsset->ID,
				pParentBoneJoint->pBoneJoint->GetName(),
				pPreviewMeshNode->OldName,
				pPreviewMeshNode->Name );

			// After the event the names can now match.
			pPreviewMeshNode->OldName = pPreviewMeshNode->Name;
		}
	}

	void SkeletonBoneHierarchyPanel::DisplayBoneHierarchy( SkelItemNode* pBoneNode, int level /*= 0 */ )
	{
		ImGuiTreeNodeFlags flags = ( m_pSelectedBone == pBoneNode ? ImGuiTreeNodeFlags_Selected : 0 ) | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_OpenOnArrow;

		if( level == 0 )
			flags |= ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed;

		bool clicked = false;
		switch( pBoneNode->pItem->Type )
		{
			case SkelItemType::Bone:
			{
				SkelBoneItem* pBoneItem = dynamic_cast< SkelBoneItem* >( pBoneNode->pItem );
				if( pBoneItem )
				{
					const auto& rBoneName = m_SkeletonAsset->GetBoneName( pBoneItem->BoneIndex );
					clicked = ImGui::TreeNodeEx( ( void* ) &pBoneNode->pItem, flags, rBoneName.c_str() );
				}
			} break;

			case SkelItemType::AttachmentPoint:
			{
				SkelAttachmentPoint* pAttachmentPoint = dynamic_cast< SkelAttachmentPoint* >( pBoneNode->pItem );
				if( pAttachmentPoint )
				{
					clicked = ImGui::TreeNodeEx( ( void* ) &pBoneNode->pItem, flags, pAttachmentPoint->pBoneJoint->GetName().c_str() );
				}
			} break;

			case SkelItemType::AttachmentPoint_PreviewMesh:
			{
				SkelPreviewMesh* pPreviewMesh = dynamic_cast< SkelPreviewMesh* >( pBoneNode->pItem );
				if( pPreviewMesh )
				{
					clicked = ImGui::TreeNodeEx( ( void* ) &pBoneNode->pItem, flags, pPreviewMesh->Name.c_str() );
				}
			} break;

			default:
				break;
		}

		if( ImGui::IsItemClicked( ImGuiMouseButton_Left ) || ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
		{
			m_pSelectedBone = pBoneNode;
		}

		if( clicked )
		{
			for( auto* pChildBone : pBoneNode->Children )
			{
				DisplayBoneHierarchy( pChildBone, level + 1 );
			}

			Auxiliary::EndTreeNode();
		}
	}

}
