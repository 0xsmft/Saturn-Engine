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
#include "PhysicsFoundation.h"

#include "PhysicsAuxiliary.h"
#include "PhysicsRigidBody.h"

namespace Saturn {

	void PhysicsContact::onConstraintBreak( physx::PxConstraintInfo* pConstraints, physx::PxU32 Count )
	{
	}

	void PhysicsContact::onWake( physx::PxActor** ppActors, physx::PxU32 Count )
	{
	}

	void PhysicsContact::onSleep( physx::PxActor** ppActors, physx::PxU32 Count )
	{
	}

	void PhysicsContact::onTrigger( physx::PxTriggerPair* pPairs, physx::PxU32 Count )
	{
	}

	void PhysicsContact::onAdvance( const physx::PxRigidBody* const* pBodyBuffer, const physx::PxTransform* PoseBuffer, const physx::PxU32 Count )
	{
	}

	void PhysicsContact::onContact( const physx::PxContactPairHeader& rPairHeader, const physx::PxContactPair* pPairs, physx::PxU32 Pairs )
	{
		PhysicsRigidBody* A = ( PhysicsRigidBody* ) rPairHeader.actors[ 0 ]->userData;
		PhysicsRigidBody* B = ( PhysicsRigidBody* ) rPairHeader.actors[ 1 ]->userData;

		if( !A || !B )
			return;

		auto callCollisonBeginMethod = []( PhysicsRigidBody* A, PhysicsRigidBody* B )
		{
			if( A->m_OnMeshHit )
				A->OnCollisionHit( B->GetEntity() );
			
			if( B->m_OnMeshHit )
				B->OnCollisionHit( A->GetEntity() );
		};

		auto callCollisonExitMethod = []( PhysicsRigidBody* A, PhysicsRigidBody* B )
		{
			if( A->m_OnMeshExit )
				A->OnCollisionExit( B->GetEntity() );

			if( B->m_OnMeshExit )
				B->OnCollisionExit( A->GetEntity() );
		};

		if( pPairs->flags == physx::PxContactPairFlag::eACTOR_PAIR_HAS_FIRST_TOUCH ) 
		{
			callCollisonBeginMethod( A, B );
		} 
		else if( pPairs->flags == physx::PxContactPairFlag::eACTOR_PAIR_LOST_TOUCH ) 
		{
			callCollisonExitMethod( A, B );
		}
	}

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
		physx::PxTolerancesScale Scale;
		Scale.length = 10.0f;

		m_Foundation = PxCreateFoundation( PX_PHYSICS_VERSION, m_AllocatorCallback, m_ErrorCallback );

#if defined( SAT_DEBUG ) || defined( SAT_RELEASE )
		m_Pvd = PxCreatePvd( *m_Foundation );
		m_Physics = PxCreatePhysics( PX_PHYSICS_VERSION, *m_Foundation, Scale, true, m_Pvd );
#else
		m_Physics = PxCreatePhysics( PX_PHYSICS_VERSION, *m_Foundation, Scale, true );
#endif
		m_Cooking = PxCreateCooking( PX_PHYSICS_VERSION, *m_Foundation, Scale );

		m_Dispatcher = physx::PxDefaultCpuDispatcherCreate( std::thread::hardware_concurrency() / 2 );

		physx::PxSetAssertHandler( m_AssertCallback );
	}

	void PhysicsFoundation::Terminate()
	{
#if defined( SAT_DEBUG ) || defined( SAT_RELEASE )
		m_Pvd->disconnect();
#endif

		PHYSX_TERMINATE_ITEM( m_Dispatcher );
		PHYSX_TERMINATE_ITEM( m_Cooking );
		PHYSX_TERMINATE_ITEM( m_Physics );
		PHYSX_TERMINATE_ITEM( m_Pvd );
		PHYSX_TERMINATE_ITEM( m_Foundation );
	}

	bool PhysicsFoundation::ConnectPVD()
	{
#if defined( SAT_DEBUG ) || defined( SAT_RELEASE )
		physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate( "127.0.0.1", 5425, 1 );
		return m_Pvd->connect( *transport, physx::PxPvdInstrumentationFlag::eALL );
#endif
		return true;
	}

	void PhysicsFoundation::DisconnectPVD()
	{
#if defined( SAT_DEBUG ) || defined( SAT_RELEASE )
		m_Pvd->disconnect();
#endif
	}

}