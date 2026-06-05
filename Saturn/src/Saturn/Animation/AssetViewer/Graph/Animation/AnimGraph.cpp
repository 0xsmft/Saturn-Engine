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
#include "AnimGraph.h"

#include "Saturn/Animation/AssetViewer/Graph/AnimGraphPreCompiler.h"

// ANIMATION EDITOR
#include "Saturn/Animation/AssetViewer/Graph/Animation/AnimGraphOutputNode.h"
#include "Saturn/Animation/AssetViewer/Graph/Animation/AnimGraphStateMachinePlayerNode.h"
#include "Saturn/Animation/AssetViewer/Graph/Animation/AnimGraphNodeLibrary.h"

// STATE MACHINE
#include "Saturn/Animation/AssetViewer/Graph/StateMachine/AnimGraphStateMachineStateNode.h"
#include "Saturn/Animation/AssetViewer/Graph/StateMachine/AnimGraphStateMachineTransitionNode.h"
#include "Saturn/Animation/AssetViewer/Graph/StateMachine/StateMachineNodeLibrary.h"

// STATE MACHINE STATE
#include "Saturn/Animation/AssetViewer/Graph/StateMachine/AnimGraphStateMachineOutNode.h"
#include "Saturn/Animation/AssetViewer/Graph/StateMachine/AnimGraphStateMachinePlayAnimNode.h"
#include "Saturn/Animation/AssetViewer/Graph/StateMachine/StateMachineStateNodeLibrary.h"

// TRANSITION
#include "Saturn/Animation/AssetViewer/Graph/StateMachine/AnimGraphTransitionGraphNodes.h"

// TASKS
#include "Saturn/Animation/AssetViewer/Graph/Tasks/GraphTask.h"
#include "Saturn/Animation/AssetViewer/Graph/Tasks/AnimGraphStateMachineState.h"
#include "Saturn/Animation/AssetViewer/Graph/Tasks/AnimGraphStateMachineTask.h"

#include "Saturn/NodeEditor/NodeEditorVariableNode.h"
#include "Saturn/NodeEditor/NodeEditorHintNode.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "Saturn/ImGui/ImGuiAuxiliary.h"

namespace Saturn {

	AnimGraph::AnimGraph()
		: FDependentNodeEditorSuper()
	{
		m_PreCompiler = Ref<AnimGraphPreCompiler>::Create();
	}

	AnimGraph::AnimGraph( AssetID id )
		: FDependentNodeEditorSuper( id )
	{
		m_PreCompiler = Ref<AnimGraphPreCompiler>::Create();
	}

	AnimGraph::~AnimGraph()
	{
	}

	SharedPtr<NodeEditorNodeBase> AnimGraph::SetupNewNodeEditor()
	{
		ed::SetCurrentEditor( m_Editor );

		auto node = AnimGraphNodeLibrary::SpawnOutputNode( SharedFromThis() );
		ed::SetNodePosition( ed::NodeId( node->ID ), { 0.0f, 0.0f } );

		return node;
	}

