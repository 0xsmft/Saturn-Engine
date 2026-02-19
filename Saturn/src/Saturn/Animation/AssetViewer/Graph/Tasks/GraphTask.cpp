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
#include "GraphTask.h"

#include "AnimGraphTransitionTasks.h"

#include "Saturn/NodeEditor/NodeEditorBase.h"

namespace Saturn {

	SGraphTask::SGraphTask()
	{
	}

	SGraphTask::~SGraphTask()
	{
		for( auto& rItem : m_Tasks )
		{
			delete rItem.pTask;
		}
	}

	void SGraphTask::AddTask( UUID nodeID, NodeEditorTaskBase* pTask )
	{
		m_Tasks.emplace_back( nodeID, pTask );
	}

	void SGraphTask::InitialiseTask( NodeEditorTaskHandler* pHandler, NodeEditorBase* pEditor, NodeEditorNodeBase* pNode )
	{
		pParentHandler = pHandler;

		std::reverse( m_Tasks.begin(), m_Tasks.end() );

		for( auto& rItem : m_Tasks )
		{
			if( rItem.pTask )
			{
				auto node = pEditor->FindNode( rItem.NodeID );
				rItem.pTask->InitialiseTask( pHandler, pEditor, node.Get() );
			}
		}
	}

	bool SGraphTask::NextTask()
	{
		if( m_CurrentTaskIndex + 1 > m_Tasks.size() )
		{
			// At end, restart from the root.
			ResetTaskData();
			return true;
		}
		else
		{
			++m_CurrentTaskIndex;
		}

		return false;
	}

	NodeEditorTaskState SGraphTask::Tick( Timestep ts )
	{
		auto* pCurrentTask = GetCurrentTask();
		if( !pCurrentTask || !pCurrentTask->pTask )
		{
			// If NextTask returns true we are at the end
			if( NextTask() ) 
				return NodeEditorTaskState::Completed;
			else
				return NodeEditorTaskState::Running;
		}

		NodeEditorTaskState activeState = NodeEditorTaskState::Unknown;
		if( pCurrentTask->pTask )
		{
			const auto status = pCurrentTask->pTask->Tick( ts );

			switch( status )
			{
				case NodeEditorTaskState::Completed:
				{
					activeState = NodeEditorTaskState::Running;
					NextTask();
				} break;

				case NodeEditorTaskState::Running:
				{
					// If a task is "running", we must restart this graph so it can update the result of the node that cannot continue.
					// We don't call ResetTaskData() here because there is no point re-evaulating tasks that have not changed, we just want to start from the first node.
					Reset();
					activeState = NodeEditorTaskState::Running;
				} break;

				default: break;
			}

			if( pCurrentTask->pTask->GetClass() == AnimGraphTransitionResultTask::StaticClass() )
			{
				// Check if we should transition out.
				const AnimGraphTransitionResultTask* pTransitionRT = ( AnimGraphTransitionResultTask* )pCurrentTask->pTask;
				if( pTransitionRT )
				{
					if( pTransitionRT->GetResult() )
					{
#if defined(SAT_DEBUG)
						SAT_CORE_INFO( "Transition says we should change..." );
#endif

						// Transition says we should change...
						ResetTaskData();
						return NodeEditorTaskState::Completed;
					}
				}
			}
		}

		return activeState;
	}

	void SGraphTask::Reset()
	{
		m_CurrentTaskIndex = 0;
	}

	void SGraphTask::ResetTaskData()
	{
		for( auto& rTaskItem : m_Tasks )
		{
			if( rTaskItem.pTask )
				rTaskItem.pTask->Reset();
		}

		Reset();
	}

	GraphTaskItem* SGraphTask::GetCurrentTask()
	{
		if( m_CurrentTaskIndex < m_Tasks.size() )
			return &m_Tasks[ m_CurrentTaskIndex ];

		return nullptr;
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( SGraphTask );
