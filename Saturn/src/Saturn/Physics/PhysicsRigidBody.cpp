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
#include "PhysicsRigidBody.h"

#include "PhysicsFoundation.h"
#include "PhysicsAuxiliary.h"

#include "Saturn/AI/Navigation/RecastInputGeometry.h"

namespace Saturn {

	PhysicsRigidBody::PhysicsRigidBody( SharedPtr<Entity> entity )
		: m_Entity( entity )
	{
		const TransformComponent& tc = entity->GetComponent<TransformComponent>();
		const RigidbodyComponent& rb = entity->GetComponent<RigidbodyComponent>();

		physx::PxRigidDynamic* pBody = PhysicsFoundation::Get().GetPhysics().createRigidDynamic( Auxiliary::GLMTransformToPx( tc.GetTransform() ) );
		
		m_Actor = pBody;
		m_Actor->setActorFlag( physx::PxActorFlag::eVISUALIZATION, true );

		SetKinematic( rb.IsKinematic );
		SetMass( rb.Mass );
		SetLockFlags( (RigidbodyLockFlags)rb.LockFlags, true );

		physx::PxRigidBodyExt::updateMassAndInertia( *pBody, (physx::PxReal)rb.Mass );
	}

	PhysicsRigidBody::~PhysicsRigidBody()
	{
		Destroy();
	}

	void PhysicsRigidBody::CreateShape()
	{
		RigidbodyComponent& rb = m_Entity->GetComponent<RigidbodyComponent>();
		
		// Normal Collider Component's have more priority over the static mesh.
		if( m_Entity->HasComponent<BoxColliderComponent>() )
		{
			AttachPhysicsShape( ShapeType::Box );
		}
		else if( m_Entity->HasComponent<SphereColliderComponent>() )
		{
			AttachPhysicsShape( ShapeType::Sphere );
		}
		else if ( m_Entity->HasComponent<CapsuleColliderComponent>() )
		{
			AttachPhysicsShape( ShapeType::Capusle );
		}
		else if( m_Entity->HasComponent<StaticMeshComponent>() )
		{
			AttachPhysicsShape( m_Entity->GetComponent<StaticMeshComponent>().Mesh->GetAttachedShape() );
		}
		else
		{
			SAT_CORE_WARN( "No physics shape component was found! No shape will be attached." );
		}

		m_Actor->userData = this;

		// The settings might of changed.
		SetKinematic( rb.IsKinematic );
		SetMass( rb.Mass );
	}

	void PhysicsRigidBody::AttachPhysicsShape( ShapeType type )
	{
		switch( type )
		{
			case Saturn::ShapeType::ConvexMesh: 
			{
				m_Shape = Ref<ConvexMeshShape>::Create( m_Entity );
			} break;

			case Saturn::ShapeType::TriangleMesh:
			{
				// PhysX requires all non-kinematic dynamic rigid bodies with the flag eSIMULATION_SHAPE to be kinematic.
				auto& rb = m_Entity->GetComponent<RigidbodyComponent>();
				
				if( !rb.IsKinematic )
				{
					SAT_CORE_WARN( "PhysX requires all non-kinematic dynamic rigid bodies with the flag eSIMULATION_SHAPE to be kinematic!" );
					SAT_CORE_WARN( "This happened because you are using a Triangle mesh shape!" );
					
					rb.IsKinematic = true;
					SetKinematic( true );
				}

				m_Shape = Ref<TriangleMeshShape>::Create( m_Entity );
			} break;

			case Saturn::ShapeType::Box: 
			{
				m_Shape = Ref<BoxShape>::Create( m_Entity );
			} break;

			case Saturn::ShapeType::Sphere:
			{
				m_Shape = Ref<SphereShape>::Create( m_Entity );
			} break;

			case Saturn::ShapeType::Capusle:
			{
				m_Shape = Ref<CapsuleShape>::Create( m_Entity );
			} break;

			case Saturn::ShapeType::Unknown:
			default:
				break;
		}

		if( m_Shape )
			m_Shape->Create( *m_Actor );
	}

	void PhysicsRigidBody::Destroy()
	{
		if( m_Shape )
		{
			m_Shape->Detach( *m_Actor );
		}

		m_Shape = nullptr;

		PHYSX_TERMINATE_ITEM( m_Actor );

		m_Entity = nullptr;
	}

	void PhysicsRigidBody::ExportRc( RecastInputGeometryExpData& rData, AABB& rNavMeshBounds )
	{
		if( m_Shape )
			m_Shape->ExportRc( *m_Actor, rData, rNavMeshBounds );
	}

	void PhysicsRigidBody::SetKinematic( bool val )
	{
		physx::PxRigidDynamic* pBody = ( physx::PxRigidDynamic* ) m_Actor;
		pBody->setRigidBodyFlag( physx::PxRigidBodyFlag::eKINEMATIC, val );

		m_Kinematic = val;
	}

	void PhysicsRigidBody::SetMass( float val )
	{
		physx::PxRigidDynamic* pBody = ( physx::PxRigidDynamic* ) m_Actor;
		pBody->setMass( val );
	}

