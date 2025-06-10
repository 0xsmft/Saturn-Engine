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
#include "BehaviourTreeWaitTask.h"

#include "Saturn/AI/BehaviourTree/AssetViewer/Nodes/BehaviourTreeNodeBase.h"

namespace Saturn {

	BehaviourTreeWaitTask::BehaviourTreeWaitTask( float WaitDuration )
		: m_WaitDuration( WaitDuration )
	{
	}

	void BehaviourTreeWaitTask::InitialiseTask( BehaviourTreeNodeEditor* pEditor, BehaviourTreeNodeBase* pNode )
	{
		m_NodeID = pNode->ID;
	}

	BehaviourTreeWaitTask::~BehaviourTreeWaitTask()
	{
	}

	BehaviourTreeTaskState BehaviourTreeWaitTask::Tick( Timestep ts )
	{
		if( !m_Started )
		{
			m_StartTime = std::chrono::steady_clock::now();
			m_Started = true;

			SAT_CORE_INFO( "[BehaviourTreeWaitTask] Wait started, will be awaiting for: {0} seconds", m_WaitDuration );

			m_CurrentState = BehaviourTreeTaskState::Starting;
			return m_CurrentState;
		}

		auto now = std::chrono::steady_clock::now();
		std::chrono::duration<float> elasped = now - m_StartTime;

		if( elasped.count() >= m_WaitDuration )
		{
			m_Started = false;
			SAT_CORE_INFO( "[BehaviourTreeWaitTask] Wait ended" );

			m_CurrentState = BehaviourTreeTaskState::Completed;
			return m_CurrentState;
		}

		// Still waiting
		m_CurrentState = BehaviourTreeTaskState::Running;
		return m_CurrentState;
	}

	void BehaviourTreeWaitTask::Reset()
	{
		m_Started = false;
	}

}
