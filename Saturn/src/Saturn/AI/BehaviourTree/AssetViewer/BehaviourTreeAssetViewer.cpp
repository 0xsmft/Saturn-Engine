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
#include "Nodes/BehaviourTreeTaskNode.h"

#include "BehaviourTreeNodeLibrary.h"
#include "BehaviourTreeEditorEvaluator.h"

#include "Saturn/NodeEditor/NodeEditorHintNode.h"
#include "Saturn/NodeEditor/Serialisation/NodeCache.h"

#include "Saturn/ImGui/UndoRedo/GlobalUndoRedoGroup.h"
#include "Saturn/Asset/AssetManager.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

#include <imgui_internal.h>

namespace Saturn {

	BehaviourTreeAssetViewer::BehaviourTreeAssetViewer( AssetID id )
		: AssetViewer( id )
	{
		m_AssetType = AssetType::BehaviourTree;
		AddBehaviourTree();
	}

	BehaviourTreeAssetViewer::~BehaviourTreeAssetViewer()
	{
#if !defined(SAT_DIST)
		const std::string filename = std::format( "{0}.sbt", m_Asset->Name );

		m_Asset = nullptr;

		if( m_Dirty || m_NodeEditor->IsDirty() )
		{
			m_NodeEditor->SaveSettings();
			NodeCacheEditor::WriteNodeEditorCache( m_NodeEditor, filename );
		}

		m_NodeEditor->SetRuntime( nullptr );

		GlobalUndoRedoGroup::Get().RemoveIfActionHasIdentifier( m_AssetID );
		m_NodeEditor = nullptr;

		AssetManager::Get().Save();

		for( BehaviourTreeBaseTask* pTaskClass : m_ClassCache )
		{
			delete pTaskClass;
		}

		m_ClassCache.clear();
#endif
	}

	void BehaviourTreeAssetViewer::OnImGuiRender()
	{
#if !defined(SAT_DIST)
		if( m_NodeEditor->IsOpen() )
		{
			m_NodeEditor->OnImGuiRender();
		}
		else
		{
			m_NodeEditor->Open( false );
			m_Open = false;
		}
#endif
	}

	void BehaviourTreeAssetViewer::AddBehaviourTree()
	{
		const Ref<Asset> asset = AssetManager::Get().FindAsset( m_AssetID );
		m_Asset = asset;

		m_Name = std::format( "{0}##{1}", m_Asset->Name, ( uint64_t ) m_AssetID );

		m_NodeEditor = Ref<BehaviourTreeNodeEditor>::Create( m_AssetID );
		const std::string filename = std::format( "{0}.sbt", m_Asset->Name );

		if( NodeCacheEditor::ReadNodeEditorCache( m_NodeEditor, m_AssetID, filename ) )
		{
			m_RootNodeID = m_NodeEditor->FindNode( "Root Node" )->ID;
		}
		else
		{
			SetupNewNodeEditor();
		}

#if !defined(SAT_DIST)
		m_NodeEditor->NcSetCustomName( filename );
		m_NodeEditor->SetWindowName( m_Name );
		m_NodeEditor->Open( true );
#endif
		m_Open = true;

		SetupNodeEditorCallbacks();

		BehaviourTreeEditorEvaluator::BehaviourTreeEdEvaluatorInfo info{};
		info.OutputNodeID = m_RootNodeID;

		m_Runtime = Ref<BehaviourTreeEditorEvaluator>::Create( info );
		m_Runtime->SetTargetNodeEditor( m_NodeEditor );

		m_NodeEditor->SetRuntime( m_Runtime );

#if !defined(SAT_DIST)
		// Discover all classes that are based from BehaviourTreeBaseTask for out context menu
		const auto map = ClassMetadataHandler::Get().GetAllClassesBasedFrom<BehaviourTreeBaseTask>();
		for( auto* pClass : map )
		{
			BehaviourTreeBaseTask* pObject = dynamic_cast<BehaviourTreeBaseTask*>( ClassMetadataHandler::Get().CreateClassObject( pClass ) );
			
			if( pObject ) 
			{
				// Create default task object from SClass
				m_ClassCache.push_back( pObject );
			}
		}
#endif
	}

