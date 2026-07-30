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

#include "SkeletonBoneHierarchyPanel.h"

#include "Saturn/ImGui/AssetViewer.h"
#include "Saturn/ImGui/EditorViewport.h"

namespace Saturn {

	class SceneRenderer;

	class SkeletonAssetViewer : public AssetViewer
	{
	public:
		SkeletonAssetViewer( AssetID id );
		virtual ~SkeletonAssetViewer();

		virtual void OnImGuiRender() override;
		virtual void OnUpdate( Timestep ts ) override;
		virtual void OnEvent( Event& rEvent ) override;

	private:
		void PickBestMesh();
		void DrawAdvRawViewer();
		void DrawCompatibleMeshes();
		void DrawPickCompatibleMeshWindow();

	private:
		UUID m_TemporaryCompatibleMeshID = 0;
		std::unique_ptr<EditorViewport> m_Viewport;
		Ref<SkeletonAsset> m_SkeletonAsset;
		Ref<Scene> m_Scene;
		Ref<SkeletalMesh> m_SkeletalMesh;
		SharedPtr<Entity> m_MeshEntity;

		SkeletonBoneHierarchyPanel m_BoneHierarchyPanel;

		bool m_ShowFinderModal = false;
		bool m_ShowAdvRawViewer = false;
		bool m_ShowCompatibleMeshes = true;
		// NOTE: This is different from m_DisableViewportMovement
		// We have a separate bool because our main window is not usually docked, 
		// so when we use our gizmo, we want to disable movement from our main window.
		// We need m_DisableViewportMovement because in a rare case our main window may be docked or the viewport window may be undocked from the main window, we'd want to do the same with our viewport window.
		bool m_DisableWindowMovement = false;
	};
	
}
