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
#include "BehaviourTreeMoveToTask.h"

#include "Saturn/AI/AIAgentEntity.h"

#include "Saturn/AI/BehaviourTree/AssetViewer/BehaviourTreeNodeEditor.h"
#include "Saturn/AI/BehaviourTree/AssetViewer/Nodes/BehaviourTreeNodeBase.h"

#include "Saturn/Physics/PhysicsRigidBody.h"
#include "Saturn/Physics/PhysicsCharacterController.h"

#include "Saturn/AI/Navigation/RecastCore.h"
#include <Detour/DetourNavMeshQuery.h>
#include <glm/gtc/type_ptr.hpp>

#define DT_CHECK_RETURN_FAIL_BT( x ) \
{\
unsigned int result = x; \
if( dtStatusFailed( (result) ) ) \
{ \
	const std::string errorText = Auxiliary::DetourErrorToString( result ); \
	SAT_CORE_INFO( "[BehaviourTreeMoveToTask] Detour operation failed! Error code was DETOUR ERROR/{0}", errorText ); \
	return BehaviourTreeTaskState::Failed; \
}\
}

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// Behaviour Tree Move To Task

	BehaviourTreeMoveToTask::BehaviourTreeMoveToTask( const glm::vec3& rTargetPosition )
		: m_TargetPosition( rTargetPosition )
	{
	}

	void BehaviourTreeMoveToTask::InitialiseTask( NodeEditorTaskHandler* pHandler, NodeEditorBase* pEditor, NodeEditorNodeBase* pNode )
	{
		BehaviourTreeNodeEditor* pBehaviourTreeNodeEditor = dynamic_cast< BehaviourTreeNodeEditor* >( pEditor );

		m_Agent = pBehaviourTreeNodeEditor->GetTargetAgent();
		m_NodeID = pNode->ID;
	}

	BehaviourTreeMoveToTask::~BehaviourTreeMoveToTask()
	{
		m_Agent = nullptr;
		Reset();
	}

	NodeEditorTaskState BehaviourTreeMoveToTask::Tick( Timestep ts )
	{
		if( !m_Path.IsLive() )
		{
			// InitPathTo will return Failed or Starting
			m_CurrentState = InitPathTo();

			return m_CurrentState;
		}

		m_CurrentState = WalkToNextWaypoint( ts );
		return m_CurrentState;
	}

	// TODO: This API is will be replaced when we have a proper waypoint system that will be handled by the NavSystem or the NavMeshBounds.
	NodeEditorTaskState BehaviourTreeMoveToTask::InitPathTo()
	{
		if( m_Path.RetargetPath( m_TargetPosition, m_Agent->GetComponent<TransformComponent>().Position ) )
		{
			return NodeEditorTaskState::Starting;
		}

		return NodeEditorTaskState::Failed;
	}

	NodeEditorTaskState BehaviourTreeMoveToTask::WalkToNextWaypoint( Timestep ts )
	{
		if( !m_Path.IsLive() )
			return NodeEditorTaskState::Completed;

		glm::vec3& rCurrentPosition = m_Agent->GetComponent<TransformComponent>().Position;
		const auto& rCurrentWaypoint = m_Path.GetCurrentWaypoint();

		glm::vec3 diff = rCurrentWaypoint - rCurrentPosition;
		const float step = 5.0f * ts.Seconds();

		const float distance = glm::length( diff );
		if( distance <= 0.2f )
		{
			// Move on to the next waypoint.
			m_Path.NextWaypoint();

			// If its zero after the NextWaypoint call we know that we have reached the end
			if( m_Path.GetCurrentWaypointIndex() == 0 )
			{
				return NodeEditorTaskState::Completed;
			}
		}
		else
		{
			// Walk.
			const glm::vec3 dir = glm::normalize( diff );
			glm::vec3 movement = dir * step;

			if( glm::length( movement ) > distance )
				movement = diff;

			m_Agent->GetComponent<CharacterMovementComponent>().CharacterMovement->Move( movement );
		}

		return NodeEditorTaskState::Running;
	}

	void BehaviourTreeMoveToTask::Reset()
	{
		m_Path.InvalidatePath();
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

//SAT_X31_CREATE_AUTO_REG( BehaviourTreeMoveToTask );
