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
#include "PhysicsScene.h"

#include "Saturn/Core/Profiler.h"

#include "Saturn/AI/Navigation/RecastInputGeometry.h"

#include "Saturn/Scene/Components.h"
#include "Saturn/Scene/Entity.h"

#include "PhysicsFoundation.h"
#include "PhysicsRigidBody.h"
#include "PhysicsCharacterMovement.h"
#include "PhysicsAuxiliary.h"

namespace Saturn {

	PhysicsScene::PhysicsScene( Ref<Scene> scene )
		: m_Scene( scene )
	{
		CreateScene();
	}

	PhysicsScene::~PhysicsScene()
	{
		PhysicsFoundation::Get()->DisconnectPVD();

		const auto view = m_Scene->GetAllEntitiesWith<RigidbodyComponent>();
		for( auto& rEntity : view )
		{
			auto& rb = rEntity->GetComponent<RigidbodyComponent>();

			delete rb.Rigidbody;
			rb.Rigidbody = nullptr;
		}

		m_Scene = nullptr;

		m_ControllerManager->purgeControllers();

		PHYSX_TERMINATE_ITEM( m_ControllerManager );
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

		rPairFlags = physx::PxPairFlag::eCONTACT_DEFAULT | physx::PxPairFlag::eDETECT_CCD_CONTACT | physx::PxPairFlag::eNOTIFY_CONTACT_POINTS;

		if( ( FilterData0.word0 & FilterData1.word1 ) || ( FilterData1.word0 & FilterData0.word1 ) )
		{
			rPairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_FOUND | physx::PxPairFlag::eNOTIFY_TOUCH_LOST | physx::PxPairFlag::eNOTIFY_TOUCH_CCD;
		}

		return physx::PxFilterFlag::eDEFAULT;
	}

	void PhysicsScene::CreateScene()
	{
		physx::PxSceneDesc sceneDesc( PhysicsFoundation::Get()->m_Physics->getTolerancesScale() );
		sceneDesc.gravity = physx::PxVec3( 0.0f, -9.81f, 0.0f );

		sceneDesc.cpuDispatcher = PhysicsFoundation::Get()->m_Dispatcher;
		sceneDesc.simulationEventCallback = &PhysicsFoundation::Get()->m_ContactCallback;
		sceneDesc.filterShader = CollisionFilterShader;
		sceneDesc.kineKineFilteringMode = physx::PxPairFilteringMode::eSUPPRESS;
		sceneDesc.frictionType = physx::PxFrictionType::ePATCH;
		sceneDesc.flags = physx::PxSceneFlag::eENABLE_CCD;

		m_PhysicsScene = PhysicsFoundation::Get()->GetPhysics().createScene( sceneDesc );
		PhysicsFoundation::Get()->ConnectPVD();

		m_PhysicsScene->setVisualizationParameter( physx::PxVisualizationParameter::eSCALE, 1.0f );
		m_PhysicsScene->setVisualizationParameter( physx::PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f );
		m_PhysicsScene->setVisualizationParameter( physx::PxVisualizationParameter::eCONTACT_POINT, 1.0f );

		m_ControllerManager = PxCreateControllerManager( *m_PhysicsScene );

		// Add controllers.
		auto controllerView = m_Scene->GetAllEntitiesWith<CharacterMovementComponent>();
		for( auto& rEntity : controllerView )
		{
			AddNewController( rEntity );
		}
	
		// Add all current bodies to the scene.
		auto rigidbodyView = m_Scene->GetAllEntitiesWith<RigidbodyComponent>();
		for( auto& rEntity : rigidbodyView )
		{
			auto& rb = rEntity->GetComponent<RigidbodyComponent>();
			InitialiseNewBody( rEntity, rb );
		}

		// Set user data
		// TEMP: Hack
		for( auto& rEntity : controllerView )
		{
			auto& rMovement = rEntity->GetComponent<CharacterMovementComponent>();
			rMovement.CharacterMovement->GetController()->setUserData( rEntity->GetComponent<RigidbodyComponent>().Rigidbody );
		}
	}

