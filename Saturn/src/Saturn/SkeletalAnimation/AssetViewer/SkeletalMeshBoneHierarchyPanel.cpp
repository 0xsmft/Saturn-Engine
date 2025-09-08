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
#include "SkeletalMeshBoneHierarchyPanel.h"

#include "Saturn/Vulkan/Mesh.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"

#include <imgui.h>

namespace Saturn {
	
	SkeletalMeshBoneHierarchyPanel::SkeletalMeshBoneHierarchyPanel()
		: ImGuiWindow( "Skeletal Mesh Bone Hierarchy" )
	{
	}

	void SkeletalMeshBoneHierarchyPanel::Initialise( AssetID id )
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
			m_BoneLinkedList[ i ] = new BoneNode( ( SkeletalMeshBoneInfo* ) &m_SkeletonAsset->GetBoneInfo()[ i ], {} );
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

	void SkeletalMeshBoneHierarchyPanel::ClearLinkedList()
	{
		m_BoneRoots.clear();

		for( auto* pNode : m_BoneLinkedList )
		{
			delete pNode;
		}
	}

	SkeletalMeshBoneHierarchyPanel::~SkeletalMeshBoneHierarchyPanel()
	{
		ClearLinkedList();
	}

	void SkeletalMeshBoneHierarchyPanel::OnImGuiRender()
	{
		if( ImGui::Begin( m_Name.c_str(), &m_Open ) )
		{
			for( auto* pBone : m_BoneRoots )
			{
				DisplayBoneHierarchy( pBone );
			}

			ImGui::End();
		}
	}

	void SkeletalMeshBoneHierarchyPanel::DisplayBoneHierarchy( BoneNode* pBoneNode, int level /*= 0 */ )
	{
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;

		if( level == 0 )
			flags |= ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed;

		if( ImGui::TreeNodeEx( pBoneNode->pBone->BoneName.c_str(), flags ) )
		{
			for( auto* pChildBone : pBoneNode->Children )
			{
				DisplayBoneHierarchy( pChildBone, level + 1 );
			}

			Auxiliary::EndTreeNode();
		}
	}

}
