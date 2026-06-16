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
#include "BehaviourTreeRootNode.h"

#include "Saturn/AI/BehaviourTree/Tasks/BehaviourTreeBlackboardTask.h"

#include "Saturn/NodeEditor/UI/NodeEditor.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

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
		Flags |= NodeFlags_Irremovable | NodeFlags_RejectCopyPaste;
		Color = ImColor( 48, 128, 255, 100 );
		RenderType = NodeRenderType::Tree;
#endif

		Outputs.emplace_back( Ref<Pin>::Create( "Out", PinType::Flow, PinKind::Output ) );

		for( auto& rOutput : Outputs )
		{
			rOutput->RenderType = PinRenderType::Tree;
		}
	}

	BehaviourTreeRootNode::~BehaviourTreeRootNode()
	{
	}

	NodeEditorTaskBase* BehaviourTreeRootNode::ConvertToTask()
	{
		return NewObject<BehaviourTreeBlackboardTask>( GetParentObject() );
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( BehaviourTreeRootNode );
