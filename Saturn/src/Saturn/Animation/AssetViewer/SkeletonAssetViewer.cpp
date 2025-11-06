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

			// Compatibility Information
			ImGui::Begin( "Compatible Meshes" );
			for( const auto& rMeshID : m_SkeletonAsset->GetCompatibleMeshes() )
			{
				ImGui::BeginHorizontal( ( int ) rMeshID );

				ImGui::Text( "%llu", rMeshID );
				if( ImGui::SmallButton( "-" ) ) 
				{
					m_SkeletonAsset->MarkAsUncompatibleMesh( rMeshID );
				
					// Not great having this here, but will be fine for the time being.
					ImGui::EndHorizontal();
					break;
				}

				ImGui::EndHorizontal();
			}

			if( ImGui::SmallButton( "+" ) )
			{
				m_ShowFinderModal = true;
				ImGui::OpenPopup( "PickMesh##CompSk" );
			}

			ImGui::SetNextWindowSize( { 350.0F, 0.0F } );
			if( ImGui::BeginPopupModal( "PickMesh##CompSk", &m_ShowFinderModal, ImGuiWindowFlags_NoSavedSettings ) )
			{
				ImGui::Text( "Please pick the mesh that you'd like to mark as compatible with this skeleton." );
				
				ImGui::BeginHorizontal( "##meshinfohz" );

				std::string inputTextData = m_TemporaryCompatibleMeshID == 0 ? "No Mesh" : std::to_string( m_TemporaryCompatibleMeshID );
				Auxiliary::InputText( "##name", &inputTextData, ImGuiInputTextFlags_ReadOnly );

				bool open = false;
				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), ImVec2( 24.0f, 24.0f ) ) )
				{
					open ^= 1;
				}
				Auxiliary::DrawAssetFinder( AssetType::SkeletalMesh, &open, m_TemporaryCompatibleMeshID );

				ImGui::EndHorizontal();

				ImGui::Separator();
				ImGui::BeginHorizontal( "##options" );
				
				if( ImGui::Button( "Confirm" ) )
				{
					m_SkeletonAsset->AddCompatibleMesh( m_TemporaryCompatibleMeshID );

					m_TemporaryCompatibleMeshID = 0;
					m_ShowFinderModal = false;
					ImGui::CloseCurrentPopup();
				}

				if( ImGui::Button( "Cancel" ) )
				{
					m_TemporaryCompatibleMeshID = 0;
					m_ShowFinderModal = false;
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndHorizontal();

				ImGui::EndPopup();
			}
			ImGui::End();

			ImGui::End();
		}

		if( m_Open == false )
		{
			m_SkeletonAsset->PortToNewestVersion();
			SkeletonAssetSerialiser sas;
			sas.Serialise( m_SkeletonAsset );
		}
	}

	void SkeletonAssetViewer::OnUpdate( Timestep ts )
	{
	}

	void SkeletonAssetViewer::OnEvent( Event& rEvent )
	{
	}

}
