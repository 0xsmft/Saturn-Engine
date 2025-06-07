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
#include "BehaviourTreeAssetViewer.h"

#include "Nodes/BehaviourTreeRootNode.h"
#include "Nodes/BehaviourTreeSelectorNode.h"
#include "Nodes/BehaviourTreeSequenceNode.h"
#include "Nodes/BehaviourTreeTaskNodes.h"
#include "BehaviourTreeNodeLibrary.h"

#include "BehaviourTreeEditorEvaluator.h"

#include "Saturn/NodeEditor/Serialisation/NodeCache.h"
#include "Saturn/ImGui/UndoRedo/GlobalUndoRedoGroup.h"
#include "Saturn/Asset/AssetManager.h"

namespace Saturn {

	BehaviourTreeAssetViewer::BehaviourTreeAssetViewer( AssetID id )
		: AssetViewer( id )
	{
		m_AssetType = AssetType::BehaviourTree;
		AddBehaviourTree();
	}

	BehaviourTreeAssetViewer::~BehaviourTreeAssetViewer()
	{
		std::string filename = std::format( "{0}.sbt", m_Asset->Name );

		m_Asset = nullptr;

		if( m_Dirty || m_NodeEditor->IsDirty() )
		{
			m_NodeEditor->SaveSettings();
			NodeCacheEditor::WriteNodeEditorCache( m_NodeEditor, filename );
		}

		m_NodeEditor->SetRuntime( nullptr );

		GlobalUndoRedoGroup::Get().RemoveIfActionHasIdentifier( m_AssetID );
		m_NodeEditor = nullptr;
	}

	void BehaviourTreeAssetViewer::OnImGuiRender()
	{
		if( m_NodeEditor->IsOpen() )
		{
			m_NodeEditor->OnImGuiRender();
		}
		else
		{
			m_NodeEditor->Open( false );
			m_Open = false;
		}
	}

	void BehaviourTreeAssetViewer::OnUpdate( Timestep ts )
	{
		if( m_NodeEditor->GetState() == NodeEditorState::Simulating )
		{
			m_Runtime->Tick( ts );
		}
	}

	void BehaviourTreeAssetViewer::OnEvent( RubyEvent& rEvent )
	{

	}

	void BehaviourTreeAssetViewer::AddBehaviourTree()
	{
		Ref<Asset> asset = AssetManager::Get().FindAsset( m_AssetID );
		m_Asset = asset;

		m_Name = std::format( "{0}##{1}", m_Asset->Name, ( uint64_t ) m_AssetID );

		m_NodeEditor = Ref<BehaviourTreeNodeEditor>::Create( m_AssetID );
		std::string filename = std::format( "{0}.sbt", m_Asset->Name );

		if( NodeCacheEditor::ReadNodeEditorCache( m_NodeEditor, m_AssetID, filename ) )
		{
			m_RootNodeID = m_NodeEditor->FindNode( "Root Node" )->ID;
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

		BehaviourTreeEditorEvaluator::BehaviourTreeEdEvaluatorInfo info{};
		info.OutputNodeID = m_RootNodeID;

		m_Runtime = Ref<BehaviourTreeEditorEvaluator>::Create( info );
		m_Runtime->SetTargetNodeEditor( m_NodeEditor );

		m_NodeEditor->SetRuntime( m_Runtime );
	}

	void BehaviourTreeAssetViewer::SetupNewNodeEditor()
	{
		Ref<BehaviourTreeRootNode> OutputNode = BehaviourTreeNodeLibrary::SpawnRootNode( m_NodeEditor );
		m_RootNodeID = OutputNode->ID;

		MarkDirty();
	}

	void BehaviourTreeAssetViewer::SetupNodeEditorCallbacks()
	{
		m_NodeEditor->SetCreateNewNodeFunction(
			[ & ]() -> Ref<NodeEditorNodeBase>
		{
			auto showTooltip = []( const char* pText )
			{
				if( ImGui::BeginItemTooltip() )
				{
					ImGui::Text( pText );
					ImGui::EndTooltip();
				}
			};

			Ref<NodeEditorNodeBase> result = nullptr;

			ImGui::SeparatorText( "Basic/Composite" );

			if( ImGui::MenuItem( "Selector" ) )
				result = BehaviourTreeNodeLibrary::SpawnSelectorNode( m_NodeEditor );

			if( ImGui::MenuItem( "Sequence" ) )
				result = BehaviourTreeNodeLibrary::SpawnSequenceNode( m_NodeEditor );

			ImGui::SeparatorText( "Tasks" );

			if( ImGui::MenuItem( "Wait" ) )
				result = BehaviourTreeNodeLibrary::SpawnWaitNode( m_NodeEditor );

			if( ImGui::MenuItem( "Play Sound" ) )
				result = BehaviourTreeNodeLibrary::SpawnPlaySoundNode( m_NodeEditor );

			if( ImGui::MenuItem( "Move To" ) )
				result = BehaviourTreeNodeLibrary::SpawnMoveToNode( m_NodeEditor );

			showTooltip( "Move to a predetermined Position in the NavMesh, the Position must be in the NavMesh as if it it not the task will fail!" );

			return result;
		} );
	}

}
