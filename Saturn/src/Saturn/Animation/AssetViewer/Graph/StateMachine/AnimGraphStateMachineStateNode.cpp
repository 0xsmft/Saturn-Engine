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
		: AnimGraphStateMachineNodeBase( "<NULL STATE MACHINE STATE>" )
	{
		CreateNode();
	}

	AnimGraphStateMachineStateNode::AnimGraphStateMachineStateNode( const std::string& rName )
		: AnimGraphStateMachineNodeBase( rName )
	{
		CreateNode();
	}

	void AnimGraphStateMachineStateNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::AnimGraphStateMachineStateNode;

		Outputs.push_back( Ref<Pin>::Create( "DATA-TO", PinType::Flow, PinKind::Output ) );
		Inputs.push_back( Ref<Pin>::Create( "DATA-FROM", PinType::Flow, PinKind::Input ) );

		for( auto& rOutput : Outputs )
		{
			rOutput->RenderType = PinRenderType::Tree;
			rOutput->AcceptMultipleLinks = true;
		}

		for( auto& rInput : Inputs )
		{
			rInput->RenderType = PinRenderType::Tree;
			rInput->AcceptMultipleLinks = true;
		}

#if !defined(SAT_DIST)
		Color = ImColor( 48, 128, 255, 100 );
		RenderType = NodeRenderType::Tree;
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
	void AnimGraphStateMachineStateNode::PostPlace()
	{
		// Spawn output node
		auto outNode = StateMachineStateNodeLibrary::SpawnOutputNode( pOuter->SharedFromThis() );
		outNode->pParentObject = this;
		m_OutputNodeID = outNode->ID;
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( AnimGraphStateMachineStateNode );
