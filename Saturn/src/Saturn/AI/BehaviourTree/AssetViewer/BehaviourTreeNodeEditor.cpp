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
#include "Nodes/BehaviourTreeSequenceNode.h"
#include "Nodes/BehaviourTreeSelectorNode.h"

#include "BehaviourTreeNodeEditor.h"
#include "BehaviourTreeEditorEvaluator.h"

#include "Nodes/BehaviourTreeNodeBase.h"
#include "Saturn/AI/BehaviourTree/Tasks/BehaviourTreeBaseTask.h"

#include "Saturn/AI/AIAgentEntity.h"

#if !defined(SAT_DIST)
#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include "Saturn/ImGui/EditorIcons.h"
#endif

namespace Saturn {

	BehaviourTreeNodeEditor::BehaviourTreeNodeEditor()
		: BehaviourTreeNodeEditorSuper()
	{
	}

	BehaviourTreeNodeEditor::BehaviourTreeNodeEditor( AssetID id )
		: BehaviourTreeNodeEditorSuper( id )
	{
	}

	BehaviourTreeNodeEditor::~BehaviourTreeNodeEditor()
	{
		for( auto& [id, pTask] : m_Tasks )
		{
			delete pTask;
		}

		m_Tasks.clear();
		m_Runtime = nullptr;
	}

	void BehaviourTreeNodeEditor::TraverseBehaviourTree( const Ref<NodeEditorNodeBase>& rRootNode )
	{
		Ref<BehaviourTreeEditorEvaluator> runtime = m_Runtime.As<BehaviourTreeEditorEvaluator>();
		m_EvaluationOrder.clear();

		// BehaviourTree flow in the left direction
		// We use a deques as they don't not enforce FIFO or LIFO
		// The evaluation order can be different as each level of the tree can be different
		//                    ID,   LVL/DEPTH
		std::deque<std::pair<UUID, int>> temporaryStack;
		temporaryStack.emplace_back( rRootNode->ID, 0 );

		size_t nodeEvaluationOrder = 0;
		while( !temporaryStack.empty() )
		{
			const auto [currentID, currentLevel] = temporaryStack.back();
			temporaryStack.pop_back();

			// The evaluator only needs the nodes at level one
			// As nodes at level one have to be a sequence or a selector node
			if( currentLevel == 1 )
			{
				runtime->AddForTick( currentID );
				m_EvaluationOrder.emplace_back( currentID, currentLevel );
			}
			
			// Find neighbors from outputs then keep going down that path until there is no more neighbors
			Ref<BehaviourTreeNodeBase> treeNode = FindNode( currentID ).As<BehaviourTreeNodeBase>();
			if( treeNode )
			{
				treeNode->EvaluationOrder = nodeEvaluationOrder++;

				auto rNeighbours = FindNeighborsLeft( treeNode );

				// Behaviour tree are left to right at the composite level
				std::sort( rNeighbours.begin(), rNeighbours.end(),
					[ this ]( const UUID& a, const UUID& b )
				{
					auto nodeA = FindNode( a );
					auto nodeB = FindNode( b );
					return nodeA->Position.x < nodeB->Position.x;
				} );

				switch( treeNode->ExecutionType )
				{
					default: break;

					case NodeExecutionType::BehaviourTreeSequenceNode:
					{
						Ref<BehaviourTreeSequenceNode> seq = treeNode.As<BehaviourTreeSequenceNode>();
						if( seq )
						{
							seq->Reset();
							seq->AddChildren( rNeighbours );
						}
					} break;

					case NodeExecutionType::BehaviourTreeSelectorNode:
					{
						Ref<BehaviourTreeSelectorNode> selector = treeNode.As<BehaviourTreeSelectorNode>();
						if( selector )
						{
							selector->Reset();
							selector->AddChildren( rNeighbours );
						}
					} break;
				}

				// Push right to left so that the left most node is visited first 
				for( auto it = rNeighbours.rbegin(); it != rNeighbours.rend(); it++ )
				{
					temporaryStack.emplace_back( *it, currentLevel + 1 );
				}
			}
		}
	}

	void BehaviourTreeNodeEditor::InitBehaviourTree()
	{
		size_t levelIndex = 0;
		for( const auto& [id, rNode] : m_Nodes )
		{
			Ref<BehaviourTreeNodeBase> behaviourTreeNode = rNode.As<BehaviourTreeNodeBase>();
			if( behaviourTreeNode )
			{
				auto Itr = std::find_if( m_EvaluationOrder.begin(), m_EvaluationOrder.end(), 
					[ id ](const auto& rInfo)
				{
					return rInfo.NodeID == id && rInfo.Level == 1;
				} );

				auto* pTask = behaviourTreeNode->ConvertToTask();
				if( pTask )
				{
					m_Tasks[ id ] = pTask;
				}

				if( Itr != m_EvaluationOrder.end() )
				{
					// found at level one add to list
					m_LevelOneTasks[ levelIndex++ ] = pTask;
				}
			}
		}

		// Init composite nodes
		for( const auto& [id, pTask] : m_Tasks )
		{
			BehaviourTreeNodeBase* pNode = dynamic_cast< BehaviourTreeNodeBase* >( m_Nodes[ id ].Get() );
			pTask->InitialiseTask( this, pNode );
		}
	}

