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
			ClearLinkedList();
		}

		m_SkeletonAsset = AssetManager::Get().GetAssetAs<SkeletonAsset>( id );

		m_BoneLinkedList.resize( m_SkeletonAsset->GetBoneInfo().size() );

		// Create nodes
		for( size_t i = 0; i < m_SkeletonAsset->GetBoneInfo().size(); ++i )
		{
			SkelBoneItem* pBoneItem = new SkelBoneItem();
			pBoneItem->pBone = ( SkeletalMeshBoneInfo* ) &m_SkeletonAsset->GetBoneInfo()[ i ];
			pBoneItem->Type = SkelItemType::Bone;

			m_BoneLinkedList[ i ] = new SkelItemNode( pBoneItem );

			// TODO: Fix, should not be calling empty to check if a bone is valid
			if( auto& rBone = m_SkeletonAsset->FindBoneJoint( pBoneItem->pBone->BoneName ); !rBone.GetBoneName().empty() )
			{
				SkelAttachmentPoint* pAttachmentPoint = new SkelAttachmentPoint();
				pAttachmentPoint->Type = SkelItemType::Bone;
				pAttachmentPoint->pBoneJoint = &rBone;

				m_BoneLinkedList[ i ]->Children.push_back( new SkelItemNode( pAttachmentPoint ) );
			}
		}

		for( size_t i = 0; i < m_SkeletonAsset->GetBoneInfo().size(); ++i )
		{
			const int parentIndex = m_SkeletonAsset->GetBoneInfo()[ i ].ParentIndex;

			if( parentIndex >= 0 && parentIndex < ( int ) m_BoneLinkedList.size() )
			{
				m_BoneLinkedList[ parentIndex ]->Children.push_back( m_BoneLinkedList[ i ] );
			}
			else
			{
				m_BoneRoots.push_back( m_BoneLinkedList[ i ] );
			}
		}
	}

	void SkeletonBoneHierarchyPanel::ClearLinkedList()
	{
		m_BoneRoots.clear();

		for( auto* pNode : m_BoneLinkedList )
		{
			delete pNode;
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
			ImGui::Text( "Bone Name" );
			Auxiliary::InputText( "##boneName", &pBoneItem->pBone->BoneName, ImGuiInputTextFlags_ReadOnly );
			ImGui::EndHorizontal();
		}
	}

	void SkeletonBoneHierarchyPanel::DrawInspectorForAP()
	{
		SkelAttachmentPoint* pAttachmentPoint = dynamic_cast< SkelAttachmentPoint* >( m_pSelectedBone->pItem );
		if( pAttachmentPoint )
		{
			ImGui::Text( "Bone Name: %s", pAttachmentPoint->pBoneJoint->GetBoneName().c_str() );
		}
	}

	SkeletonBoneHierarchyPanel::~SkeletonBoneHierarchyPanel()
	{
		ClearLinkedList();
	}

	void SkeletonBoneHierarchyPanel::OnImGuiRender()
	{
		if( ImGui::Begin( m_Name.c_str(), &m_Open ) )
		{
			for( auto* pBone : m_BoneRoots )
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

									m_pSelectedBone->Children.push_back( new SkelItemNode( pAttachmentPoint ) );
								}
							}
						} break;

						case SkelItemType::AttachmentPoint: 
						{
							ImGui::SeparatorText( "ATTACHMENT POINT OPTIONS" );
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

	void SkeletonBoneHierarchyPanel::DisplayBoneHierarchy( SkelItemNode* pBoneNode, int level /*= 0 */ )
	{
		ImGuiTreeNodeFlags flags = ( m_pSelectedBone == pBoneNode ? ImGuiTreeNodeFlags_Selected : 0 ) | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;

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
					clicked = ImGui::TreeNodeEx( ( void* ) &pBoneNode->pItem, flags, pBoneItem->pBone->BoneName.c_str() );
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
