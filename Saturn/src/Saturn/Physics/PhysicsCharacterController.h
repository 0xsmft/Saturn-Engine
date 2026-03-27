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

#include "Saturn/Asset/Asset.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

namespace Saturn {

	class PhysicsShape;
	class PhysicsScene;
	class Entity;
	
	class PhysicsCharacterController
	{
	public:
		PhysicsCharacterController( AssetID materialAsset, bool hasGravity, bool crtlMovementInAir, bool crtlRotationInAir );
		~PhysicsCharacterController();

		void CreateController( PhysicsScene* pScene, SharedPtr<Entity> entity, const glm::vec3& rOriginPosition );

		void PreUpdate( Timestep ts );
		void OnUpdate( Timestep ts );

	public:
		void Teleport( const glm::vec3& rPosition );
		void Move( const glm::vec3& rDisplacement );
		void Rotate( const glm::quat& rRotation );
		void Jump( float pwr );

	public:
		[[nodiscard]] bool IsGrounded() const;
		glm::vec3 GetPosition() const;

	private:
		Ref<PhysicsShape> m_Shape;
		JPH::Ref<JPH::CharacterVirtual> m_Controller;

		AssetID m_MaterialID = 0;

		JPH::Vec3 m_Displacement{};
		JPH::Vec3 m_Velocity{};

		bool m_HasGravity = true, m_ControlMovementInAir = false, m_ControlRotationInAir = false;

		float m_JumpPower = 0.0f;
	};

}
