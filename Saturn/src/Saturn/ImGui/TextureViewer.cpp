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
#include "TextureViewer.h"

#include "Saturn/Asset/AssetManager.h"

#include "Saturn/Project/Project.h"

#include "ImGuiAuxiliary.h"

#include "Saturn/Asset/TextureSourceAsset.h"

#include <imgui.h>

namespace Saturn {

	TextureViewer::TextureViewer( AssetID ID )
		: AssetViewer( ID )
	{
		m_AssetType = AssetType::Texture;
		m_Asset = AssetManager::Get()->GetAssetAs<TextureSourceAsset>( m_AssetID );
	
		AddTexture();
	}

	TextureViewer::~TextureViewer()
	{
		m_Texture = nullptr;
		m_Asset = nullptr;
	}

	void TextureViewer::OnImGuiRender()
	{
		if( ImGui::Begin( m_Name.c_str(), &m_Open ) ) 
		{
			ImGui::BeginChild( "Texture Information" );

			ImGui::Text( "Filtering flags" );

			constexpr const char* pItems[] = { "Linear", "Optimal" };
			static TextureFilteringFlags SelectedEnum = m_Asset->GetFilteringFlags();
			static const char* Selected = pItems[ ( int ) SelectedEnum ];
			if( ImGui::BeginCombo( "##setsamplerfilter", Selected ) )
			{
				for( unsigned int i = 0; i < IM_ARRAYSIZE( pItems ); i++ )
				{
					bool IsSelected = ( Selected == pItems[ i ] );

					if( ImGui::Selectable( pItems[ i ], IsSelected ) )
					{
						SelectedEnum = ( TextureFilteringFlags ) i;
						Selected = pItems[ i ];

						m_Asset->SetFilteringFlags( SelectedEnum );
					}

					if( IsSelected )
					{
						ImGui::SetItemDefaultFocus();
					}
				}

				ImGui::EndCombo();
			}

			ImGui::BeginHorizontal( "##textureInfoH" );

			ImGui::BeginVertical( "##textureInfoV" );

			ImGui::Text( "Texture Path" );
			{
				Auxiliary::ScopedDisabledFlag dis( true );
			
				const std::string texturePath = m_Asset->GetTextureAbsolutePath().string();
				ImGui::InputText( "##texturepath", ( char* ) texturePath.c_str(), texturePath.size(), ImGuiInputTextFlags_ReadOnly );
			}
			ImGui::Spring();

			const std::string sizeText = std::format( "{0}x{1}", m_Texture->Width(), m_Texture->Height() );
			ImGui::Text( "Texture Size: %s", sizeText.c_str() );

			ImGui::EndVertical();

			ImGui::EndHorizontal();

			Auxiliary::Image( m_Texture, { ( float ) m_Texture->Width() * 0.5f, ( float ) m_Texture->Height() * 0.5f } );

			ImGui::EndChild();
		}

		ImGui::End();
	}

	void TextureViewer::AddTexture()
	{
		m_Texture = m_Asset->GetTexture();

		m_Name = std::format( "{0}##{1}", m_Asset->Name, ( uint64_t ) m_Asset->ID );
		m_Open = true;
	}

}
