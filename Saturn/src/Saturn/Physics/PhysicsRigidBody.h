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

#pragma once

#include "Saturn/Scene/Entity.h"
#include "PhysicsShapes.h"

#include "PxPhysicsAPI.h"

namespace Saturn {

	class PhysicsRigidBody : public RefTarget
	{
	public:
		PhysicsRigidBody( Ref<Entity> entity );
		~PhysicsRigidBody();

		void CreateShape();

		void SetKinematic( bool val );
		void SetMass( float val );
		void SetLinearDrag( float value );
		float GetLinearDrag();
		void ApplyForce( glm::vec3 ForceAmount, ForceMode Type );
		void Rotate( const glm::vec3& rRotation );
		void Rotate( const glm::quat& rRotation );

		void SyncTransfrom();

		bool IsKiniematic() { return m_Kinematic; }

		glm::vec3 GetPosition();
		glm::vec3 GetRotation();
		glm::mat4 GetTransform();

		physx::PxRigidActor& GetActor() { return *m_Actor; }
		const physx::PxRigidActor& GetActor() const { return *m_Actor; }

		void SetLockFlags( RigidbodyLockFlags flags, bool value );
		bool IsFlagSet( RigidbodyLockFlags flags ) { return ( m_LockFlags & flags ) != 0; }
		RigidbodyLockFlags GetFlags() { return ( RigidbodyLockFlags )m_LockFlags; }
		
		bool AllRotationLocked();

		void SetOnCollisionHit( std::function<void( Ref<Entity> rOther )>&& rrFunc ) { m_OnMeshHit = rrFunc; }
		void SetOnCollisionExit( std::function<void( Ref<Entity> rOther )>&& rrFunc ) { m_OnMeshExit = rrFunc; }

		void OnCollisionHit ( Ref<Entity> rOther ) { m_OnMeshHit( rOther ); }
		void OnCollisionExit( Ref<Entity> rOther ) { m_OnMeshExit( rOther ); }

		Ref<Entity> GetEntity() { return m_Entity; }

	private:
		void AttachPhysicsShape( ShapeType type );
		void Destroy();
	private:
		physx::PxRigidActor* m_Actor = nullptr;
		Ref<Entity> m_Entity;

		bool m_Kinematic = false;

		Ref<PhysicsShape> m_Shape;

		uint32_t m_LockFlags;

		std::function<void( Ref<Entity> rOther )> m_OnMeshHit;
		std::function<void( Ref<Entity> rOther )> m_OnMeshExit;
	private:
		friend class PhysicsShape;
		friend class PhysicsFoundation;
		friend class PhysicsContact;
	};

}