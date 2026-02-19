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

// TOOD: #FixSceneRendererIncludes
#include "Saturn/Vulkan/Renderer2D.h"
#include "Saturn/Vulkan/AluraRenderer.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include "Saturn/ImGui/EditorIcons.h"
#include "Saturn/ImGui/ImGuiWindowManager.h"

#include "Saturn/Scene/Components.h"

namespace Saturn {

	SkeletalAnimationAssetViewer::SkeletalAnimationAssetViewer( AssetID id )
		: AssetViewer( id ), SubSceneRendererWindow()
	{
		m_AssetType = AssetType::SkeletalAnimation;

		Initialise();
		SetViewportWindowID( m_AssetID );

		ImportMeshAndAnimation();
		m_Name = std::format( "{0}##{1}", m_Asset->Name, ( uint64_t ) m_AssetID );
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
			m_Mesh = AssetManager::Get()->GetAssetAs<SkeletalMesh>( m_AssetFinderOut );

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

		if( ImGui::Begin( "Dopesheet" ) )
		{
			if( !m_Animator )
			{
				ImGui::Text( "<no animation is currenly playing...>" );
			}
			else
			{
				ImGui::PushStyleColor( ImGuiCol_ChildBg, ImVec4( 0.0f, 0.0f, 0.0f, 0.0f ) );

				ImGui::BeginChild( "Top Bar", ImVec2( 0.0f, 30.0f ), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings );

				ImGui::BeginHorizontal( "##tbvert" );

				ImGui::PushID( "##back" );
				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "FastForward" ), { 24.0f, 24.0f }, { 1, 0 }, { 0, 1 } ) )
				{
					m_Animator->StepTo( 0 );
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

				Ref<Texture2D> texture = m_Animator->IsPlaying() ? EditorIcons::GetIcon( "Stop" ) : EditorIcons::GetIcon( "Play" );
				if( Auxiliary::ImageButton( texture, { 24.0f, 24.0f } ) )
				{
					if( !m_Animator->IsPlaying() )
					{
						m_Animator->Begin();
					}
					else
					{
						m_Animator->Pause();
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
					m_Animator->StepTo( m_Asset->GetDuration() );
				}
				ImGui::PopID();

				ImGui::EndHorizontal();

				ImGui::EndChild();
				ImGui::PopStyleColor();

				ImVec2 size = ImGui::GetContentRegionAvail();
				ImDrawList* pDrawList = ImGui::GetWindowDrawList();

				ImVec2 cursor = ImGui::GetCursorScreenPos();
				ImVec2 end = ImVec2( cursor.x + size.x, cursor.y + size.y );

				ImGui::InvisibleButton( "scrubber", size );
				if( ImGui::IsItemHovered() && ImGui::IsMouseDown( ImGuiMouseButton_Left ) )
				{
					const float mouseX = ImGui::GetIO().MousePos.x;
					float t = ( mouseX - cursor.x ) / ( size.x );
					t = std::clamp( t, 0.0f, 1.0f );

					m_Animator->StepTo( t * m_Asset->GetDuration() );
					m_Animator->Pause();
				}

				// Draw background
				pDrawList->AddRectFilled( cursor, end, IM_COL32( 40, 40, 40, 255 ), 4.0f );

				// Draw ticks
				constexpr int SRUB_NUM_TICKS = 10;
				for( int i = 0; i <= SRUB_NUM_TICKS; ++i )
				{
					const float t = i / ( float ) SRUB_NUM_TICKS;
					const float x = cursor.x + t * size.x;
					pDrawList->AddLine( ImVec2( x, cursor.y ), ImVec2( x, cursor.y + size.y ), IM_COL32( 80, 80, 80, 255 ) );
				}

				// Draw scrubber line
				const float srubberX = cursor.x + ( m_Animator->GetCurrentAnimTime() / m_Asset->GetDuration() ) * size.x;
				pDrawList->AddLine( ImVec2( srubberX, cursor.y ),
					ImVec2( srubberX, cursor.y + size.y ),
					IM_COL32( 255, 200, 0, 255 ), 2.0f );

				// Draw playhead triangle
				pDrawList->AddTriangleFilled(
					ImVec2( srubberX - 5.0F, cursor.y - 8.0F ),
					ImVec2( srubberX + 5.0F, cursor.y - 8.0F ),
					ImVec2( srubberX, cursor.y ),
					IM_COL32( 255, 200, 0, 255 )
				);
			}

			ImGui::End();
		}

		ImGui::End(); // Root window

		if( m_Open == false )
		{
			m_Asset->PortToNewestVersion();
			SkeletalAnimationAssetSerialiser skAnimSerialiser;
			skAnimSerialiser.Serialise( m_Asset );

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
		m_Asset = AssetManager::Get()->GetAssetAs<SkeletalAnimationAsset>( m_AssetID );

		m_Entity = m_Scene->CreateEntity( "InternalViewerEntity" );
		m_Entity->AddComponent<SkeletalMeshComponent>();

		m_Open = true;
	}

}
