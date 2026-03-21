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
	}

	PhysicsRigidBody::~PhysicsRigidBody()
	{
		Destroy();
	}

	void PhysicsRigidBody::CreateShape()
	{
		const RigidbodyComponent& rb = m_Entity->GetComponent<RigidbodyComponent>();
		const TransformComponent& tc = m_Entity->GetComponent<TransformComponent>();

		m_LockFlags = rb.LockFlags;

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
			SAT_CORE_ASSERT( false );
		}

		// The settings might of changed, so update in case.
		SetKinematic( rb.IsKinematic );

		if( m_Shape->GetType() == PhysicsShapeType::TriangleMesh && rb.IsKinematic )
		{
			m_Type = PhysicsRigidBodyType::Static;
		}

		// Create body after the shape.
		JPH::BodyCreationSettings settings( 
			m_Shape->GetShape(), 
			Auxiliary::GLMToJolt( tc.Position ), 
			Auxiliary::GLMQToJoltQ( glm::normalize( tc.GetRotation() ) ), 
			( JPH::EMotionType ) m_Type, 
			m_Kinematic ? PhysLayerNotMoving : PhysLayerMoving 
		);
//		settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
//		settings.mMassPropertiesOverride.mMass = rb.Mass;
		settings.mIsSensor = m_Shape->IsTrigger();
		
		m_pBody = PhysicsFoundation::Get()->GetBodyInterface()->CreateBody( settings );

		PhysicsFoundation::Get()->GetBodyInterface()->AddBody( m_pBody->GetID(), m_Kinematic ? JPH::EActivation::DontActivate : JPH::EActivation::Activate );

		// Handle locking flags
		CreateDOFConstraint();
	}

	void PhysicsRigidBody::SetShapeTrigger( bool trigger )
	{
		if( m_Shape->IsTrigger() == trigger )
			return;

		m_pBody->SetIsSensor( trigger );
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

		settings.mPosition2 = m_pBody->GetPosition();

		m_DOFConstraint = static_cast<JPH::SixDOFConstraint*>( settings.Create( JPH::Body::sFixedToWorld, *m_pBody ) );

		PhysicsFoundation::Get()->GetPhysicsSystem()->AddConstraint( m_DOFConstraint );
	}

	void PhysicsRigidBody::Destroy()
	{
		if( m_pBody->IsInBroadPhase() )
			PhysicsFoundation::Get()->GetBodyInterface()->RemoveBody( m_pBody->GetID() );
		else
			PhysicsFoundation::Get()->GetBodyInterface()->DeactivateBody( m_pBody->GetID() );

		PhysicsFoundation::Get()->GetPhysicsSystem()->RemoveConstraint( m_DOFConstraint );

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

			case ForceMode::ForceAndTorque:
				PhysicsFoundation::Get()->GetBodyInterface()->AddForceAndTorque( m_pBody->GetID(), Auxiliary::GLMToJolt( ForceAmount ), Auxiliary::GLMToJolt( ForceAmount ) );
				break;

			case ForceMode::Torque:
				PhysicsFoundation::Get()->GetBodyInterface()->AddTorque( m_pBody->GetID(), Auxiliary::GLMToJolt( ForceAmount ) );
				break;

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
		if( flags != m_LockFlags )
		{
			m_LockFlags = flags;

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
		TransformComponent& tc = m_Entity->GetComponent<TransformComponent>();

		tc.Position = GetPosition();

		if( !AllRotationLocked() )
			tc.SetRotation( GetRotation() );
	}

}
