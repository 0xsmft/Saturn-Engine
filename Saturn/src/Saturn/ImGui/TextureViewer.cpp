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
		if( m_Dirty )
		{
			TextureSourceAssetSerialiser tsas;
			tsas.Serialise( m_Asset );
		}

		m_Asset = nullptr;
	}

	void TextureViewer::OnImGuiRender()
	{
		ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;
		if( m_Dirty )
			flags |= ImGuiWindowFlags_UnsavedDocument;

		if( ImGui::Begin( m_Name.c_str(), &m_Open, flags ) ) 
		{ 
			if( ImGui::BeginMenuBar() )
			{
				if( ImGui::BeginMenu( "File" ) )
				{
					if( ImGui::MenuItem( "Save" ) )
					{
						TextureSourceAssetSerialiser tsas;
						tsas.Serialise( m_Asset );

						m_Dirty = false;
					}

					if( ImGui::MenuItem( "Close" ) )
					{
						m_Open = false;
					}

					ImGui::EndMenu();
				}

				if( ImGui::BeginMenu( "View" ) )
				{
					if( ImGui::MenuItem( "Fit 1 to 1" ) )
					{
						const auto texture = m_Asset->GetTexture();
						m_TextureDisplaySize = { texture->Width(), texture->Height() };
					}

					if( ImGui::MenuItem( "Half" ) )
					{
						m_TextureDisplaySize *= 0.5f;

						// Clamp to 1x1 texture.
						if( m_TextureDisplaySize.x < 1 || m_TextureDisplaySize.y < 1 )
						{
							m_TextureDisplaySize = { 1.0f, 1.0f };
						}
					}

					ImGui::EndMenu();
				}

				ImGui::EndMenuBar();
			}

			ImGui::Text( "Load Flags" );
			{
				bool flip = m_Asset->IsFlagSet( TextureLoadFlags_FlipVertically );
				
				// We are not using columns so we must draw the text ourself.
				ImGui::Text( "Flip Vertically" );
				ImGui::SameLine();
				if( Auxiliary::DrawBoolControl( "##flip", flip, false ) )
				{
					m_Asset->SetFlag( TextureLoadFlags_FlipVertically, flip );
					m_PendingTextureReload = true;
					
					m_Dirty = true;
				}
			}

			ImGui::Separator();

			ImGui::Text( "Filtering flags" );
			{
				const char* pItems[] = { "Linear", "Optimal" };
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
							m_PendingTextureReload = true;
							m_Dirty = true;
						}

						if( IsSelected )
						{
							ImGui::SetItemDefaultFocus();
						}
					}

					ImGui::EndCombo();
				}
			}

			ImGui::Separator();

			{
				ImGui::Text( "Texture Path" );
				ImGui::SameLine();

				// We don't actually want to make it disabled, we just want to fake it and make it look like it is.
				ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 1.0f, 1.0f, 1.0f, 0.5f ) );
				ImGui::InputText( "##texturepath", ( char* ) m_Asset->GetTextureAbsolutePath().string().c_str(), 4096, ImGuiInputTextFlags_ReadOnly );
				ImGui::PopStyleColor();
			}

			Ref<Texture2D> texture = m_Asset->GetTexture();

			const std::string sizeText = std::format( "{0}x{1} Displaying as {2}x{3}", texture->Width(), texture->Height(), m_TextureDisplaySize.x, m_TextureDisplaySize.y );
			ImGui::Text( "Texture Size: %s", sizeText.c_str() );

			Auxiliary::Image( texture, { m_TextureDisplaySize.x, m_TextureDisplaySize.y } );
		}

		if( m_ShowDirtyPopup ) DrawDirtyPopup();

		ImGui::End();

		if( !m_Open && m_Dirty )
		{
			m_Open = true;
			m_ShowDirtyPopup = true;
		}
	}

	void TextureViewer::OnUpdate( Timestep ts )
	{
		if( m_PendingTextureReload )
		{
			m_Asset->Load();
			m_PendingTextureReload = false;
		}
	}

	void TextureViewer::AddTexture()
	{
		m_Name = std::format( "{0}##{1}", m_Asset->Name, ( uint64_t ) m_Asset->ID );
		
		auto texture = m_Asset->GetTexture();
		m_TextureDisplaySize = { texture->Width() * 0.5f, texture->Height() * 0.5f };

		m_Open = true;
	}

	void TextureViewer::DrawDirtyPopup()
	{
		ImGui::OpenPopup( "Texture Source is Dirty##unsavedchanges" );

		if( ImGui::BeginPopupModal( "Texture Source is Dirty##unsavedchanges", nullptr, ImGuiWindowFlags_NoSavedSettings ) )
		{
			ImGui::Text( "There are unsaved changes to this Texture would you like to save them?" );

			ImGui::BeginHorizontal( "##optionshz" );

			if( ImGui::Button( "Save" ) )
			{
				TextureSourceAssetSerialiser tsas;
				tsas.Serialise( m_Asset );

				m_Open = false;
				m_Dirty = false;
				ImGui::CloseCurrentPopup();
			}

			if( ImGui::Button( "Discard Changes" ) )
			{
				m_Open = false;
				m_Dirty = false;
				ImGui::CloseCurrentPopup();
			}

			if( ImGui::Button( "Cancel" ) )
			{
				m_ShowDirtyPopup = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndHorizontal();

			ImGui::EndPopup();
		}
	}

}
