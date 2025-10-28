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

#include "Saturn/AI/BehaviourTree/Conditions/BehaviourTreeCondition.h"

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

	void BehaviourTreeSelectorTask::InitialiseTask( NodeEditorTaskHandler* pHandler, NodeEditorBase* pEditor, NodeEditorNodeBase* pNode )
	{
		BehaviourTreeNodeEditor* pBehaviourTreeNodeEditor = dynamic_cast< BehaviourTreeNodeEditor* >( pEditor );

		BehaviourTreeSelectorNode* pSelectorNode = dynamic_cast< BehaviourTreeSelectorNode* >( pNode );
		if( pSelectorNode )
		{
			for( const auto& rNode : pSelectorNode->GetChildren() )
			{
				m_Children.push_back( pBehaviourTreeNodeEditor->GetTaskFor( rNode ).Get() );
			}

			m_NodeID = pSelectorNode->ID;

			m_pNodeCondition = pSelectorNode->NodeCondition.Get();
			if( m_pNodeCondition )
			{
				m_pNodeCondition->InitialiseTask( pHandler, pEditor, pNode );
			}
		}
	}

	BehaviourTreeSelectorTask::~BehaviourTreeSelectorTask()
	{
		Reset();
	}

	NodeEditorTaskState BehaviourTreeSelectorTask::Tick( Timestep ts )
	{
		// Next, check our condition, if any
		if( m_pNodeCondition )
		{
			if( const auto status = m_pNodeCondition->Tick( ts ); status != NodeEditorTaskState::Completed )
				return status;
		}

		for( auto* pTask : m_Children )
		{
			if( pTask )
			{
				const auto status = pTask->Tick( ts );
				if( status != NodeEditorTaskState::Failed )
				{
					m_CurrentState = status;
					return status;
				}
			}
		}

		m_CurrentState = NodeEditorTaskState::Failed;
		return m_CurrentState;
	}

	//////////////////////////////////////////////////////////////////////////
	// SEQUENCE TASK

	BehaviourTreeSequenceTask::BehaviourTreeSequenceTask()
	{
	}

	void BehaviourTreeSequenceTask::InitialiseTask( NodeEditorTaskHandler* pHandler, NodeEditorBase* pEditor, NodeEditorNodeBase* pNode )
	{
		BehaviourTreeNodeEditor* pBehaviourTreeNodeEditor = dynamic_cast< BehaviourTreeNodeEditor* >( pEditor );

		BehaviourTreeSequenceNode* pSequenceNode = dynamic_cast< BehaviourTreeSequenceNode* >( pNode );
		if( pSequenceNode )
		{
			for( const auto& rNode : pSequenceNode->GetChildren() )
			{
				m_Children.push_back( pBehaviourTreeNodeEditor->GetTaskFor( rNode ).Get() );
			}

			m_NodeID = pSequenceNode->ID;

			m_pNodeCondition = pSequenceNode->NodeCondition.Get();
			if( m_pNodeCondition )
			{
				m_pNodeCondition->InitialiseTask( pHandler, pEditor, pNode );
			}
		}
	}

	BehaviourTreeSequenceTask::~BehaviourTreeSequenceTask()
	{
		Reset();
	}

	NodeEditorTaskState BehaviourTreeSequenceTask::Tick( Timestep ts )
	{
		// All tasks completed, then this sequence is also completed.
		if( m_CurrentTaskIndex >= m_Children.size() )
		{
			m_CurrentState = NodeEditorTaskState::Completed;
			return m_CurrentState;
		}

		// Next, check our condition, if any
		if( m_pNodeCondition )
		{
			if( const auto status = m_pNodeCondition->Tick( ts ); status != NodeEditorTaskState::Completed )
				return status;
		}

		// Get try current task.
		if( m_pCurrentTask == nullptr )
		{
			BehaviourTreeBaseTask* pTask = m_Children[ m_CurrentTaskIndex ];
			if( !pTask )
			{
				Reset();
				return NodeEditorTaskState::Failed;
			}

			m_pCurrentTask = pTask;
		}

		// Tick current task.
		const auto status = m_pCurrentTask->Tick( ts );
		m_CurrentState = status;

		if( status == NodeEditorTaskState::Completed )
		{
			// Try move onto the next task.
			m_pCurrentTask = nullptr;
			++m_CurrentTaskIndex;

			// All tasks completed, then this sequence is also completed.
			if( m_CurrentTaskIndex >= m_Children.size() ) 
			{
				return m_CurrentState;
			}
			else
			{
				// If not, then we are still running, we still have more to do.
				m_CurrentState = NodeEditorTaskState::Running;
			}
		}
		else if( status == NodeEditorTaskState::Failed )
		{
			m_pCurrentTask = nullptr;
			Reset();

			return m_CurrentState;
		}

		// Still waiting for current task to complete.
		return m_CurrentState;
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( BehaviourTreeCompositeBaseTask );
SAT_X31_CREATE_AUTO_REG( BehaviourTreeSelectorTask );
SAT_X31_CREATE_AUTO_REG( BehaviourTreeSequenceTask );