	void BehaviourTreeAssetViewer::SetupNewNodeEditor()
	{
		Ref<BehaviourTreeRootNode> OutputNode = BehaviourTreeNodeLibrary::SpawnRootNode( m_NodeEditor );
		m_RootNodeID = OutputNode->ID;

		MarkDirty();
	}

	void BehaviourTreeAssetViewer::SetupNodeEditorCallbacks()
	{
#if !defined(SAT_DIST)
		m_NodeEditor->SetCreateNewNodeFunction(
			[ & ]() -> Ref<NodeEditorNodeBase>
		{
			Ref<NodeEditorNodeBase> result;

			ImGui::SeparatorText( "Basic/Composite" );
			
			if( ImGui::MenuItem( "Selector" ) )
				result = BehaviourTreeNodeLibrary::SpawnSelectorNode( m_NodeEditor );

			if( ImGui::MenuItem( "Sequence" ) )
				result = BehaviourTreeNodeLibrary::SpawnSequenceNode( m_NodeEditor );

			ImGui::SeparatorText( "Tasks" );
			for( auto* pClass : m_ClassCache )
			{
				if( !pClass->IsSpawnableNode() ) continue;

				if( ImGui::MenuItem( pClass->GetTaskName() ) ) 
				{
					// NOTE: Raw ptr converted to Ref<> by BehaviourTreeTaskNode
					SObject* pNewTaskObject = ClassMetadataHandler::Get().CreateClassObject( pClass->GetClass()->GetHash() );

					BehaviourTreeTaskNode* pNode = ClassMetadataHandler::Get().CreateClassObject<BehaviourTreeTaskNode>( BehaviourTreeTaskNode::StaticClass(),( BehaviourTreeBaseTask* ) pNewTaskObject );

					result = pNode;
					m_NodeEditor->AddNode( result );

					break;
				}
			}

			/*
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

			showTooltip( "Move to a predetermined Position in the NavMesh, the Position must be in the NavMesh as if not the task will fail!" );
			*/

			ImGui::SeparatorText( "Auxiliary" );

			if( ImGui::MenuItem( "Hint (Comment) Node" ) )
				result = NodeEditorHintNode::SpawnHintNode( m_NodeEditor );

			return result;
		} );

		m_NodeEditor->SetTopBarFunction( [ & ]()
		{
			if( !GActiveScene->IsRuntimeActive() )
				return;

			ImGui::SeparatorEx( ImGuiSeparatorFlags_Vertical );

			// drop down
			ImGui::Text( "References" );

			ImGui::SetNextItemWidth( 134.0F );
			if( ImGui::BeginCombo( "##References", "" ) )
			{
				for( const auto& rAsset : m_ReferencingAssets )
				{
					if( ImGui::Selectable( rAsset->GetAsset()->Name.c_str() ) )
					{
						// TODO: There isn't technically API to support this asset viewer changing its node editor
						//       however, maybe we should think of a different way to show what the referencing assets are doing
						m_NodeEditor = rAsset->GetNodeEditor();
						m_NodeEditor->Open( true );
						m_NodeEditor->SetState( NodeEditorState::Simulating );

						SetupNodeEditorCallbacks();
					}
				}

				ImGui::EndCombo();
			}
		} );
#endif
	}

#if !defined(SAT_DIST)
	void BehaviourTreeAssetViewer::OnRuntimeStateChanged( RuntimeState newState, RuntimeState oldState )
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

	void BehaviourTreeAssetViewer::AddBehviourTreeReference( Ref<BehaviourTree> asset )
	{
		m_ReferencingAssets.push_back( asset );
	}

#endif
}
