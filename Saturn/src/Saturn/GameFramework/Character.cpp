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
#include "Character.h"

#include "Saturn/Physics/PhysicsRigidBody.h"
#include "Saturn/Physics/PhysicsCharacterMovement.h"

#include "Core/ClassMetadataHandler.h"

#include "Saturn/Audio/AudioSystem.h"

namespace Saturn {

	Character::Character()
	{
		m_MouseSensitivity = 3.0f;
		m_MouseUpMovement = 0.0f;

		AddComponent<StaticMeshComponent>();

		auto& rRigidbodyComponent = AddComponent<RigidbodyComponent>();
		rRigidbodyComponent.LockFlags = RigidbodyLockFlags::RigidbodyLock_RotationX | RigidbodyLockFlags::RigidbodyLock_RotationY | RigidbodyLockFlags::RigidbodyLock_RotationZ;
		rRigidbodyComponent.IsKinematic = true;

		AddComponent<CharacterMovementComponent>();
		AddComponent<AudioListenerComponent>();
	}

	Character::~Character()
	{
		GetScene()->RemoveController( m_PlayerInputController );

		m_PlayerInputController = nullptr;
		m_CameraEntity = nullptr;
	}

	void Character::BeginPlay()
	{
		Super::BeginPlay();

		m_PlayerInputController = Ref<PlayerInputController>::Create();
		GetScene()->AddController( m_PlayerInputController );

		SetupInputBindings();

		// If the scene already has a camera, we can take ownership of it, if not we create our own camera as a child.
		auto wTemporaryCameraEntity = GetScene()->GetMainCameraEntity();
		if( wTemporaryCameraEntity.Expired() )
		{
			// Create the main camera.
			CreateEntityParameters params{};
			params.Parent = SharedFromThis();
			params.Tag = "Camera";
			params.pClass = Entity::StaticClass();

			m_CameraEntity = GetScene()->CreateEntity( params );
			m_CameraEntity->AddComponent<CameraComponent>().MainCamera = true;
		}
		else
		{
			m_CameraEntity = wTemporaryCameraEntity.Access();
		}

		m_RigidBody = GetComponent<RigidbodyComponent>().Rigidbody;
		Input::Get().SetCursorMode( RubyCursorMode::Locked );
	}

	void Character::OnUpdate( Timestep ts )
	{
		Super::OnUpdate( ts );

//		m_PlayerInputController->UpdateState();

		if( Input::Get().KeyPressed( RubyKey_Esc ) && Input::Get().GetCursorMode() == RubyCursorMode::Locked )
		{
			Input::Get().SetCursorMode( RubyCursorMode::Normal, true );
			m_CameraEntity->GetComponent<CameraComponent>().Camera->SetActive( false );
		}
		else if( Input::Get().MouseButtonPressed( RubyMouseButton_Left ) && Input::Get().GetCursorMode() != RubyCursorMode::Locked ) 
		{
			Input::Get().SetCursorMode( RubyCursorMode::Locked );

#if !defined(SAT_DIST)
			if( Input::Get().CanSetCursorMode() )
#endif
				m_CameraEntity->GetComponent<CameraComponent>().Camera->SetActive( true );

			m_LastMousePos = Input::Get().MousePosition();
		}
	}

	void Character::OnPhysicsUpdate( Timestep ts )
	{
		Super::OnPhysicsUpdate( ts );

		////

		TransformComponent& tc = GetComponent<TransformComponent>();
		CharacterMovementComponent& rMovementComp = GetComponent<CharacterMovementComponent>();
		auto* pController = rMovementComp.CharacterMovement;

		if( pController->IsGrounded() )
		{
			glm::vec3 right, forward;
			right = CalculateRight();
			forward = CalculateForward();

			glm::vec3 direction = right * m_MovementDirection.x + forward * m_MovementDirection.y;
			direction.y = 0.0f;

			if( glm::length( direction ) > 0.0f )
			{
				direction = glm::normalize( direction );
			}

			const glm::vec3 displacement = ( direction * m_MovementSpeed ) * ts.Seconds();
			pController->Move( displacement );

			m_LastMovement = displacement / ts.Seconds();

			if( GetComponent<AudioListenerComponent>().Primary )
			{
				AudioSystem::Get().SetPrimaryListenerDirection( forward );
			}
		}
		else
		{
			pController->Move( m_LastMovement * ts.Seconds() );
		}

		if( Input::Get().GetCursorMode() == RubyCursorMode::Locked )
		{
			const auto& up = TransformComponent::Up;
			tc.SetRotation( tc.GetRotationEuler() + glm::vec3( up * m_MouseUpMovement * 0.05f ) );
		}
	}

	glm::vec3 Character::CalculateRight()
	{
		return m_CameraEntity->GetComponent<CameraComponent>().Camera->GetRightDirection();
	}

	glm::vec3 Character::CalculateForward()
	{
		return m_CameraEntity->GetComponent<CameraComponent>().Camera->GetForwardDirection();
	}

	void Character::MoveForward()
	{
		m_MovementDirection.y = 1.0f;
	}

	void Character::MoveBack()
	{
		m_MovementDirection.y = -1.0f;
	}

	void Character::MoveLeft()
	{
		m_MovementDirection.x = -1.0f;
	}

	void Character::MoveRight()
	{
		m_MovementDirection.x = 1.0f;
	}

	void Character::MoveForwardEnd()
	{
		m_MovementDirection.y = 0.0f;
	}

	void Character::MoveBackEnd()
	{
		MoveForwardEnd();
	}

	void Character::MoveLeftEnd()
	{
		m_MovementDirection.x = 0.0f;
	}

	void Character::MoveRightEnd()
	{
		MoveLeftEnd();
	}

	void Character::StartSprint()
	{
		m_MovementSpeed = 12.0f;
	}

	void Character::EndSprint()
	{
		m_MovementSpeed = 5.0f;
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG_SPWN( Character );
