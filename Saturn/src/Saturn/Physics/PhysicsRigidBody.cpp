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
#include "PhysicsRigidBody.h"

#include "PhysicsFoundation.h"
#include "PhysicsAuxiliary.h"
#include "PhysicsCharacterMovement.h"

#include "Saturn/AI/Navigation/RecastInputGeometry.h"

namespace Saturn {

	PhysicsRigidBody::PhysicsRigidBody( SharedPtr<Entity> entity )
		: m_Entity( entity )
	{
	}

	PhysicsRigidBody::~PhysicsRigidBody()
	{
		Destroy();
	}

	void PhysicsRigidBody::CreateShape()
	{
		const RigidbodyComponent& rb = m_Entity->GetComponent<RigidbodyComponent>();
		const TransformComponent& tc = m_Entity->GetComponent<TransformComponent>();
	
		// No shape is created when we have a CharacterMovementComponent, a DynamicRigidBody is created by PhysX.
		if( m_Entity->HasComponent<CharacterMovementComponent>() )
		{
			auto* pController = m_Entity->GetComponent<CharacterMovementComponent>().CharacterMovement;
			m_ActorOwned = false;
		}
		// Normal Collider Component's have more priority over the static mesh.
		if( m_Entity->HasComponent<BoxColliderComponent>() )
		{
			AttachPhysicsShape( PhysicsShapeType::Box );
		}
		else if( m_Entity->HasComponent<SphereColliderComponent>() )
		{
			AttachPhysicsShape( PhysicsShapeType::Sphere );
		}
		else if ( m_Entity->HasComponent<CapsuleColliderComponent>() )
		{
			AttachPhysicsShape( PhysicsShapeType::Capusle );
		}
		else if( m_Entity->HasComponent<StaticMeshComponent>() )
		{
			AttachPhysicsShape( m_Entity->GetComponent<StaticMeshComponent>().Mesh->GetAttachedShape() );
		}
		else
		{
			SAT_CORE_WARN( "No physics shape component was found! No shape will be attached." );
		}

		// The settings might of changed, so update in case.
		SetKinematic( rb.IsKinematic );

		// Create body after the shape.
		JPH::BodyCreationSettings settings( 
			m_Shape->GetShape(), 
			Auxiliary::GLMToJolt( tc.Position ), 
			Auxiliary::GLMQToJoltQ( glm::normalize( tc.GetRotation() ) ), 
			( JPH::EMotionType ) m_Type, 
			PhysLayerMoving 
		);
		
		m_pBody = PhysicsFoundation::Get()->GetBodyInterface()->CreateBody( settings );

		PhysicsFoundation::Get()->GetBodyInterface()->AddBody( m_pBody->GetID(), m_Kinematic ? JPH::EActivation::DontActivate : JPH::EActivation::Activate );

		SetMass( rb.Mass );
	}

	void PhysicsRigidBody::SetShapeTrigger( bool trigger )
	{
		/*
		if( m_Shape )
			m_Shape->SetTrigger( trigger );
		*/
	}

	void PhysicsRigidBody::AttachPhysicsShape( PhysicsShapeType type )
	{
		switch( type )
		{
			/*
			case Saturn::PhysicsShapeType::ConvexMesh: 
			{
				m_Shape = Ref<ConvexMeshShape>::Create( m_Entity );
			} break;
			*/

			case Saturn::PhysicsShapeType::TriangleMesh:
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

			case Saturn::PhysicsShapeType::Box: 
			{
				m_Shape = Ref<BoxShape>::Create( m_Entity );
			} break;

			case Saturn::PhysicsShapeType::Sphere:
			{
				m_Shape = Ref<SphereShape>::Create( m_Entity );
			} break;

			case Saturn::PhysicsShapeType::Capusle:
			{
				m_Shape = Ref<CapsuleShape>::Create( m_Entity );
			} break;

			case Saturn::PhysicsShapeType::Unknown:
			default:
				break;
		}

		if( m_Shape )
			m_Shape->Create();
	}

	void PhysicsRigidBody::Destroy()
	{
		PhysicsFoundation::Get()->GetBodyInterface()->RemoveBody( m_pBody->GetID() );
		PhysicsFoundation::Get()->GetBodyInterface()->DestroyBody( m_pBody->GetID() );

		m_pBody = nullptr;
		m_Shape = nullptr;
		m_Entity = nullptr;
	}

