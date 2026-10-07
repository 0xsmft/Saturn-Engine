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

// #FixWrongIncludeOrder
#include "Saturn/Asset/Asset.h"
#include "AssetCreationPopups.h"

#include "ImGuiAuxiliary.h"
#include "EditorIcons.h"

#include <imgui.h>

namespace Saturn {

	MaterialAssetCreationPopup::MaterialAssetCreationPopup( const std::filesystem::path& rPathToCreateIn )
		: AssetCreationPopupBase( rPathToCreateIn )
	{
	}

	MaterialAssetCreationPopup::~MaterialAssetCreationPopup()
	{
	}

	void MaterialAssetCreationPopup::Initialise()
	{
		m_Open = true;
	}

	void MaterialAssetCreationPopup::OnImGuiRender()
	{
		if( m_Open )
			ImGui::OpenPopup( "Create new MaterialAsset##NEWMATASSET" );

		ImGui::SetNextWindowSize( { 450.0F, 0.0F } );
		ImGui::SetNextWindowPos( ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2( 0.5f, 0.5f ) );

		if( ImGui::BeginPopupModal( "Create new MaterialAsset##NEWMATASSET", &m_Open, ImGuiWindowFlags_NoSavedSettings ) )
		{
			ImGuiIO& rIO = ImGui::GetIO();
			ImGui::PushFont( rIO.Fonts->Fonts[ 1 ] );
			ImGui::Text( "New Material Asset" );
			ImGui::PopFont();

			ImGui::Separator();

			ImGui::Text( "Materials need a base shader, by default it's universal PBR shader." );
			ImGui::Text( "You can pick using the dropdown below what custom shader you'd like to pick." );
			ImGui::Text( "You can leave this blank if you want to use the default shader." );

			ImGui::BeginHorizontal( "##shaderOptionsH" );
			ImGui::Text( "Custom Shader Base:" );

			if( m_ShaderSource.ShaderID == 0llu )
			{
				ImGui::Text( "Engine PBR" );
			}
			else
			{
				Ref<Asset> shaderAsset = AssetManager::Get()->FindAsset( m_ShaderSource.ShaderID );
				if( shaderAsset )
				{
					ImGui::Text( "%s", shaderAsset->Name.c_str() );
				}
				else
				{
					ImGui::TextColored( { 1.0f, 0.0f, 0.0f, 1.0f }, "Looking for %" PRIu64 " but couldn't find it in the AssetRegistry", ( uint64_t ) m_ShaderSource.ShaderID );
				}
			}

			ImGui::Spring();

			bool assetFinderOpen = false;
			if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), { 24.0f, 24.0f } ) )
			{
				assetFinderOpen ^= 1;
			}

			if( Auxiliary::DrawAssetFinder( AssetType::ShaderPrototype, &assetFinderOpen, m_ShaderSource.ShaderID ) )
			{
				m_ShaderSource.SourceType = MaterialAssetShaderSourceType::Custom;
			}

			if( m_ShaderSource.ShaderID != 0llu )
			{
				if( ImGui::Button( "Reset" ) )
				{
					m_ShaderSource.SourceType = MaterialAssetShaderSourceType::EngineDefault;
					m_ShaderSource.ShaderID = 0llu;
				}
			}

			ImGui::EndHorizontal();
			
			ImGui::BeginHorizontal( "##actionsH" );

			if( ImGui::Button( "Create" ) )
			{
				Close();
				m_CreationState = AssetCreationPopupState::Accepted;
				ImGui::CloseCurrentPopup();
			}

			if( ImGui::Button( "Cancel" ) )
			{
				Close();
				m_CreationState = AssetCreationPopupState::Rejected;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndHorizontal();

			ImGui::EndPopup();
		}
	}

}
