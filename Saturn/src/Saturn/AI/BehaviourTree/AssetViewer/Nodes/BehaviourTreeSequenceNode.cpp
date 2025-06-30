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
#include "BehaviourTreeSequenceNode.h"

#include "Saturn/NodeEditor/NodeEditorBase.h"

#include "Saturn/AI/BehaviourTree/Tasks/BehaviourTreeCompositeTasks.h"
#include "Saturn/AI/BehaviourTree/AssetViewer/BehaviourTreeEditorEvaluator.h"

namespace Saturn {

	BehaviourTreeSequenceNode::BehaviourTreeSequenceNode()
		: BehaviourTreeNodeBase( "Sequence" )
	{
		CreateNode();
	}

	void BehaviourTreeSequenceNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::BehaviourTreeSequenceNode;

#if !defined(SAT_DIST)
		Color = ImColor( 48, 128, 255, 100 );
		Type = NodeRenderType::Tree;
#endif

		Inputs.push_back( Ref<Pin>::Create( "In", PinType::BehaviourTreeCompositeLink, PinKind::Input ) );
		Outputs.push_back( Ref<Pin>::Create( "Out", PinType::Flow, PinKind::Output ) );

		for( auto& rOutput : Outputs )
		{
			rOutput->RenderType = PinRenderType::Tree;
		}

		for( auto& rInput : Inputs )
		{
			rInput->RenderType = PinRenderType::Tree;
		}
	}

	BehaviourTreeSequenceNode::~BehaviourTreeSequenceNode()
	{
	}

	NodeEvaluationState BehaviourTreeSequenceNode::EvaluateNode( NodeEditorRuntime* pEvaluator )
	{
		return m_Children.empty() ? NodeEvaluationState::Failed : NodeEvaluationState::Evaluated;
	}

	void BehaviourTreeSequenceNode::OnSerialise( std::ofstream& rStream ) const
	{
		RawSerialisation::WriteVector( m_Children, rStream );
	}

	void BehaviourTreeSequenceNode::OnDeserialise( IStream& rStream )
	{
		RawSerialisation::ReadVector( m_Children, rStream );
	}

	BehaviourTreeBaseTask* BehaviourTreeSequenceNode::ConvertToTask()
	{
		return new BehaviourTreeSequenceTask();
	}

	void BehaviourTreeSequenceNode::Reset()
	{
		m_Children.clear();
	}

	void BehaviourTreeSequenceNode::AddChildren( const std::vector<UUID>& rChildrenID )
	{
		for( auto& rID : rChildrenID )
		{
			m_Children.emplace_back( rID );
		}
	}

	void BehaviourTreeSequenceNode::Reset()
	{
		m_Children.clear();
		m_CurrentNode = nullptr;
		m_CurrentNodeID = 0;
	}

}
