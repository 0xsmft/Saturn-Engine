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

#include "PhysicsCooking.h"
#include "PhysicsErrorCallbacks.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

namespace Saturn {

	enum class PhysicsContactType : uint8_t
	{
		Unknown,
		Hit,
		HitTrigger,
		Leave,
		LeaveTrigger
	};

	struct PhysicsContactEventInfo
	{
		Entity* pA = nullptr;
		Entity* pB = nullptr;
		PhysicsContactType Type = PhysicsContactType::Unknown;
	};

	class PhysicsCharacterContact : public JPH::CharacterContactListener
	{
	public:
		PhysicsCharacterContact( const JPH::BodyLockInterfaceNoLock* pBody )
			: m_pBodyInterface( pBody )
		{
		}

		virtual ~PhysicsCharacterContact() = default;

	public:
		virtual void OnContactAdded( 
			const JPH::CharacterVirtual* inCharacter, 
			const JPH::BodyID& inBodyID2, 
			const JPH::SubShapeID& inSubShapeID2, 
			JPH::RVec3Arg inContactPosition, 
			JPH::Vec3Arg inContactNormal, 
			JPH::CharacterContactSettings& ioSettings 
		) override;
	
		virtual void OnContactRemoved( 
			const JPH::CharacterVirtual* inCharacter, 
			const JPH::BodyID& inBodyID2, 
			const JPH::SubShapeID& inSubShapeID2 
		) override;

	public:
		void DispatchAllContactEvents();
		void IgnoreAll();

	private:
		std::vector<PhysicsContactEventInfo> m_PendingEvents;
		const JPH::BodyLockInterfaceNoLock* m_pBodyInterface = nullptr;
	};

	class PhysicsContact : public JPH::ContactListener
	{
	public:
		PhysicsContact( const JPH::BodyLockInterfaceNoLock* pBody )
			: m_pBodyInterface( pBody ) 
		{
		}

	public:
		virtual JPH::ValidateResult OnContactValidate( const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult ) override;

		void OnContactAdded( const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings ) override;
		
		void OnContactPersisted( const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings ) override;
		
		virtual void OnContactRemoved( const JPH::SubShapeIDPair& inSubShapePair ) override;

	public:
		void DispatchAllContactEvents();
		void IgnoreAll();

	private:
		std::vector<PhysicsContactEventInfo> m_PendingEvents;
		const JPH::BodyLockInterfaceNoLock* m_pBodyInterface = nullptr;
	};

	class JoltBodyActivationListener : public JPH::BodyActivationListener
	{
	public:
		virtual void OnBodyActivated( const JPH::BodyID& inBodyID, uint64_t inBodyUserData ) override;
		virtual void OnBodyDeactivated( const JPH::BodyID& inBodyID, uint64_t inBodyUserData ) override;
	};

	class JoltObjectVsBroadPhaseLayerFilter : public JPH::ObjectVsBroadPhaseLayerFilter
	{
	public:
		virtual bool ShouldCollide( JPH::ObjectLayer layer1, JPH::BroadPhaseLayer layer2 ) const override;
	};

	class JoltObjectLayerPairFilter : public JPH::ObjectLayerPairFilter
	{
	public:
		virtual bool ShouldCollide( JPH::ObjectLayer inLayer1, JPH::ObjectLayer inLayer2 ) const override;
	};

	enum PhysicsBroadPhaseLayer
	{
		PhysBPL_NotMoving,
		PhysBPL_Moving,

		PhysBPL_COUNT,
	};

	static constexpr JPH::BroadPhaseLayer PhysBPLayerNotMoving( 0 );
	static constexpr JPH::BroadPhaseLayer PhysBPLayerMoving( 1 );

	static constexpr JPH::ObjectLayer PhysLayerNotMoving( 0 );
	static constexpr JPH::ObjectLayer PhysLayerMoving( 1 );

	class JoltBPLayerInterface : public JPH::BroadPhaseLayerInterface
	{
	public:
		JoltBPLayerInterface();
		~JoltBPLayerInterface();

		uint32_t GetNumBroadPhaseLayers() const override;
		JPH::BroadPhaseLayer GetBroadPhaseLayer( JPH::ObjectLayer inLayer ) const override;

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
		const char* GetBroadPhaseLayerName( JPH::BroadPhaseLayer inLayer ) const override;
#endif

	private:
		JPH::BroadPhaseLayer m_ObjectToBroadPhase[ PhysicsBroadPhaseLayer::PhysBPL_COUNT ];
	};

	//////////////////////////////////////////////////////////////////////////
	// PHYSICS FOUNDATION

	class PhysicsFoundation
	{
	public:
		static inline PhysicsFoundation* Get() { return SingletonStorage::GetSingleton<PhysicsFoundation>(); }
	public:
		PhysicsFoundation();
		~PhysicsFoundation();

		void Init();
		void Terminate();

		JPH::JobSystem*		GetJobSystem()     const { return m_pJobSystem; }
		JPH::PhysicsSystem* GetPhysicsSystem() const { return m_pPhysicsSystem; }
		JPH::BodyInterface* GetBodyInterface() const { return m_pBodyInterface; }
		JPH::TempAllocator* GetTempAllocator() const { return m_pTempAllocator; }

		PhysicsCooking& GetCooking() { return m_Cooking; }
		const PhysicsCooking& GetCooking() const { return m_Cooking; }

		std::shared_ptr<PhysicsContact> GetContactHandler() { return m_ContactHandler; }
		std::shared_ptr<PhysicsCharacterContact> GetCharacterContactHandler() { return m_CharacterContactHandler; }

	private:
		JPH::JobSystem* m_pJobSystem = nullptr;
		JPH::PhysicsSystem* m_pPhysicsSystem = nullptr;
		JPH::BodyInterface* m_pBodyInterface = nullptr;
		JPH::TempAllocator* m_pTempAllocator = nullptr;

		PhysicsCooking m_Cooking;
		std::shared_ptr<PhysicsContact> m_ContactHandler;
		std::shared_ptr<PhysicsCharacterContact> m_CharacterContactHandler;
		JoltBodyActivationListener m_BodyActivationListener;
		JoltObjectVsBroadPhaseLayerFilter m_ObjectVsBPLayerFilter{};
		JoltObjectLayerPairFilter m_ObjectVsObjectLayerFilter{};
		JoltBPLayerInterface m_BPLayerInterface{};

	private:
		friend class PhysicsScene;
		friend class PhysicsCooking;
	};

}
