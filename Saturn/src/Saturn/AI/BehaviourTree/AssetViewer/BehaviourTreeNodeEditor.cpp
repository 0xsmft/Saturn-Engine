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

#include "Nodes/BehaviourTreeTaskNode.h"
#include "Nodes/BehaviourTreeNodeBase.h"

#include "BehaviourTreeEditorEvaluator.h"

#include "Saturn/AI/BehaviourTree/Tasks/BehaviourTreeBaseTask.h"
#include "Saturn/AI/BehaviourTree/Tasks/BehaviourTreeCompositeTasks.h"

#include "Saturn/AI/AIAgentEntity.h"
#include "Saturn/AI/BehaviourTree/BehaviourTreeMemoryAssetViewer.h"

#include "Saturn/ImGui/ImGuiWindowManager.h"

#include "Saturn/GameFramework/SClass.h"

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
		m_CurrentTask = nullptr;
		m_LevelOneTasks.clear();
		m_Tasks.clear();

		m_Runtime = nullptr;
	}

	void BehaviourTreeNodeEditor::TraverseBehaviourTree( const Ref<NodeEditorNodeBase>& rRootNode )
	{
#if !defined(SAT_DIST)
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
					const auto nodeA = FindNode( a );
					const auto nodeB = FindNode( b );
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
#endif
	}

	void BehaviourTreeNodeEditor::SetTargetAgent( AIAgentEntity* pAgent )
	{
		m_pAIAgentEntity = pAgent;
	}

	void BehaviourTreeNodeEditor::InitBBAndTasks()
	{
		if( m_BlackboardSpec )
		{
			m_Blackboard = Ref<BehaviourTreeMemory>::Create();
			m_Blackboard->InitialiseVariables( m_BlackboardSpec->ID );
		}

		size_t levelIndex = 0;
		for( const auto& [id, rNode] : m_Nodes )
		{
			if( rNode->GetClass()->IsChildOf( BehaviourTreeTaskNode::StaticClass() ) )
			{
				Ref<BehaviourTreeTaskNode> taskNode = rNode.As<BehaviourTreeTaskNode>();
				m_Tasks[ id ] = taskNode->GetTaskInstance().Get();

				continue;
			}
			else
			{
				Ref<BehaviourTreeNodeBase> behaviourTreeNode = rNode.As<BehaviourTreeNodeBase>();

				auto Itr = std::find_if( m_EvaluationOrder.begin(), m_EvaluationOrder.end(),
					[ id ]( const auto& rInfo )
				{
					return rInfo.NodeID == id;
				} );

				if( Itr != m_EvaluationOrder.end() )
				{
					auto* pTask = behaviourTreeNode->ConvertToTask();

					// Found at level one add to list.
					m_LevelOneTasks[ levelIndex++ ] = pTask;
					m_Tasks[ id ] = pTask;
				}
			}
		}

		// Init all tasks
		for( auto& [id, pTask] : m_Tasks )
		{
			BehaviourTreeNodeBase* pNode = dynamic_cast< BehaviourTreeNodeBase* >( m_Nodes[ id ].Get() );

			// NOTE: Use raw ptr because we don't want tasks to stop the destruction of the blackboard
			pTask->SetBlackboard( m_Blackboard.Get() );
			pTask->InitialiseTask( this, pNode );
		}
	}

	Ref<BehaviourTreeBaseTask> BehaviourTreeNodeEditor::GetTaskFor( UUID node ) const
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
		if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Settings" ), { 24.0f, 24.0f } ) )
		{
			ImGui::OpenPopup( "##SETTINGSBTNE" );
		}

		if( ImGui::BeginItemTooltip() )
		{
			ImGui::Text( "Open the settings of this Node Editor" );

			ImGui::EndTooltip();
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

		if( ImGui::Button( "Open Tree Memory" ) )
		{
			const Ref<Asset> asset = AssetManager::Get().FindAsset( m_BehaviourTreeMemoryAssetID );
			if( asset )
			{
				std::string windowName = std::format( "{0}##{1}", asset->Name, ( uint64_t ) m_BehaviourTreeMemoryAssetID );
				ImGuiWindowManager::Get().OpenOrShowWindow<BehaviourTreeMemoryAssetViewer>( windowName, m_BehaviourTreeMemoryAssetID );

				ImGui::SetWindowFocus( windowName.c_str() );
			}
		}
	}

	void BehaviourTreeNodeEditor::OnExtraRender()
	{
		std::vector<UUID> nodes = GetSelectedNodes();
		if( nodes.size() == 1 )
		{
			Ref<BehaviourTreeNodeBase> treeNode = FindNode( nodes[ 0 ] ).As<BehaviourTreeNodeBase>();
			if( treeNode && treeNode->Type == NodeRenderType::Tree )
			{
				ImGui::Text( "%s/%llu", treeNode->Name.c_str(), treeNode->ID );
				ImGui::Text( "Order: %i", treeNode->EvaluationOrder );
					
				ImGui::Separator();

				treeNode->RenderDetails();
			}
		}
		else
		{
			if( Auxiliary::TreeNode( "Behaviour Tree Memory (blackboard)" ) ) 
			{
				bool open = false;

				ImGui::BeginHorizontal( ( int ) m_AssetID );

				ImGui::Text( "Asset" );

				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), ImVec2( 24.0f, 24.0f ) ) )
				{
					open = true;
				}

				ImGui::Spring();

				UUID tempID = m_BehaviourTreeMemoryAssetID;
				if( Auxiliary::DrawAssetFinder( AssetType::BehaviourTreeMemory, &open, tempID ) )
				{
					AssetManager::Get().UnregisterAssetDependency( m_AssetID, m_BehaviourTreeMemoryAssetID );
					
					MarkDirty();
					m_BehaviourTreeMemoryAssetID = tempID;

					AssetManager::Get().RegisterAssetDependency( m_AssetID, m_BehaviourTreeMemoryAssetID );

					m_BlackboardSpec = AssetManager::Get().GetAssetAs< BehaviourTreeMemorySpecification>( m_BehaviourTreeMemoryAssetID );
				}

				if( m_BehaviourTreeMemoryAssetID != 0 )
				{
					ImGui::InputText( "##asset", (char*)std::to_string( m_BehaviourTreeMemoryAssetID ).data(), 256, ImGuiInputTextFlags_ReadOnly );
				}
				else
				{
					ImGui::InputText( "##asset", (char*)"", 1, ImGuiInputTextFlags_ReadOnly );
				}

				ImGui::EndHorizontal();

				Auxiliary::EndTreeNode();
			}
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

		m_CurrentTask = nullptr;
		m_CurrentTaskIndex = 0;
	}

	AIAgentEntity* BehaviourTreeNodeEditor::GetTargetAgent() const
	{
		return m_pAIAgentEntity;
	}

	void BehaviourTreeNodeEditor::Tick( Timestep ts )
	{
		if( m_CurrentTask )
		{
			const auto status = m_CurrentTask->Tick( ts );
			if( status == BehaviourTreeTaskState::Completed )
			{
				// Move on to the next one
				m_CurrentTask = nullptr;			
			}
#if !defined(SAT_DIST)
			else if( status == BehaviourTreeTaskState::Starting )
			{
				FindTreeFlow();
			}
#endif
		}

		if( m_CurrentTask == nullptr )
		{
			if( m_CurrentTaskIndex + 1 > m_LevelOneTasks.size() )
			{
				m_CurrentTaskIndex = 0;
				m_CurrentTask = nullptr;
			
				// Tree is completed or a node has failed, restart from the root node
				ResetAllTasks();

#if !defined(SAT_DIST)
				ed::StopFlow();

				SAT_CORE_INFO( "Tree completed, restarting..." );
#endif
			}
			else
			{
				m_CurrentTask = m_LevelOneTasks.at( m_CurrentTaskIndex++ );
			}
		}

#if !defined(SAT_DIST)
		ShowTreeFlow();
#endif
	}

