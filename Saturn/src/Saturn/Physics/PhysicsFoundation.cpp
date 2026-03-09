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
#include "PhysicsRigidBody.h"

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/JobSystemThreadPool.h>

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////

	JPH::ValidateResult PhysicsContact::OnContactValidate( const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult )
	{
		return JPH::ValidateResult::AcceptContact;
	}

	void PhysicsContact::OnContactAdded( const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings )
	{
	}

	void PhysicsContact::OnContactPersisted( const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings )
	{
	}

	void PhysicsContact::OnContactRemoved( const JPH::SubShapeIDPair& inSubShapePair )
	{
	}

	//////////////////////////////////////////////////////////////////////////

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

	void PhysicsFoundation::Init()
	{
		JPH::RegisterDefaultAllocator();

		JPH::Factory::sInstance = new JPH::Factory();
		
		JPH::RegisterTypes();
	
		m_pJobSystem = new JPH::JobSystemThreadPool( JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() / 2 );

		m_pTempAllocator = new JPH::TempAllocatorImpl( 10 * 1024 * 1024 );

		m_pPhysicsSystem = new JPH::PhysicsSystem();
		m_pPhysicsSystem->Init( 1024, 0, 1024, 1024, m_BPLayerInterface, m_ObjectVsBPLayerFilter, m_ObjectVsObjectLayerFilter );

		m_pPhysicsSystem->SetContactListener( &m_ContactHandler );
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

}
