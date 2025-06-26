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

#pragma once

#include "Saturn/NodeEditor/NodeEditorTreeNode.h"
#include "Saturn/AI/BehaviourTree/BehaviourTreeMemorySpecification.h"

namespace Saturn {

	class BehaviourTreeBaseTask;
	class BehaviourTreeNodeEditor;

	// The base class for all nodes in the Behaviour Tree
	// This class exists because it allows us to convert the node to BehaviourTree Tasks
	class BehaviourTreeNodeBase : public NodeEditorTreeNode
	{
		SAT_NODE_EDITOR_NODE_BODY( NodeExecutionType::None );
	public:
		BehaviourTreeNodeBase() = default;
		BehaviourTreeNodeBase( const std::string& rName ) 
			: NodeEditorTreeNode( rName )
		{
		}

		virtual ~BehaviourTreeNodeBase() = default;

		virtual BehaviourTreeBaseTask* ConvertToTask() = 0;
		virtual void PostDeserialise() {}

#if !defined(SAT_DIST)
		virtual void RenderDetails() {}

	public:
		// TODO: Weak Ref #WREF_BehaviourTreeBaseTask
		Ref<BehaviourTreeMemorySpecification> BehaviourTreeMemorySpecification;
#endif
	};
	
}
