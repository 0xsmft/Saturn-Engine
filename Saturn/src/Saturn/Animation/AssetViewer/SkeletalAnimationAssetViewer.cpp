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
#include "SkeletalAnimationAssetViewer.h"

//#include "SkeletonAssetViewer.h"

#include "Saturn/Core/Renderer/RenderThread.h"

#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Vulkan/Renderer2D.h"
#include "Saturn/Vulkan/SceneRenderer.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include "Saturn/ImGui/EditorIcons.h"
#include "Saturn/ImGui/ImGuiWindowManager.h"

#include "Saturn/Scene/Components.h"

namespace Saturn {

	SkeletalAnimationAssetViewer::SkeletalAnimationAssetViewer( AssetID id )
		: AssetViewer( id ), SubSceneRendererWindow()
	{
		m_AssetType = AssetType::SkeletalAnimation;

		m_Camera.SetActive( true );

		m_Scene = Ref<Scene>::Create();

		m_SceneRenderer = Ref<SceneRenderer>::Create( SceneRendererFlag_RenderGrid );

		m_SceneRenderer->SetDynamicSky( 2.0f, 0.0f, 0.0f );
		m_SceneRenderer->SetCurrentScene( m_Scene.Get() );

		ImportMeshAndAnimation();
		m_Name = std::format( "{0}##{1}", m_Asset->Name, ( uint64_t ) m_AssetID );

		SetViewportWindowID( m_AssetID );
	}

	SkeletalAnimationAssetViewer::~SkeletalAnimationAssetViewer()
	{
		m_Animator = nullptr;
		m_Entity = nullptr;
	}

	void SkeletalAnimationAssetViewer::OnImGuiRender()
	{
		// Root Window.
		ImGui::SetNextWindowPos( ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Once );
		ImGui::SetNextWindowSize( ImVec2( 350.0f, 350.0f ), ImGuiCond_FirstUseEver );

		ImGui::Begin( m_Name.c_str(), &m_Open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse );

		// Create custom dockspace.
		const ImGuiID dockID = ImGui::GetID( "SkMeshDckspc" );
		ImGui::DockSpace( dockID, ImVec2( 0.0f, 0.0f ), ImGuiDockNodeFlags_None );

		//////////////////////////////////////////////////////////////////////////

		RenderViewport();

		ImGui::Begin( "Sidebar" );

		ImGui::BeginHorizontal( "##setpreviewmesh" );

		ImGui::Text( "Preview Mesh" );

		ImGui::TextDisabled( "%s", m_Mesh == nullptr ? "<NULL>" : m_Mesh->Name.c_str() );

		bool open = false;

		if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), ImVec2( 24.0f, 24.0f ) ) )
		{
			open = true;
		}

		ImGui::Spring();

		ImGui::EndHorizontal();

		if( Auxiliary::DrawAssetFinder( AssetType::SkeletalMesh, &open, m_AssetFinderOut, 0 ) )
		{
			m_Mesh = AssetManager::Get().GetAssetAs<SkeletalMesh>( m_AssetFinderOut );

			auto& mc = m_Entity->GetComponent<SkeletalMeshComponent>();

			mc.Mesh = m_Mesh;
			mc.LocalAnimator = Ref<Animator>::Create();
			m_Animator = mc.LocalAnimator;
			mc.LocalAnimator->InitAnimation( m_Asset->ID, m_Mesh, AnimatorType::Single );
		}

		if( Auxiliary::TreeNode( "Root Motion" ) )
		{
			bool value = m_Asset->IsUsingRootMotion();
			if( Auxiliary::DrawBoolControl( "Use Root Motion", value ) ) 
			{
				m_Asset->UseRootMotion( value );
			}

			Auxiliary::EndTreeNode();
		}

		ImGui::End();

		ImGui::PopStyleVar(); // ImGuiStyleVar_WindowPadding
		ImGui::End(); // Root window

		if( m_Open == false )
		{
			m_Open = false;

			RenderThread::Get().Queue( [ = ]()
			{
				m_SceneRenderer = nullptr;
			} );
		}
	}

	void SkeletalAnimationAssetViewer::OnUpdate( Timestep ts )
	{
		OnUpdateRenderer( ts );
	}

	void SkeletalAnimationAssetViewer::OnEvent( Event& rEvent )
	{
		OnCameraEvent( rEvent );
	}

	void SkeletalAnimationAssetViewer::ImportMeshAndAnimation()
	{
		m_Asset = AssetManager::Get().GetAssetAs<SkeletalAnimationAsset>( m_AssetID );

		m_Entity = m_Scene->CreateEntity( "InternalViewerEntity" );
		m_Entity->AddComponent<SkeletalMeshComponent>();

		m_Open = true;
	}

}