	void PhysicsRigidBody::ExportRc( RecastInputGeometryExpData& rData, AABB& rNavMeshBounds )
	{
	}

	void PhysicsRigidBody::SetKinematic( bool val )
	{
		if( val )
		{
			m_Type = PhysicsRigidBodyType::Kinematic;
		}

		m_Kinematic = val;
	}

	void PhysicsRigidBody::SetMass( float val )
	{
		m_pBody->GetMotionProperties()->ScaleToMass( val );
	}

	void PhysicsRigidBody::SetLinearDrag( float value )
	{
	}

	void PhysicsRigidBody::SetLinearVelocity( const glm::vec3& rVelocity )
	{
		PhysicsFoundation::Get()->GetBodyInterface()->SetLinearVelocity( m_pBody->GetID(), Auxiliary::GLMToJolt( rVelocity ) );
	}

	float PhysicsRigidBody::GetLinearDrag()
	{
		return 0.0f;
	}

	void PhysicsRigidBody::ApplyForce( glm::vec3 ForceAmount, ForceMode Type )
	{
		switch( Type )
		{
			case ForceMode::Force:
				PhysicsFoundation::Get()->GetBodyInterface()->AddForce( m_pBody->GetID(), Auxiliary::GLMToJolt( ForceAmount ) );
				break;

			case ForceMode::Impulse:
				PhysicsFoundation::Get()->GetBodyInterface()->AddImpulse( m_pBody->GetID(), Auxiliary::GLMToJolt( ForceAmount ) );
				break;

			case ForceMode::VelocityChange:
			case ForceMode::Acceleration:
			default:
				break;
		}
	}

	void PhysicsRigidBody::Rotate( const glm::vec3& rRotation )
	{
	}

	void PhysicsRigidBody::Rotate( const glm::quat& rRotation )
	{
		PhysicsFoundation::Get()->GetBodyInterface()->SetRotation( m_pBody->GetID(), Auxiliary::GLMQToJoltQ( rRotation ), JPH::EActivation::Activate );
	}

	void PhysicsRigidBody::SetPosition( const glm::vec3& rPosition )
	{
		if( m_Kinematic )
		{
			PhysicsFoundation::Get()->GetBodyInterface()->MoveKinematic( m_pBody->GetID(), Auxiliary::GLMToJolt( rPosition ), JPH::Quat::sIdentity(), 0.0f );
		}
		else
		{
			PhysicsFoundation::Get()->GetBodyInterface()->SetPosition( m_pBody->GetID(), Auxiliary::GLMToJolt( rPosition ), JPH::EActivation::Activate );
		}
	}

	glm::vec3 PhysicsRigidBody::GetPosition()
	{
		return Auxiliary::JoltToGLM( PhysicsFoundation::Get()->GetBodyInterface()->GetPosition( m_pBody->GetID() ) );
	}

	glm::vec3 PhysicsRigidBody::GetRotation()
	{
		auto eular = glm::eulerAngles( Auxiliary::JoltQToGLMQ( PhysicsFoundation::Get()->GetBodyInterface()->GetRotation( m_pBody->GetID() ) ) );

		return eular;
	}

	glm::mat4 PhysicsRigidBody::GetTransform()
	{
		return glm::mat4{};
	}

	glm::vec3 PhysicsRigidBody::GetLinearVelocity() const
	{
		return Auxiliary::JoltToGLM( PhysicsFoundation::Get()->GetBodyInterface()->GetLinearVelocity( m_pBody->GetID() ) );
	}

	void PhysicsRigidBody::SetLockFlags( RigidbodyLockFlags flags, bool value )
	{
		if( value )
			m_LockFlags |= flags;
		else
			m_LockFlags &= ~flags;
	}

	bool PhysicsRigidBody::AllRotationLocked() const
	{
		return m_LockFlags & RigidbodyLockFlags::RigidbodyLock_RotationX && m_LockFlags & RigidbodyLockFlags::RigidbodyLock_RotationY && m_LockFlags & RigidbodyLockFlags::RigidbodyLock_RotationZ;
	}

	void PhysicsRigidBody::SyncTransfrom()
	{
		TransformComponent& tc = m_Entity->GetComponent<TransformComponent>();

		tc.Position = GetPosition();
	}

}
