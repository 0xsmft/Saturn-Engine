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
#include "GraphTask.h"

namespace Saturn {

	SGraphTask::SGraphTask()
	{
	}

	SGraphTask::~SGraphTask()
	{
	}

	void SGraphTask::AddTask( NodeEditorTaskBase* pTask )
	{
		m_Tasks.push_back( pTask );
	}

	void SGraphTask::InitialiseTask( NodeEditorTaskHandler* pHandler, NodeEditorBase* pEditor, NodeEditorNodeBase* pNode )
	{
		pParentHandler = pHandler;
	}

	NodeEditorTaskState SGraphTask::Tick( Timestep ts )
	{
		NodeEditorTaskState activeState = NodeEditorTaskState::Unknown;

		if( m_pCurrentTask )
		{
			const auto status = m_pCurrentTask->Tick( ts );
			switch( status )
			{
				case NodeEditorTaskState::Completed:
				{
					activeState = NodeEditorTaskState::Running;
					m_pCurrentTask = nullptr;
				} break;

				case NodeEditorTaskState::Running:
				{
					activeState = NodeEditorTaskState::Running;
				} break;

				default:
					break;
			}
		}

		if( m_pCurrentTask == nullptr )
		{
			if( m_CurrentTaskIndex + 1 > m_Tasks.size() )
			{
				// At end, restart from the root.
				Reset();
				activeState = NodeEditorTaskState::Completed;
			}
			else
			{
				m_pCurrentTask = m_Tasks.at( m_CurrentTaskIndex++ );
			}
		}

		return activeState;
	}

	void SGraphTask::Reset()
	{
		m_CurrentTaskIndex = 0;
		m_pCurrentTask = nullptr;
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( SGraphTask );
