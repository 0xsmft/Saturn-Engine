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

	struct GraphTaskItem
	{
		// The node that we came from
		UUID NodeID = 0;
		// The task
		NodeEditorTaskBase* pTask = nullptr;
	};

	//
	// A GraphTask holds all the tasks in one sub-graph for the Animation Graph
	// As the Animation Graph is built from sub-graphs
	// TaskHandler
	//  GraphTask
	//   Task
	// A GraphTask will return a state of Running if it's not complete if it is it will then return Completed
	// However, for a state machine graph it may return Completed if we need to transition out of the graph.
	//
	SCLASS( NoExtendedMetadata, NodeEditorNode )
	class SGraphTask : public NodeEditorTaskBase
	{
		SAT_DECLARE_CLASS( SGraphTask, NodeEditorTaskBase );
	public:
		SGraphTask();
		~SGraphTask();

		void AddTask( UUID nodeID, NodeEditorTaskBase* pTask );

	public:
		NodeEditorTaskHandler* GetParentObject() const { return pParentHandler; }
		const std::vector<GraphTaskItem>& GetTasks() const { return m_Tasks; }

	public:
		//////////////////////////////////////////////////////////////////////////
		// NODE EDITOR TASK BASE
#if !defined(SAT_DIST)
		virtual void PreInitialiseTask( NodeEditorBase* pEditor, NodeEditorNodeBase* pNode ) override;
#endif
		virtual void InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther ) override;

		virtual NodeEditorTaskState Tick( Timestep ts ) override;
		virtual void Reset() override;

		virtual void Serialise( std::ofstream& rStream ) const override;
		virtual void Deserialise( std::ifstream& rStream ) override;

	private:
		void ResetTaskData();

		GraphTaskItem* GetCurrentTask();
		bool NextTask();

	private:
		NodeEditorTaskHandler* pParentHandler = nullptr;
		size_t m_CurrentTaskIndex = 0;

		std::vector<GraphTaskItem> m_Tasks;
	};
	
}
