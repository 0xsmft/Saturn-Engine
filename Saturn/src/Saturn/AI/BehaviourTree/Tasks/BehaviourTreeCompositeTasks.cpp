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
#include "BehaviourTreeCompositeTasks.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

#include "Saturn/AI/BehaviourTree/Conditions/BehaviourTreeCondition.h"

#include "Saturn/AI/AIAgentEntity.h"

#if !defined(SAT_DIST)
#include "Saturn/AI/BehaviourTree/AssetViewer/BehaviourTreeNodeEditor.h"

#include "Saturn/AI/BehaviourTree/AssetViewer/Nodes/BehaviourTreeSelectorNode.h"
#include "Saturn/AI/BehaviourTree/AssetViewer/Nodes/BehaviourTreeSequenceNode.h"

#include "Saturn/NodeEditor/Debugging/NodeBreakPointManager.h"
#endif

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// COMPOSITE BASE TASK

	void BehaviourTreeCompositeBaseTask::Reset()
	{
		m_pCurrentTask = nullptr;
		m_CurrentTaskIndex = 0;
	}
	
	void BehaviourTreeCompositeBaseTask::InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther )
	{
		Super::InitialiseTaskWithOther( pHandler, pOther );

		// Get the seq node.
		BehaviourTreeCompositeBaseTask* pCompositeBaseTask = dynamic_cast< BehaviourTreeCompositeBaseTask* >( pOther );
		if( pCompositeBaseTask )
		{
			// Create local copy of children from the template node.
			m_Children.reserve( pCompositeBaseTask->m_Children.size() );

			for( const auto& rTask : pCompositeBaseTask->m_Children )
			{
				auto* pChildTask = ( BehaviourTreeBaseTask* ) ClassMetadataHandler::Get().CreateClassObject( rTask->GetClass() );

				pChildTask->InitialiseTaskWithOther( pHandler, ( NodeEditorTaskBase* ) rTask.Get() );

				// NB: Converted to Ref!
				m_Children.push_back( pChildTask );
			}
		}
	}

	void BehaviourTreeCompositeBaseTask::Serialise( std::ofstream& rStream ) const
	{
		Super::Serialise( rStream );

		RawSerialisation::WriteObject( m_Children.size(), rStream );

		for( const auto& rChildTask : m_Children )
		{
			RawSerialisation::WriteObject( rChildTask->GetClass()->GetHash(), rStream );

			rChildTask->Serialise( rStream );
		}
	}

	void BehaviourTreeCompositeBaseTask::Deserialise( FDependentIStream& rStream )
	{
		Super::Deserialise( rStream );

		size_t childrenCount = 0llu;
		RawSerialisation::ReadObject( childrenCount, rStream );

		for( size_t i = 0; i < childrenCount; ++i )
		{
			uint64_t classHash = 0llu;
			RawSerialisation::ReadObject( classHash, rStream );

			BehaviourTreeBaseTask* pBaseObject = ( BehaviourTreeBaseTask* )ClassMetadataHandler::Get().CreateClassObject( classHash );

			pBaseObject->Deserialise( rStream );

			// NB: pBaseObject Converted to Ref<>
			m_Children.push_back( pBaseObject );
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// SELECTOR TASK

	BehaviourTreeSelectorTask::BehaviourTreeSelectorTask()
	{
	}

	BehaviourTreeSelectorTask::~BehaviourTreeSelectorTask()
	{
		Reset();
	}

#if !defined(SAT_DIST)
	void BehaviourTreeSelectorTask::PreInitialiseTask( NodeEditor* pEditor, NodeEditorNodeBase* pNode )
	{
		Super::PreInitialiseTask( pEditor, pNode );

		// Get the seq node.
		BehaviourTreeSelectorNode* pSelectorNode = dynamic_cast< BehaviourTreeSelectorNode* >( pNode );
		if( pSelectorNode )
		{
			// The children are added... but no tasks exist
			// so lets create them!
			m_Children.reserve( pSelectorNode->GetChildren().size() );

			for( const auto& rNodeID : pSelectorNode->GetChildren() )
			{
				if( const auto rNode = pEditor->FindNode( rNodeID ) )
				{
					auto* pChildTask = ( BehaviourTreeBaseTask* ) rNode->ConvertToTask();
					pChildTask->PreInitialiseTask( pEditor, rNode.Get() );

					// NB: Converted to Ref!
					m_Children.push_back( pChildTask );
				}
			}
		}
	}
#endif

	void BehaviourTreeSelectorTask::InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther )
	{
		Super::InitialiseTaskWithOther( pHandler, pOther );
	}

	NodeEditorTaskState BehaviourTreeSelectorTask::Tick( Timestep ts )
	{
		// Next, check our condition, if any
		if( m_pNodeCondition )
		{
			if( const auto status = m_pNodeCondition->Tick( ts ); status != NodeEditorTaskState::Completed )
				return status;
		}

		for( auto& rTask : m_Children )
		{
			if( rTask )
			{
				const auto status = rTask->Tick( ts );
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

	BehaviourTreeSequenceTask::~BehaviourTreeSequenceTask()
	{
		Reset();

		for( auto& rChild : m_Children )
		{
			rChild->Reset();
		}

		m_Children.clear();
	}

#if !defined(SAT_DIST)
	void BehaviourTreeSequenceTask::PreInitialiseTask( NodeEditor* pEditor, NodeEditorNodeBase* pNode )
	{
		Super::PreInitialiseTask( pEditor, pNode );

		// Get the seq node.
		BehaviourTreeSequenceNode* pSequenceNode = dynamic_cast< BehaviourTreeSequenceNode* >( pNode );
		if( pSequenceNode )
		{
			// The children are added... but no tasks exist
			// so lets create them!
			m_Children.reserve( pSequenceNode->GetChildren().size() );

			for( const auto& rNodeID : pSequenceNode->GetChildren() )
			{
				if( const auto rNode = pEditor->FindNode( rNodeID ) )
				{
					auto* pChildTask = ( BehaviourTreeBaseTask* ) rNode->ConvertToTask();
					pChildTask->PreInitialiseTask( pEditor, rNode.Get() );

					// NB: Converted to Ref!
					m_Children.push_back( pChildTask );
				}
			}
		}
	}
#endif

	void BehaviourTreeSequenceTask::InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther )
	{
		Super::InitialiseTaskWithOther( pHandler, pOther );
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
			Ref<BehaviourTreeBaseTask> task = m_Children[ m_CurrentTaskIndex ];
			if( !task )
			{
				Reset();
				return NodeEditorTaskState::Failed;
			}

			m_pCurrentTask = task.Get();
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
