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
#include "AnimGraphStateMachineTask.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

namespace Saturn {

	AnimGraphStateMachineTask::AnimGraphStateMachineTask()
	{
	}

	AnimGraphStateMachineTask::~AnimGraphStateMachineTask()
	{
	}

#if !defined(SAT_DIST)
	void AnimGraphStateMachineTask::PreInitialiseTask( NodeEditor* pEditor, NodeEditorNodeBase* pNode )
	{
		Super::PreInitialiseTask( pEditor, pNode );

		/*
		for( auto& rState : m_States )
		{
			rState->PreInitialiseTask( pEditor, nullptr );
		}
		*/
	}
#endif

	void AnimGraphStateMachineTask::InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther )
	{
		Super::InitialiseTaskWithOther( pHandler, pOther );

		AnimGraphStateMachineTask* pThisOther = dynamic_cast< AnimGraphStateMachineTask* >( pOther );
		if( pThisOther )
		{
			m_States.reserve( pThisOther->m_States.size() );

			for( auto& rState : pThisOther->m_States )
			{
				AnimGraphStateMachineState* pNewState = NewObject<AnimGraphStateMachineState>( this );
				pNewState->InitialiseTaskWithOther( pHandler, rState.Get() );

				m_States.push_back( pNewState );
			}
		}

		m_CurrentTask = m_States.front();
	}

	NodeEditorTaskState AnimGraphStateMachineTask::Tick( Timestep ts )
	{
		const auto status = m_CurrentTask->Tick( ts );

		switch( status )
		{
			case NodeEditorTaskState::TransitionShouldTransition:
			{
				auto itr = std::find_if( m_States.begin(), m_States.end(),
					[ this ]( const auto task )
				{
					return task->GetNodeID() == m_CurrentTask->GetNextState();
				} );

				if( itr != m_States.end() )
					m_CurrentTask = *itr;
			} break;

			default:
				break;
		}

		return NodeEditorTaskState::Completed;
	}

	void AnimGraphStateMachineTask::Reset()
	{
	}

	void AnimGraphStateMachineTask::Serialise( std::ofstream& rStream ) const
	{
		Super::Serialise( rStream );

		RawSerialisation::WriteObject( m_States.size(), rStream );
		for( const auto& rState : m_States )
		{
			rState->Serialise( rStream );
		}
	}

	void AnimGraphStateMachineTask::Deserialise( FDependentIStream& rStream )
	{
		Super::Deserialise( rStream );

		size_t size = 0llu;
		RawSerialisation::ReadObject( size, rStream );
		
		m_States.reserve( size );

		for( size_t i = 0; i < size; ++i )
		{
			AnimGraphStateMachineState* pNewState = NewObject<AnimGraphStateMachineState>( this );
			pNewState->Deserialise( rStream );

			m_States.push_back( pNewState );
		}
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( AnimGraphStateMachineTask );