	void PhysicsScene::Simulate( Timestep ts )
	{
		SAT_PF_EVENT();

		constexpr float FIXED_TIMESTEP = 1.0f / 100.0f;
		m_PhysicsScene->simulate( FIXED_TIMESTEP );
		m_PhysicsScene->fetchResults( true );
	}

	bool PhysicsScene::Raycast( const glm::vec3& rOrigin, const glm::vec3& rDirection, float maxDistance, RaycastHitResult* pOut )
	{
		RaycastHitResult hit = {};

		physx::PxRaycastBuffer physXOutHit = {};

		const bool success = m_PhysicsScene->raycast( Auxiliary::GLMToPx( rOrigin ), Auxiliary::GLMToPx( glm::normalize( rDirection ) ), maxDistance, physXOutHit );

		hit.Success = success;
		if( hit.Success )
		{
			const physx::PxRaycastHit& rTarget = physXOutHit.block;

			PhysicsRigidBody* pBody = ( PhysicsRigidBody* ) rTarget.actor->userData;

			// pBody should not be null as the user data should always be set.
			// So if this assert is hit, then the rigid body is not set up correctly.
			SAT_CORE_ASSERT( pBody );

			hit.Hit = pBody->GetEntity();
			hit.Distance = rTarget.distance;
			hit.Position = Auxiliary::PxToGLM( rTarget.position );
		}

		*pOut = hit;
		return success;
	}

	void PhysicsScene::ExportRc( RecastInputGeometryExpData& rData, AABB& rNavMeshBounds )
	{
		auto view = m_Scene->GetAllEntitiesWith<RigidbodyComponent>();
		for( auto& rEntity : view )
		{
			auto& rb = rEntity->GetComponent<RigidbodyComponent>();
			rb.Rigidbody->ExportRc( rData, rNavMeshBounds );
		}
	}

	void PhysicsScene::AddToScene( physx::PxRigidActor& rBody )
	{
		m_PhysicsScene->addActor( rBody );
	}

	void PhysicsScene::InitialiseNewBody( SharedPtr<Entity>& rEntity, RigidbodyComponent& rRigidbodyComponent )
	{
		// Bad, this shouldn't be true and should be an assert.
		if( rRigidbodyComponent.Rigidbody )
		{
			delete rRigidbodyComponent.Rigidbody;
			SAT_CORE_WARN( "A rigidbody already exists in this component! Removing it and creating a new one!" );
		}

		rRigidbodyComponent.Rigidbody = new PhysicsRigidBody( rEntity );
		
		if( rEntity->HasComponent<CharacterMovementComponent>() )
		{
			auto& pController = rEntity->GetComponent<CharacterMovementComponent>().CharacterMovement;
			if( pController == nullptr )
			{
				AddNewController( rEntity );
			}
		}

		rRigidbodyComponent.Rigidbody->CreateShape();

		AddToScene( rRigidbodyComponent.Rigidbody->GetActor() );
	}

	void PhysicsScene::AddNewController( SharedPtr<Entity>& rEntity )
	{
		auto& rb = rEntity->GetComponent<RigidbodyComponent>();
		auto& movementComp = rEntity->GetComponent<CharacterMovementComponent>();

		// Bad, this shouldn't be true and should be an assert.
		if( movementComp.CharacterMovement )
		{
			delete movementComp.CharacterMovement;
			SAT_CORE_WARN( "A movement controller already exists in this component! Removing and creating a new one." );
		}

		movementComp.CharacterMovement = new PhysicsCharacterMovement( rb.MaterialAssetID, movementComp.Height, movementComp.Radius );
		movementComp.CharacterMovement->CreateController( this, rEntity, rEntity->GetLocalPosition() );
	}

}
