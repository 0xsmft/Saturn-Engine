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
#include "PhysicsCharacterController.h"

#include "Saturn/AI/Navigation/RecastInputGeometry.h"

namespace Saturn {

	PhysicsRigidBody::PhysicsRigidBody( SharedPtr<Entity> entity )
		: m_Entity( entity )
	{
		const RigidbodyComponent& rb = m_Entity->GetComponent<RigidbodyComponent>();
		m_LockFlags = rb.LockFlags;
		m_Type = rb.BodyType;
	}

	PhysicsRigidBody::~PhysicsRigidBody()
	{
		Destroy();
	}

	void PhysicsRigidBody::CreateShape()
	{
		const RigidbodyComponent& rb = m_Entity->GetComponent<RigidbodyComponent>();
		const TransformComponent& tc = m_Entity->GetComponent<TransformComponent>();

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
			SAT_CORE_WARN( "No physics shape component was found! Box shape will be attached." );
		
			AttachPhysicsShape( PhysicsShapeType::Box );
		}

		// The settings might of changed, so update in case.
		if( m_Shape->GetType() == PhysicsShapeType::TriangleMesh )
		{
			m_Type = PhysicsRigidBodyType::Static;
		}

		// Create body after the shape.
		JPH::BodyCreationSettings settings( 
			m_Shape->GetShape(), 
			Auxiliary::GLMToJolt( tc.Position ), 
			Auxiliary::GLMQToJoltQ( glm::normalize( tc.GetRotation() ) ), 
			( JPH::EMotionType ) m_Type, 
			m_Type == PhysicsRigidBodyType::Static ? PhysLayerNotMoving : PhysLayerMoving 
		);
		settings.mIsSensor = m_Shape->IsTrigger();
		
		auto* pBody = PhysicsFoundation::Get()->GetBodyInterface()->CreateBody( settings );
		m_BodyID = pBody->GetID();

		PhysicsFoundation::Get()->GetBodyInterface()->AddBody( 
			m_BodyID,
			m_Type == PhysicsRigidBodyType::Static ? JPH::EActivation::DontActivate : JPH::EActivation::Activate 
		);

		// FIXME: Might not be viable to use the handle! (Entity ID may be better, however it's faster to use the handle)
		//		  If we ever crash tell me to revise this!
		pBody->SetUserData( ( uint64_t ) m_Entity->GetHandle() );

