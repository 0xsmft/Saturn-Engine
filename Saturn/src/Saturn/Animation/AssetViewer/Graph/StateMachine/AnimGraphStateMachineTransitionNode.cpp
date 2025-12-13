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

#include "TransitionNodeLibrary.h"
#include "AnimGraphTransitionGraphNodes.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

#include "Saturn/Animation/AssetViewer/Graph/Animation/AnimGraph.h"
#include "Saturn/Animation/AssetViewer/Graph/Tasks/AnimGraphTransitionTasks.h"

#include "Saturn/NodeEditor/NodeEditorBase.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"

namespace Saturn {

	AnimGraphStateMachineTransitionNode::AnimGraphStateMachineTransitionNode()
		: NodeEditorNodeBase( "Transition" )
	{
		CreateNode();
	}

	void AnimGraphStateMachineTransitionNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::AnimGraphStateMachineTransitionNode;

#if !defined(SAT_DIST)
		Color = ImColor( 48, 128, 255, 100 );
		RenderType = NodeRenderType::Blueprint;
#endif

		Inputs.emplace_back( Ref<Pin>::Create( "In", PinType::AnimGraphAnimation, PinKind::Input ) );
		Outputs.emplace_back( Ref<Pin>::Create( "Out", PinType::AnimGraphAnimation, PinKind::Output ) );
	}

	AnimGraphStateMachineTransitionNode::~AnimGraphStateMachineTransitionNode()
	{
	}

	static float PointToOffsetArrowDistanceSquared( const ImVec2& point, ImVec2 startPoint, ImVec2 endPoint, const float offset )
	{
		ImVec2 arrowDir = endPoint - startPoint;
		float arrowLengthSquared = ImLengthSqr( arrowDir );
		float arrowLength = sqrt( arrowLengthSquared );
		if( arrowLength > 0.0 )
		{
			arrowDir /= arrowLength;
		}
		ImVec2 orthogonal = { arrowDir.y, -arrowDir.x };
		ImVec2 offsetVec = ( orthogonal * offset );

		startPoint += offsetVec;
		endPoint += offsetVec;

		ImVec2 pointToStart = point - startPoint;
		ImVec2 pointToEnd = point - endPoint;

		float dot1 = pointToStart.x * arrowDir.x + pointToStart.y * arrowDir.y;

		// return "infinity" if not directly to one side of the line or the other
		if( dot1 < 0 )
			return FLT_MAX;
		else if( ( dot1 * dot1 ) > arrowLengthSquared )
			return FLT_MAX;

		ImVec2 projection = startPoint + arrowDir * dot1;
		ImVec2 pointToProjection = point - projection;
		return ( pointToProjection.x * pointToProjection.x ) + ( pointToProjection.y * pointToProjection.y );
	}

	void AnimGraphStateMachineTransitionNode::Render( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder )
	{
#if !defined(SAT_DIST)
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

		if( PointToOffsetArrowDistanceSquared( ImGui::GetMousePos(), startPoint, endPoint, 5.0f ) < 25.0f )
		{
			if( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
			{
				auto* AG = dynamic_cast< AnimGraph* >( pOuter );
				if( AG )
				{
					AG->AddSubGraph( SharedFromThis() );
					AG->ChangeEditorNextFrame( SharedFromThis() );
				}
			}
		
			const auto color = ed::GetStyle().Colors[ ed::StyleColor_HovLinkBorder ];

			constexpr float sizeOffset = 1.5f;
			Auxiliary::DrawArrowOffset( startPoint, endPoint, ImColor( color ), 2.0F + sizeOffset, 6.0F, 5.0F );
		}

		Auxiliary::DrawArrowOffset( startPoint, endPoint, IM_COL32( 255, 255, 255, 255 ), 2.0F, 6.0F, 5.0F );
#endif
	}

	NodeEvaluationState AnimGraphStateMachineTransitionNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		return NodeEvaluationState::Evaluated;
	}

	void AnimGraphStateMachineTransitionNode::PostPlace()
	{
		// Create output node for this transition.
		auto outNode = TransitionNodeLibrary::SpawnOutputNode( pOuter->SharedFromThis() );
		outNode->pParentObject = this;
		m_OutputNodeID = outNode->ID;
	}

	void AnimGraphStateMachineTransitionNode::Serialise( std::ofstream& rStream, bool isForDist ) const
	{
		Super::Serialise( rStream, isForDist );
		RawSerialisation::WriteObjectChecked( m_OutputNodeID, rStream );
	}

	void AnimGraphStateMachineTransitionNode::Deserialise( FDependentIStream& rStream )
	{
		Super::Deserialise( rStream );
		RawSerialisation::ReadObjectChecked( m_OutputNodeID, rStream );
	}

	NodeEditorTaskBase* AnimGraphStateMachineTransitionNode::ConvertToTask()
	{
//		return NewObject<AnimGraphTransitionTask>();
		return nullptr;
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( AnimGraphStateMachineTransitionNode );