#if !defined(SAT_DIST)
	void BehaviourTreeNodeEditor::ShowTreeFlow()
	{
		VariableGuard<ed::EditorContext*> guard( m_Editor );

		ShowFlow( m_EditorLinkPath );
	}

	void BehaviourTreeNodeEditor::FindTreeFlow()
	{
		if( !m_CurrentTask )
			return;

		m_EditorLinkPath.clear();
		ed::StopFlow();

		Ref<BehaviourTreeNodeBase> rootNode = FindNode( m_CurrentTask->GetNodeID() ).As<BehaviourTreeNodeBase>();
		BuildFlow( rootNode );
	}

	void BehaviourTreeNodeEditor::BuildFlow( Ref<BehaviourTreeNodeBase> node )
	{
		if( !node || node->Inputs.empty() )
			return;

		const Ref<Link> link = FindLinkByPin( node->Inputs[ 0 ]->ID );
		if( link )
			m_EditorLinkPath.push_back( link );

		// TODO: To avoid code dupe we should create a base class or a template function
		switch( node->ExecutionType )
		{
			case NodeExecutionType::BehaviourTreeSequenceNode:
			{
				Ref<BehaviourTreeSequenceNode> seq = node.As<BehaviourTreeSequenceNode>();
				if( !seq )
					break;

				for( const auto& childNodeID : seq->GetChildren() )
				{
					auto itr = m_Tasks.find( childNodeID );
					if( itr != m_Tasks.end() )
					{
						auto& rChildTask = itr->second;
						if( !rChildTask )
							continue;

						// Only recurse into running or starting tasks
						if( rChildTask->GetState() == BehaviourTreeTaskState::Running ||
							rChildTask->GetState() == BehaviourTreeTaskState::Starting )
						{
							Ref<BehaviourTreeNodeBase> childNode = FindNode( childNodeID ).As<BehaviourTreeNodeBase>();
							if( childNode )
							{
								BuildFlow( childNode );
								break;
							}
						}
					}
				}
			} break;

			case NodeExecutionType::BehaviourTreeSelectorNode:
			{
				Ref<BehaviourTreeSelectorNode> selector = node.As<BehaviourTreeSelectorNode>();
				if( !selector )
					break;

				for( const auto& childNodeID : selector->GetChildren() )
				{
					auto itr = m_Tasks.find( childNodeID );
					if( itr != m_Tasks.end() )
					{
						auto& rChildTask = itr->second;
						if( !rChildTask )
							continue;

						// Only recurse into running or starting tasks
						if( rChildTask->GetState() == BehaviourTreeTaskState::Running ||
							rChildTask->GetState() == BehaviourTreeTaskState::Starting )
						{
							Ref<BehaviourTreeNodeBase> childNode = FindNode( childNodeID ).As<BehaviourTreeNodeBase>();
							if( childNode )
							{
								// Recursive step
								BuildFlow( childNode ); 
								break;
							}
						}

					}
				}
			} break;

			default: break;
		}
	}