	AnimGraph::AnimGraphSortMap AnimGraph::TraverseAndCreateTasks()
	{
		// SORTING:
		// 1. Sort the animation graph first
		// 2. Then, we go into the state machine graph, and we push the entry state node
		// 2.5 Sort the state machine state graph
		// Now from here the sorting order will be wrong, as transitions dictate where to go next, meaning that runtime will not follow the same order,
		// but we still push everything the same expect we look for neighbours in outputs (left) direction
		// 3: Push transitions and other state machine states and state machine state graphs
		// 4. Combine into a map of sub-graph parent ID to tasks for runtime.

		AnimGraphSortMap resultToChildren;
#if !defined(SAT_DIST)

		// 1: PARENT ID -> CHILDREN
		std::unordered_map<UUID, std::vector<SharedPtr<NodeEditorNodeBase>>> parentToChildren;
		for( const auto& [id, node] : m_Nodes )
		{
			if( !node->pParentObject ) 
			{
				if( !parentToChildren[ 0llu ].size() )
				{
					resultToChildren[ 0llu ].pGraphTask = NewObject<SGraphTask>( this );

					resultToChildren[ 0llu ].pGraphTask->SetDebugName( "Root AG" );
				}
			}
		}

		// Anim Graph
		SortAnimGraph( resultToChildren );

		auto& rGraphTaskInfo = resultToChildren[ UUID( 0 ) ];

		auto task = ( AnimGraphStateMachineTask* ) rGraphTaskInfo.pGraphTask->GetTasks()[ 0 ].pTask;

		// Entry point of state machine then the rest of the state machine
		SortStateMachineEntry( task );
#endif

		return resultToChildren;
	}

#if !defined(SAT_DIST)
	void AnimGraph::SortAnimGraph( AnimGraphSortMap& rMap )
	{
		std::stack<UUID> stack;
		stack.push( FindNode( "Output Node" )->ID );

		while( !stack.empty() )
		{
			const auto currentID = stack.top();
			stack.pop();

			SharedPtr<NodeEditorNodeBase> currentNode = FindNode( currentID );

			const UUID subGraphID = currentNode->pParentObject ? currentNode->pParentObject->ID : UUID( 0 );
			auto& rInfo = rMap[ subGraphID ];
			
			auto* pTask = currentNode->ConvertToTask();
			if( pTask )
			{
				rInfo.pGraphTask->AddTask( currentID, pTask );
			}

			// Find neighbors from inputs and continue until there is no neighbors
			for( const auto& rNeighbor : FindNeighborsRight( currentNode ) )
			{
				stack.push( rNeighbor );
			}
		}
	}

	void AnimGraph::SortStateMachineEntry( AnimGraphStateMachineTask* pStateMachine )
	{
		auto stateNode = m_StateMachineEntryNode.As<AnimGraphStateMachineStateNode>();

		std::vector<UUID> visited;
		std::vector<UUID> visitedStates;
		std::queue<UUID> temporaryStack;
		temporaryStack.push( stateNode->ID );
		visited.push_back( stateNode->ID );

		while( !temporaryStack.empty() )
		{
			const UUID currentID = temporaryStack.front();
			temporaryStack.pop();

			// Find neighbors from outputs and continue until there is no neighbors.
			SharedPtr<NodeEditorNodeBase> currentNode = FindNode( currentID );

			// Add state machine task only if we are a state node, because we may be a transition node.
			if( currentNode->GetClass() == AnimGraphStateMachineStateNode::StaticClass() )
			{
				if( std::find( visitedStates.begin(), visitedStates.end(), currentID ) == visitedStates.end() )
				{
					AnimGraphStateMachineState* pState = NewObject<AnimGraphStateMachineState>( nullptr );
					pState->PreInitialiseTask( this, currentNode.Get() );

					pStateMachine->AddState( pState );

					visitedStates.push_back( currentID );
				}
			}

			const auto& rNeighbours = FindNeighborsLeft( currentNode );
			for( auto Itr = rNeighbours.rbegin(); Itr != rNeighbours.rend(); ++Itr )
			{
				if( std::find( visited.begin(), visited.end(), *Itr ) == visited.end() )
				{
					temporaryStack.push( *Itr );
					visited.push_back( *Itr );
				}
			}
		}
	}

	void AnimGraph::BuildTaskCache()
	{
		auto order = TraverseAndCreateTasks();
		m_TaskCache.BuildMasterListForAnimGraph( this, order );
	}

	void AnimGraph::OnExtraRender()
	{
		std::vector<UUID> nodes = GetSelectedNodes();
		if( nodes.size() == 1 )
		{
			auto node = FindNode( nodes[ 0 ] );

			ImGui::BeginHorizontal( "##editnodeinfo" );
			ImGui::Text( "Name" );
			Auxiliary::InputText( "##editname", &node->Name );
			ImGui::EndHorizontal();
		}
	}

