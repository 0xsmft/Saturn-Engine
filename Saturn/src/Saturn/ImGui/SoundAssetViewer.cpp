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
#include "SoundAssetViewer.h"

#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Audio/Sound.h"
#include "Saturn/Audio/AudioSystem.h"

#include "Saturn/Project/Project.h"

#include "Saturn/Core/Process.h"

#include "ImGuiAuxiliary.h"
#include "EditorIcons.h"

#include <imgui.h>

namespace Saturn {

	SoundAssetViewer::SoundAssetViewer( AssetID id )
		: AssetViewer( id )
	{
		m_AssetType = AssetType::Sound;
		
		Ref<SoundSpecification> snd = AssetManager::Get()->GetAssetAs<SoundSpecification>( m_AssetID );
		m_SoundAsset = snd;

		m_Open = true;
		m_Name = std::format( "{0}##{1}", m_SoundAsset->Name, std::to_string( m_SoundAsset->ID ) );
	}

	SoundAssetViewer::~SoundAssetViewer()
	{
		AudioSystem::Get().UnloadSound( m_PreviewSound );
		m_PreviewSound = nullptr;
		 
		AudioSystem::Get().StopSoundGroups();
	}

	void SoundAssetViewer::OnImGuiRender()
	{
		if( ImGui::Begin( m_Name.c_str(), &m_Open ) ) 
		{
			ImGui::BeginVertical( "##settings_hor" );

			ImGui::Text( "Source Sound file" );

			ImGui::BeginHorizontal( "##srcsndfile" );

			if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), { 24.0f, 24.0f } ) )
			{
				const std::filesystem::path path = Application::Get()->OpenFile( "Supported asset types (*.wav, *.mp3, *.ogg)|*.wav; *.mp3; *.ogg" );

				if( !path.empty() )
				{
					std::filesystem::path currentRawPath = "";
					currentRawPath = currentRawPath.parent_path();

					std::filesystem::path newPath = currentRawPath;
					currentRawPath /= path.filename();

					if( !std::filesystem::exists( path ) )
						std::filesystem::copy_file( path, newPath );

					m_SoundAsset->SoundSourcePath = newPath;
					m_SoundAsset->OriginalImportPath = path;

					// TODO: Show an editor dialog message if the user would like to delete the old source.
				}
			}

			ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 1.0f, 1.0f, 1.0f, 0.5f ) );
			ImGui::InputText( "##filepath", ( char* ) m_SoundAsset->SoundSourcePath.string().c_str(), 4096, ImGuiInputTextFlags_ReadOnly );
			ImGui::PopStyleColor();

			if( ImGui::Button( "Open externally" ) )
			{
				Application::Get()->OpenNativeFileExplorer( m_SoundAsset->SoundSourcePath );
			}

			if( ImGui::Button( "Show in explorer" ) )
			{
				Application::Get()->OpenNativeFileExplorer( m_SoundAsset->SoundSourcePath, true );
			}

			ImGui::EndHorizontal();

			ImGui::Text( "Original Import Path" );

			ImGui::BeginHorizontal( "##importPath" );

			ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 1.0f, 1.0f, 1.0f, 0.5f ) );
			ImGui::InputText( "##importPath", ( char* ) m_SoundAsset->OriginalImportPath.string().c_str(), 4096, ImGuiInputTextFlags_ReadOnly );
			ImGui::PopStyleColor();

#if !defined(SAT_DIST)
			ImGui::Spacing();
			ImGui::Text( "%s", m_SoundAsset->LastWriteTime.c_str() );

			if( ImGui::BeginItemTooltip() )
			{
				ImGui::Text( "Date when the original file was created. (YYYY-MM-DD)" );
				ImGui::EndTooltip();
			}
