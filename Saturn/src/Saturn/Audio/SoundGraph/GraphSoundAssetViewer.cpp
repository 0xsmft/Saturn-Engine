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
#include "GraphSoundAssetViewer.h"

#include "SoundNodeLibrary.h"
#include "SoundGraphTaskHandler.h"
#include "Nodes/SoundGraphNodes.h"

#include "Saturn/Audio/AudioSystem.h"

#include "Saturn/NodeEditor/Serialisation/NodeCache.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include "Saturn/ImGui/EditorIcons.h"
#include "Saturn/ImGui/UndoRedo/GlobalUndoRedoGroup.h"

#include "Saturn/Asset/AssetManager.h"

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
		if( m_Dirty || m_SoundGraph->IsDirty() )
		{
			m_SoundGraph->SaveAndMarkClean();
		}
		
		m_Asset = nullptr;

		GlobalUndoRedoGroup::Get()->RemoveIfActionHasIdentifier( m_AssetID );
		m_SoundGraph = nullptr;
	}

	void GraphSoundAssetViewer::OnImGuiRender()
	{
		if( m_SoundGraph->IsOpen() )
		{
			m_SoundGraph->OnImGuiRender();
		}
		else
		{
			m_SoundGraph->OpenWindow( false );
			m_Open = false;
		}
	}

	void GraphSoundAssetViewer::AddSoundAsset()
	{
		m_Asset = AssetManager::Get()->FindAsset( m_AssetID );
		m_Name = std::format( "{0}##{1}", m_Asset->Name, ( uint64_t ) m_AssetID );

		m_SoundGraph = SharedPtr<SoundGraph>::Create( m_AssetID );

		const std::string filename = std::format( "{0}.gsnd", m_Asset->Name );
		if( NodeCacheEditor::ReadNodeEditorCache( m_SoundGraph, m_AssetID, filename ) )
		{
			m_OutputNodeID = m_SoundGraph->GetOutputNodeID();
		}
		else
		{
			SetupNewNodeEditor();
		}
		
		m_SoundGraph->NcSetCustomName( filename );
		m_SoundGraph->SetWindowName( m_Name );

		m_SoundGraph->OpenWindow( true );
		m_Open = true;

		SetupNodeEditorCallbacks();
	}

	void GraphSoundAssetViewer::SetupNewNodeEditor()
	{
		m_OutputNodeID = m_SoundGraph->SetupNewNodeEditor()->ID;

		m_SoundGraph->OnNodeEditorEvent( NodeEditorAction::PostLoad );

		MarkDirty();
	}

	void GraphSoundAssetViewer::SetupNodeEditorCallbacks()
	{
#if !defined(SAT_DIST)
		m_SoundGraph->SetCreateNewNodeFunction(
			[&]() -> SharedPtr<NodeEditorNodeBase>
			{
				SharedPtr<NodeEditorNodeBase> result = nullptr;

				ImGui::SeparatorText( "Sound" );

				if( ImGui::MenuItem( "Sound Player" ) )
					result = SoundNodeLibrary::SpawnPlayerNode( m_SoundGraph );

				if( ImGui::MenuItem( "Random Sound" ) )
					result = SoundNodeLibrary::SpawnRandomNode( m_SoundGraph );

				if( ImGui::MenuItem( "Set Sound Pitch" ) )
					result = SoundNodeLibrary::SpawnPitchNode( m_SoundGraph );

				if( ImGui::MenuItem( "Random Pitch" ) )
					result = SoundNodeLibrary::SpawnRandPitch( m_SoundGraph );

				return result;
			} );

		m_SoundGraph->SetTopBarFunction( [&]() 
			{
				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Billboard_Audio" ), { 24, 24 } ) )
				{
					if( !m_TaskHandler )
					{
						m_TaskHandler = Ref<SoundGraphTaskHandler>::Create();
						m_TaskHandler->Init( m_SoundGraph->GetNodeTaskCache() );
					}
				}

				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Billboard_AudioMuted" ), { 24, 24 } ) )
				{
					if( m_TaskHandler )
					{
						m_TaskHandler->DestroyAliveSounds();
						m_TaskHandler = nullptr;
					}
				}

				if( ImGui::IsItemHovered() )
				{
					ImGui::BeginTooltip();
					ImGui::Text( "Stop all sounds." );
					ImGui::EndTooltip();
				}
			} );
#endif
	}

	void GraphSoundAssetViewer::OnUpdate( Timestep ts )
	{
		if( m_TaskHandler )
			m_TaskHandler->Tick( ts );
	}

	void GraphSoundAssetViewer::OnEvent( Event& rEvent )
	{
	}

#if !defined(SAT_DIST)
	void GraphSoundAssetViewer::OnRuntimeStateChanged( RuntimeState newState, RuntimeState oldState )
	{
		switch( newState )
		{
			default:
			case RuntimeState::Starting:
			case RuntimeState::NoState:
			case RuntimeState::Suspended:
				break;

			case RuntimeState::Running:
			{
			} break;

			case RuntimeState::Ending:
			{
			} break;
		}
	}

	void GraphSoundAssetViewer::AddSoundReference( Ref<GraphSound> sound )
	{
	}
#endif

}
