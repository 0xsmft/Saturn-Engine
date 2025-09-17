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

#include "Saturn/Animation/AssetViewer/Graph/StateMachine/AnimGraphStateMachineStateNode.h"
#include "Saturn/Animation/AssetViewer/Graph/StateMachine/AnimGraphStateMachineTransitionNode.h"
#include "Saturn/Animation/AssetViewer/Graph/StateMachine/StateMachineNodeLibrary.h"

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

	void AnimGraph::DrawGraph()
	{
		switch( m_ViewMode )
		{
			case AnimGraphViewMode::Animation:
				FDependentNodeEditorSuper::DrawGraph();
				return;

			case AnimGraphViewMode::StateMachine:
				DrawStateMachineNodes();
				return;

			case AnimGraphViewMode::StateMachineTransition:
			default:
				break;
		}
	}

	void AnimGraph::DrawStateMachineNodes()
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
			switch( rNode->ExecutionType )
			{
				default: break;

				case NodeExecutionType::AnimGraphStateMachineStateNode:
				case NodeExecutionType::AnimGraphStateMachineTransitionNode:
				case NodeExecutionType::HintNode:
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
						}
					}
				} break;
			}
		}

		for( const auto& rLink : m_Links )
		{
			ed::Link( ed::LinkId( rLink->ID ), ed::PinId( rLink->StartPinID ), ed::PinId( rLink->EndPinID ), rLink->Color );
		}

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
					SharedPtr<AnimGraphStateMachineTransitionNode> node = SharedPtr<AnimGraphStateMachineTransitionNode>::Create();

					AddNode( node );

					CreateLink( startNode->Outputs[ 0 ], node->Inputs[ 0 ] );
					CreateLink( node->Outputs[ 0 ], m_HoveredNode->Inputs[ 0 ] );

					ed::ClearSelection();
					m_TransitionStartNode = 0;
				}
			}
		}
	}

}
