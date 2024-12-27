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
#include "SoundAssetViewer.h"

#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Audio/Sound.h"

#include "ImGuiAuxiliary.h"
#include "EditorIcons.h"

#include <imgui.h>

namespace Saturn {

	SoundAssetViewer::SoundAssetViewer( AssetID id )
		: AssetViewer( id )
	{
		m_AssetType = AssetType::Sound;
		
		Ref<SoundSpecification> snd = AssetManager::Get().GetAssetAs<SoundSpecification>( m_AssetID );
		m_SoundAsset = snd;

		m_Open = true;
	}

	SoundAssetViewer::~SoundAssetViewer()
	{
		m_PreviewSound = nullptr;
	}

	void SoundAssetViewer::OnImGuiRender()
	{
		ImGui::PushID( static_cast<int>( m_SoundAsset->ID ) );

		std::string windowName = std::format( "{0}##{1}", m_SoundAsset->Name, std::to_string( m_SoundAsset->ID ) );

		ImGui::Begin( windowName.c_str(), &m_Open );

		ImGui::BeginVertical( "##settings_hor" );

		ImGui::Text( "Source Sound file" );

		ImGui::BeginHorizontal( "##srcsndfile" );

		if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), { 24, 24 } ) ) 
		{
			std::string filepath = Application::Get().OpenFile( "Supported asset types (*.wav, *.mp3, *.ogg)\0*.wav; *.mp3; *.ogg\0" );

			if( !filepath.empty() )
			{
				std::filesystem::path currentRawPath = "";
				currentRawPath = currentRawPath.parent_path();

				std::filesystem::path path = filepath;

				std::filesystem::path newPath = currentRawPath;
				currentRawPath /= path.filename();

				if( !std::filesystem::exists( filepath ) )
					std::filesystem::copy_file( filepath, newPath );

				m_SoundAsset->SoundSourcePath = newPath;
				m_SoundAsset->OriginalImportPath = path;

				// TODO: Show an editor dialog message if the user would like to delete the old source.
			}
		}

		ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 1.0f, 1.0f, 1.0f, 0.5f ) );
		ImGui::InputText( "##filepath", ( char* ) m_SoundAsset->SoundSourcePath.string().c_str(), 4096, ImGuiInputTextFlags_ReadOnly );
		ImGui::PopStyleColor();

		ImGui::EndHorizontal();

		ImGui::Text( "Original Import Path" );

		ImGui::BeginHorizontal( "##importPath" );
		
		ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 1.0f, 1.0f, 1.0f, 0.5f ) );
		ImGui::InputText( "##importPath", ( char* ) m_SoundAsset->OriginalImportPath.string().c_str(), 4096, ImGuiInputTextFlags_ReadOnly );
		ImGui::PopStyleColor();

#if !defined(SAT_DIST)
		ImGui::Spring();
		ImGui::Text( "%s", m_SoundAsset->LastWriteTime );
#endif

		ImGui::EndHorizontal();

		if( m_PreviewSound && m_PreviewSound->IsPlaying() )
		{
			if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Stop" ), { 24.0f, 24.0f } ) )
			{
				m_PreviewSound->Stop();
				m_PreviewSound->Reset();
			}
		}
		else
		{
			if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Play" ), { 24.0f, 24.0f } ) )
			{
				if( !m_PreviewSound )
					m_PreviewSound = Ref<Sound>::Create( m_SoundAsset );

				m_PreviewSound->Play();
			}
		}

		ImGui::EndVertical();

		ImGui::End();

		ImGui::PopID();

		if( !m_Open )
		{
			SoundSpecificationAssetSerialiser spec;
			spec.Serialise( m_SoundAsset );

			AssetViewer::DestroyViewer( m_AssetID );
		}
	}

}