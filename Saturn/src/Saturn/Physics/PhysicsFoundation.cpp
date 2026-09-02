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
#include "PhysicsFoundation.h"

#include "PhysicsAuxiliary.h"
#include "PhysicsMaterialAsset.h"

#include "Saturn/Scene/Scene.h"

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/JobSystemThreadPool.h>

#if defined(SAT_PLATFORM_LINUX)
#include <stdarg.h> // For va_start et al.
#endif

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////

	static void PhysGetFrictionAndRestitution( const JPH::Body& inBody1, const JPH::SubShapeID& rSubShapeId, float& rOutFriction, float& rOutRestitution )
	{
		const JPH::PhysicsMaterial* pMaterial = inBody1.GetShape()->GetMaterial( rSubShapeId );

		if( pMaterial == JPH::PhysicsMaterial::sDefault )
		{
			rOutFriction = inBody1.GetFriction();
			rOutFriction = inBody1.GetRestitution();
		}
		else
		{
			const PhysicsInternalMaterial* pSaturnMaterial = static_cast< const PhysicsInternalMaterial* >( pMaterial );
			rOutFriction = pSaturnMaterial->GetFriction();
			rOutRestitution = pSaturnMaterial->GetRestitution();
		}
	}

	static void PhysAppendPhysMat( const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings )
	{
		float f1, f2, r1, r2;
		PhysGetFrictionAndRestitution( inBody1, inManifold.mSubShapeID1, f1, r1 );
		PhysGetFrictionAndRestitution( inBody2, inManifold.mSubShapeID2, f2, r2 );

		ioSettings.mCombinedFriction = f1 * f2;
		ioSettings.mCombinedRestitution = glm::max( r1, r2 );
	}

	JPH::ValidateResult PhysicsContact::OnContactValidate( const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult )
	{
		return JPH::ValidateResult::AcceptContact;
	}

	void PhysicsContact::OnContactAdded( const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings )
	{
		SharedPtr<Entity> entityA = g_ActiveScene->FindEntityByHandle( ( entt::entity ) inBody1.GetUserData() );
		SharedPtr<Entity> entityB = g_ActiveScene->FindEntityByHandle( ( entt::entity ) inBody2.GetUserData() );

		if( entityA && entityB )
		{
			auto& rEventA = m_PendingEvents.emplace_back();

			// Handle entity A
			rEventA.Type = inBody2.IsSensor() ? PhysicsContactType::HitTrigger : PhysicsContactType::Hit;
			rEventA.pA = entityA.Get();
			rEventA.pB = entityB.Get();

			auto& rEventB = m_PendingEvents.emplace_back();

			// Handle entity B
			rEventB.Type = inBody1.IsSensor() ? PhysicsContactType::HitTrigger : PhysicsContactType::Hit;
			rEventB.pA = entityB.Get();
			rEventB.pB = entityA.Get();
		}

		PhysAppendPhysMat( inBody1, inBody2, inManifold, ioSettings );
	}

	void PhysicsContact::OnContactPersisted( const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings )
	{
		PhysAppendPhysMat( inBody1, inBody2, inManifold, ioSettings );
	}

	void PhysicsContact::OnContactRemoved( const JPH::SubShapeIDPair& inSubShapePair )
	{
		auto* pBodyA = m_pBodyInterface->TryGetBody( inSubShapePair.GetBody1ID() );
		auto* pBodyB = m_pBodyInterface->TryGetBody( inSubShapePair.GetBody2ID() );

		if( !pBodyA || !pBodyB )
			return;

		SharedPtr<Entity> entityA = g_ActiveScene->FindEntityByHandle( ( entt::entity ) pBodyA->GetUserData() );
		SharedPtr<Entity> entityB = g_ActiveScene->FindEntityByHandle( ( entt::entity ) pBodyA->GetUserData() );

		if( entityA && entityB )
		{
			auto& rEventA = m_PendingEvents.emplace_back();

			// Handle entity A
			rEventA.Type = pBodyB->IsSensor() ? PhysicsContactType::LeaveTrigger : PhysicsContactType::Leave;
			rEventA.pA = entityA.Get();
			rEventA.pB = entityB.Get();

			auto& rEventB = m_PendingEvents.emplace_back();

			// Handle entity B
			rEventB.Type = pBodyA->IsSensor() ? PhysicsContactType::LeaveTrigger : PhysicsContactType::Leave;
			rEventB.pA = entityB.Get();
			rEventB.pB = entityA.Get();
		}
	}
	void PhysicsContact::DispatchAllContactEvents()
	{
		for( const auto& rAwatingEvent : m_PendingEvents )
		{
			switch( rAwatingEvent.Type )
			{
				case PhysicsContactType::Hit:
				case PhysicsContactType::HitTrigger:
				{
					rAwatingEvent.pA->OnEntityHit( rAwatingEvent.pB, rAwatingEvent.Type == PhysicsContactType::HitTrigger );
					
					rAwatingEvent.pB->OnEntityHit( rAwatingEvent.pA, rAwatingEvent.Type == PhysicsContactType::HitTrigger );
				} break;

				case PhysicsContactType::Leave:
				case PhysicsContactType::LeaveTrigger:
				{
					rAwatingEvent.pA->OnEntityLeave( rAwatingEvent.pB, rAwatingEvent.Type == PhysicsContactType::LeaveTrigger );

					rAwatingEvent.pB->OnEntityLeave( rAwatingEvent.pA, rAwatingEvent.Type == PhysicsContactType::LeaveTrigger );
				} break;

				default:
					break;
			}
		}

		IgnoreAll();
	}

	void PhysicsContact::IgnoreAll()
	{
		m_PendingEvents.clear();
	}

	//////////////////////////////////////////////////////////////////////////
	// PhysicsCharacterContact

	void PhysicsCharacterContact::OnContactAdded( const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2, JPH::RVec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::CharacterContactSettings& ioSettings )
	{
		auto* pBodyB = m_pBodyInterface->TryGetBody( inBodyID2 );

		if( !pBodyB )
			return;

		SharedPtr<Entity> entityA = g_ActiveScene->FindEntityByHandle( ( entt::entity ) inCharacter->GetUserData() );
		SharedPtr<Entity> entityB = g_ActiveScene->FindEntityByHandle( ( entt::entity ) pBodyB->GetUserData() );

		if( !entityA || !entityB )
			return;

		if( entityA && entityB )
		{
			auto& rEventA = m_PendingEvents.emplace_back();

			// Handle entity A
			rEventA.Type = PhysicsContactType::Hit;
			rEventA.pA = entityA.Get();
			rEventA.pB = entityB.Get();

			auto& rEventB = m_PendingEvents.emplace_back();

			// Handle entity B
			rEventB.Type = PhysicsContactType::Hit;
			rEventB.pA = entityB.Get();
			rEventB.pB = entityA.Get();
		}
	}

	void PhysicsCharacterContact::OnContactRemoved(
		const JPH::CharacterVirtual* inCharacter,
		const JPH::BodyID& inBodyID2,
		const JPH::SubShapeID& inSubShapeID2 )
	{
		auto* pBodyB = m_pBodyInterface->TryGetBody( inBodyID2 );

		if( !pBodyB )
			return;

		SharedPtr<Entity> entityA = g_ActiveScene->FindEntityByHandle( ( entt::entity ) inCharacter->GetUserData() );
		SharedPtr<Entity> entityB = g_ActiveScene->FindEntityByHandle( ( entt::entity ) pBodyB->GetUserData() );

		if( !entityA || !entityB )
			return;

		if( entityA && entityB )
		{
			auto& rEventA = m_PendingEvents.emplace_back();

			// Handle entity A
			rEventA.Type = PhysicsContactType::Leave;
			rEventA.pA = entityA.Get();
			rEventA.pB = entityB.Get();

			auto& rEventB = m_PendingEvents.emplace_back();

			// Handle entity B
			rEventB.Type = PhysicsContactType::Leave;
			rEventB.pA = entityB.Get();
			rEventB.pB = entityA.Get();
		}
	}

	void PhysicsCharacterContact::DispatchAllContactEvents()
	{
		for( const auto& rAwatingEvent : m_PendingEvents )
		{
			switch( rAwatingEvent.Type )
			{
				case PhysicsContactType::Hit:
				case PhysicsContactType::HitTrigger:
				{
					rAwatingEvent.pA->OnEntityHit( rAwatingEvent.pB, rAwatingEvent.Type == PhysicsContactType::HitTrigger );

					rAwatingEvent.pB->OnEntityHit( rAwatingEvent.pA, rAwatingEvent.Type == PhysicsContactType::HitTrigger );
				} break;

				case PhysicsContactType::Leave:
				case PhysicsContactType::LeaveTrigger:
				{
					rAwatingEvent.pA->OnEntityLeave( rAwatingEvent.pB, rAwatingEvent.Type == PhysicsContactType::LeaveTrigger );

					rAwatingEvent.pB->OnEntityLeave( rAwatingEvent.pA, rAwatingEvent.Type == PhysicsContactType::LeaveTrigger );
				} break;

				default:
					break;
			}
		}

		IgnoreAll();
	}

	void PhysicsCharacterContact::IgnoreAll()
	{
		m_PendingEvents.clear();
	}

	//////////////////////////////////////////////////////////////////////////
	// JoltBodyActivationListener

	void JoltBodyActivationListener::OnBodyActivated( const JPH::BodyID& inBodyID, uint64_t inBodyUserData )
	{

	}

	void JoltBodyActivationListener::OnBodyDeactivated( const JPH::BodyID& inBodyID, uint64_t inBodyUserData )
	{

	}

	//////////////////////////////////////////////////////////////////////////

	bool JoltObjectVsBroadPhaseLayerFilter::ShouldCollide( JPH::ObjectLayer layer1, JPH::BroadPhaseLayer layer2 ) const
	{
		switch( layer1 )
		{
			case PhysLayerNotMoving:
				return layer2 == PhysBPLayerMoving;

			case PhysLayerMoving:
				return true;

			default:
				return false;
		}
	}

	//////////////////////////////////////////////////////////////////////////

	bool JoltObjectLayerPairFilter::ShouldCollide( JPH::ObjectLayer inLayer1, JPH::ObjectLayer inLayer2 ) const
	{
		switch( inLayer1 )
		{
			case PhysLayerNotMoving:
				return inLayer2 == PhysLayerMoving;

			case PhysLayerMoving:
				return true;

			default:
				return false;
		}
	}

	//////////////////////////////////////////////////////////////////////////

	JoltBPLayerInterface::JoltBPLayerInterface()
	{
		m_ObjectToBroadPhase[ PhysLayerMoving ] = PhysBPLayerMoving;
		m_ObjectToBroadPhase[ PhysLayerNotMoving ] = PhysBPLayerNotMoving;
	}

	JoltBPLayerInterface::~JoltBPLayerInterface()
	{
	}

	uint32_t JoltBPLayerInterface::GetNumBroadPhaseLayers() const
	{
		return 2u;
	}

	JPH::BroadPhaseLayer JoltBPLayerInterface::GetBroadPhaseLayer( JPH::ObjectLayer inLayer ) const
	{
		return m_ObjectToBroadPhase[ inLayer ];
	}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	const char* JoltBPLayerInterface::GetBroadPhaseLayerName( JPH::BroadPhaseLayer inLayer ) const
	{
		return "<NULL>";
	}
