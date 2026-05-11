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

#pragma once

#include "BehaviourTreeNodeBase.h"

namespace Saturn {

	class NodeEditorTreeNode;

	// Sequence	Node -- Could be thought of as a logical AND operator as it requires all children to succeed.
	// NOTE: Calls to EvaluateNode only check if the Sequence has children, it does not actually do the sequence as that is done in the SequenceTask class
	class BehaviourTreeSequenceNode : public BehaviourTreeNodeBase
	{
		SAT_DECLARE_CLASS( BehaviourTreeSequenceNode, BehaviourTreeNodeBase );
	public:
		BehaviourTreeSequenceNode();
		virtual ~BehaviourTreeSequenceNode();

		void Reset();
		void AddChildren( const std::vector<UUID>& rChildrenID );

		const std::vector<UUID>& GetChildren() const { return m_Children; }

	public:
		//////////////////////////////////////////////////////////////////////////
		// NodeEditorNodeBase
		virtual void Serialise( std::ofstream& rStream ) const override;
		virtual void Deserialise( FDependentIStream& rStream ) override;
		
		//////////////////////////////////////////////////////////////////////////
		// BehaviourTreeNodeBase
		virtual NodeEditorTaskBase* ConvertToTask() override;

#if !defined(SAT_DIST)
		virtual void PostDeserialise() override;
		virtual void RenderDetails() override;

		//////////////////////////////////////////////////////////////////////////
		// NodeEditorNodeBase (non Dist)
		virtual void RenderContextWindow() override;
#endif

	private:
		void CreateNode();

	private:
		std::vector<UUID> m_Children;
	};
	
}
