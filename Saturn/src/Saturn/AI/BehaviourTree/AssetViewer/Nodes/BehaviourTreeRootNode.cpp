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
#include "BehaviourTreeRootNode.h"

#if !defined( SAT_DIST )
#include "Saturn/NodeEditor/UI/NodeEditor.h"
#else
#include "Saturn/NodeEditor/NodeEditorBase.h"
#endif

namespace Saturn {

	BehaviourTreeRootNode::BehaviourTreeRootNode()
		: BehaviourTreeNodeBase( "Root Node" )
	{
		CreateNode();
	}

	void BehaviourTreeRootNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::BehaviourTreeRootNode;

#if !defined(SAT_DIST)
		CanBeDeleted = false;
		Color = ImColor( 48, 128, 255, 100 );
		Type = NodeRenderType::Tree;
#endif

		Outputs.push_back( Ref<Pin>::Create( "Out", PinType::BehaviourTreeCompositeLink, PinKind::Output ) );

		for( auto& rOutput : Outputs )
		{
			rOutput->RenderType = PinRenderType::Tree;
		}
	}

	BehaviourTreeRootNode::~BehaviourTreeRootNode()
	{
	}

	NodeEvaluationState BehaviourTreeRootNode::EvaluateNode( NodeEditorRuntime* pEvaluator )
	{	
		return NodeEvaluationState::Evaluated;
	}

}
