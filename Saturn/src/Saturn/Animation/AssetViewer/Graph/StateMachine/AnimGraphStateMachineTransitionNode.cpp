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
#include "AnimGraphStateMachineTransitionNode.h"

#include "Saturn/NodeEditor/NodeEditorBase.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"

namespace Saturn {

	AnimGraphStateMachineTransitionNode::AnimGraphStateMachineTransitionNode()
		: NodeEditorNodeBase()
	{
		CreateNode();
	}

	void AnimGraphStateMachineTransitionNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::AnimGraphStateMachineTransitionNode;

#if !defined(SAT_DIST)
		CanBeDeleted = false;
		Color = ImColor( 48, 128, 255, 100 );
		RenderType = NodeRenderType::Blueprint;
#endif

		Inputs.emplace_back( Ref<Pin>::Create( "In", PinType::AnimGraphAnimation, PinKind::Input ) );
		Outputs.emplace_back( Ref<Pin>::Create( "Out", PinType::AnimGraphAnimation, PinKind::Output ) );
	}

	AnimGraphStateMachineTransitionNode::~AnimGraphStateMachineTransitionNode()
	{
	}

	void AnimGraphStateMachineTransitionNode::Render( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder )
	{
		if( !pOuter->IsLinked( Inputs[ 0 ]->ID ) )
			return;

		const auto linksOut = pOuter->FindLinksByPin( Inputs[ 0 ]->ID );
		const auto linksIn = pOuter->FindLinksByPin( Outputs[ 0 ]->ID );

		const auto startNode = pOuter->FindNodeByPin( linksOut[ 0 ]->StartPinID );
		const auto endNode = pOuter->FindNodeByPin( linksIn[ 0 ]->EndPinID );

		ImVec2 startPoint;
		ImVec2 endPoint;
		const ImVec2 startNodePos = ed::GetNodePosition( ed::NodeId( startNode->ID ) );
		const ImVec2 startNodeSize = ed::GetNodeSize( ed::NodeId( startNode->ID ) );

		if( endNode )
		{
			const ImVec2 endNodePos = ed::GetNodePosition( ed::NodeId( endNode->ID ) );
			const ImVec2 endNodeSize = ed::GetNodeSize( ed::NodeId( endNode->ID ) );

			Auxiliary::GetConnectionPointsBetweenRects( { startNodePos, startNodePos + startNodeSize }, { endNodePos, endNodePos + endNodeSize }, startPoint, endPoint );
		}

		Auxiliary::DrawArrow( startPoint, endPoint, IM_COL32( 255, 255, 255, 255 ) );
	}

	NodeEvaluationState AnimGraphStateMachineTransitionNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		return NodeEvaluationState::Evaluated;
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( AnimGraphStateMachineTransitionNode );
