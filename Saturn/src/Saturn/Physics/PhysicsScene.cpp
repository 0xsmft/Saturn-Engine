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
#include "PhysicsScene.h"

#include "Saturn/Core/Profiler.h"

#include "Saturn/AI/Navigation/RecastInputGeometry.h"

#include "Saturn/Scene/Components.h"
#include "Saturn/Scene/Entity.h"

#include "PhysicsFoundation.h"
#include "PhysicsRigidBody.h"
#include "PhysicsCharacterController.h"
#include "PhysicsAuxiliary.h"

#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>

namespace Saturn {

	PhysicsScene::PhysicsScene( Ref<Scene> scene )
		: m_Scene( scene )
	{
		CreateScene();
	}

	PhysicsScene::~PhysicsScene()
	{
#if !defined(SAT_DIST)
		m_DebugRecorder.EndRecord();
#endif

		const auto view = m_Scene->GetAllEntitiesWith<RigidbodyComponent>();
		for( auto& rEntity : view )
		{
			auto& rb = rEntity->GetComponent<RigidbodyComponent>();

			delete rb.Rigidbody;
			rb.Rigidbody = nullptr;
		}

		m_Scene = nullptr;
	}

	void PhysicsScene::CreateScene()
	{	
		// Add all current bodies to the scene.
		auto rigidbodyView = m_Scene->GetAllEntitiesWith<RigidbodyComponent>();
		for( auto& rEntity : rigidbodyView )
		{
			auto& rb = rEntity->GetComponent<RigidbodyComponent>();
			InitialiseNewBody( rEntity, rb );
		}

		// Add controllers.
		auto controllerView = m_Scene->GetAllEntitiesWith<CharacterMovementComponent>();
		for( auto& rEntity : controllerView )
		{
			AddNewController( rEntity );
		}

#if !defined(SAT_DIST)
		m_DebugRecorder.BeginRecord();
#endif
	}

	void PhysicsScene::Simulate( Timestep ts )
	{
		SAT_PF_EVENT();
		
		// Pre sim
		auto controllerView = m_Scene->GetAllEntitiesWith<CharacterMovementComponent>();
		for( auto& rEntity : controllerView )
		{
			rEntity->GetComponent<CharacterMovementComponent>().CharacterMovement->PreUpdate( 1.0f / 60.0f );
			rEntity->GetComponent<CharacterMovementComponent>().CharacterMovement->OnUpdate( 1.0f / 60.0f );
		}

		// sim
		PhysicsFoundation::Get()->GetPhysicsSystem()->Update( 1.0f / 60.0f, 1u, PhysicsFoundation::Get()->GetTempAllocator(), PhysicsFoundation::Get()->GetJobSystem() );

#if !defined(SAT_DIST)
		m_DebugRecorder.NewFrame();
#endif

		// post sim
		PhysicsFoundation::Get()->GetContactHandler()->DispatchAllContactEvents();
	}

	bool PhysicsScene::Raycast( const glm::vec3& rOrigin, const glm::vec3& rDirection, float maxDistance, RaycastHitResult* pOut )
	{
		JPH::RRayCast ray{ Auxiliary::GLMToJolt( rOrigin ), Auxiliary::GLMToJolt( rDirection ) };

		RaycastHitResult outHit{};

		JPH::RayCastResult joltHit{};
		outHit.Success = PhysicsFoundation::Get()->GetPhysicsSystem()->GetNarrowPhaseQuery().CastRay( ray, joltHit );

		if( outHit.Success )
		{
			entt::entity entityHandle = ( entt::entity ) PhysicsFoundation::Get()->GetBodyInterface()->GetUserData( joltHit.mBodyID );

			outHit.Position = Auxiliary::JoltToGLM( ray.GetPointOnRay( joltHit.mFraction ) );
			outHit.Hit = m_Scene->FindEntityByHandle( entityHandle );
			outHit.Distance = glm::distance( outHit.Position, rOrigin );
		}

		*pOut = outHit;

		return outHit.Success;
	}

	void PhysicsScene::ExportRc( RecastInputGeometryExpData& rData, AABB& rNavMeshBounds )
	{
		auto view = m_Scene->GetAllEntitiesWith<RigidbodyComponent>();
		for( auto& rEntity : view )
		{
			auto& rb = rEntity->GetComponent<RigidbodyComponent>();
			rb.Rigidbody->ExportRc( rData, rNavMeshBounds );
		}
	}

	void PhysicsScene::InitialiseNewBody( SharedPtr<Entity>& rEntity, RigidbodyComponent& rRigidbodyComponent )
	{
		// Bad, this shouldn't be true and should be an assert.
		if( rRigidbodyComponent.Rigidbody )
		{
			delete rRigidbodyComponent.Rigidbody;
			SAT_CORE_WARN( "A rigidbody already exists in this component! Removing it and creating a new one!" );
		}

		rRigidbodyComponent.Rigidbody = new PhysicsRigidBody( rEntity );
		rRigidbodyComponent.Rigidbody->CreateShape();
	}

	void PhysicsScene::AddNewController( SharedPtr<Entity>& rEntity )
	{
		auto& rMovementComp = rEntity->GetComponent<CharacterMovementComponent>();

		// Bad, this shouldn't be true and should be an assert.
		if( rMovementComp.CharacterMovement )
		{
			delete rMovementComp.CharacterMovement;
			SAT_CORE_WARN( "A movement controller already exists in this component! Removing and creating a new one." );
		}

		rMovementComp.CharacterMovement = new PhysicsCharacterController( 0, !rMovementComp.NoGravity, rMovementComp.ControlMovementInAir, rMovementComp.ControlRotationInAir );
		rMovementComp.CharacterMovement->CreateController( this, rEntity, rEntity->GetLocalPosition() );
	}

}
