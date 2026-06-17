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
#include "PhysicsCharacterController.h"

#include "PhysicsMaterialAsset.h"
#include "PhysicsFoundation.h"
#include "PhysicsScene.h"
#include "PhysicsAuxiliary.h"
#include "PhysicsRigidBody.h"

#include "Saturn/Scene/Entity.h"

#include "Saturn/Project/Project.h"

#include "Saturn/Asset/AssetManager.h"

namespace Saturn {

	PhysicsCharacterController::PhysicsCharacterController( AssetID materialAsset, bool hasGravity, bool crtlMovementInAir, bool crtlRotationInAir )
		: m_MaterialID( materialAsset ), 
		m_HasGravity( hasGravity ), 
		m_ControlMovementInAir( crtlMovementInAir ), 
		m_ControlRotationInAir( crtlRotationInAir )
	{
	}

	PhysicsCharacterController::~PhysicsCharacterController()
	{
		m_Controller->Release();
	}

	void PhysicsCharacterController::PreUpdate( Timestep ts )
	{
		if( ts <= 0.0f )
			return;

		auto* pPhysSystem = PhysicsFoundation::Get()->GetPhysicsSystem();

		m_Velocity = m_Displacement / ts.Seconds();
		m_Controller->UpdateGroundVelocity();
		
		const JPH::Vec3 currentVerticalVelocity = JPH::Vec3( 0.0f, m_Controller->GetLinearVelocity().GetY(), 0.0f );
		const JPH::Vec3 groundVelocity = m_Controller->GetGroundVelocity();

		const bool jumping = ( currentVerticalVelocity.GetY() - groundVelocity.GetY() ) >= 0.01f;

		JPH::Vec3 newVelocity{};
		if( m_HasGravity )
		{
			if( m_Controller->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround && ( !m_Controller->IsSlopeTooSteep( m_Controller->GetGroundNormal() ) ) )
			{
				newVelocity = groundVelocity;

				if( m_JumpPower > 0.0f && !jumping )
				{
					newVelocity += JPH::Vec3( 0.0f, m_JumpPower, 0.0f );
					m_JumpPower = 0.0f;
				}
			}
			else
			{
				newVelocity = currentVerticalVelocity;
			}

			newVelocity += ( pPhysSystem->GetGravity() ) * ts.Seconds();
		}
		else
		{
			newVelocity = JPH::Vec3::sZero();
		}

		if( m_Controller->IsSupported() )
		{
			newVelocity += m_Velocity;
		}
		else
		{
			JPH::Vec3 currentHz = m_Controller->GetLinearVelocity() - currentVerticalVelocity;
			newVelocity += currentHz;
		}

		m_Controller->SetLinearVelocity( newVelocity );
	}

	void PhysicsCharacterController::OnUpdate( Timestep ts )
	{
		auto* pPhysSystem = PhysicsFoundation::Get()->GetPhysicsSystem();

		JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
		updateSettings.mWalkStairsStepUp = { 0.0f, 1.0f, 0.0f };
		updateSettings.mWalkStairsStepForwardTest = m_Controller->GetShape()->GetInnerRadius();

		m_Controller->ExtendedUpdate(
			ts,
			pPhysSystem->GetGravity(),
			updateSettings,
			pPhysSystem->GetDefaultBroadPhaseLayerFilter( 1 ),
			pPhysSystem->GetDefaultLayerFilter( 1 ),
			{},
			{},
			*PhysicsFoundation::Get()->GetTempAllocator() 
		);

		if( m_Controller->IsSupported() )
		{
			m_Displacement = JPH::Vec3::sZero();
		}
	}

	static Ref<PhysicsMaterialAsset> GetMaterial( AssetID materialID )
	{
		Ref<PhysicsMaterialAsset> materialAsset;

		Ref<Project> activeProject = Project::GetActiveProject();
		if( materialID == 0 || materialID == activeProject->GetDefaultPhysicsMaterialAsset() )
		{
			materialAsset = AssetManager::Get()->GetAssetAs<PhysicsMaterialAsset>( activeProject->GetDefaultPhysicsMaterialAsset() );
		}
		else
		{
			materialAsset = AssetManager::Get()->GetAssetAs<PhysicsMaterialAsset>( materialID );
		}

		return materialAsset;
	}

	void PhysicsCharacterController::CreateController( PhysicsScene* pScene, SharedPtr<Entity> entity, const glm::vec3& rOriginPosition )
	{
		if( entity->HasComponent<CapsuleColliderComponent>() ) 
		{
			m_Shape = Ref<CapsuleShape>::Create( entity );
			m_Shape->Create( 100.0f );
		}

		// Null if there is no capsule collider
		SAT_CORE_ASSERT( m_Shape, "We only support Capsule Colliders for controllers right now!" );

		auto& rTc = entity->GetComponent<TransformComponent>();

		JPH::Ref<JPH::CharacterVirtualSettings> settings = new JPH::CharacterVirtualSettings();
		settings->mShape = m_Shape->GetShape();
		settings->mInnerBodyShape = m_Shape->GetShape();
		settings->mInnerBodyLayer = PhysLayerMoving;

		m_Controller = new JPH::CharacterVirtual( 
			settings, 
			Auxiliary::GLMToJolt( rTc.Position ), 
			Auxiliary::GLMQToJoltQ( rTc.GetRotation() ), 
			PhysicsFoundation::Get()->GetPhysicsSystem() 
		);

		m_Controller->SetListener( PhysicsFoundation::Get()->GetCharacterContactHandler().get() );

		m_Controller->SetUserData( ( uint64_t ) entity->GetHandle() );

		// FIXME: Might not be viable to use the handle! (Entity ID may be better, however it's faster to use the handle)
		//		  If we ever crash tell me to revise this!
		PhysicsFoundation::Get()->GetBodyInterface()->SetUserData( m_Controller->GetInnerBodyID(), ( uint64_t ) entity->GetHandle() );
	}

	void PhysicsCharacterController::Move( const glm::vec3& rDisplacement )
	{
		if( m_Controller->IsSupported() )
		{
			m_Displacement += Auxiliary::GLMToJolt( rDisplacement );
		}
	}

	void PhysicsCharacterController::Rotate( const glm::quat& rRotation )
	{
		if( m_Controller->IsSupported() )
		{
			m_Controller->SetRotation( Auxiliary::GLMQToJoltQ( rRotation ) );
		}
	}

	void PhysicsCharacterController::Jump( float pwr )
	{
		m_JumpPower = pwr;
	}

	void PhysicsCharacterController::Teleport( const glm::vec3& rPosition )
	{
		m_Controller->SetPosition( Auxiliary::GLMToJolt( rPosition ) );
	}

	bool PhysicsCharacterController::IsGrounded() const
	{
		return m_Controller->IsSupported();
	}

	glm::vec3 PhysicsCharacterController::GetPosition() const
	{
		return Auxiliary::JoltToGLM( m_Controller->GetPosition() );
	}

}