#endif

	//////////////////////////////////////////////////////////////////////////

	PhysicsFoundation::PhysicsFoundation()
	{
		SingletonStorage::AddSingleton<PhysicsFoundation>( this );
	}

	PhysicsFoundation::~PhysicsFoundation()
	{
		Terminate();
	}

	static void JphTrace( const char* inFMT, ... )
	{
		// Format the message
		va_list list;
		va_start( list, inFMT );
		char buffer[ 1024 ];
		vsnprintf( buffer, sizeof( buffer ), inFMT, list );
		va_end( list );

		SAT_CORE_INFO( "Jolt: {0}", buffer );
	}

	void PhysicsFoundation::Init()
	{
		JPH::RegisterDefaultAllocator();

		JPH::Trace = JphTrace;

		JPH::Factory::sInstance = new JPH::Factory();
		
		JPH::RegisterTypes();
	
		m_pJobSystem = new JPH::JobSystemThreadPool( JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() / 2 );

		m_pTempAllocator = new JPH::TempAllocatorImpl( 300 * 1024 * 1024 );

		m_pPhysicsSystem = new JPH::PhysicsSystem();
		m_pPhysicsSystem->Init( 1024, 0, 1024, 1024, m_BPLayerInterface, m_ObjectVsBPLayerFilter, m_ObjectVsObjectLayerFilter );

		m_ContactHandler = std::make_shared<PhysicsContact>( &m_pPhysicsSystem->GetBodyLockInterfaceNoLock() );
		m_CharacterContactHandler = std::make_shared<PhysicsCharacterContact>( &m_pPhysicsSystem->GetBodyLockInterfaceNoLock() );

		m_pPhysicsSystem->SetContactListener( m_ContactHandler.get() );
		m_pPhysicsSystem->SetBodyActivationListener( &m_BodyActivationListener );

		m_pBodyInterface = &m_pPhysicsSystem->GetBodyInterface();
	}

	void PhysicsFoundation::Terminate()
	{
		SAT_JPH_TERMINATE_ITEM( m_pPhysicsSystem );
		SAT_JPH_TERMINATE_ITEM( m_pTempAllocator );
		SAT_JPH_TERMINATE_ITEM( m_pJobSystem );

		JPH::UnregisterTypes();
	
		SAT_JPH_TERMINATE_ITEM( JPH::Factory::sInstance );
	}

	const JPH::NarrowPhaseQuery& PhysicsFoundation::GetNarrowPhaseQuery() const
	{
		SAT_CORE_ASSERT( m_pPhysicsSystem, "GetNarrowPhaseQuery called while there is no physics system created!" );

		return m_pPhysicsSystem->GetNarrowPhaseQuery();
	}

}
