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
#include "SkeletonAssetViewer.h"

#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Vulkan/SceneRenderer.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include "Saturn/ImGui/EditorIcons.h"

#include "Saturn/Core/Renderer/EditorCamera.h"

#include <imgui.h>

namespace Saturn {

	SkeletonAssetViewer::SkeletonAssetViewer( AssetID id )
		: AssetViewer( id )
	{
		m_AssetType = AssetType::Skeleton;

		Ref<SkeletonAsset> snd = AssetManager::Get().GetAssetAs<SkeletonAsset>( m_AssetID );
		m_SkeletonAsset = snd;

		m_Open = true;
		m_Name = std::format( "{0}##{1}", m_SkeletonAsset->Name, std::to_string( m_SkeletonAsset->ID ) );

		/*
		m_Scene = Ref<Scene>::Create();

		SceneRendererFlags flags = SceneRendererFlag_RenderGrid;
		m_SceneRenderer = Ref<SceneRenderer>::Create( flags );

		m_SceneRenderer->SetDynamicSky( 2.0f, 0.0f, 0.0f );
		m_SceneRenderer->SetCurrentScene( m_Scene.Get() );
		*/

		m_BoneHierarchyPanel.Initialise( id );
	}

	SkeletonAssetViewer::~SkeletonAssetViewer()
	{
	}

	void SkeletonAssetViewer::OnImGuiRender()
	{
		if( ImGui::Begin( m_Name.c_str(), &m_Open ) )
		{
			m_BoneHierarchyPanel.OnImGuiRender();

			ImGui::End();
		}
	}

	void SkeletonAssetViewer::OnUpdate( Timestep ts )
	{

	}

	void SkeletonAssetViewer::OnEvent( Event& rEvent )
	{
//		if( m_MouseOverViewport && m_AllowCameraEvents )
//			m_Camera.OnEvent( rEvent );
	}

}