	void PhysicsRigidBody::SetLinearDrag( float value )
	{
		physx::PxRigidDynamic* pBody = ( physx::PxRigidDynamic* ) m_Actor;
		pBody->setLinearDamping( value );
	}

	void PhysicsRigidBody::SetLinearVelocity( const glm::vec3& rVelocity )
	{
		physx::PxRigidDynamic* pBody = ( physx::PxRigidDynamic* ) m_Actor;
		pBody->setLinearVelocity( Auxiliary::GLMToPx( rVelocity ) );
	}

	float PhysicsRigidBody::GetLinearDrag()
	{
		physx::PxRigidDynamic* pBody = ( physx::PxRigidDynamic* ) m_Actor;
		return pBody->getLinearDamping();
	}

	void PhysicsRigidBody::ApplyForce( glm::vec3 ForceAmount, ForceMode Type )
	{
		physx::PxRigidDynamic* pBody = ( physx::PxRigidDynamic* ) m_Actor;

		pBody->addForce( Auxiliary::GLMToPx( ForceAmount ), ( physx::PxForceMode::Enum ) Type );
	}

	void PhysicsRigidBody::Rotate( const glm::vec3& rRotation )
	{
		physx::PxTransform trans = m_Actor->getGlobalPose();

		trans.q *= ( physx::PxQuat( glm::radians( rRotation.x ), { 1.0f, 0.0f, 0.0f } )
			* physx::PxQuat( glm::radians( rRotation.y ), { 0.0f, 1.0f, 0.0f } )
			* physx::PxQuat( glm::radians( rRotation.z ), { 0.0f, 0.0f, 1.0f } ) );
		
		m_Actor->setGlobalPose( trans );
	}

	void PhysicsRigidBody::Rotate( const glm::quat& rRotation )
	{
		physx::PxTransform trans = m_Actor->getGlobalPose();

		trans.q *= Auxiliary::QGLMToPx( rRotation );

		m_Actor->setGlobalPose( trans );
	}

	glm::vec3 PhysicsRigidBody::GetPosition()
	{
		float xpos = m_Actor->getGlobalPose().p.x;
		float ypos = m_Actor->getGlobalPose().p.y;
		float zpos = m_Actor->getGlobalPose().p.z;

		glm::vec3 pos{};

		pos.x = xpos;
		pos.y = ypos;
		pos.z = zpos;

		return  pos;
	}

	glm::vec3 PhysicsRigidBody::GetRotation()
	{
		auto xq = m_Actor->getGlobalPose().q.x;
		auto yq = m_Actor->getGlobalPose().q.y;
		auto zq = m_Actor->getGlobalPose().q.z;
		auto wq = m_Actor->getGlobalPose().q.w;

		glm::vec3 q = {};
		q.x = xq;
		q.y = yq;
		q.z = zq;

		return q;
	}

	glm::mat4 PhysicsRigidBody::GetTransform()
	{
		auto xpos = m_Actor->getGlobalPose().p.x;
		auto ypos = m_Actor->getGlobalPose().p.y;
		auto zpos = m_Actor->getGlobalPose().p.z;

		auto xq = m_Actor->getGlobalPose().q.x;
		auto yq = m_Actor->getGlobalPose().q.y;
		auto zq = m_Actor->getGlobalPose().q.z;

		auto pos = glm::mat4( xq * ypos * zpos );
		auto rot = glm::mat4( xpos * yq * zq );

		return glm::mat4( pos * rot );
	}

	glm::vec3 PhysicsRigidBody::GetLinearVelocity() const
	{
		physx::PxRigidDynamic* pBody = ( physx::PxRigidDynamic* ) m_Actor;
		physx::PxVec3 vel = pBody->getLinearVelocity();

		return Auxiliary::PxToGLM( vel );
	}

	void PhysicsRigidBody::SetLockFlags( RigidbodyLockFlags flags, bool value )
	{
		if( value )
			m_LockFlags |= flags;
		else
			m_LockFlags &= ~flags;

		physx::PxRigidDynamic* pBody = ( physx::PxRigidDynamic* ) m_Actor;

		pBody->setRigidDynamicLockFlag( ( physx::PxRigidDynamicLockFlag::Enum ) flags, value );
	}

	bool PhysicsRigidBody::AllRotationLocked() const
	{
		return m_LockFlags & RigidbodyLockFlags::RigidbodyLock_RotationX && m_LockFlags & RigidbodyLockFlags::RigidbodyLock_RotationY && m_LockFlags & RigidbodyLockFlags::RigidbodyLock_RotationZ;
	}

	void PhysicsRigidBody::SyncTransfrom()
	{
		TransformComponent& tc = m_Entity->GetComponent<TransformComponent>();

		physx::PxTransform actorPose = m_Actor->getGlobalPose();
		tc.Position = Auxiliary::PxToGLM( actorPose.p );

		if( !AllRotationLocked() )
			tc.SetRotation( Auxiliary::QPxToGLM( actorPose.q ) );
	}

}
