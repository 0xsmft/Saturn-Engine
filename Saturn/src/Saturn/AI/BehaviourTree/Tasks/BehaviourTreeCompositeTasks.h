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

#include "BehaviourTreeBaseTask.h"

#include <chrono>

namespace Saturn {

	class BehaviourTreeCondition;

	// The base class for all composite tasks
	SCLASS()
	class BehaviourTreeCompositeBaseTask : public BehaviourTreeBaseTask
	{
		SAT_DECLARE_CLASS_MOVE( BehaviourTreeCompositeBaseTask, BehaviourTreeBaseTask )
	public:
		virtual void Reset() override;

	public:
#if !defined(SAT_DIST)
		// Composite tasks aren't normal tasks and thus cannot be created from the right click context menu (and would be a TaskNode) instead they are BehaviourTreeSelectorNode/SBehaviourTreeSequenceNode
		[[nodiscard]] virtual bool IsSpawnableNode() const override final { return false; }
		virtual const char* GetTaskName() const override final { return ""; }
#endif

	public:
		virtual void InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther ) override;

	public:
		virtual void Serialise( std::ofstream& rStream ) const override;
		virtual void Deserialise( FDependentIStream& rStream ) override;

	protected:
		size_t m_CurrentTaskIndex = 0;
		BehaviourTreeBaseTask* m_pCurrentTask = nullptr;
		std::vector<BehaviourTreeBaseTask*> m_Children;

		BehaviourTreeCondition* m_pNodeCondition = nullptr;
	};

	// Selector
	// Find the first child that returns Completed, if none are found return Failed.
	// Could be thought of as a logical OR operator as it finds the branch that does not fail.
	// Though not atomic tasks, composite nodes like Selectors implement the same API as BehaviourTreeBaseTask.
	SCLASS()
	class BehaviourTreeSelectorTask : public BehaviourTreeCompositeBaseTask
	{
		SAT_DECLARE_CLASS_MOVE( BehaviourTreeSelectorTask, BehaviourTreeCompositeBaseTask )
	public:
		BehaviourTreeSelectorTask();
		virtual ~BehaviourTreeSelectorTask();

#if !defined(SAT_DIST)
		virtual void PreInitialiseTask( NodeEditor* pEditor, NodeEditorNodeBase* pNode ) override;
#endif
		virtual void InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther ) override;

	public:
		virtual NodeEditorTaskState Tick( Timestep ts ) override;
	};

	// Sequence 
	// Run all children and as soon as one task returns Failed the sequence will break and return failed.
	// If they all succeed the sequence will return Completed.
	// Could be thought of as a logical AND operator as it requires all children to succeed.
	// Though not atomic tasks, composite nodes like Sequence implement the same API as BehaviourTreeBaseTask.
	SCLASS()
	class BehaviourTreeSequenceTask : public BehaviourTreeCompositeBaseTask
	{
		SAT_DECLARE_CLASS_MOVE( BehaviourTreeSequenceTask, BehaviourTreeCompositeBaseTask )
	public:
		BehaviourTreeSequenceTask();
		virtual ~BehaviourTreeSequenceTask();

#if !defined(SAT_DIST)
		virtual void PreInitialiseTask( NodeEditor* pEditor, NodeEditorNodeBase* pNode ) override;
#endif
		virtual void InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther ) override;

	public:
		virtual NodeEditorTaskState Tick( Timestep ts ) override;
	};

}
