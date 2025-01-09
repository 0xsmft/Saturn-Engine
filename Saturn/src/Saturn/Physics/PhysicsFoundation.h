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

#include "PhysicsErrorCallbacks.h"

#include "PxPhysicsAPI.h"

namespace Saturn {

	class PhysicsContact : public physx::PxSimulationEventCallback, public RefTarget
	{
	public:
		void onConstraintBreak( physx::PxConstraintInfo* pConstraints, physx::PxU32 Count ) override;
		void onWake( physx::PxActor** ppActors, physx::PxU32 Count ) override;
		void onSleep( physx::PxActor** ppActors, physx::PxU32 Count ) override;
		void onContact( const physx::PxContactPairHeader& rPairHeader, const physx::PxContactPair* pPairs, physx::PxU32 Pairs ) override;
		void onTrigger( physx::PxTriggerPair* pPairs, physx::PxU32 Count ) override;
		void onAdvance( const physx::PxRigidBody* const* pBodyBuffer, const physx::PxTransform* PoseBuffer, const physx::PxU32 Count ) override;
	};

	class PhysicsFoundation
	{
	public:
		static inline PhysicsFoundation& Get() { return *SingletonStorage::GetSingleton<PhysicsFoundation>(); }
	public:
		PhysicsFoundation();
		~PhysicsFoundation();

		void Init();
		void Terminate();

		bool ConnectPVD();
		void DisconnectPVD();

		physx::PxPhysics& GetPhysics() { return *m_Physics; }
		const physx::PxPhysics& GetPhysics() const { return *m_Physics; }

		physx::PxFoundation& GetFoundation() { return *m_Foundation; }
		const physx::PxFoundation& GetFoundation() const { return *m_Foundation; }

		physx::PxDefaultAllocator& GetAllocator() { return m_AllocatorCallback; }
		const physx::PxDefaultAllocator& GetAllocator() const { return m_AllocatorCallback; }

	private:
		physx::PxFoundation*		   m_Foundation = nullptr;
		physx::PxPhysics*			   m_Physics = nullptr;
		physx::PxCooking*			   m_Cooking = nullptr;
		physx::PxPvd*				   m_Pvd = nullptr;
		physx::PxDefaultCpuDispatcher* m_Dispatcher = nullptr;

		physx::PxDefaultAllocator m_AllocatorCallback;

		PhysicsErrorCallback m_ErrorCallback;
		PhysicsAssertCallback m_AssertCallback;
		PhysicsContact m_ContantCallback;
	private:
		friend class PhysicsScene;
		friend class PhysicsCooking;
	};

}