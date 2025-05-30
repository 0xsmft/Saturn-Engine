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
#include "BehaviourTreeNodeLibrary.h"

#include "Nodes/BehaviourTreeRootNode.h"
#include "Nodes/BehaviourTreeSelectorNode.h"
#include "Nodes/BehaviourTreeSequenceNode.h"

#include "Saturn/NodeEditor/GlobalNodesList.h"

namespace Saturn {

	void BehaviourTreeNodeLibrary::RegisterAllNodes()
	{
		GlobalNodesList::RegisterLibrary( {
			{ NodeExecutionType::BehaviourTreeRootNode,     BehaviourTreeNodeLibrary::SpawnRootNode     },
			{ NodeExecutionType::BehaviourTreeSelectorNode, BehaviourTreeNodeLibrary::SpawnSelectorNode },
			{ NodeExecutionType::BehaviourTreeSequenceNode, BehaviourTreeNodeLibrary::SpawnSequenceNode },
		} );
	}

	Ref<BehaviourTreeSelectorNode> BehaviourTreeNodeLibrary::SpawnSelectorNode( Ref<NodeEditorBase> nodeEditor )
	{
		Ref<BehaviourTreeSelectorNode> node = Ref<BehaviourTreeSelectorNode>::Create();
		nodeEditor->AddNode( node );

		return node;
	}

	Ref<BehaviourTreeSequenceNode> BehaviourTreeNodeLibrary::SpawnSequenceNode( Ref<NodeEditorBase> nodeEditor )
	{
		Ref<BehaviourTreeSequenceNode> node = Ref<BehaviourTreeSequenceNode>::Create();
		nodeEditor->AddNode( node );

		return node;
	}

	Ref<BehaviourTreeRootNode> BehaviourTreeNodeLibrary::SpawnRootNode( Ref<NodeEditorBase> nodeEditor )
	{
		Ref<BehaviourTreeRootNode> node = Ref<BehaviourTreeRootNode>::Create();
		nodeEditor->AddNode( node );

		return node;
	}

}
