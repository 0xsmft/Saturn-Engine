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

#pragma once

#include "Saturn/Scene/Entity.h"
#include "PhysicsShapes.h"
#include "PhysicsBodyType.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Constraints/SixDOFConstraint.h>

namespace Saturn {

	struct RecastInputGeometryExpData;

	class PhysicsRigidBody : public RefTarget
	{
	public:
		PhysicsRigidBody( SharedPtr<Entity> entity );
		~PhysicsRigidBody();

		void CreateShape();

		// Runtime only!
		void SetShapeTrigger( bool trigger );

		void SetKinematic( bool val );
		void SetMass( float val );
		void SetLinearDrag( float value );
		void SetLinearVelocity( const glm::vec3& rVelocity );
		float GetLinearDrag();
		void ApplyForce( glm::vec3 ForceAmount, ForceMode Type );
		void Rotate( const glm::vec3& rRotation );
		void Rotate( const glm::quat& rRotation );
		void SetPosition( const glm::vec3& rPosition );

		void SyncTransfrom();

		bool IsKinematic() const { return m_Kinematic; }

		glm::vec3 GetPosition();
		glm::vec3 GetRotation();
		glm::mat4 GetTransform();

		glm::vec3 GetLinearVelocity() const;

		void SetLockFlags( RigidbodyLockFlags flags, bool value );
		bool IsFlagSet( RigidbodyLockFlags flags ) const { return ( m_LockFlags & flags ) != 0; }
		RigidbodyLockFlags GetFlags() const { return ( RigidbodyLockFlags )m_LockFlags; }
		
		bool AllRotationLocked() const;

		void SetOnCollisionHit( std::function<void( SharedPtr<Entity> rOther )>&& rrFunc ) { m_OnMeshHit = rrFunc; }
		void SetOnCollisionExit( std::function<void( SharedPtr<Entity> rOther )>&& rrFunc ) { m_OnMeshExit = rrFunc; }

		void OnCollisionHit ( SharedPtr<Entity> rOther ) { m_OnMeshHit( rOther ); }
		void OnCollisionExit( SharedPtr<Entity> rOther ) { m_OnMeshExit( rOther ); }

		SharedPtr<Entity> GetEntity() { return m_Entity; }

		Ref<PhysicsShape> GetShape() { return m_Shape; }

		void ExportRc( RecastInputGeometryExpData& rData, AABB& rNavMeshBounds );

	private:
		void AttachPhysicsShape( PhysicsShapeType type );
		void CreateDOFConstraint();
		void Destroy();

	private:
		JPH::Body* m_pBody = nullptr;
		JPH::Ref<JPH::SixDOFConstraint> m_DOFConstraint;

		SharedPtr<Entity> m_Entity;
		Ref<PhysicsShape> m_Shape;

		bool m_Kinematic = false;
		PhysicsRigidBodyType m_Type = PhysicsRigidBodyType::Dynamic;
		uint32_t m_LockFlags = 0u;

		std::function<void( SharedPtr<Entity> rOther )> m_OnMeshHit;
		std::function<void( SharedPtr<Entity> rOther )> m_OnMeshExit;
	private:
		friend class PhysicsShape;
		friend class PhysicsFoundation;
		friend class PhysicsContact;
		friend class PhysicsControllerContact;
	};

}
