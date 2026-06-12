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
#include "AnimGraphStateMachineState.h"

#include "../StateMachine/AnimGraphStateMachineStateNode.h"

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

#include "Saturn/NodeEditor/NodeEditorBase.h"

namespace Saturn {

	AnimGraphStateMachineState::AnimGraphStateMachineState()
	{
	}

	AnimGraphStateMachineState::~AnimGraphStateMachineState()
	{
	}

#if !defined(SAT_DIST)
	void AnimGraphStateMachineState::SortAnimStateNodesAndConvertToTasks( NodeEditor* pEditor, UUID startingID )
	{
		auto node = pEditor->FindNode( startingID );
		if( !node )
		{
			return;
		}

		std::deque<SharedPtr<NodeEditorNodeBase>> nodes;
		pEditor->TraverseFromStart( node, NodeEditorFlowDirection::GoToRootNode,
			[ & ]( const auto id )
		{
			nodes.push_back( pEditor->FindNode( id ) );
		} );

		m_InnerTasks.reserve( nodes.size() );
		
		while( !nodes.empty() )
		{
			SharedPtr<NodeEditorNodeBase> sortedNode = nodes.back();
			
			auto* pTask = sortedNode->ConvertToTask();
			if( pTask )
			{
				pTask->PreInitialiseTask( pEditor, sortedNode.Get() );
				m_InnerTasks.push_back( pTask );
			}

			nodes.pop_back();
		}
	}

	void AnimGraphStateMachineState::PreInitialiseTask( NodeEditor* pEditor, NodeEditorNodeBase* pNode )
	{
		Super::PreInitialiseTask( pEditor, pNode );

		// Inner tasks
		AnimGraphStateMachineStateNode* pOther = dynamic_cast< AnimGraphStateMachineStateNode* >( pNode );
		if( pOther )
		{
			const UUID outputAnimNodeID = pOther->GetOutputNodeID();
			SortAnimStateNodesAndConvertToTasks( pEditor, outputAnimNodeID );
		}

		// Transitions out are linked to the nodes output pins
		const auto nodes = pEditor->FindNeighborsViaOutputs( pNode->SharedFromThis() );
		m_Transitions.reserve( nodes.size() );

		for( const auto& rNodeID : nodes )
		{
			SharedPtr<NodeEditorNodeBase> node = pEditor->FindNode( rNodeID );
			if( node )
			{
				auto* pTransTask = ( AnimGraphStateMachineTransitionTask* ) node->ConvertToTask();
				pTransTask->PreInitialiseTask( pEditor, node.Get() );

				m_Transitions.push_back( pTransTask );
			}
		}
	}
#endif

	void AnimGraphStateMachineState::InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther )
	{
		Super::InitialiseTaskWithOther( pHandler, pOther );

		AnimGraphStateMachineState* pThisOther = dynamic_cast< AnimGraphStateMachineState* >( pOther );

		if( pThisOther )
		{
			// Construct new inner tasks.
			m_InnerTasks.reserve( pThisOther->m_InnerTasks.size() );
			for( auto& rInnerTask : pThisOther->m_InnerTasks )
			{
				auto* pNewTask = ( NodeEditorTaskBase* )ClassMetadataHandler::Get().CreateClassObject( rInnerTask->GetClass(), this );
				pNewTask->InitialiseTaskWithOther( pHandler, rInnerTask.Get() );

				m_InnerTasks.push_back( pNewTask );
			}
			
			// Construct new transitions.
			m_Transitions.reserve( pThisOther->m_Transitions.size() );
			for( auto& rTransition : pThisOther->m_Transitions )
			{
				AnimGraphStateMachineTransitionTask* pTransTask = NewObject<AnimGraphStateMachineTransitionTask>( this );
				pTransTask->InitialiseTaskWithOther( pHandler, rTransition.Get() );

				m_Transitions.push_back( pTransTask );
			}
		}
	}

	NodeEditorTaskState AnimGraphStateMachineState::Tick( Timestep ts )
	{
		// First we tick our inner tasks
		for( auto& rInnerTask : m_InnerTasks )
		{
			rInnerTask->Tick( ts );
		}

		// Now transitions.
		for( auto& rTransition : m_Transitions )
		{
			const auto result = rTransition->Tick( ts );

			if( result == NodeEditorTaskState::TransitionShouldTransition )
			{
				for( auto& rInnerTask : m_InnerTasks )
				{
					rInnerTask->Reset();
				}

				m_NextStateToTransitionOutTo = rTransition->GetDestinationID();
				return NodeEditorTaskState::TransitionShouldTransition;
			}
		}

		// Still active in this state.
		return NodeEditorTaskState::Running;
	}

	void AnimGraphStateMachineState::Reset()
	{
	}

	void AnimGraphStateMachineState::Serialise( std::ofstream& rStream ) const
	{
		Super::Serialise( rStream );

		RawSerialisation::WriteObject( m_InnerTasks.size(), rStream );
	
		for( const auto& rInnerTask : m_InnerTasks )
		{
			RawSerialisation::WriteObject( rInnerTask->GetClass()->GetHash(), rStream );

			rInnerTask->Serialise( rStream );
		}

		RawSerialisation::WriteObject( m_Transitions.size(), rStream );

		for( const auto& rTransition : m_Transitions )
		{
			rTransition->Serialise( rStream );
		}
	}

	void AnimGraphStateMachineState::Deserialise( FDependentIStream& rStream )
	{
		Super::Deserialise( rStream );

		size_t size = 0llu;
		RawSerialisation::ReadObject( size, rStream );

		m_InnerTasks.reserve( size );

		for( size_t i = 0; i < size; ++i )
		{
			uint64_t classHash = 0llu;
			RawSerialisation::ReadObject( classHash, rStream );

			SObject* pObject = ClassMetadataHandler::Get().CreateClassObject( classHash, this );
			SAT_CORE_VERIFY( pObject );

			NodeEditorTaskBase* pTask = dynamic_cast< NodeEditorTaskBase* >( pObject );
			SAT_CORE_VERIFY( pTask );

			pTask->Deserialise( rStream );

			m_InnerTasks.push_back( pTask );
		}

		RawSerialisation::ReadObject( size, rStream );

		m_Transitions.reserve( size );

		for( size_t i = 0; i < size; ++i )
		{
			AnimGraphStateMachineTransitionTask* pTsTask = NewObject<AnimGraphStateMachineTransitionTask>( this );
			pTsTask->Deserialise( rStream );

			m_Transitions.push_back( pTsTask );
		}
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( AnimGraphStateMachineState );
