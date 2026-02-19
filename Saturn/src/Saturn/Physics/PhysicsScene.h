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

#include "Saturn/Scene/Scene.h"

#include "PxPhysicsAPI.h"

namespace Saturn {

	struct RigidbodyComponent;

	struct RaycastHitResult
	{
		bool Success = false;
		float Distance = 0.0f;
		SharedPtr<Entity> Hit = nullptr;
		glm::vec3 Position;
	};

	struct RecastInputGeometryExpData;

	class PhysicsScene : public RefTarget
	{
	public:
		PhysicsScene( Ref<Scene> scene );
		~PhysicsScene();

		void CreateScene();
		void Simulate( Timestep ts );

		[[nodiscard]] bool Raycast( const glm::vec3& rOrigin, const glm::vec3& rDirection, float maxDistance, RaycastHitResult* pOut );

		void ExportRc( RecastInputGeometryExpData& rData, AABB& rNavMeshBounds );

		physx::PxControllerManager* GetControllerManager() { return m_ControllerManager; }
		const physx::PxControllerManager* GetControllerManager() const { return m_ControllerManager; }

	private:
		void AddToScene( physx::PxRigidActor& rBody );
		void InitialiseNewBody( SharedPtr<Entity>& rEntity, RigidbodyComponent& rRigidbodyComponent );

	private:
		physx::PxScene* m_PhysicsScene = nullptr;
		physx::PxControllerManager* m_ControllerManager = nullptr;
		Ref<Scene> m_Scene;

	private:
		friend class Scene;
	};
}
