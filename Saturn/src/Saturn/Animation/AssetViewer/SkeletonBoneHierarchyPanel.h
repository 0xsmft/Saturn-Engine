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

#pragma once

#include "Saturn/ImGui/ImGuiWindow.h"
#include "Saturn/Animation/SkeletonAsset.h"

namespace Saturn {

	enum class SkelItemType : uint8_t
	{
		Unknown,
		Bone,
		AttachmentPoint
	};

	//
	// Base class for an item in the skeleton tree
	//
	class SkelItem
	{
	public:
		virtual ~SkelItem() = default;

		SkelItemType Type = SkelItemType::Unknown;
	};

	//
	// Represents a bone in the skeleton tree.
	//
	class SkelBoneItem : public SkelItem
	{
	public:
		virtual ~SkelBoneItem() = default;

		uint32_t BoneIndex = ~0u;
	};

	//
	// Represents a bone attachment point in the skeleton tree.
	//
	class SkelAttachmentPoint : public SkelItem
	{
	public:
		virtual ~SkelAttachmentPoint() = default;

		// The underlying bone joint stored in the Asset.
		BoneJoint* pBoneJoint = nullptr;
	};

	class SkeletonBoneHierarchyPanel : public ImGuiWindow
	{
	public:
		struct SkelItemNode
		{
			~SkelItemNode() 
			{
				Clear();
			}

			void Clear() 
			{
//				delete pItem;
				for( auto* pChild : Children )
				{
					pChild->Clear();
					delete pChild;
				}

				Children.clear();
			}

			// The item we represent, can be a bone or a joint.
			SkelItem* pItem = nullptr;

			// Parent item.
			SkelItemNode* pParent = nullptr;

			std::vector<SkelItemNode*> Children;
		};

	public:
		SkeletonBoneHierarchyPanel();
		virtual ~SkeletonBoneHierarchyPanel();

		void AppendToName( const std::string& rName );
		void Initialise( AssetID id );

		//////////////////////////////////////////////////////////////////////////
		// ImGuiWindow

		virtual void OnImGuiRender() override;
		virtual void OnEvent( Event& rEvent ) {}
		virtual void OnUpdate( Timestep ts ) {}

	public:
		SkelItemNode* GetSelectedItem() const { return m_pSelectedBone; }

	private:
		void DisplayBoneHierarchy( SkelItemNode* pBoneNode, int level = 0 );
		void ClearTree();

		void DrawInspector();
		void DrawInspectorForAP();
		void DrawInspectorForBone();

		void DrawContextOptionsBone();
		void DrawContextOptionsAP();

	private:
		Ref<SkeletonAsset> m_SkeletonAsset;

		SkelItemNode* m_pSelectedBone = nullptr;

		std::vector<SkelItemNode*> m_BoneTree;
		std::vector<SkelItemNode*> m_BoneTreeRoots;
	};

}
