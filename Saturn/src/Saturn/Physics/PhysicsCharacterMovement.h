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

#include "Saturn/Asset/Asset.h"
#include "PxPhysicsAPI.h"

namespace Saturn {

	class PhysicsScene;

	enum PhysicsControllerCollisionFlag
	{
		PhysControllerCollision_None  = 0,
		PhysControllerCollision_Sides = BIT( 0 ),
		PhysControllerCollision_Up    = BIT( 1 ),
		PhysControllerCollision_Down  = BIT( 2 )
	};
	
	class PhysicsCharacterMovement
	{
	public:
		PhysicsCharacterMovement( AssetID materialAsset, float height, float radius );
		~PhysicsCharacterMovement();

		void CreateController( PhysicsScene* pScene, const glm::vec3& rOriginPosition );

		void OnUpdate( Timestep ts );

	public:
		void Teleport( const glm::vec3& rPosition );
		void Move( const glm::vec3& rDisplacement );
		void Jump( float pwr );

	public:
		physx::PxController* GetController() { return m_pController; }
		const physx::PxController* GetController() const { return m_pController; }

		PhysicsControllerCollisionFlag GetFlags() const { return m_CollisionFlags; }

		[[nodiscard]] bool IsGrounded() const;

		glm::vec3 GetPosition() const;

	private:
		physx::PxController* m_pController = nullptr;
		AssetID m_MaterialID = 0;

		glm::vec3 m_Displacement{};
		float m_Height = 0.0f, m_Radius = 0.0f;
		PhysicsControllerCollisionFlag m_CollisionFlags = PhysControllerCollision_None;

		float m_DownSpeed = 0.0f;
		float m_Gravity = {};
	};

}
