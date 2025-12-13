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
#include "AnimGraphStateMachineStateNode.h"

#include "StateMachineStateNodeLibrary.h"
#include "AnimGraphStateMachineOutNode.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

#include "Saturn/Animation/AssetViewer/Graph/Animation/AnimGraph.h"
#include "Saturn/Animation/AssetViewer/Graph/Tasks/AnimGraphStateMachineStateTask.h"

namespace Saturn {

	AnimGraphStateMachineStateNode::AnimGraphStateMachineStateNode()
		: Super( "New State" )
	{
		CreateNode();
	}

	AnimGraphStateMachineStateNode::AnimGraphStateMachineStateNode( const std::string& rName )
		: Super( rName )
	{
		CreateNode();
	}

	void AnimGraphStateMachineStateNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::AnimGraphStateMachineStateNode;

		Outputs.push_back( Ref<Pin>::Create( "Out", PinType::Flow, PinKind::Output ) );
		Inputs.push_back( Ref<Pin>::Create( "In", PinType::Flow, PinKind::Input ) );

		for( auto& rOutput : Outputs )
		{
			rOutput->AcceptMultipleLinks = true;
		}

		for( auto& rInput : Inputs )
		{
			rInput->AcceptMultipleLinks = true;
		}

#if !defined(SAT_DIST)
		Color = ImColor( 48, 128, 255, 100 );
		// Render type doesn't matter here...
		RenderType = NodeRenderType::Blueprint;
#endif
	}

	AnimGraphStateMachineStateNode::~AnimGraphStateMachineStateNode()
	{
	}

	void AnimGraphStateMachineStateNode::Serialise( std::ofstream& rStream, bool isForDist ) const
	{
		Super::Serialise( rStream, isForDist );
		RawSerialisation::WriteObjectChecked( m_OutputNodeID, rStream );
	}

	void AnimGraphStateMachineStateNode::Deserialise( FDependentIStream& rStream )
	{
		Super::Deserialise( rStream );
		RawSerialisation::ReadObjectChecked( m_OutputNodeID, rStream );
	}

	NodeEditorTaskBase* AnimGraphStateMachineStateNode::ConvertToTask()
	{
//		return NewObject<AnimGraphStateMachineStateTask>();
		return nullptr;
	}

	void AnimGraphStateMachineStateNode::Render( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder )
	{
#if !defined(SAT_DIST)
		ImVec4 normalisedColor = Color;
		normalisedColor.x /= 2.0f;
		normalisedColor.y /= 2.0f;
		normalisedColor.z /= 2.0f;

		ed::PushStyleColor( ed::StyleColor_NodeBg, normalisedColor );

		rBuilder.Begin( ed::NodeId( ID ) );
		rBuilder.Middle();

		ImGui::BeginHorizontal( "##state" );

		// TODO: Much better to store the ID rather than the Node itself!
		auto* AG = dynamic_cast< AnimGraph* >( pOuter );
		if( AG )
		{
			const auto entryNode = AG->GetEntryNode();
			if( ImGui::RadioButton( "##markasentry", entryNode == SharedFromThis() ) )
			{
				// TODO: Entry node cannot be deleted, if the user wants to delete the entry node they must add new state and mark that as the entry.
//				CanBeDeleted = false;
				AG->MarkNodeAsEntry( SharedFromThis() );
			}
		}

		const ImVec2 textSize = ImGui::CalcTextSize( Name.c_str() );
		ImGui::SetNextItemWidth( textSize.x );
		ImGui::TextUnformatted( Name.c_str() );

		ImGui::EndHorizontal();
		rBuilder.End();

		ed::PopStyleColor();
#endif
	}

	void AnimGraphStateMachineStateNode::PostPlace()
	{
		// Spawn output node
		auto outNode = StateMachineStateNodeLibrary::SpawnOutputNode( pOuter->SharedFromThis() );
		outNode->pParentObject = this;
		m_OutputNodeID = outNode->ID;
	}

	NodeEvaluationState AnimGraphStateMachineStateNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		return NodeEvaluationState::Failed;
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( AnimGraphStateMachineStateNode );