#endif

	void BehaviourTreeNodeEditor::SerialiseData( std::ofstream& rStream, bool isForDist )
	{
		BehaviourTreeNodeEditorSuper::SerialiseData( rStream, isForDist );
		RawSerialisation::WriteVector( m_EvaluationOrder, rStream );

		RawSerialisation::WriteObject( m_BehaviourTreeMemoryAssetID, rStream );
	}

	void BehaviourTreeNodeEditor::DeserialiseData( FDependentIStream& rStream )
	{
		BehaviourTreeNodeEditorSuper::DeserialiseData( rStream );
		RawSerialisation::ReadVector( m_EvaluationOrder, rStream );

		RawSerialisation::ReadObject( m_BehaviourTreeMemoryAssetID, rStream );

#if !defined(SAT_DIST)
		m_BlackboardSpec = AssetManager::Get().GetAssetAs< BehaviourTreeMemorySpecification>( m_BehaviourTreeMemoryAssetID );

		if( m_BlackboardSpec )
		{
			for( auto&& [id, rNode] : m_Nodes )
			{
				Ref<BehaviourTreeNodeBase> treeNode = rNode.As<BehaviourTreeNodeBase>();
				if( treeNode )
				{
					treeNode->PostDeserialise();
				}
			}
		}
#endif
	}

}
