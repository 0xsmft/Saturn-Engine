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
#include "AIAgentEntity.h"

#include "Saturn/Core/Random.h"

#include "Saturn/Physics/PhysicsRigidBody.h"

#include "Saturn/NodeEditor/NodeEditorBase.h"
#include "BehaviourTree/BehaviourTree.h"

#include <Detour/DetourNavMeshQuery.h>
#include <glm/gtc/type_ptr.hpp>

namespace Saturn {

	AIAgentEntity::AIAgentEntity()
	{
		AddComponent<StaticMeshComponent>();
		AddComponent<CapsuleColliderComponent>();
//		AddComponent<RigidbodyComponent>().LockFlags = RigidbodyLockFlags::RigidbodyLock_RotationX | RigidbodyLockFlags::RigidbodyLock_RotationY | RigidbodyLockFlags::RigidbodyLock_RotationZ;
	}

	AIAgentEntity::~AIAgentEntity()
	{
	}

	void AIAgentEntity::BeginPlay()
	{
		Super::BeginPlay();

		AssetID id = 12076914912353493827llu;
		m_BehaviourTree = Ref<BehaviourTree>::Create( id );

		auto e = m_Scene->GetAllEntitiesWith<NavigationMeshSpecificationComponent>();
		m_NavBoundsEntity = e[ 0 ].As<NavBoundsEntity>();

		m_BehaviourTree->Initialise( this );
		m_BehaviourTree->FirstEvaluate();
	}

	static float RcRandomFunction() 
	{
		return Random::RandomFloatInRange( 0.0f, 1.0f );
	}

	void AIAgentEntity::OnUpdate( Saturn::Timestep ts )
	{
		Super::OnUpdate( ts );

		m_BehaviourTree->Tick( ts );
	}

	void AIAgentEntity::OnPhysicsUpdate( Saturn::Timestep ts )
	{
		Super::OnPhysicsUpdate( ts );
	}

}