	void AnimGraph::OnNodeEditorEvent( NodeEditorAction action )
	{
		switch( action )
		{
			case NodeEditorAction::DestroyNode:
			{
				std::vector<UUID> nodesToDelete;
				// Worse case we need to delete more than 5 nodes, we reallocate the vector.
				nodesToDelete.reserve( 5 );

				// Find all ill-formatted transition nodes, if a state machine state node was destroyed.
				for( const auto& [id, node] : m_Nodes )
				{
					if( node->GetClass() != AnimGraphStateMachineTransitionNode::StaticClass() )
						continue;

					// Check inputs and outputs, if any are not linked after destruction, we must delete the transition node.
					if( !IsLinked( node->Inputs[ 0 ]->ID ) || !IsLinked( node->Outputs[ 0 ]->ID ) )
					{
						nodesToDelete.push_back( id );
					}
				}
		
				for( const auto& id : nodesToDelete )
				{
					DeleteNode( id, true );
				}
			} break;

			case NodeEditorAction::PreEvaluate:
			{

			} break;

			case NodeEditorAction::PostEvaluateSuccess:
			{
				BuildTaskCache();
				SaveAndMarkClean();
			} break;

			default: break;
		}
	}

	void AnimGraph::SerialiseData( std::ofstream& rStream )
	{
		FDependentNodeEditorSuper::SerialiseData( rStream );

		// We may not actually have an entry node yet, this graph might not contain any state machines yet.
		if( m_StateMachineEntryNode )
			RawSerialisation::WriteObjectChecked( m_StateMachineEntryNode->ID, rStream );
		else
			RawSerialisation::WriteObjectChecked( 0llu, rStream );
	}

	void AnimGraph::DeserialiseData( std::ifstream& rStream )
	{
		FDependentNodeEditorSuper::DeserialiseData( rStream );

		UUID entryID = 0;
		RawSerialisation::ReadObjectChecked( entryID, rStream );

		// We may not actually have an entry node yet, this graph might not contain any state machines yet.
		if( entryID )
			m_StateMachineEntryNode = m_Nodes[ entryID ];
	}

