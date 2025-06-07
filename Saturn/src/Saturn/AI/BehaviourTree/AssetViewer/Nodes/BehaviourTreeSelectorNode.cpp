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
#include "BehaviourTreeSelectorNode.h"

#include "Saturn/AI/BehaviourTree/Tasks/BehaviourTreeCompositeTasks.h"

#include "Saturn/NodeEditor/NodeEditorBase.h"
#include "Saturn/AI/BehaviourTree/AssetViewer/BehaviourTreeEditorEvaluator.h"

namespace Saturn {

	BehaviourTreeSelectorNode::BehaviourTreeSelectorNode()
		: BehaviourTreeNodeBase( "Selector" )
	{
		CreateNode();
	}

	void BehaviourTreeSelectorNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::BehaviourTreeSelectorNode;

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

	BehaviourTreeSelectorNode::~BehaviourTreeSelectorNode()
	{
		Reset();
	}

	NodeEvaluationState BehaviourTreeSelectorNode::EvaluateNode( NodeEditorRuntime* pEvaluator )
	{
		BehaviourTreeEditorEvaluator* pBehaviourEvaluator = dynamic_cast< BehaviourTreeEditorEvaluator* >( pEvaluator );
		if( !pBehaviourEvaluator )
			return NodeEvaluationState::Failed;

		Ref<NodeEditorBase> base = pBehaviourEvaluator->GetTargetNodeEditor();
		if( !base )
			return NodeEvaluationState::Failed;

		for( auto& rNodeID : m_Children )
		{
			Ref<NodeEditorTreeNode> treeNode = base->FindNode( rNodeID ).As<NodeEditorTreeNode>();
			if( treeNode )
			{
				auto status = treeNode->EvaluateNode( pEvaluator );
				if( status != NodeEvaluationState::Failed )
				{
					return status;
				}
			}
		}

		return NodeEvaluationState::Failed;
	}

	void BehaviourTreeSelectorNode::OnSerialise( std::ofstream& rStream ) const
	{
		RawSerialisation::WriteVector( m_Children, rStream );
	}

	void BehaviourTreeSelectorNode::OnDeserialise( IStream& rStream )
	{
		RawSerialisation::ReadVector( m_Children, rStream );
	}

	BehaviourTreeBaseTask* BehaviourTreeSelectorNode::ConvertToTask()
	{
		return new BehaviourTreeSelectorTask();
	}

	void BehaviourTreeSelectorNode::AddChildren( const std::vector<UUID>& rChildrenID )
	{
		for( auto& rID : rChildrenID )
		{
			m_Children.emplace_back( rID );
		}
	}

	void BehaviourTreeSelectorNode::Reset()
	{
		m_Children.clear();
		m_CurrentNode = nullptr;
		m_CurrentNodeID = 0;
	}

}
