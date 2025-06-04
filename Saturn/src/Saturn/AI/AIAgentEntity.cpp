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
#include "AIAgentEntity.h"

#include "Saturn/Core/Random.h"

#include "Saturn/Physics/PhysicsRigidBody.h"

#include "Saturn/NodeEditor/NodeEditorBase.h"
#include "BehaviourTree/BehaviourTree.h"

#include <Detour/DetourNavMeshQuery.h>
#include <glm/gtc/type_ptr.hpp>

namespace Saturn {

	AIAgentEntity::AIAgentEntity()
	{
		AddComponent<StaticMeshComponent>();
		AddComponent<CapsuleColliderComponent>();
//		AddComponent<RigidbodyComponent>().LockFlags = RigidbodyLockFlags::RigidbodyLock_RotationX | RigidbodyLockFlags::RigidbodyLock_RotationY | RigidbodyLockFlags::RigidbodyLock_RotationZ;
	}

	AIAgentEntity::AIAgentEntity( const std::string& rName, UUID rId )
		: Super( rName, rId )
	{
		AddComponent<StaticMeshComponent>();
		AddComponent<CapsuleColliderComponent>();
//		AddComponent<RigidbodyComponent>().LockFlags = RigidbodyLockFlags::RigidbodyLock_RotationX | RigidbodyLockFlags::RigidbodyLock_RotationY | RigidbodyLockFlags::RigidbodyLock_RotationZ;
	}

	AIAgentEntity::~AIAgentEntity()
	{
	}

	void AIAgentEntity::BeginPlay()
	{
		Super::BeginPlay();

		AssetID id = 12076914912353493827llu;
		m_BehaviourTree = Ref<BehaviourTree>::Create( id );

		auto e = m_Scene->GetAllEntitiesWith<NavigationMeshSpecificationComponent>();
		m_NavBoundsEntity = e[ 0 ].As<NavBoundsEntity>();

		m_BehaviourTree->Initialise( this );
		m_BehaviourTree->FirstEvaluate();
	}

	static float RcRandomFunction() 
	{
		return Random::RandomFloatInRange( 0.0f, 1.0f );
	}

	void AIAgentEntity::OnUpdate( Saturn::Timestep ts )
	{
		Super::OnUpdate( ts );

		m_FrameCount--;

		m_BehaviourTree->Tick( ts );

		if( m_FrameCount <= 0 && !m_Moving )
		{
			// move to random location
			auto* pNavMeshQuery = m_Scene->GetNavMeshQuery();

			dtQueryFilter filter;
			dtPolyRef randomRef;

			dtStatus status = pNavMeshQuery->findRandomPoint( &filter, RcRandomFunction, &randomRef, glm::value_ptr( m_TargetPosition ) );

			auto& rPosition = GetComponent<TransformComponent>().Position;

			dtPolyRef start, end;
			float polyPickExt[ 3 ] = { 2.0f, 2.0f, 2.0f }; // Extent of the poly pick

			pNavMeshQuery->findNearestPoly( glm::value_ptr( rPosition ), polyPickExt, &filter, &start, 0 );
			pNavMeshQuery->findNearestPoly( glm::value_ptr( m_TargetPosition ), polyPickExt, &filter, &end, 0 );

			// find path
			dtPolyRef path[ 256 ];
			int pathCount = 0;

			status = pNavMeshQuery->findPath( start, end, glm::value_ptr( rPosition ), glm::value_ptr( m_TargetPosition ), &filter, path, &pathCount, 256 );

			if( dtStatusFailed( status ) || pathCount == 0 )
			{
				// If we failed to find a path, just snap to the target position
				m_TargetPosition = rPosition;
			}
			else
			{
				float straightPath[ 256 * 3 ];
				unsigned char straightPathFlags[ 256 ];
				dtPolyRef straightPathPolys[ 256 ];
				int straightPathCount = 0;

				status = pNavMeshQuery->findStraightPath(
					glm::value_ptr( rPosition ),
					glm::value_ptr( m_TargetPosition ), path, pathCount, straightPath, straightPathFlags, straightPathPolys, &straightPathCount, 256 );

				if( dtStatusSucceed( status ) && straightPathCount > 0 )
				{
					m_PathPoints.clear();

					for( size_t i = 0; i < straightPathCount; i++ )
					{
						float* p = &straightPath[ i * 3 ];
						m_PathPoints.push_back( glm::vec3( p[ 0 ], p[ 1 ], p[ 2 ] ) );
						m_Moving = true;
					}
				}
			}

			m_FrameCount = ( int ) glm::ceil( Random::RandomFloatInRange( 0.0f, 100.0f ) );
		}

		if( m_Moving )
		{
			auto& rPosition = GetComponent<TransformComponent>().Position;
			const glm::vec3& target = m_PathPoints[ m_CurrentPathIndex ];

			glm::vec3 diff = target - rPosition;
			float step = 5.0f * ts.Seconds(); // speed * deltaTime

			float dist = glm::length( diff );
			if( dist <= 0.2f )
			{
				// Snap to current target
				m_CurrentPathIndex++;

				if( m_CurrentPathIndex >= m_PathPoints.size() )
				{
					// Reached final destination
					m_Moving = false;
					m_FrameCount = (int)glm::ceil( Random::RandomFloatInRange( 0.0f, 100.0f ) );
					m_PathPoints.clear();
					m_CurrentPathIndex = 0;
				}
			}
			else
			{
				glm::vec3 direction = glm::normalize( diff );
				glm::vec3 movement = direction * step;

				if( glm::length( movement ) > dist )
					movement = diff;

				rPosition += movement;
			}
		}
	}

	void AIAgentEntity::OnPhysicsUpdate( Saturn::Timestep ts )
	{
		Super::OnPhysicsUpdate( ts );
	}

}
