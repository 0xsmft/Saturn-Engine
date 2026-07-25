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
#include "SkeletalAnimationAssetViewer.h"

//#include "SkeletonAssetViewer.h"

#include "Saturn/Core/Renderer/RenderThread.h"

#include "Saturn/Asset/AssetManager.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include "Saturn/ImGui/EditorIcons.h"
#include "Saturn/ImGui/ImGuiWindowManager.h"

#include "Saturn/Scene/Components.h"

#include "Saturn/Animation/SkeletonAsset.h"

#include <ImTimeline/TimelineCore/TimelinePlayer.h>
#include <ImTimeline/ImTimeline.h>

namespace Saturn {

	SkeletalAnimationAssetViewer::SkeletalAnimationAssetViewer( AssetID id )
		: AssetViewer( id )
	{
		m_AssetType = AssetType::SkeletalAnimation;

		m_Scene = Ref<Scene>::Create();
		m_Viewport = std::make_unique<EditorViewport>( VP_DefaultSub );

		const std::string vpName = std::format( "Viewport##{0}", ( uint64_t ) m_AssetID );
		m_Viewport->Initialise( SceneRendererFlag_NoAlura, m_Scene, vpName, m_AssetID );

		ImportMeshAndAnimation();
		m_Name = std::format( "{0}##{1}", m_Asset->Name, ( uint64_t ) m_AssetID );

		m_BoneHierarchyPanel.Initialise( m_Asset->GetSkeletonID() );
		
		// Make sure this panel has a unique name.
		m_BoneHierarchyPanel.AppendToName( std::format( "##{}", ( uint64_t ) m_AssetID ) );
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

		ImGui::Begin( m_Name.c_str(), &m_Open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar );

		// Create custom dockspace.
		const ImGuiID dockID = ImGui::GetID( "SkMeshDckspc" );
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

				ImGui::EndMenu();
			}

