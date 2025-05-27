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
#include "PhysicsScene.h"

#include "Saturn/Core/OptickProfiler.h"

#include "Saturn/AI/Navigation/RecastInputGeometry.h"

#include "Saturn/Scene/Components.h"
#include "Saturn/Scene/Entity.h"

#include "PhysicsFoundation.h"
#include "PhysicsRigidBody.h"
#include "PhysicsAuxiliary.h"

namespace Saturn {

	PhysicsScene::PhysicsScene( Ref<Scene> scene )
		: m_Scene( scene )
	{
		CreateScene();
	}

	PhysicsScene::~PhysicsScene()
	{
		PhysicsFoundation::Get().DisconnectPVD();

		auto rView = m_Scene->GetAllEntitiesWith<RigidbodyComponent>();

		for( auto& rEntity : rView )
		{
			auto& rb = rEntity->GetComponent<RigidbodyComponent>();

			delete rb.Rigidbody;
			rb.Rigidbody = nullptr;
		}

		m_Scene = nullptr;

		PHYSX_TERMINATE_ITEM( m_PhysicsScene );
	}

	static physx::PxFilterFlags CollisionFilterShader(
		physx::PxFilterObjectAttributes Attributes0, physx::PxFilterData FilterData0,
		physx::PxFilterObjectAttributes Attributes1, physx::PxFilterData FilterData1,
		physx::PxPairFlags& rPairFlags, const void* pConstantBlock, physx::PxU32 ConstantBlockSize )
	{
		if( physx::PxFilterObjectIsTrigger( Attributes0 ) || physx::PxFilterObjectIsTrigger( Attributes1 ) )
		{
			rPairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
			return physx::PxFilterFlag::eDEFAULT;
		}

		rPairFlags = physx::PxPairFlag::eCONTACT_DEFAULT;
		rPairFlags |= physx::PxPairFlag::eDETECT_CCD_CONTACT;

		if( ( FilterData0.word0 & FilterData1.word1 ) || ( FilterData1.word0 & FilterData0.word1 ) )
		{
			rPairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_FOUND;
			rPairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_LOST;
			rPairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_CCD;

			return physx::PxFilterFlag::eDEFAULT;
		}

		return physx::PxFilterFlag::eDEFAULT;
	}

	void PhysicsScene::CreateScene()
	{
		physx::PxSceneDesc SceneDesc( PhysicsFoundation::Get().m_Physics->getTolerancesScale() );
		SceneDesc.gravity = physx::PxVec3( 0.0f, -9.81f, 0.0f );

		SceneDesc.cpuDispatcher = PhysicsFoundation::Get().m_Dispatcher;
		SceneDesc.simulationEventCallback = &PhysicsFoundation::Get().m_ContactCallback;
		SceneDesc.filterShader = CollisionFilterShader;

		SceneDesc.broadPhaseType = physx::PxBroadPhaseType::eABP;
		SceneDesc.frictionType = physx::PxFrictionType::ePATCH;
		SceneDesc.flags = physx::PxSceneFlag::eENABLE_CCD;

		m_PhysicsScene = PhysicsFoundation::Get().GetPhysics().createScene( SceneDesc );
		PhysicsFoundation::Get().ConnectPVD();

		m_PhysicsScene->setVisualizationParameter( physx::PxVisualizationParameter::eSCALE, 1.0f );
		m_PhysicsScene->setVisualizationParameter( physx::PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f );

		// Add all current bodies to the scene.

		auto rView = m_Scene->GetAllEntitiesWith<RigidbodyComponent>();

		for( auto& rEntity : rView )
		{
			auto& rb = rEntity->GetComponent<RigidbodyComponent>();

			rb.Rigidbody = new PhysicsRigidBody( rEntity );
			rb.Rigidbody->CreateShape();

			// Maybe we could use addActors?
			AddToScene( rb.Rigidbody->GetActor() );
		}
	}

	void PhysicsScene::Update( Timestep ts )
	{
		SAT_PF_EVENT();

		float FixedTimestep = 1.0f / 100.0f;

		m_PhysicsScene->simulate( FixedTimestep );
		m_PhysicsScene->fetchResults( true );
	}

	bool PhysicsScene::Raycast( const glm::vec3& Origin, const glm::vec3& Direction, float MaxDistance, RaycastHitResult* pOut )
	{
		RaycastHitResult Hit = {};

		physx::PxRaycastBuffer PhysXOutHit = {};

		bool success = m_PhysicsScene->raycast( Auxiliary::GLMToPx( Origin ), Auxiliary::GLMToPx( glm::normalize( Direction ) ), MaxDistance, PhysXOutHit );

		Hit.Success = success;
		if( Hit.Success )
		{
			physx::PxRaycastHit& Target = PhysXOutHit.block;

			PhysicsRigidBody* body = ( PhysicsRigidBody* ) Target.actor->userData;
			
			Hit.Hit = body->GetEntity();
			Hit.Distance = Target.distance;
			Hit.Position = Auxiliary::PxToGLM( Target.position );
		}

		*pOut = Hit;
		return success;
	}

	void PhysicsScene::ExportRc( RecastInputGeometryExpData& rData, AABB& rNavMeshBounds )
	{
		auto rView = m_Scene->GetAllEntitiesWith<RigidbodyComponent>();
		for( auto& rEntity : rView )
		{
			auto& rb = rEntity->GetComponent<RigidbodyComponent>();
			
			rb.Rigidbody->ExportRc( rData, rNavMeshBounds );
		}
	}

	void PhysicsScene::AddToScene( physx::PxRigidActor& rBody )
	{
		m_PhysicsScene->addActor( rBody );
	}

	void PhysicsScene::InitialiseNewBody( Ref<Entity>& rEntity, RigidbodyComponent& rRigidbodyComponent )
	{
		rRigidbodyComponent.Rigidbody = new PhysicsRigidBody( rEntity );
		rRigidbodyComponent.Rigidbody->CreateShape();

		AddToScene( rRigidbodyComponent.Rigidbody->GetActor() );
	}
}