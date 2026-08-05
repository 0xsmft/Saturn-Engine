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
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>

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

		PhysicsFoundation::Get()->GetPhysicsSystem()->OptimizeBroadPhase();

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
			// Wait until next frame, to create, avoid creating here, expensive.
			auto* pCharacterMovement = rEntity->GetComponent<CharacterMovementComponent>().CharacterMovement;
			if( !pCharacterMovement )
				continue;

			pCharacterMovement->PreUpdate( 1.0f / 60.0f );
			pCharacterMovement->OnUpdate( 1.0f / 60.0f );
		}

		// sim
		PhysicsFoundation::Get()->GetPhysicsSystem()->Update( 1.0f / 60.0f, 1u, PhysicsFoundation::Get()->GetTempAllocator(), PhysicsFoundation::Get()->GetJobSystem() );

#if !defined(SAT_DIST)
		m_DebugRecorder.NewFrame();
#endif

		// post sim
		PhysicsFoundation::Get()->GetContactHandler()->DispatchAllContactEvents();
		PhysicsFoundation::Get()->GetCharacterContactHandler()->DispatchAllContactEvents();
	}

	class JoltSelfFilter : public JPH::ShapeFilter
	{
	public:
		JoltSelfFilter( const JPH::Shape* pShape ) 
			: m_pShape( pShape )
		{
		}

		virtual bool ShouldCollide( 
			const JPH::Shape* inShape2, 
			const JPH::SubShapeID& inSubShapeIDOfShape2 ) const override
		{
			return m_pShape != inShape2;
		}

		virtual bool ShouldCollide( 
			const JPH::Shape* inShape1, 
			const JPH::SubShapeID& inSubShapeIDOfShape1, 
			const JPH::Shape* inShape2, 
			const JPH::SubShapeID& inSubShapeIDOfShape2 ) const override
		{
			return m_pShape != inShape1 || m_pShape != inShape2;
		}

	private:
		const JPH::Shape* m_pShape = nullptr;
	};

	bool PhysicsScene::Raycast( const glm::vec3& rOrigin, const glm::vec3& rDirection, float maxDistance, RaycastHitResult* pOut )
	{
		return RaycastIgnoringSelf( nullptr, rOrigin, rDirection, maxDistance, pOut );
	}

	bool PhysicsScene::RaycastIgnoringSelf( SharedPtr<Entity> entity, const glm::vec3& rOrigin, const glm::vec3& rDirection, float maxDistance, RaycastHitResult* pOut )
	{
		const JPH::RayCast ray{ Auxiliary::GLMToJolt( rOrigin ), Auxiliary::GLMToJolt( glm::normalize( rDirection ) ) * maxDistance };

		// TODO: This is bad.
		//		 But unfortunately not everything has a rigidbody...
		//		 but everything has a shape and it can only come from two places.
		JPH::Shape const* pShape = nullptr;
		if( entity )
		{
			// 1. Check rigidbody first
			if( auto* pRB = entity->TryGetComponent<RigidbodyComponent>(); pRB )
			{
				pShape = pRB->Rigidbody->GetShape()->GetShape().GetPtr();
			}

			// 2. Check for character controller next.
			// NOTE: This is not an "else if" block because if the user has both 
			//       we want to pick the character controller over the rigidbody.
			if( auto* pCharacterController = entity->TryGetComponent<CharacterMovementComponent>(); pCharacterController )
			{
				pShape = pCharacterController->CharacterMovement->GetShape()->GetShape().GetPtr();
			}
		}

		RaycastHitResult outHit{};
		JPH::ClosestHitCollisionCollector<JPH::CastRayCollector> collector;
		JPH::RayCastSettings settings;
		PhysicsFoundation::Get()->GetPhysicsSystem()->GetNarrowPhaseQuery().CastRay( JPH::RRayCast( ray ), settings, collector, {}, {}, {}, JoltSelfFilter( pShape ) );

		if( !collector.HadHit() )
			return false;

		JPH::BodyLockRead lock( PhysicsFoundation::Get()->GetPhysicsSystem()->GetBodyLockInterface(), collector.mHit.mBodyID );
		if( lock.Succeeded() )
		{
			const JPH::Body& rBody = lock.GetBody();

			entt::entity entityHandle = ( entt::entity ) rBody.GetUserData();

			outHit.Success = true;
			outHit.Position = Auxiliary::JoltToGLM( ray.GetPointOnRay( collector.mHit.mFraction ) );
			outHit.Hit = m_Scene->FindEntityByHandle( entityHandle );
			outHit.Distance = glm::distance( rOrigin, outHit.Position );
		}

		*pOut = outHit;

		return outHit.Success;
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

	void PhysicsScene::BuildNavMesh( RecastInputGeometryExpData& rData, AABB& rReflectiveBounds )
	{
		const auto& rNarrowPhaseQuery = PhysicsFoundation::Get()->GetNarrowPhaseQuery();
		
		const JPH::AABox box( Auxiliary::GLMToJolt( rData.DesiredBounds.Min ), Auxiliary::GLMToJolt( rData.DesiredBounds.Max ) );

		JPH::AllHitCollisionCollector<JPH::TransformedShapeCollector> collector;
		rNarrowPhaseQuery.CollectTransformedShapes( box, collector );

		for( const auto& rTransformedShape : collector.mHits )
		{
			// The division here is important because, we ant the vertex
			// indices not the float indices.
			// rData.VertexBuffer is a flat buffer of vertices
			// and looks like:
			//
			// 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17
			// x0 y0 z0 x1 y1 z1 x2 y2 z2 x3 y3 z3 x4 y4 z4 x5 y5 z5 
			// vertex 0 ...
			// if the last shape only had 3 vertices the size would be 9 (because 3 sets of Vec3s) 
			// and the starting point would be 3.
			int vertStart = ( int ) rData.VertexBuffer.size() / 3;
			
			const auto& shapeBoundingBox = rTransformedShape.GetWorldSpaceBounds();
			JPH::Shape::GetTrianglesContext context{};
			rTransformedShape.GetTrianglesStart( context, shapeBoundingBox, Auxiliary::GLMToJolt( glm::vec3{ 0.0f } ) );

			while( true )
			{
				JPH::Float3 data[ 64 * 3 ];

				const int count = rTransformedShape.GetTrianglesNext( context, 64, data, nullptr );
				if( count == 0 ) break;

				for( int i = 0; i < count; ++i )
				{
					const JPH::Float3 v0 = data[ i * 3 + 0 ];
					const JPH::Float3 v1 = data[ i * 3 + 1 ];
					const JPH::Float3 v2 = data[ i * 3 + 2 ];

					const glm::vec3 glmV0( v0.x, v0.y, v0.z );
					const glm::vec3 glmV1( v1.x, v1.y, v1.z );
					const glm::vec3 glmV2( v2.x, v2.y, v2.z );

					// This is super important, because Jolt will do it's job
					// and find the shapes in the AABB, but we aren't interested
					// in full shapes, we want the vertices that are in the AABB
					// so if a shape extends outside of the max AABB, we only
					// submit the part thats in it.
					if( rData.DesiredBounds.Contains( glmV0 ) ||
						rData.DesiredBounds.Contains( glmV1 ) ||
						rData.DesiredBounds.Contains( glmV2 ) )
					{
						rData.VertexBuffer.push_back( v0.x ); rData.VertexBuffer.push_back( v0.y ); rData.VertexBuffer.push_back( v0.z );
						rData.VertexBuffer.push_back( v1.x ); rData.VertexBuffer.push_back( v1.y ); rData.VertexBuffer.push_back( v1.z );
						rData.VertexBuffer.push_back( v2.x ); rData.VertexBuffer.push_back( v2.y ); rData.VertexBuffer.push_back( v2.z );

						rData.IndexBuffer.push_back( vertStart + 0 );
						rData.IndexBuffer.push_back( vertStart + 1 );
						rData.IndexBuffer.push_back( vertStart + 2 );

						// Set the "real" bounding box size
						rReflectiveBounds.Min = glm::min( rReflectiveBounds.Min, glm::min( glmV0, glm::min( glmV1, glmV2 ) ) );
						rReflectiveBounds.Max = glm::max( rReflectiveBounds.Max, glm::max( glmV0, glm::max( glmV1, glmV2 ) ) );

						vertStart += 3;
					}
				}
			}
		}
	}

}