	void AnimGraph::DrawGraph()
	{
		// Suspend user input if we are currently dragging.
		if( ImGui::IsMouseDown( ImGuiMouseButton_Left ) && ImGui::IsKeyDown( ImGuiKey_LeftCtrl ) )
		{
			ed::SuspendUserInput();
		}
		else
		{
			ed::ResumeUserInput();
		}

		if( m_CanResetHoveredNode )
		{
			m_HoveredNode = nullptr;
			m_CanResetHoveredNode = false;
		}

		for( auto& [id, rNode] : m_Nodes )
		{
			if( rNode->pParentObject != m_ActiveSubGraph.Get() )
				continue;

			// Render the node.
			rNode->Render( m_Builder );

			if( m_ActiveSubGraph && m_ActiveSubGraph->ExecutionType == NodeExecutionType::AnimGraphStateMachinePlayerNode )
			{
				if( !ImGui::IsItemActive() )
				{
					const ImVec2 nodePosition = ed::GetNodePosition( ed::NodeId( id ) );
					const ImVec2 nodeSize = ed::GetNodeSize( ed::NodeId( id ) );
					const ImRect nodeRectangle( nodePosition, nodePosition + nodeSize );

					if( nodeRectangle.Contains( ImGui::GetMousePos() ) )
					{
						m_StateNodeHovered = true;
						m_HoveredNode = rNode;

						if( ImGui::IsMouseDown( ImGuiMouseButton_Left ) && ImGui::IsKeyDown( ImGuiKey_LeftCtrl ) )
						{
							// Set the starting point of a new transition to be the hovered state node.
							if( m_TransitionStartNode == 0 )
							{
								m_TransitionStartNode = id;
							}
						}

						// If we double click, go into the state node.
						if( rNode->ExecutionType == NodeExecutionType::AnimGraphStateMachineStateNode && ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
						{
							AddSubGraph( rNode );
							ChangeEditorNextFrame( rNode );
						}
					}
					else if( m_StateNodeHovered && !m_CanResetHoveredNode )
					{
						// We can only reset the hovered node after we've moved away from it, if the
						// right click popup is not open.
						m_CanResetHoveredNode = !ImGui::IsPopupOpen( "NE_NodeAction" );
					}
				}
			}
		}

		if( m_ActiveSubGraph && m_ActiveSubGraph->ExecutionType == NodeExecutionType::AnimGraphStateMachinePlayerNode )
			DrawStateMachineNodes();

		for( const auto& rLink : m_Links )
			ed::Link( ed::LinkId( rLink->ID ), ed::PinId( rLink->StartPinID ), ed::PinId( rLink->EndPinID ), rLink->Color );

		HandleCreate();
	}

	void AnimGraph::DrawStateMachineNodes()
	{
		if( m_TransitionStartNode != 0 )
		{
			auto showLabel = []( const char* label, ImColor color )
			{
				ImGui::SetCursorPosY( ImGui::GetCursorPosY() - ImGui::GetTextLineHeight() );
				const auto size = ImGui::CalcTextSize( label );

				const auto padding = ImGui::GetStyle().FramePadding;
				const auto spacing = ImGui::GetStyle().ItemSpacing;

				ImGui::SetCursorPos( ImGui::GetCursorPos() + ImVec2( spacing.x, -spacing.y ) );

				const auto rectMin = ImGui::GetCursorScreenPos() - padding;
				const auto rectMax = ImGui::GetCursorScreenPos() + size + padding;

				auto pDrawList = ImGui::GetWindowDrawList();
				pDrawList->AddRectFilled( rectMin, rectMax, color, size.y * 0.15f );
				ImGui::TextUnformatted( label );
			};

			if( HasUserAuthority( NodeEditorUserAuthority::Editing ) && ( ImGui::IsMouseDown( ImGuiMouseButton_Left ) || m_CreateNewNode || ImGui::IsPopupOpen( "Create New Node" ) ) )
			{
				ImVec2 startPoint;
				ImVec2 endPoint;
//				ImColor color;
				const ImVec2 startNodePos = ed::GetNodePosition( ed::NodeId( m_TransitionStartNode ) );
				const ImVec2 startNodeSize = ed::GetNodeSize( ed::NodeId( m_TransitionStartNode ) );

				if( m_HoveredNode )
				{
					const ImVec2 endNodePos = ed::GetNodePosition( ed::NodeId( m_HoveredNode->ID ) );
					const ImVec2 endNodeSize = ed::GetNodeSize( ed::NodeId( m_HoveredNode->ID ) );

					Auxiliary::GetConnectionPointsBetweenRects( { startNodePos, startNodePos + startNodeSize }, { endNodePos, endNodePos + endNodeSize }, startPoint, endPoint );
				}
				else
				{
					endPoint = m_CreateNewNode ? ImVec2( 0.0F, 0.0F ) : ImGui::GetMousePos();
					startPoint = Auxiliary::GetConnectionPointBetweenRectAndPoint( { startNodePos, startNodePos + startNodeSize }, endPoint );
				}

				Auxiliary::DrawArrow( startPoint, endPoint, IM_COL32( 255, 255, 255, 255 ) );

				if( !m_HoveredNode )
				{
					auto* pDrawList = ImGui::GetWindowDrawList();

					auto labelPoint = endPoint + ImVec2( 0.0F, -ImGui::GetTextLineHeight() );
					const auto size = ImGui::CalcTextSize( "+ Create State" );
					const auto padding = ImGui::GetStyle().FramePadding;
					const auto spacing = ImGui::GetStyle().ItemSpacing;

					labelPoint += ImVec2( spacing.x, spacing.y );

					const auto rectMin = labelPoint - padding;
					const auto rectMax = labelPoint + size + padding;

					pDrawList->AddRectFilled( rectMin, rectMax, IM_COL32( 32, 45, 32, 180 ), size.y * 0.15f );
					pDrawList->AddText( labelPoint, IM_COL32( 255, 255, 255, 255 ), "+ Create State" );
				}
			}
			else
			{
				if( m_HoveredNode )
				{
					auto startNode = FindNode( m_TransitionStartNode );

					// Create transition node
					SharedPtr<AnimGraphStateMachineTransitionNode> node = NewObject<AnimGraphStateMachineTransitionNode>( this );

					node->pParentObject = m_ActiveSubGraph.Get();
					AddNode( node );

					node->PostPlace();

					CreateLink( startNode->Outputs[ 0 ], node->Inputs[ 0 ], startNode->Outputs[ 0 ]->GetPinColor() );
					CreateLink( node->Outputs[ 0 ], m_HoveredNode->Inputs[ 0 ], node->Outputs[ 0 ]->GetPinColor() );

					node->Name = std::format( "Transition from {0} to {1}", startNode->Name, m_HoveredNode->Name );

					MarkDirty();

					ed::ClearSelection();
					m_TransitionStartNode = 0;
				}
			}
		}
	}
#endif

}
