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

#pragma once

#include "Saturn/ImGui/ImGuiWindow.h"
#include "Saturn/Animation/SkeletonAsset.h"

namespace Saturn {

	class SkeletalMeshBoneHierarchyPanel : public ImGuiWindow
	{
	public:
		struct BoneNode
		{
			SkeletalMeshBoneInfo* pBone = nullptr;
			std::vector<BoneNode*> Children;
		};

	public:
		SkeletalMeshBoneHierarchyPanel();
		~SkeletalMeshBoneHierarchyPanel();

		void Initialise( AssetID id );

		//////////////////////////////////////////////////////////////////////////
		// ImGuiWindow

		virtual void OnImGuiRender() override;
		virtual void OnEvent( Event& rEvent ) {}
		virtual void OnUpdate( Timestep ts ) {}

	private:
		void DisplayBoneHierarchy( BoneNode* pBoneNode, int level = 0 );
		void ClearLinkedList();

	private:
		Ref<SkeletonAsset> m_SkeletonAsset;

		std::vector<BoneNode*> m_BoneLinkedList;
		std::vector<BoneNode*> m_BoneRoots;
	};

}
