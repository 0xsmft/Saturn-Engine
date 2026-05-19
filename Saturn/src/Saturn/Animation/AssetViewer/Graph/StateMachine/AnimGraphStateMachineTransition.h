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

#include "Saturn/NodeEditor/NodeEditorTaskBase.h"

namespace Saturn {

	SCLASS();
	class AnimGraphStateMachineTransitionTask : public NodeEditorTaskBase
	{
		SAT_DECLARE_CLASS( AnimGraphStateMachineTransitionTask, NodeEditorTaskBase );
	public:
		AnimGraphStateMachineTransitionTask();
		virtual ~AnimGraphStateMachineTransitionTask();

#if !defined(SAT_DIST)
		virtual void PreInitialiseTask( NodeEditor* pEditor, NodeEditorNodeBase* pNode ) override;
#endif

		virtual NodeEditorTaskState Tick( Timestep ts ) override;

		virtual void InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther ) override;
		virtual void Reset() override;
		virtual void Serialise( std::ofstream& rStream ) const override;
		virtual void Deserialise( FDependentIStream& rStream ) override;

	public:
		UUID GetSourceID() const { return m_Source; }
		UUID GetDestinationID() const { return m_Destination; }

#if !defined(SAT_DIST)
	private:
		void SortTsNodesAndConvertToTasks( NodeEditor* pEditor, UUID startingID );
#endif

	private:
		// The State that we transition OUT of.
		UUID m_Source = 0;

		// The State that we transition IN TO of.
		UUID m_Destination = 0;

		size_t m_CurrentTaskIndex = 0llu;

		// Tasks need for us to determine if we should transition or not.
		std::vector<Ref<NodeEditorTaskBase>> m_InnerTasks;
	};
	
}
