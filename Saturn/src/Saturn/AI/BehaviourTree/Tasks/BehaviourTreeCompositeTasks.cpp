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
#include "BehaviourTreeCompositeTasks.h"

#include "Saturn/AI/AIAgentEntity.h"
#include "Saturn/AI/BehaviourTree/AssetViewer/BehaviourTreeNodeEditor.h"

#include "Saturn/AI/BehaviourTree/AssetViewer/Nodes/BehaviourTreeSelectorNode.h"
#include "Saturn/AI/BehaviourTree/AssetViewer/Nodes/BehaviourTreeSequenceNode.h"

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// COMPOSITE BASE TASK

	void BehaviourTreeCompositeBaseTask::Reset()
	{
		m_pCurrentTask = nullptr;
		m_CurrentTaskIndex = 0;
	}

	//////////////////////////////////////////////////////////////////////////
	// SELECTOR TASK

	BehaviourTreeSelectorTask::BehaviourTreeSelectorTask()
	{
	}

	void BehaviourTreeSelectorTask::InitialiseTask( BehaviourTreeNodeEditor* pEditor, BehaviourTreeNodeBase* pNode )
	{
		BehaviourTreeNodeEditor* pBehaviourTreeNodeEditor = dynamic_cast< BehaviourTreeNodeEditor* >( pEditor );

		BehaviourTreeSelectorNode* pSelectorNode = dynamic_cast< BehaviourTreeSelectorNode* >( pNode );
		if( pSelectorNode )
		{
			for( const auto& rNode : pSelectorNode->GetChildren() )
			{
				m_Children.push_back( pBehaviourTreeNodeEditor->GetTaskFor( rNode ) );
			}

			m_NodeID = pSelectorNode->ID;
		}
	}

	BehaviourTreeSelectorTask::~BehaviourTreeSelectorTask()
	{
		Reset();
	}

	BehaviourTreeTaskState BehaviourTreeSelectorTask::Tick( Timestep ts )
	{
		for( auto* pTask : m_Children )
		{
			if( pTask )
			{
				auto status = pTask->Tick( ts );
				if( status != BehaviourTreeTaskState::Failed )
				{
					m_CurrentState = status;
					return status;
				}
			}
		}

		m_CurrentState = BehaviourTreeTaskState::Failed;
		return m_CurrentState;
	}

	//////////////////////////////////////////////////////////////////////////
	// SEQUENCE TASK

	BehaviourTreeSequenceTask::BehaviourTreeSequenceTask()
	{
	}

	void BehaviourTreeSequenceTask::InitialiseTask( BehaviourTreeNodeEditor* pEditor, BehaviourTreeNodeBase* pNode )
	{
		BehaviourTreeNodeEditor* pBehaviourTreeNodeEditor = dynamic_cast< BehaviourTreeNodeEditor* >( pEditor );

		BehaviourTreeSequenceNode* pSelectorNode = dynamic_cast< BehaviourTreeSequenceNode* >( pNode );
		if( pSelectorNode )
		{
			for( const auto& rNode : pSelectorNode->GetChildren() )
			{
				m_Children.push_back( pBehaviourTreeNodeEditor->GetTaskFor( rNode ) );
			}

			m_NodeID = pSelectorNode->ID;
		}
	}

	BehaviourTreeSequenceTask::~BehaviourTreeSequenceTask()
	{
		Reset();
	}

	BehaviourTreeTaskState BehaviourTreeSequenceTask::Tick( Timestep ts )
	{
		// All tasks completed, then this sequence is also completed.
		if( m_CurrentTaskIndex >= m_Children.size() )
		{
			m_CurrentState = BehaviourTreeTaskState::Completed;
			return m_CurrentState;
		}

		// Get try current task.
		if( m_pCurrentTask == nullptr )
		{
			BehaviourTreeBaseTask* pTask = m_Children[ m_CurrentTaskIndex ];
			if( !pTask )
			{
				Reset();
				return BehaviourTreeTaskState::Failed;
			}

			m_pCurrentTask = pTask;
		}

		// Tick current task.
		auto status = m_pCurrentTask->Tick( ts );
		m_CurrentState = status;

		if( status == BehaviourTreeTaskState::Completed )
		{
			// Try move onto the next task.
			m_pCurrentTask = nullptr;
			m_CurrentTaskIndex++;

			// All tasks completed, then this sequence is also completed.
			if( m_CurrentTaskIndex >= m_Children.size() ) 
			{
				return m_CurrentState;
			}
			else
			{
				// If not, then we are still running, we still have more to do.
				m_CurrentState = BehaviourTreeTaskState::Running;
			}
		}
		else if( status == BehaviourTreeTaskState::Failed )
		{
			m_pCurrentTask = nullptr;
			Reset();

			return m_CurrentState;
		}

		// Still waiting for current task to complete.
		return m_CurrentState;
	}

}
