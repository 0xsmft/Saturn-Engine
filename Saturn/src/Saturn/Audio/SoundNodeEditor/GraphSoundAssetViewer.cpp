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
#include "GraphSoundAssetViewer.h"

#include "Saturn/NodeEditor/Serialisation/NodeCache.h"

#include "SoundEditorEvaluator.h"

#include "Nodes/SoundRandomNode.h" 
#include "Nodes/SoundOutputNode.h" 
#include "Nodes/SoundPlayerNode.h"
#include "Nodes/SoundMixerNode.h"

#include "SoundNodeLibrary.h"

#include "Saturn/Audio/AudioSystem.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include "Saturn/ImGui/EditorIcons.h"

#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Core/OptickProfiler.h"

namespace Saturn {

	GraphSoundAssetViewer::GraphSoundAssetViewer( AssetID id )
		: AssetViewer( id )
	{
		m_AssetType = AssetType::GraphSound;
		AddSoundAsset();
	}

	GraphSoundAssetViewer::~GraphSoundAssetViewer()
	{
		std::string filename = std::format( "{0}.gsnd", m_Asset->Name );

		m_Asset = nullptr;

		if( m_Dirty || m_NodeEditor->IsDirty() )
		{
			m_NodeEditor->SaveSettings();
			NodeCacheEditor::WriteNodeEditorCache( m_NodeEditor, filename );
		}

		m_NodeEditor->SetRuntime( nullptr );
		m_Runtime = nullptr;

		m_NodeEditor = nullptr;
	}

	void GraphSoundAssetViewer::OnImGuiRender()
	{
		SAT_PF_EVENT();

		if( m_NodeEditor->IsOpen() )
		{
			m_NodeEditor->OnImGuiRender();
		}
		else
		{
			AudioSystem::Get().StopPreviewSounds( m_AssetID );
			m_NodeEditor->Open( false );
			m_Open = false;

			DestroyViewer( m_AssetID );
		}
	}

	void GraphSoundAssetViewer::AddSoundAsset()
	{
		Ref<Asset> asset = AssetManager::Get().FindAsset( m_AssetID );
		m_Asset = asset;

		m_NodeEditor = Ref<NodeEditor>::Create( m_AssetID );

		std::string filename = std::format( "{0}.gsnd", m_Asset->Name );
		if( NodeCacheEditor::ReadNodeEditorCache( m_NodeEditor, m_AssetID, filename ) )
		{
			m_OutputNodeID = m_NodeEditor->FindNode( "Sound Output" )->ID;
		}
		else
		{
			SetupNewNodeEditor();
		}

		m_NodeEditor->SetWindowName( asset->Name );

		m_NodeEditor->Open( true );
		m_Open = true;

		SetupNodeEditorCallbacks();

		SoundEditorEvaluator::SoundEdEvaluatorInfo info;
		info.SoundGroup = nullptr;
		info.OutputNodeID = m_OutputNodeID;
		
		m_Runtime = Ref<SoundEditorEvaluator>::Create( info );
		m_Runtime->SetTargetNodeEditor( m_NodeEditor );

		m_NodeEditor->SetRuntime( m_Runtime );
	}

	void GraphSoundAssetViewer::SetupNewNodeEditor()
	{
		Ref<SoundOutputNode> OutputNode = SoundNodeLibrary::SpawnOutputNode( m_NodeEditor );

		m_OutputNodeID = OutputNode->ID;

		MarkDirty();
	}

	void GraphSoundAssetViewer::SetupNodeEditorCallbacks()
	{
		m_NodeEditor->SetCreateNewNodeFunction(
			[&]() -> Ref<Node>
			{
				Ref<Node> result = nullptr;

				ImGui::SeparatorText( "SOUND" );

				if( ImGui::MenuItem( "Sound Player" ) )
					result = SoundNodeLibrary::SpawnPlayerNode( m_NodeEditor );

				if( ImGui::MenuItem( "Random" ) )
					result = SoundNodeLibrary::SpawnRandomNode( m_NodeEditor );

				if( ImGui::MenuItem( "Mixer" ) )
					result = SoundNodeLibrary::SpawnMixerNode( m_NodeEditor );

				return result;
			} );

		m_NodeEditor->SetTopBarFunction( [&]() 
			{
				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Billboard_AudioMuted" ), { 24, 24 } ) )
				{
					for( auto& rSound : m_Runtime->AliveSounds )
					{
						rSound->Stop();
						rSound->Reset();
					}
				}

				if( ImGui::IsItemHovered() )
				{
					ImGui::BeginTooltip();
					ImGui::Text( "Stop all sounds." );
					ImGui::EndTooltip();
				}
			} );
	}

	void GraphSoundAssetViewer::ShowDirtyModal()
	{
		// Keep the window open until user selects an option.
		m_NodeEditor->Open( true );
		m_Open = true;

		if( m_ShowDirtyModal )
		{
			ImGui::OpenPopup( "DirtyModal" );
		}

		if( ImGui::BeginPopupModal( "DirtyModal", &m_ShowDirtyModal ) )
		{
			ImGui::Text( "You have unsaved changes to this editor." );
			ImGui::Text( "Would you like to save before closing?" );

			ImGui::BeginHorizontal( "##DirtyModalOpt" );

			ImGui::Button( "Save" );
			ImGui::Button( "Close without saving" );
			ImGui::Button( "Cancel" );

			ImGui::EndHorizontal();

			ImGui::EndPopup();
		}
	}

	void GraphSoundAssetViewer::OnUpdate( Timestep ts )
	{
	}

	void GraphSoundAssetViewer::OnEvent( RubyEvent& rEvent )
	{
	}
}