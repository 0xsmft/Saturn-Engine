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
#include "SkeletalMeshAssetViewer.h"

#include "SkeletonAssetViewer.h"

#include "Saturn/Core/Renderer/RenderThread.h"

#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Vulkan/Renderer2D.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include "Saturn/ImGui/EditorIcons.h"
#include "Saturn/ImGui/ImGuiWindowManager.h"

#include "Saturn/Scene/Components.h"

namespace Saturn {
	
	SkeletalMeshAssetViewer::SkeletalMeshAssetViewer( AssetID id )
		: AssetViewer( id )
	{
		m_AssetType = AssetType::SkeletalMesh;

		Initialise();
		SetViewportWindowID( m_AssetID );
	
		AddMesh();
		m_Name = std::format( "{0}##{1}", m_Mesh->Name, std::to_string( m_AssetID ) );
	}

	SkeletalMeshAssetViewer::~SkeletalMeshAssetViewer()
	{
		m_Entity = nullptr;
	}

	void SkeletalMeshAssetViewer::OnImGuiRender()
	{
		// Root Window.
		ImGui::SetNextWindowPos( ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Once );
		ImGui::SetNextWindowSize( ImVec2( 350.0f, 350.0f ), ImGuiCond_FirstUseEver );

		ImGui::Begin( m_Name.c_str(), &m_Open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar );

		// Create custom dockspace.
		const ImGuiID dockID = ImGui::GetID( "AxDckspc" );
		ImGui::DockSpace( dockID, ImVec2( 0.0f, 0.0f ), ImGuiDockNodeFlags_None );

		//////////////////////////////////////////////////////////////////////////

		if( ImGui::BeginMenuBar() )
		{
			if( ImGui::BeginMenu( "File" ) )
			{
				if( ImGui::MenuItem( "Close" ) )
				{
					m_Open = false;
				}

				if( ImGui::MenuItem( "Save" ) )
				{
					SkeletalMeshAssetSerialiser sma;
					sma.Serialise( m_Mesh );
				}

				ImGui::EndMenu();
			}

			if( ImGui::BeginMenu( "View" ) )
			{
				if( ImGui::MenuItem( "Open Skeleton Asset Viewer" ) )
				{
					const AssetID skeletonID = m_Mesh->GetSkeletonAsset()->ID;
					const Ref<Asset> asset = AssetManager::Get().FindAsset( skeletonID );
					if( asset )
					{
						const std::string windowName = std::format( "{0}##{1}", asset->Name, ( uint64_t ) skeletonID );
						ImGuiWindowManager::Get().OpenOrShowWindow<SkeletonAssetViewer>( windowName, skeletonID );

						ImGui::SetWindowFocus( windowName.c_str() );
					}
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		//////////////////////////////////////////////////////////////////////////

		RenderViewport();

		//////////////////////////////////////////////////////////////////////////

		RenderSidebar();

		//////////////////////////////////////////////////////////////////////////

		ImGui::End();

		if( m_Open == false )
		{
			m_Open = false;

			RenderThread::Get().Queue( [ = ]()
			{
				m_SceneRenderer = nullptr;
			} );
		}
	}

	void SkeletalMeshAssetViewer::RenderSidebar() 
	{
		ImGui::PushID( ( int ) m_AssetID );
		ImGui::Begin( "Sidebar" );

		if( Auxiliary::TreeNode( "Materials" ) )
		{
			int i = 0;
			for( auto& rMaterial : m_Mesh->GetMaterialAssets() )
			{
				ImGui::PushID( i );

				if( ImGui::TreeNodeEx( rMaterial->Name.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding ) )
				{
					ImGui::BeginHorizontal( i );

					ImGui::TextDisabled( "%s", rMaterial->Name.empty() ? "<NULL>" : rMaterial->Name.c_str() );

					bool open = false;

					if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), ImVec2( 24.0f, 24.0f ) ) )
					{
						open = true;
					}

					ImGui::Spring();

					ImGui::EndHorizontal();

					if( Auxiliary::DrawAssetFinder( AssetType::Material, &open, m_AssetFinderOut, 0 ) )
					{
						Ref<MaterialAsset> newAsset = AssetManager::Get().GetAssetAs<MaterialAsset>( m_AssetFinderOut );
						rMaterial->SetMaterial( newAsset->GetMaterial() );

						// Update Pure Dependencies & Update ADN Dependencies
						AssetManager::Get().UnregisterAssetDependency( m_AssetID, rMaterial->ID );

						m_Mesh->GetMaterialRegistry()->SetMaterial( i, m_AssetFinderOut );

						AssetManager::Get().RegisterAssetDependency( m_AssetID, m_AssetFinderOut );
					}

					Auxiliary::EndTreeNode();
				}

				ImGui::PopID();

				++i;
			}

			Auxiliary::EndTreeNode();
		}

		if( Auxiliary::TreeNode( "Preview Animation" ) )
		{
			ImGui::TextDisabled( "%s", m_PreviewAnimation == nullptr ? "<NULL>" : m_PreviewAnimation->Name.c_str() );

			bool open = false;

			if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), ImVec2( 24.0f, 24.0f ) ) )
			{
				open = true;
			}

			if( Auxiliary::DrawAssetFinder( AssetType::SkeletalAnimation, &open, m_AssetFinderOut, 0 ) )
			{
				Ref<SkeletalAnimationAsset> newAsset = AssetManager::Get().GetAssetAs<SkeletalAnimationAsset>( m_AssetFinderOut );

				m_PreviewAnimation = newAsset;

				m_Entity->GetComponent<SkeletalMeshComponent>().LocalAnimator->InitAnimation( m_AssetFinderOut, m_Mesh, AnimatorType::Single );
			}

			ImGui::Separator();

			if( ImGui::Button( "Reset" ) )
			{
				m_PreviewAnimation = nullptr;
				m_Entity->GetComponent<SkeletalMeshComponent>().LocalAnimator->Clear();
			}

			Auxiliary::EndTreeNode();
		}

		ImGui::End();
		ImGui::PopID();
	}

	void SkeletalMeshAssetViewer::OnUpdate( Timestep ts )
	{
		OnUpdateRenderer( ts );
	}

	void SkeletalMeshAssetViewer::OnEvent( Event& rEvent )
	{
		OnCameraEvent( rEvent );
	}

	void SkeletalMeshAssetViewer::AddMesh()
	{
		Ref<SkeletalMesh> mesh = AssetManager::Get().GetAssetAs<SkeletalMesh>( m_AssetID );

		m_Mesh = mesh;

		m_Entity = m_Scene->CreateEntity( "InternalViewerEntity" );
		auto& mc = m_Entity->AddComponent<SkeletalMeshComponent>();
		mc.Mesh = mesh;
		mc.LocalAnimator = Ref<Animator>::Create();

		m_Open = true;
	}

}
