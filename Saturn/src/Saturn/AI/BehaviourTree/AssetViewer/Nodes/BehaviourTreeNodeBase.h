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
#include "Saturn/AI/BehaviourTree/Conditions/BehaviourTreeCondition.h"

namespace Saturn {

	class BehaviourTreeBaseTask;
	class BehaviourTreeNodeEditor;

	// The base class for all nodes in the Behaviour Tree
	// This class exists because it allows us to convert the node to BehaviourTree Tasks
	class BehaviourTreeNodeBase : public NodeEditorTreeNode
	{
		// NOTE: SAT_DECLARE_CLASS expanded
	private:
		NodeEditorNodeBase& operator=( NodeEditorNodeBase&& );
		NodeEditorNodeBase& operator=( const NodeEditorNodeBase& );
		static SClass* GetStaticClassInternal();

	public:
		inline static [[nodiscard]] SClass* StaticClass()
		{
			return GetStaticClassInternal();
		}
	public:
		typedef NodeEditorNodeBase ThisClass;
		typedef NodeEditorTreeNode Super;

	public:
		BehaviourTreeNodeBase() = default;
		BehaviourTreeNodeBase( const std::string& rName )
			: NodeEditorTreeNode( rName )
		{
		}

		virtual ~BehaviourTreeNodeBase() = default;

		virtual void PostDeserialise() {}

#if !defined(SAT_DIST)
	public:
		virtual void RenderDetails() {}
		virtual void OnRenderNextSection() override final;
#endif

	public:
		// TODO: Weak Ref #ReplaceRawPtrOrRefWithWeakRef
		Ref<BehaviourTreeCondition> NodeCondition;

	protected:
		[[nodiscard]] BehaviourTreeNodeEditor* GetParentAsBTNodeEditor();
	};
	
}
