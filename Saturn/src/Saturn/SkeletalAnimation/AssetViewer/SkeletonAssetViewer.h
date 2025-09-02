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

#include "Saturn/ImGui/AssetViewer.h"
#include "Saturn/SkeletalAnimation/SkeletonAsset.h"

struct ImVec2;

namespace Saturn {

	class SceneRenderer;
	class EditorCamera;

	struct SkAVBoneNode
	{
		SkeletalMeshBoneInfo* pBone = nullptr;
		std::vector<SkAVBoneNode*> Children;
	};

	class SkeletonAssetViewer : public AssetViewer
	{
	public:
		SkeletonAssetViewer( AssetID id );
		~SkeletonAssetViewer();

		virtual void OnImGuiRender() override;
		virtual void OnUpdate( Timestep ts ) override;
		virtual void OnEvent( Event& rEvent ) override;

	private:
		void DisplayBoneHierarchy( SkAVBoneNode* pBoneNode, int level = 0 );

	private:
		Ref<SkeletonAsset> m_SkeletonAsset;
		Ref<SceneRenderer> m_SceneRenderer;
//		Ref<Scene> m_Scene;
//		EditorCamera m_Camera;

		bool m_AllowCameraEvents = false;
		bool m_StartedRightClickInViewport = false;
		bool m_ViewportFocused = false;
		bool m_MouseOverViewport = false;

//		ImVec2 m_ViewportSize{};

		std::vector<SkAVBoneNode*> m_BoneLinkedList;
		std::vector<SkAVBoneNode*> m_BoneRoots;
	};
	
}