	BehaviourTreeBaseTask* BehaviourTreeNodeEditor::GetTaskFor( UUID node )
	{
		auto Itr = m_Tasks.find( node );
		if( Itr != m_Tasks.end() )
		{
			return Itr->second;
		}

		return nullptr;
	}

#if !defined(SAT_DIST)
	void BehaviourTreeNodeEditor::OnTopBarRender()
	{
		if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Settings" ), { 24, 24 } ) )
		{
			ImGui::OpenPopup( "##SETTINGSBTNE" );
		}

		if( ImGui::BeginPopup( "##SETTINGSBTNE", ImGuiWindowFlags_NoSavedSettings ) )
		{
			ImGui::Checkbox( "Auto Evaluate", &m_AutoEvaluate );

			if( ImGui::BeginItemTooltip() )
			{
				ImGui::Text( "Re-evaluate the Graph when it is modified, turn this off when dealing with large behaviour trees." );

				ImGui::EndTooltip();
			}

			ImGui::EndPopup();
		}
	}

	void BehaviourTreeNodeEditor::OnExtraRender()
	{
		if( ImGui::Begin( "Details" ) ) 
		{
			std::vector<UUID> nodes = GetSelectedNodes();
			if( nodes.size() == 1 )
			{
				Ref<BehaviourTreeNodeBase> treeNode = FindNode( nodes[ 0 ] ).As<BehaviourTreeNodeBase>();
				if( treeNode )
				{
					ImGui::Text( "%s/%llu", treeNode->Name.c_str(), treeNode->ID );
					ImGui::Text( "Order: %i", treeNode->EvaluationOrder );
					
					ImGui::Separator();

					treeNode->RenderDetails();
				}
			}
			else
			{
				ImGui::Text( "Multiple or no Nodes are selected." );
			}

			ImGui::End();
		}
	}

	void BehaviourTreeNodeEditor::OnNodeEditorEvent( NodeEditorAction action )
	{
		if( !m_AutoEvaluate )
			return;

		switch( action )
		{
			case NodeEditorAction::CreateLink:
			case NodeEditorAction::BreakLink:
			case NodeEditorAction::CreateNode:
			case NodeEditorAction::DestroyNode:
			case NodeEditorAction::MoveNode:
				Evaluate();
				break;

			case NodeEditorAction::PreEvaluate:
			case NodeEditorAction::PostEvaluate:
			case NodeEditorAction::SelectNode:
			case NodeEditorAction::DeselectNode:
			case NodeEditorAction::SelectLink:
			case NodeEditorAction::DeselectLink:
			default: break;
		}
	}
#endif

	void BehaviourTreeNodeEditor::ResetAllTasks()
	{
		for( auto& [id, pTask] : m_Tasks )
		{
			pTask->Reset();
		}
	}

	Ref<AIAgentEntity> BehaviourTreeNodeEditor::GetTargetAgent() const
	{
		return m_AIAgentEntity;
	}

	void BehaviourTreeNodeEditor::Tick( Timestep ts )
	{
		if( m_CurrentTask )
		{
			if( m_CurrentTask->Tick( ts ) == BehaviourTreeTaskState::Completed )
			{
				// Move on to the next one
				m_CurrentTask = nullptr;
			}
		}

		if( m_CurrentTask == nullptr )
		{
			if( m_CurrentTaskIndex + 1 > m_LevelOneTasks.size() )
			{
				m_CurrentTaskIndex = 0;
				m_CurrentTask = nullptr;
				
//				ResetAllTasks();
			}
			else
			{
				m_CurrentTask = m_LevelOneTasks.at( m_CurrentTaskIndex++ );
			}
		}
	}

#if !defined(SAT_DIST)
	void BehaviourTreeNodeEditor::SerialiseData( std::ofstream& rStream )
	{
		BehaviourTreeNodeEditorSuper::SerialiseData( rStream );
		RawSerialisation::WriteVector( m_EvaluationOrder, rStream );
	}

	void BehaviourTreeNodeEditor::DeserialiseData( std::ifstream& rStream )
	{
		BehaviourTreeNodeEditorSuper::DeserialiseData( rStream );
		RawSerialisation::ReadVector( m_EvaluationOrder, rStream );
	}
#endif

}
