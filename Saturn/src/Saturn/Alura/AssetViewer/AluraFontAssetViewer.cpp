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
#include "AluraFontAssetViewer.h"

#include "Saturn/Asset/AssetManager.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"

#include <imgui.h>

namespace Saturn {

	AluraFontAssetViewer::AluraFontAssetViewer( AssetID id )
		: AssetViewer( id )
	{
		m_AssetType = AssetType::Font;
		m_Open = true;

		m_Font = AssetManager::Get().GetAssetAs<AluraFont>( m_AssetID );
		m_Name = std::format( "{0} - Alura Font##{1}", m_Font->Name, ( uint64_t ) m_Font->ID );
	}

	AluraFontAssetViewer::~AluraFontAssetViewer()
	{
	}

	void AluraFontAssetViewer::OnImGuiRender()
	{
		if( ImGui::Begin( m_Name.c_str(), &m_Open ) )
		{
			ImGui::Text( "Alura Compatible Font" );
			ImGui::Text( "Font name: %s", m_Font->GetFontName().c_str() );
			ImGui::Separator();

			const auto texture = m_Font->GetTexture();
			Auxiliary::Image( texture, ImVec2{ ( float ) texture->Width(), ( float ) texture->Height() } );

			if( ImGui::BeginItemTooltip() )
			{
				ImGui::Text( "%s - Multi-Channel True Signed Distance Field (MTSDF), %ix%i", m_Font->GetFontName().c_str(), texture->Width(), texture->Height() );
				ImGui::EndTooltip();
			}

			if( m_pLoadedImGuiFont )
			{
				ImGui::Separator();
				ImGui::PushFont( m_pLoadedImGuiFont );

				ImGui::Text( "abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 1234567890-=_[]{};:'@#~,.<>/?|\\`!$%^&*()\"\'" );

				ImGui::Separator();
				
				ImGui::Text( "18" );
				ImGui::SameLine();
				ImGui::Text( "The quick brown fox jumps over the lazy dog. 1234567890" );

				ImGui::PopFont();
			}
			else
			{
				ImGui::TextColored( { 1.0f, 0.0f, 0.0f, 1.0f }, "ImGui was unable to load the font..." );
			}
		}

		ImGui::End();

		/* TODO: In ImGui version 1.92, dynamic fonts will be supported, meaning that we can unload the font at anytime.
		if( !m_Open )
		{
			ImGuiIO& rIO = ImGui::GetIO();
			rIO.Fonts->PopFont( m_pLoadedImGuiFont );
		}
		*/
	}

	void AluraFontAssetViewer::OnUpdate( Timestep ts )
	{
		if( !m_AttemptedToLoadFont && !m_pLoadedImGuiFont )
		{
			ImGuiIO& rIO = ImGui::GetIO();
			m_pLoadedImGuiFont = rIO.Fonts->AddFontFromFileTTF( m_Font->GetFontFilepath().string().c_str(), 18.0f );
			m_AttemptedToLoadFont = true;
		}
	}

	void AluraFontAssetViewer::OnEvent( Event& rEvent )
	{
	}

}
