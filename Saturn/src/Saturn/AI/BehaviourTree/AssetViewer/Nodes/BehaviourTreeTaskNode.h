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

	//
	// General Task Node
	//
	SCLASS()
	class BehaviourTreeTaskNode : public BehaviourTreeNodeBase
	{
		SAT_DECLARE_CLASS( BehaviourTreeTaskNode, BehaviourTreeNodeBase );
	public:
		// NOTE: INTERNAL, FOR USE WHEN DESERIALSING NODE EDITOR!
		BehaviourTreeTaskNode();

		BehaviourTreeTaskNode( BehaviourTreeBaseTask* pTaskInstance );
		virtual ~BehaviourTreeTaskNode();

	public:
		//////////////////////////////////////////////////////////////////////////
		// NodeEditorNodeBase

		virtual void Serialise( std::ofstream& rStream ) const override;
		virtual void Deserialise( FDependentIStream& rStream ) override;

	public:
		//////////////////////////////////////////////////////////////////////////
		// BehaviourTreeNodeBase

		virtual NodeEditorTaskBase* ConvertToTask() override { return m_TaskInstance.Get(); }
		virtual void PostDeserialise() override;

#if !defined(SAT_DIST)
		virtual void RenderDetails() override;

	public:
		//////////////////////////////////////////////////////////////////////////
		// NodeEditorTreeNode

		virtual void OnRenderExtra() override;
#endif
		void SetTaskInstance( BehaviourTreeBaseTask* pTaskInstance );

	public:
		inline Ref<BehaviourTreeBaseTask> GetTaskInstance() const { return m_TaskInstance; }

	private:
		void CreateNode();

	private:
		Ref<BehaviourTreeBaseTask> m_TaskInstance;
		UUID m_MemoryVariableID = 0;

#if !defined(SAT_DIST)
		// Only when in Editor, used for selecting a memory variable
		Ref<BehaviourTreeMemoryKeySpec> m_MemVariable;
#endif
	};
		
}
