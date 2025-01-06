/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2024 BEAST                                                           *
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

#include "ImGuiAuxiliary.h"

#include <imgui.h>

namespace Saturn {

	TextureViewer::TextureViewer( AssetID ID )
		: AssetViewer( ID )
	{
		m_AssetType = AssetType::Texture;

		AddTexture();
	}

	TextureViewer::~TextureViewer()
	{
		m_Asset = nullptr;
		m_Texture = nullptr;
	}

	void TextureViewer::OnImGuiRender()
	{
		ImGui::PushID( (int)m_Asset->ID );

		ImGui::Begin( "Texture Viewer", &m_Open );

		ImGui::BeginChild( "Texture Information" );

		ImGui::BeginHorizontal( "##textureInfoH" );

		ImGui::BeginVertical( "##textureInfoV" );

		ImGui::Text( "Texture Path" );
		std::string texturePath = m_Asset->Path.string();

		ImGui::InputText( "##texturepath", (char*) texturePath.c_str(), texturePath.size(), ImGuiInputTextFlags_ReadOnly );

		ImGui::Spring();

		std::string sizeText = std::format( "{0}x{1}", m_Texture->Width(), m_Texture->Height() );
		ImGui::Text( "Texture Size: %s", sizeText.c_str() );

		ImGui::EndVertical();

		ImGui::EndHorizontal();

		Auxiliary::Image( m_Texture, { (float)m_Texture->Width() / 2, ( float ) m_Texture->Height() / 2 } );

		ImGui::EndChild();

		ImGui::End();

		ImGui::PopID();
	}

	void TextureViewer::AddTexture()
	{
		Ref<Asset> textureAsset = AssetManager::Get().FindAsset( m_AssetID );
		m_Asset = textureAsset;

		// Load the real texture.
		// TODO: Check if we have already loaded the texture somewhere else in the engine!

		m_Texture = Ref<Texture2D>::Create( Project::GetActiveProject()->FilepathAbs( m_Asset->Path ), AddressingMode::Repeat );

		m_Open = true;
	}

}