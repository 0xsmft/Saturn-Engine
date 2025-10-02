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
#include "AnimGraph.h"

// ANIMATION EDITOR
#include "Saturn/Animation/AssetViewer/Graph/Animation/AnimGraphOutputNode.h"
#include "Saturn/Animation/AssetViewer/Graph/Animation/AnimGraphStateMachinePlayerNode.h"

// STATE MACHINE
#include "Saturn/Animation/AssetViewer/Graph/StateMachine/AnimGraphStateMachineStateNode.h"
#include "Saturn/Animation/AssetViewer/Graph/StateMachine/AnimGraphStateMachineTransitionNode.h"
#include "Saturn/Animation/AssetViewer/Graph/StateMachine/StateMachineNodeLibrary.h"

// STATE MACHINE STATE
#include "Saturn/Animation/AssetViewer/Graph/StateMachine/AnimGraphStateMachineOutNode.h"
#include "Saturn/Animation/AssetViewer/Graph/StateMachine/AnimGraphStateMachinePlayAnimNode.h"
#include "Saturn/Animation/AssetViewer/Graph/StateMachine/StateMachineStateNodeLibrary.h"

#include "Saturn/NodeEditor/NodeEditorVariableNode.h"
#include "Saturn/NodeEditor/NodeEditorHintNode.h"

#include "Saturn/NodeEditor/Maths/MathsNodes.h"
#include "Saturn/NodeEditor/Maths/MathsNodeLibrary.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"

namespace Saturn {

	AnimGraph::AnimGraph()
		: FDependentNodeEditorSuper()
	{
	}

	AnimGraph::AnimGraph( AssetID id )
		: FDependentNodeEditorSuper( id )
	{
	}

	AnimGraph::~AnimGraph()
	{
	}

	std::vector<UUID> AnimGraph::TraverseAnimGraph()
	{
		std::map<UUID, std::vector<SharedPtr<NodeEditorNodeBase>>> parentToChildren;
		for( const auto& [id, node] : m_Nodes )
		{
			if( node->pParentObject )
				parentToChildren[ node->pParentObject->ID ].push_back( node );
		}

		std::vector<UUID> order;

		std::stack<UUID> temporaryStack;
		temporaryStack.push( FindNode( "Output Node" )->ID );

		while( !temporaryStack.empty() )
		{
			const auto id = temporaryStack.top();
			temporaryStack.pop();

			const SharedPtr<NodeEditorNodeBase> treeNode = FindNode( id );
			if( treeNode )
			{
				// Find Links
				const auto rNeighbours = FindNeighborsRight( treeNode );

				// Push right to left so that the left most node is visited first 
				for( auto it = rNeighbours.begin(); it != rNeighbours.end(); ++it )
				{
					if( std::find( order.begin(), order.end(), *it ) == order.end() )
					{
						temporaryStack.push( *it );
						order.push_back( *it );
					}
				}

				// If this graph has children (i.e. sub-graph) push them after.
				if( const auto itr = parentToChildren.find( id ); itr != parentToChildren.end() )
				{
					for( const auto& rChild : itr->second )
					{
						if( std::find( order.begin(), order.end(), rChild->ID ) == order.end() )
						{
							temporaryStack.push( rChild->ID );
							order.push_back( rChild->ID );
						}
					}
				}
			}
		}

		return order;
	}

#if !defined(SAT_DIST)
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
#endif

	static std::vector<SClass*> s_AnimGraphAllowedNodes
	{ 
		//////////////////////////////////////////////////////////////////////////
		{ AnimGraphOutputNode::StaticClass()             },
		{ AnimGraphStateMachinePlayerNode::StaticClass() },

		//////////////////////////////////////////////////////////////////////////
		{ NodeEditorVariableNode::StaticClass()          },
		{ NodeEditorSetVariableNode::StaticClass()       },

		//////////////////////////////////////////////////////////////////////////
		{ NodeEditorHintNode::StaticClass()              },

		//////////////////////////////////////////////////////////////////////////
		{ MathsAddFloats::StaticClass()                  },
		{ MathsSubFloats::StaticClass()                  },
		{ MathsMulFloats::StaticClass()                  },
		{ MathsDivideFloats::StaticClass()               }
	};

