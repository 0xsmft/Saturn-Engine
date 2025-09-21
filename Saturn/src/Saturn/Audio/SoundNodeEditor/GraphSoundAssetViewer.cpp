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

#include "Nodes/SoundRandomSoundNode.h" 
#include "Nodes/SoundOutputNode.h" 
#include "Nodes/SoundPlayerNode.h"
#include "Nodes/SoundMixerNode.h"
#include "Nodes/SoundPitchNode.h"
#include "Nodes/SoundRandomPitchNode.h"

#include "SoundNodeLibrary.h"

#include "Saturn/Audio/AudioSystem.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include "Saturn/ImGui/EditorIcons.h"

#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Core/OptickProfiler.h"

#include "Saturn/Scene/Scene.h"

#include "Saturn/ImGui/UndoRedo/GlobalUndoRedoGroup.h"

namespace Saturn {

	GraphSoundAssetViewer::GraphSoundAssetViewer( AssetID id )
		: AssetViewer( id )
	{
		m_AssetType = AssetType::GraphSound;
		AddSoundAsset();
	}

	GraphSoundAssetViewer::~GraphSoundAssetViewer()
	{		
		// Rare case, may only happen if this viewer without the user ever saving it.
		if( m_Dirty || m_NodeEditor->IsDirty() )
		{
			m_NodeEditor->SaveAndMarkClean();
		}
		
		m_Asset = nullptr;

		m_NodeEditor->SetRuntime( nullptr );
		m_Runtime = nullptr;

		GlobalUndoRedoGroup::Get().RemoveIfActionHasIdentifier( m_AssetID );
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
		}
	}

	void GraphSoundAssetViewer::AddSoundAsset()
	{
		Ref<Asset> asset = AssetManager::Get().FindAsset( m_AssetID );
		m_Asset = asset;

		m_Name = std::format( "{0}##{1}", m_Asset->Name, ( uint64_t ) m_AssetID );

		m_NodeEditor = SharedPtr<NodeEditor>::Create( m_AssetID );

		const std::string filename = std::format( "{0}.gsnd", m_Asset->Name );
		if( NodeCacheEditor::ReadNodeEditorCache( m_NodeEditor, m_AssetID, filename ) )
		{
			m_OutputNodeID = m_NodeEditor->FindNode( "Sound Output" )->ID;
		}
		else
		{
			SetupNewNodeEditor();
		}
		
		m_NodeEditor->NcSetCustomName( filename );
		m_NodeEditor->SetWindowName( m_Name );

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
		SharedPtr<SoundOutputNode> OutputNode = SoundNodeLibrary::SpawnOutputNode( m_NodeEditor );

		m_OutputNodeID = OutputNode->ID;

		MarkDirty();
	}

	void GraphSoundAssetViewer::SetupNodeEditorCallbacks()
	{
#if !defined(SAT_DIST)
		m_NodeEditor->SetCreateNewNodeFunction(
			[&]() -> SharedPtr<NodeEditorNodeBase>
			{
				SharedPtr<NodeEditorNodeBase> result = nullptr;

				ImGui::SeparatorText( "Sound" );

				if( ImGui::MenuItem( "Sound Player" ) )
					result = SoundNodeLibrary::SpawnPlayerNode( m_NodeEditor );

				if( ImGui::MenuItem( "Random Sound" ) )
					result = SoundNodeLibrary::SpawnRandomNode( m_NodeEditor );

				if( ImGui::MenuItem( "Sound Mixer" ) )
					result = SoundNodeLibrary::SpawnMixerNode( m_NodeEditor );

				if( ImGui::MenuItem( "Set Sound Pitch" ) )
					result = SoundNodeLibrary::SpawnPitchNode( m_NodeEditor );

				if( ImGui::MenuItem( "Random Pitch" ) )
					result = SoundNodeLibrary::SpawnRandPitch( m_NodeEditor );
				
				//ImGui::SeparatorText( "Maths" );

				//if( ImGui::MenuItem( "Constant Float" ) )
				//	result = SoundNodeLibrary::SpawnFloatConst( m_NodeEditor );

				return result;
			} );

		m_NodeEditor->SetTopBarFunction( [&]() 
			{
				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Billboard_AudioMuted" ), { 24, 24 } ) )
				{
					m_Runtime->TerminateEvaluation();
				}

				if( ImGui::IsItemHovered() )
				{
					ImGui::BeginTooltip();
					ImGui::Text( "Stop all sounds." );
					ImGui::EndTooltip();
				}

				ImGui::SeparatorEx( ImGuiSeparatorFlags_Vertical );

				// drop down
				ImGui::Text( "References" );

				ImGui::SetNextItemWidth( 134.0F );
				if( ImGui::BeginCombo( "##References", "" ) ) 
				{
					for( const auto& rAsset : m_ReferencingAssets )
					{
						const std::string name = std::to_string( rAsset->GetPlayerID() );
						if( ImGui::Selectable( name.c_str() ) )
						{
							// TODO: There isn't technically API to support this asset viewer changing its node editor
							//       however, maybe we should think of a different way to show what the referencing assets are doing
							m_NodeEditor = rAsset->GetNodeEditor();
							m_NodeEditor->Open( true );

							SetupNodeEditorCallbacks();
						}
					}

					ImGui::EndCombo();
				}
			} );
#endif
	}

	void GraphSoundAssetViewer::OnUpdate( Timestep ts )
	{
	}

	void GraphSoundAssetViewer::OnEvent( Event& rEvent )
	{
	}

#if !defined(SAT_DIST)
	void GraphSoundAssetViewer::OnRuntimeStateChanged( RuntimeState newState, RuntimeState oldState )
	{
		switch( newState )
		{
			case RuntimeState::Starting:
			case RuntimeState::NoState:
			case RuntimeState::Suspended:
				break;

			case RuntimeState::Running:
			{
				if( oldState == RuntimeState::Starting || oldState == RuntimeState::NoState )
					m_OriginalNodeEditor = m_NodeEditor;
			} break;

			case RuntimeState::Ending:
			{
				m_NodeEditor = m_OriginalNodeEditor;
				m_ReferencingAssets.clear();
			} break;
		}
	}

	void GraphSoundAssetViewer::AddSoundReference( Ref<GraphSound> sound )
	{
		m_ReferencingAssets.push_back( sound );
	}
#endif

}