		if( m_Type != PhysicsRigidBodyType::Static )
		{
			// Handle locking flags
			CreateDOFConstraint();
		}
	}

	void PhysicsRigidBody::SetShapeTrigger( bool trigger )
	{
		if( m_Shape->IsTrigger() == trigger )
			return;

		// Access
		auto& bodyInterface = PhysicsFoundation::Get()->GetPhysicsSystem()->GetBodyLockInterface();

		JPH::BodyLockWrite lock( bodyInterface, m_BodyID );
		if( lock.Succeeded() )
		{
			JPH::Body& rBody = lock.GetBody();
			rBody.SetIsSensor( trigger );
		}

		m_Shape->SetTrigger( trigger );
	}

	void PhysicsRigidBody::AttachPhysicsShape( PhysicsShapeType type )
	{
		switch( type )
		{
			case Saturn::PhysicsShapeType::Unknown:
			default:
				break;

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

			case Saturn::PhysicsShapeType::ConvexMesh:
			{
				m_Shape = Ref<ConvexMeshShape>::Create( m_Entity );
			} break;

			case Saturn::PhysicsShapeType::TriangleMesh:
			{
				m_Shape = Ref<TriangleMeshShape>::Create( m_Entity );
			} break;
		}

		if( m_Shape ) 
		{
			const RigidbodyComponent& rb = m_Entity->GetComponent<RigidbodyComponent>();
			m_Shape->Create( rb.Mass );
		}
	}

	void PhysicsRigidBody::CreateDOFConstraint()
	{
		using EAxis = JPH::SixDOFConstraintSettings::EAxis;
		JPH::SixDOFConstraintSettings settings;

		if( ( m_LockFlags & RigidbodyLock_PositionX ) )
		{
			settings.SetLimitedAxis( EAxis::TranslationX, 1.0F, 0.0F );
		}
		if( ( m_LockFlags & RigidbodyLock_PositionY ) )
		{
			settings.SetLimitedAxis( EAxis::TranslationY, 1.0F, 0.0F );
		}
		if( ( m_LockFlags & RigidbodyLock_PositionZ ) )
		{
			settings.SetLimitedAxis( EAxis::TranslationZ, 1.0F, 0.0F );
		}
		if( ( m_LockFlags & RigidbodyLock_RotationX ) )
		{
			settings.SetLimitedAxis( EAxis::RotationX, 1.0F, 0.0F );
		}
		if( ( m_LockFlags & RigidbodyLock_RotationY ) )
		{
			settings.SetLimitedAxis( EAxis::RotationY, 1.0F, 0.0F );
		}
		if( ( m_LockFlags & RigidbodyLock_RotationZ ) )
		{
			settings.SetLimitedAxis( EAxis::RotationY, 1.0F, 0.0F );
		}

		// Access
		auto& bodyInterface = PhysicsFoundation::Get()->GetPhysicsSystem()->GetBodyLockInterface();

		JPH::BodyLockWrite lock( bodyInterface, m_BodyID );
		if( lock.Succeeded() )
		{
			JPH::Body& rBody = lock.GetBody();

			settings.mPosition2 = rBody.GetPosition();

			m_DOFConstraint = static_cast<JPH::SixDOFConstraint*>( settings.Create( JPH::Body::sFixedToWorld, rBody ) );

			PhysicsFoundation::Get()->GetPhysicsSystem()->AddConstraint( m_DOFConstraint );
		}
		else
		{
			SAT_CORE_WARN( "Failed to create Degrees of Freedom constraint for locking flags!" );
		}
	}

	void PhysicsRigidBody::Destroy()
	{
		auto& bodyInterface = PhysicsFoundation::Get()->GetPhysicsSystem()->GetBodyLockInterfaceNoLock();
		JPH::BodyLockWrite lock( bodyInterface, m_BodyID );
		if( lock.Succeeded() )
		{
			JPH::Body& rBody = lock.GetBody();

			if( rBody.IsInBroadPhase() )
			{
				PhysicsFoundation::Get()->GetBodyInterface()->RemoveBody( m_BodyID );
			}
			else
			{
				PhysicsFoundation::Get()->GetBodyInterface()->DeactivateBody( m_BodyID );
			}
		}

		if( m_DOFConstraint )
		{
			PhysicsFoundation::Get()->GetPhysicsSystem()->RemoveConstraint( m_DOFConstraint );
		}

		PhysicsFoundation::Get()->GetBodyInterface()->DestroyBody( m_BodyID );

		m_Shape = nullptr;
		m_Entity = nullptr;
	}

	void PhysicsRigidBody::ExportRc( RecastInputGeometryExpData& rData, AABB& rNavMeshBounds )
	{
		if( m_Shape )
		{
			m_Shape->ExportRc( rData, rNavMeshBounds );
		}
	}

	void PhysicsRigidBody::SetMass( float val )
	{
		if( m_Type == PhysicsRigidBodyType::Static )
		{
			SAT_CORE_WARN( "[PhysicsRigidBody]: Cannot set mass of a static rigid body!" );
			return;
		}

		auto& bodyInterface = PhysicsFoundation::Get()->GetPhysicsSystem()->GetBodyLockInterface();

		JPH::BodyLockWrite lock( bodyInterface, m_BodyID );
		if( lock.Succeeded() )
		{
			JPH::Body& rBody = lock.GetBody();

			rBody.GetMotionProperties()->ScaleToMass( val );
		}
	}

	void PhysicsRigidBody::SetLinearDrag( float value )
	{
	}

	void PhysicsRigidBody::SetLinearVelocity( const glm::vec3& rVelocity )
	{
		if( m_Type == PhysicsRigidBodyType::Static )
		{
			SAT_CORE_WARN( "[PhysicsRigidBody]: Cannot set linear velocity of a static rigid body!" );
			return;
		}

		PhysicsFoundation::Get()->GetBodyInterface()->SetLinearVelocity( m_BodyID, Auxiliary::GLMToJolt( rVelocity ) );
	}

	float PhysicsRigidBody::GetLinearDrag()
	{
		return 0.0f;
	}

	void PhysicsRigidBody::ApplyForce( glm::vec3 ForceAmount, ForceMode Type )
	{
		if( m_Type == PhysicsRigidBodyType::Static )
		{
			SAT_CORE_WARN( "[PhysicsRigidBody]: Cannot apply force to static immovable rigid body!" );
			return;
		}

		switch( Type )
		{
			case ForceMode::Force:
				PhysicsFoundation::Get()->GetBodyInterface()->AddForce( m_BodyID, Auxiliary::GLMToJolt( ForceAmount ) );
				break;

			case ForceMode::Impulse:
				PhysicsFoundation::Get()->GetBodyInterface()->AddImpulse( m_BodyID, Auxiliary::GLMToJolt( ForceAmount ) );
				break;

			case ForceMode::ForceAndTorque:
				PhysicsFoundation::Get()->GetBodyInterface()->AddForceAndTorque( m_BodyID, Auxiliary::GLMToJolt( ForceAmount ), Auxiliary::GLMToJolt( ForceAmount ) );
				break;

			case ForceMode::Torque:
				PhysicsFoundation::Get()->GetBodyInterface()->AddTorque( m_BodyID, Auxiliary::GLMToJolt( ForceAmount ) );
				break;

			default:
				break;
		}
	}

	void PhysicsRigidBody::Rotate( const glm::vec3& rRotation )
	{
		Rotate( glm::quat( rRotation ) );
	}

	void PhysicsRigidBody::Rotate( const glm::quat& rRotation )
	{
		if( m_Type == PhysicsRigidBodyType::Static )
		{
			SAT_CORE_WARN( "[PhysicsRigidBody]: Cannot rotate immovable rigid body!" );
			return;
		}

		PhysicsFoundation::Get()->GetBodyInterface()->SetRotation( m_BodyID, Auxiliary::GLMQToJoltQ( rRotation ), JPH::EActivation::Activate );
	}

	void PhysicsRigidBody::SetPosition( const glm::vec3& rPosition )
	{
		switch( m_Type )
		{
			case PhysicsRigidBodyType::Static:
				SAT_CORE_WARN( "[PhysicsRigidBody]: Cannot move immovable rigid body!" );
				break;
			
			case PhysicsRigidBodyType::Kinematic: 
			{
				PhysicsFoundation::Get()->GetBodyInterface()->MoveKinematic( m_BodyID, Auxiliary::GLMToJolt( rPosition ), JPH::Quat::sIdentity(), 0.0f );
			} break;
			
			case PhysicsRigidBodyType::Dynamic:
			{
				PhysicsFoundation::Get()->GetBodyInterface()->SetPosition( m_BodyID, Auxiliary::GLMToJolt( rPosition ), JPH::EActivation::Activate );
			} break;
		
			default:
				break;
		}
	}

	glm::vec3 PhysicsRigidBody::GetPosition() const
	{
		return Auxiliary::JoltToGLM( PhysicsFoundation::Get()->GetBodyInterface()->GetPosition( m_BodyID ) );
	}

	glm::vec3 PhysicsRigidBody::GetRotation() const
	{
		auto eular = glm::eulerAngles( Auxiliary::JoltQToGLMQ( PhysicsFoundation::Get()->GetBodyInterface()->GetRotation( m_BodyID ) ) );

		return eular;
	}

	glm::mat4 PhysicsRigidBody::GetTransform()
	{
		return glm::mat4{};
	}

	glm::vec3 PhysicsRigidBody::GetLinearVelocity() const
	{
		return Auxiliary::JoltToGLM( PhysicsFoundation::Get()->GetBodyInterface()->GetLinearVelocity( m_BodyID ) );
	}

	void PhysicsRigidBody::SetLockFlags( RigidbodyLockFlags flags, bool value )
	{
		if( flags != m_LockFlags )
		{
			m_LockFlags = flags;

			if( m_DOFConstraint )
				PhysicsFoundation::Get()->GetPhysicsSystem()->RemoveConstraint( m_DOFConstraint );

			CreateDOFConstraint();
		}
	}

	bool PhysicsRigidBody::AllRotationLocked() const
	{
		return m_LockFlags & RigidbodyLockFlags::RigidbodyLock_RotationX && m_LockFlags & RigidbodyLockFlags::RigidbodyLock_RotationY && m_LockFlags & RigidbodyLockFlags::RigidbodyLock_RotationZ;
	}

	void PhysicsRigidBody::SyncTransfrom()
	{
		switch( m_Type )
		{
			case PhysicsRigidBodyType::Dynamic:
			{
				TransformComponent& tc = m_Entity->GetComponent<TransformComponent>();

				tc.Position = GetPosition();

				if( !AllRotationLocked() )
					tc.SetRotation( GetRotation() );
			} break;

			default: break;
		}
	}

}