	static std::vector<SClass*> s_StateMachineAllowedNodes
	{
		//////////////////////////////////////////////////////////////////////////
		{ AnimGraphStateMachineStateNode::StaticClass()      },
		{ AnimGraphStateMachineTransitionNode::StaticClass() },

		//////////////////////////////////////////////////////////////////////////
		{ NodeEditorHintNode::StaticClass()					 },
	};

	static std::vector<SClass*> s_StateMachineStateAllowedNodes
	{
		//////////////////////////////////////////////////////////////////////////
		{ AnimGraphStateMachinePlayAnimNode::StaticClass() },
		{ AnimGraphStateMachineOutNode::StaticClass()      },

		//////////////////////////////////////////////////////////////////////////
		{ NodeEditorHintNode::StaticClass()                },

		//////////////////////////////////////////////////////////////////////////
		{ MathsAddFloats::StaticClass()                    },
		{ MathsSubFloats::StaticClass()                    },
		{ MathsMulFloats::StaticClass()                    },
		{ MathsDivideFloats::StaticClass()                 }
	};

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

		m_HoveredNode = nullptr;
		for( auto& [id, rNode] : m_Nodes )
		{
			if( rNode->pParentObject != m_ActiveSubGraph.Get() )
				continue;

			// Determine current view mode from current sub graph
			if( !m_ActiveSubGraph )
			{
				if( std::find( s_AnimGraphAllowedNodes.begin(), s_AnimGraphAllowedNodes.end(), rNode->GetClass() ) != s_AnimGraphAllowedNodes.end() )
				{
					rNode->Render( m_Builder );
				}
			}
			else if( m_ActiveSubGraph->ExecutionType == NodeExecutionType::AnimGraphStateMachinePlayerNode )
			{
				// Draw state machine nodes
				if( std::find( s_StateMachineAllowedNodes.begin(), s_StateMachineAllowedNodes.end(), rNode->GetClass() ) != s_StateMachineAllowedNodes.end() )
				{
					rNode->Render( m_Builder );

					if( !ImGui::IsItemActive() )
					{
						const ImVec2 nodePosition = ed::GetNodePosition( ed::NodeId( id ) );
						const ImVec2 nodeSize = ed::GetNodeSize( ed::NodeId( id ) );
						const ImRect nodeRectangle( nodePosition, nodePosition + nodeSize );

						if( nodeRectangle.Contains( ImGui::GetMousePos() ) )
						{
							m_HoveredNode = rNode;

							if( ImGui::IsMouseDown( ImGuiMouseButton_Left ) && ImGui::IsKeyDown( ImGuiKey_LeftCtrl ) )
							{
								if( m_TransitionStartNode == 0 )
								{
									m_TransitionStartNode = id;
								}
							}

							if( rNode->ExecutionType == NodeExecutionType::AnimGraphStateMachineStateNode && ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
							{
								AddSubGraph( rNode );
								ChangeEditorNextFrame( rNode );
							}
						}
					}
				}
			}
			else if( m_ActiveSubGraph->GetClass() == AnimGraphStateMachineStateNode::StaticClass() )
			{
				// Render State Machine state nodes
				if( std::find( s_StateMachineStateAllowedNodes.begin(), s_StateMachineStateAllowedNodes.end(), rNode->GetClass() ) != s_StateMachineStateAllowedNodes.end() )
				{
					rNode->Render( m_Builder );
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

				auto drawList = ImGui::GetWindowDrawList();
				drawList->AddRectFilled( rectMin, rectMax, color, size.y * 0.15f );
				ImGui::TextUnformatted( label );
			};

			if( HasPrivilege( NodeEditorUserAuthority::Editing ) && ( ImGui::IsMouseDown( ImGuiMouseButton_Left ) || m_CreateNewNode || ImGui::IsPopupOpen( "Create New Node" ) ) )
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
					SharedPtr<AnimGraphStateMachineTransitionNode> node = NewObject<AnimGraphStateMachineTransitionNode>();

					node->pParentObject = m_ActiveSubGraph.Get();
					AddNode( node );

					CreateLink( startNode->Outputs[ 0 ], node->Inputs[ 0 ] );
					CreateLink( node->Outputs[ 0 ], m_HoveredNode->Inputs[ 0 ] );

					MarkDirty();

					ed::ClearSelection();
					m_TransitionStartNode = 0;
				}
			}
		}
	}

}
