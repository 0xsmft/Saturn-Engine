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

#include "Saturn/Scene/Scene.h"

#include "PxPhysicsAPI.h"

namespace Saturn {

	struct RigidbodyComponent;

	struct RaycastHitResult
	{
		bool Success = false;
		Ref<Entity> Hit = nullptr;
		glm::vec3 Position;
		float Distance = 0.0f;
	};

	class PhysicsScene : public RefTarget
	{
	public:
		PhysicsScene( const Ref<Scene>& rScene );
		~PhysicsScene();

		void CreateScene();
		void Update( Timestep ts );

		[[nodiscard]] bool Raycast( const glm::vec3& Origin, const glm::vec3& Direction, float MaxDistance, RaycastHitResult* pOut );

	private:
		void AddToScene( physx::PxRigidActor& rBody );
		void InitialiseNewBody( Ref<Entity>& rEntity, RigidbodyComponent& rRigidbodyComponent );
	
	private:
		physx::PxScene* m_PhysicsScene;
		Ref<Scene> m_Scene;

	private:
		friend class Scene;
	};
}