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
#include "AnimGraphStateMachineTransition.h"

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

#include "AnimGraphStateMachineTransitionNode.h"
#include "Saturn/NodeEditor/NodeEditorBase.h"

namespace Saturn {

	AnimGraphStateMachineTransitionTask::AnimGraphStateMachineTransitionTask()
	{
	}

	AnimGraphStateMachineTransitionTask::~AnimGraphStateMachineTransitionTask()
	{
	}

#if !defined( SAT_DIST )
	void AnimGraphStateMachineTransitionTask::SortTsNodesAndConvertToTasks( NodeEditor* pEditor, UUID startingID )
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
			SharedPtr<NodeEditorNodeBase> node = nodes.back();

			auto* pTask = node->ConvertToTask();
			if( pTask )
			{
				pTask->PreInitialiseTask( pEditor, node.Get() );
				m_InnerTasks.push_back( pTask );
			}

			nodes.pop_back();
		}
	}

	void AnimGraphStateMachineTransitionTask::PreInitialiseTask( NodeEditor* pEditor, NodeEditorNodeBase* pNode )
	{
		Super::PreInitialiseTask( pEditor, pNode );

		// Inner tasks
		AnimGraphStateMachineTransitionNode* pOther = dynamic_cast< AnimGraphStateMachineTransitionNode* >( pNode );
		if( pOther )
		{
			const UUID outputAnimNodeID = pOther->GetOutputNodeID();
			SortTsNodesAndConvertToTasks( pEditor, outputAnimNodeID );

			// Get source and destination ID.
			if( auto lnk = pEditor->FindLinkByPin( pNode->Inputs[ 0 ]->ID ); lnk )
			{
				m_Source = pEditor->FindNodeByPin( lnk->StartPinID )->ID;
			}

			if( auto lnk = pEditor->FindLinkByPin( pNode->Outputs[ 0 ]->ID ); lnk )
			{
				m_Destination = pEditor->FindNodeByPin( lnk->EndPinID )->ID;
			}
		}
	}
#endif

	void AnimGraphStateMachineTransitionTask::InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther )
	{
		Super::InitialiseTaskWithOther( pHandler, pOther );

		AnimGraphStateMachineTransitionTask* pThisOther = dynamic_cast< AnimGraphStateMachineTransitionTask* >( pOther );

		if( pThisOther )
		{
			m_Source = pThisOther->m_Source;
			m_Destination = pThisOther->m_Destination;

			// Construct new inner tasks.
			m_InnerTasks.reserve( pThisOther->m_InnerTasks.size() );
			for( auto& rInnerTask : pThisOther->m_InnerTasks )
			{
				Ref<NodeEditorTaskBase> pNewTask = ( NodeEditorTaskBase* ) ClassMetadataHandler::Get().CreateClassObject( rInnerTask->GetClass(), this );
				pNewTask->InitialiseTaskWithOther( pHandler, rInnerTask.Get() );

				m_InnerTasks.emplace_back( pNewTask );
			}
		}
	}

	NodeEditorTaskState AnimGraphStateMachineTransitionTask::Tick( Timestep ts )
	{
		for( auto& rInnerTask : m_InnerTasks )
		{
			const auto result = rInnerTask->Tick( ts );

			if( result == NodeEditorTaskState::TransitionShouldTransition )
			{
				return NodeEditorTaskState::TransitionShouldTransition;
			}
		}

		// Allow the state to try a different transition as we can't transition yet...
		return NodeEditorTaskState::TransitionCannotTransition;
	}

	void AnimGraphStateMachineTransitionTask::Reset()
	{
	}

	void AnimGraphStateMachineTransitionTask::Serialise( std::ofstream& rStream ) const
	{
		Super::Serialise( rStream );

		RawSerialisation::WriteObject( m_Source, rStream );
		RawSerialisation::WriteObject( m_Destination, rStream );
	
		RawSerialisation::WriteObject( m_InnerTasks.size(), rStream );
		for( const auto& rTask : m_InnerTasks )
		{
			RawSerialisation::WriteObject( rTask->GetClass()->GetHash(), rStream );

			rTask->Serialise( rStream );
		}
	}

	void AnimGraphStateMachineTransitionTask::Deserialise( FDependentIStream& rStream )
	{
		Super::Deserialise( rStream );

		RawSerialisation::ReadObject( m_Source, rStream );
		RawSerialisation::ReadObject( m_Destination, rStream );

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
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( AnimGraphStateMachineTransitionTask );
