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

#include "AnimGraphStateMachineTransition.h"

namespace Saturn {

	//
	// AnimGraphStateMachineState
	// 
	// Represents a single state in a state machine.
	// 
	// This task keeps hold of its inner task (i.e. the tasks when you double click into the node)
	// and a list of the transitions out into different states.
	//
	SCLASS()
	class AnimGraphStateMachineState : public NodeEditorTaskBase
	{
		SAT_DECLARE_CLASS( AnimGraphStateMachineState, NodeEditorTaskBase );
	public:
		AnimGraphStateMachineState();
		virtual ~AnimGraphStateMachineState();

#if !defined(SAT_DIST)
		virtual void PreInitialiseTask( NodeEditor* pEditor, NodeEditorNodeBase* pNode ) override;
#endif

		virtual void InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther ) override;

		virtual	NodeEditorTaskState Tick( Timestep ts ) override;

		virtual void Reset() override;

		virtual void Serialise( std::ofstream& rStream ) const override;
		virtual void Deserialise( FDependentIStream& rStream ) override;

	public:
		UUID GetNextState() const { return m_NextStateToTransitionOutTo; }

#if !defined(SAT_DIST)
	private:
		void SortAnimStateNodesAndConvertToTasks( NodeEditor* pEditor, UUID startingID );
#endif

	private:
		// Animation
		std::vector<Ref<NodeEditorTaskBase>> m_InnerTasks;
		
		// Transitions out
		std::vector<Ref<AnimGraphStateMachineTransitionTask>> m_Transitions;
	
		UUID m_NextStateToTransitionOutTo = 0llu;
	};

}
