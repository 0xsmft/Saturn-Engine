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

#include "Saturn/Vulkan/Mesh.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"

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
			/*
			if( auto* pBone = m_SkeletonAsset->FindBoneJoint( pBoneItem->pBone->BoneName ) )
			{
				SkelAttachmentPoint* pAttachmentPoint = new SkelAttachmentPoint();
				pAttachmentPoint->Type = SkelItemType::AttachmentPoint;
				pAttachmentPoint->pBoneJoint = pBone;

				SkelItemNode* pNode = new SkelItemNode( pAttachmentPoint );
				pNode->pParent = m_BoneTree[ i ];

				m_BoneTree[ i ]->Children.push_back( pNode );
			}
			*/
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
		ImGui::Begin( "Inspector##skel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse );
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

				default:
					break;
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
			ImGui::Text( "Bone Name: %s", m_SkeletonAsset->GetBoneName( pBoneItem->BoneIndex ) );
			ImGui::EndHorizontal();
		}
	}

	void SkeletonBoneHierarchyPanel::DrawInspectorForAP()
	{
		SkelAttachmentPoint* pAttachmentPoint = dynamic_cast< SkelAttachmentPoint* >( m_pSelectedBone->pItem );
		if( pAttachmentPoint )
		{
			BoneJoint* pBoneJoint = pAttachmentPoint->pBoneJoint;

			ImGui::Text( "Bone Name: %s", pBoneJoint->GetBoneName().c_str() );

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
#if SAT_FEATURE_BONE_ATTACHMENT
		ImGui::SeparatorText( "BONE OPTIONS" );
		if( ImGui::MenuItem( "Create new attachment point" ) )
		{
			const SkelBoneItem* pBoneItem = dynamic_cast< const SkelBoneItem* >( m_pSelectedBone->pItem );
			if( pBoneItem )
			{
				auto& rBone = m_SkeletonAsset->AddNewBoneJoint( pBoneItem->pBone->BoneName, "New Attachment" );

				SkelAttachmentPoint* pAttachmentPoint = new SkelAttachmentPoint();
				pAttachmentPoint->Type = SkelItemType::AttachmentPoint;
				pAttachmentPoint->pBoneJoint = &rBone;

				SkelItemNode* pNode = new SkelItemNode( pAttachmentPoint );
				pNode->pParent = m_pSelectedBone;

				m_pSelectedBone->Children.push_back( pNode );
			}
		}
#endif
	}

	void SkeletonBoneHierarchyPanel::DrawContextOptionsAP()
	{
		ImGui::SeparatorText( "ATTACHMENT POINT OPTIONS" );
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
