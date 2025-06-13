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
#include "BehaviourTreeMoveToTask.h"

#include "Saturn/AI/AIAgentEntity.h"

#include "Saturn/AI/BehaviourTree/AssetViewer/BehaviourTreeNodeEditor.h"
#include "Saturn/AI/BehaviourTree/AssetViewer/Nodes/BehaviourTreeNodeBase.h"

#include "Saturn/Core/Random.h"

#include "Saturn/Physics/PhysicsRigidBody.h"

#include "Saturn/AI/Navigation/RecastCore.h"
#include <Detour/DetourNavMeshQuery.h>
#include <glm/gtc/type_ptr.hpp>

namespace Saturn {

	static float RcRandomFunction()
	{
		return Random::RandomFloatInRange( 0.0f, 1.0f );
	}

	//////////////////////////////////////////////////////////////////////////
	// Behaviour Tree Move To Task

	BehaviourTreeMoveToTask::BehaviourTreeMoveToTask( const glm::vec3& rTargetPosition )
		: m_TargetPosition( rTargetPosition )
	{
	}

	void BehaviourTreeMoveToTask::InitialiseTask( BehaviourTreeNodeEditor* pEditor, BehaviourTreeNodeBase* pNode )
	{
		m_Agent = pEditor->GetTargetAgent();
		m_NodeID = pNode->ID;
	}

	BehaviourTreeMoveToTask::~BehaviourTreeMoveToTask()
	{
		m_Agent = nullptr;
	}

	BehaviourTreeTaskState BehaviourTreeMoveToTask::Tick( Timestep ts )
	{
		return BehaviourTreeTaskState::Completed;

		if( !m_Moving )
		{
			// InitPathTo will return Failed or Starting
			m_CurrentState = InitPathTo();

			return m_CurrentState;
		}

		m_CurrentState = WalkToNextWaypoint( ts );
		return m_CurrentState;
	}

	// TODO: This API is will be replaced when we have a proper waypoint system that will be handled by the NavSystem or the NavMeshBouding.
	BehaviourTreeTaskState BehaviourTreeMoveToTask::InitPathTo()
	{
		auto* pNavMeshQuery = GActiveScene->GetNavMeshQuery();

		dtQueryFilter filter;
		dtPolyRef startPoly, endPoly;
		float polyPickExt[ 3 ] = { 2.0f, 2.0f, 2.0f }; // Extent of the poly pick.

		float outStartNearest[ 3 ], outEndNearest[ 3 ];

		glm::vec3& rCurrentPosition = m_Agent->GetComponent<TransformComponent>().Position;
		DT_CHECK( pNavMeshQuery->findNearestPoly( glm::value_ptr( rCurrentPosition ), polyPickExt, &filter, &startPoly, outStartNearest ) );
		
		auto status = pNavMeshQuery->findNearestPoly( glm::value_ptr( m_TargetPosition ), polyPickExt, &filter, &endPoly, outEndNearest );

		if( dtStatusFailed( status ) )
		{
			SAT_CORE_ERROR( "[BehaviourTreeMoveToTask] Invalid position! so unable to move Agent/{0} to {1}", m_Agent->GetName(), m_TargetPosition );
			return BehaviourTreeTaskState::Failed;
		}

		// Now that we have found the polys we can now build a path.
		dtPolyRef pathRefs[ 256 ];
		int pathCount = 0;
		
		DT_CHECK( pNavMeshQuery->findPath( startPoly, endPoly, outStartNearest, outEndNearest, &filter, pathRefs, &pathCount, 256 ) );

		float straightPath[ 256 * 3 ];
		unsigned char straightPathFlags[ 256 ];
		dtPolyRef straightPathPolys[ 256 ];
		int straightPathCount = 0;

		status = pNavMeshQuery->findStraightPath( outStartNearest, outEndNearest, pathRefs, pathCount, straightPath, straightPathFlags, straightPathPolys, &straightPathCount, 256 );

		if( dtStatusSucceed( status ) && straightPathCount > 0 )
		{
			m_Waypoints.clear();
			m_Waypoints.reserve( straightPathCount );

			for( size_t i = 0; i < straightPathCount; i++ )
			{
				float* p = &straightPath[ i * 3 ];
				m_Waypoints.push_back( glm::vec3( p[ 0 ], p[ 1 ], p[ 2 ] ) );
				m_Moving = true;
			}
		}
		else
			return BehaviourTreeTaskState::Failed;

		return BehaviourTreeTaskState::Starting;
	}

	BehaviourTreeTaskState BehaviourTreeMoveToTask::WalkToNextWaypoint( Timestep ts )
	{
		if( !m_Moving )
			return BehaviourTreeTaskState::Completed;

		glm::vec3& rCurrentPosition = m_Agent->GetComponent<TransformComponent>().Position;
		const auto& rCurrentWaypoint = m_Waypoints[ m_CurrentWaypointIndex ];

		glm::vec3 diff = rCurrentWaypoint - rCurrentPosition;
		float step = 5.0f * ts.Seconds();

		float distance = glm::length( diff );
		if( distance <= 0.2f )
		{
			// Move on to the next waypoint.
			m_CurrentWaypointIndex++;

			if( m_CurrentWaypointIndex >= m_Waypoints.size() )
			{
				ClearWaypoints();

				return BehaviourTreeTaskState::Completed;
			}
		}
		else
		{
			// Walk.
			glm::vec3 dir = glm::normalize( diff );
			glm::vec3 movement = dir * step;

			if( glm::length( movement ) > distance )
				movement = diff;

			rCurrentPosition += movement;
		}

		return BehaviourTreeTaskState::Running;
	}

	void BehaviourTreeMoveToTask::Reset()
	{
		ClearWaypoints();
	}

	void BehaviourTreeMoveToTask::ClearWaypoints() 
	{
		m_Moving = false;
		m_Waypoints.clear();
		m_CurrentWaypointIndex = 0;
	}

}