			if( ImGui::BeginMenu( "View" ) )
			{
				if( ImGui::MenuItem( "Bone Hierarchy Panel" ) )
				{
					m_BoneHierarchyPanel.ShowOrHide();
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		//////////////////////////////////////////////////////////////////////////

		m_Viewport->Draw();

		//////////////////////////////////////////////////////////////////////////

		if( m_BoneHierarchyPanel.IsOpen() ) m_BoneHierarchyPanel.OnImGuiRender();

		//////////////////////////////////////////////////////////////////////////

		if( ImGui::Begin( "Sidebar" ) ) 
		{
			DrawSidebar();
		}

		ImGui::End();

		if( ImGui::Begin( "Dopesheet" ) )
		{
			if( !m_Animator )
			{
				ImGui::Text( "<Animator is null, select a mesh!>" );
			}
			else
			{
				ImGui::BeginHorizontal( "##anmimcrtl" );

				ImGui::PushID( "##backall" );
				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "FastForward" ), { 24.0f, 24.0f }, { 1, 0 }, { 0, 1 } ) )
				{
					m_Timeline->GetPlayer()->Stop();
					m_Animator->PlayFromStart();
					m_Animator->Pause();
				}
				ImGui::PopID();

				ImGui::PushID( "##backone" );
				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "NextMultiMedia" ), { 24.0f, 24.0f }, { 1, 0 }, { 0, 1 } ) )
				{
					const float frameStep = 1.0f / m_Asset->GetTicksPerSecond();
					const float t = glm::max( 0.0f, m_Animator->GetCurrentAnimTime() - frameStep );
					m_Animator->StepTo( t );
				}
				ImGui::PopID();

				if( Auxiliary::ImageButton( m_Animator->IsPlaying() ? EditorIcons::GetIcon( "Stop" ) : EditorIcons::GetIcon( "Play" ), { 24.0f, 24.0f } ) )
				{
					if( m_Animator->IsPlaying() )
					{
						m_Timeline->GetPlayer()->Stop();
						m_Animator->Pause();
					}
					else
					{
						m_Timeline->GetPlayer()->Play();
						m_Animator->Begin();
					}
				}

				ImGui::PushID( "##fwdone" );
				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "NextMultiMedia" ), { 24.0f, 24.0f } ) )
				{
					const float frameStep = 1.0f / m_Asset->GetTicksPerSecond();
					const float t = glm::min( ( float ) m_Asset->GetDuration(), m_Animator->GetCurrentAnimTime() + frameStep );
					m_Animator->StepTo( t );
				}
				ImGui::PopID();

				ImGui::PushID( "##ff" );
				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "FastForward" ), { 24.0f, 24.0f } ) )
				{
					m_Timeline->GetPlayer()->SetStartTimestamp( m_Asset->GetDuration() );
					m_Animator->StepTo( m_Asset->GetDuration() );
				}
				ImGui::PopID();

				ImGui::EndHorizontal();

				// ~Timelime draw
				m_Timeline->DrawTimeline();
			}

			ImGui::End();
		}

		ImGui::End(); // Root window

		if( m_Open == false )
		{
			m_Asset->PortToNewestVersion();

			SkeletalAnimationAssetSerialiser skAnimSerialiser;
			skAnimSerialiser.Serialise( m_Asset );
		}
	}

	void SkeletalAnimationAssetViewer::DrawSidebar()
	{
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
			InitMeshAndAnimator( m_AssetFinderOut );
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
	}

	void SkeletalAnimationAssetViewer::InitMeshAndAnimator( UUID id )
	{
		m_Mesh = AssetManager::Get()->GetAssetAs<SkeletalMesh>( id );
		if( m_Mesh )
		{
			auto& mc = m_Entity->GetComponent<SkeletalMeshComponent>();

			mc.Mesh = m_Mesh;
			mc.LocalAnimator = Ref<Animator>::Create();
			m_Animator = mc.LocalAnimator;
			mc.LocalAnimator->InitAnimation( m_Asset->ID, m_Mesh, AnimatorType::Single );
		}
	}

	void SkeletalAnimationAssetViewer::OnUpdate( Timestep ts )
	{
		m_Viewport->OnUpdate( ts );
	}

	void SkeletalAnimationAssetViewer::OnEvent( Event& rEvent )
	{
		if( ( rEvent.Category & EC_Ruby ) != 0 )
			m_Viewport->OnEvent( rEvent );
	}

	void SkeletalAnimationAssetViewer::ImportMeshAndAnimation()
	{
		m_Asset = AssetManager::Get()->GetAssetAs<SkeletalAnimationAsset>( m_AssetID );
		
		m_Entity = m_Scene->CreateEntity( "InternalViewerEntity" );
		SkeletalMeshComponent& rSkComp = m_Entity->AddComponent<SkeletalMeshComponent>();

#if !defined(SAT_DIST)
		if( const auto asset = AssetManager::Get()->GetAssetAs<SkeletonAsset>( m_Asset->GetSkeletonID() ) )
		{
			if( asset->GetCompatibleMeshes().size() )
			{
				// Just pick the first compatible mesh.
				InitMeshAndAnimator( asset->GetCompatibleMeshes()[ 0 ] );
			}
		}
#endif

		// Create timeline object
		m_Timeline = std::make_unique<ImTimeline::Timeline>();
		m_Timeline->SetMaxFrame( m_Asset->GetDuration() );

		// Add animation node
		auto& rNode = m_Timeline->AddNewNode( 0, 0.0f, m_Asset->GetDuration() );
		rNode.Flags.set( ImTimelineNodeFlags_CannotBeDragged );
		rNode.Flags.set( ImTimelineNodeFlags_CannotBeDeleted );

		// Set display props
		m_Timeline->SetTimelineName( 0, m_Asset->Name );

		ImTimeline::TimelineStyle timelineStyle{};
		timelineStyle.HeaderHeight = 25;
		m_Timeline->SetTimelineStyle( timelineStyle );

		m_Open = true;
	}

}