#endif

			ImGui::EndHorizontal();

			ImGui::BeginHorizontal( "##media_controls" );

			if( m_PreviewSound && m_PreviewSound->IsPlaying() )
			{
				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Stop" ), { 24.0f, 24.0f } ) )
				{
					m_PreviewSound->Stop();

					AudioSystem::Get().StopSoundGroups();
				}
			}
			else
			{
				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Play" ), { 24.0f, 24.0f } ) )
				{
					AudioSystem::Get().StartSoundGroups();

					if( !m_PreviewSound )
						m_PreviewSound = AudioSystem::Get().RequestNewSound( m_AssetID, UUID(), false );

					m_PreviewSound->Play();
				}
			}

			if( m_PreviewSound )
			{
				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "FastForward" ), { 24.0f, 24.0f }, { 1, 0 }, { 0, 1 } ) )
				{
					m_PreviewSound->SeekTo( 0 );
				}

				ImGui::Text( "%s", m_PreviewSound->FormatSeconds( m_PreviewSound->GetCursorInSeconds() ).c_str() );

				const uint64_t totalFrames = m_PreviewSound->GetDurationInPCM();
				m_CurrentProgress = ( float ) m_PreviewSound->GetCursorInPCM() / ( float ) totalFrames;

				if( ImGui::SliderFloat( "##SeekBar", &m_CurrentProgress, 0.0f, 1.0f, "%.4f", ImGuiSliderFlags_NoInput ) )
				{
					if( m_LastProgress != m_CurrentProgress )
					{
						m_PreviewSound->SeekTo( static_cast< uint64_t >( m_CurrentProgress * totalFrames ) );
						m_LastProgress = m_CurrentProgress;
					}
				}

				if( ImGui::IsItemActive() && !m_WasPlaying )
				{
					m_PreviewSound->Stop();
					m_WasPlaying = true;
				}

				if( !ImGui::IsItemActive() && m_WasPlaying )
				{
					m_PreviewSound->Play();
					m_WasPlaying = false;
				}

				ImGui::Text( "%s", m_PreviewSound->FormatSeconds( m_PreviewSound->GetDurationInSeconds() ).c_str() );

				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "FastForward" ), { 24.0f, 24.0f } ) )
				{
					m_PreviewSound->SeekTo( m_PreviewSound->GetCursorInPCM() );
				}

				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Settings" ), { 24.0f, 24.0f } ) )
				{
					m_OpenSoundSettingsPopup = true;
				}
			}
			else
			{
				// We have to create a separate branch as we can't use m_PreviewSound when it's null
				Auxiliary::ScopedDisabledFlag disabled( true );

				ImGui::Text( "--:--:--" );
				ImGui::SliderFloat( "##SeekBar", &m_CurrentProgress, 0.0f, 1.0f, "%.4f", ImGuiSliderFlags_NoInput );
				ImGui::Text( "--:--:--" );
			}

			ImGui::EndHorizontal();

			ImGui::EndVertical();

			if( m_OpenSoundSettingsPopup && !ImGui::IsPopupOpen( "SoundSettings" ) )
			{
				ImGui::OpenPopup( "SoundSettings" );
				m_OpenSoundSettingsPopup = false;
			}

			ImGui::SetNextWindowSize( { 250.0f, 0.0f } );
			if( ImGui::BeginPopup( "SoundSettings", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize ) )
			{
				float pitch = m_PreviewSound->GetPitch();
				float volume = m_PreviewSound->GetVolume();

				if( Auxiliary::DrawFloatControl( "Pitch", pitch, 0.0f, 100.0f ) )
				{
					m_PreviewSound->SetPitch( pitch );
				}

				if( Auxiliary::DrawFloatControl( "Volume", volume, 0.0f, 100.0f ) )
				{
					m_PreviewSound->SetVolume( volume );
				}

				ImGui::EndPopup();
			}
		}

		ImGui::End();

		if( !m_Open )
		{
			SoundSpecificationAssetSerialiser spec;
			spec.Serialise( m_SoundAsset );
		}
	}

}
