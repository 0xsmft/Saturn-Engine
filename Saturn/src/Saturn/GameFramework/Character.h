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

#include "Saturn/Scene/Entity.h"
#include "Core/GameScript.h"

#include "PlayerInputController.h"

namespace Saturn {

	class Character : public Entity
	{
		SAT_DECLARE_CLASS( Character, Entity );

	public:
		Character();
		virtual ~Character();

	public:
		//////////////////////////////////////////////////////////////////////////
		// Entity overrides

		virtual void BeginPlay() override;
		virtual void OnUpdate( Timestep ts ) override;
		virtual void OnPhysicsUpdate( Timestep ts ) override;

	public:
		// Character API

		Mesh* GetMesh() { return m_Mesh; }
		const Mesh* GetMesh() const { return m_Mesh; }
		
		//
		// Gets the SkeletalMeshComponent (if any)
		// 
		// So it may return null if this character does not have the component.
		//
		SkeletalMeshComponent* GetSkeletalMeshComponent() { return TryGetComponent<SkeletalMeshComponent>(); }
		const SkeletalMeshComponent* GetSkeletalMeshComponent() const { return TryGetComponent<SkeletalMeshComponent>(); }
		
		//
		// Gets the StaticMeshComponent (if any)
		// 
		// So it may return null if this character does not have the component.
		//
		StaticMeshComponent* GetStaticMeshComponent() { return TryGetComponent<StaticMeshComponent>(); }
		const StaticMeshComponent* GetStaticMeshComponent() const { return TryGetComponent<StaticMeshComponent>(); }

		// Gets the animation graph (if any)
		//
		// So it may return null if this character does have an animator.
		//
		Ref<Animator> GetAnimator();

		float GetMovementSpeed() { return m_MovementSpeed; }
		const float GetMovementSpeed() const { return m_MovementSpeed; }

		[[nodiscard]] bool IsSprinting() const { return m_Sprinting; }

		//
		// Change the animation mode from an AnimationController to a single asset.
		//
		void ChangeAnimationMode( AnimatorType type );

		//
		// Get Current Animation mode
		// 
		// @returns the current AnimationMode, you MUST check if the return value
		// is not equal to AnimatorType::Unknown, as Unknown means that this character
		// doesn't have a SkeletalMeshComponent!
		//
		[[nodiscard]] AnimatorType GetCurrentAnimationMode() const;

		// Phys character controller wrappers

		//
		// Get the movement component.
		// 
		// @returns the movement component, it should never be null. TODO: Maybe assert that.
		//
		CharacterMovementComponent* GetMovementComponent();
		const CharacterMovementComponent* GetMovementComponent() const;

		//
		// Jump
		//
		// @param pwr - power to jump.
		//
		void Jump( float pwr = 1.0f );

		//
		// @returns -- if the character is considered to be "grounded".
		//
		[[nodiscard]] bool IsGrounded() const;

		//
		// Move the character controller (and by extension the entity itself) 
		// by a certain amount. This value is then added.
		//
		// @param rDisplacement - displacement vector.
		//
		void DisplaceCharacterBy( const glm::vec3& rDisplacement );

		//
		// Stop movement now.
		// 
		// Set's the linear velocity to zero.
		//
		void StopMovementImmediately();
		
		// Audio listener

		//
		// Get the AudioListenerComponent.
		// 
		// @returns the AudioListenerComponent, it should never be null. TODO: Maybe assert that.
		//
		AudioListenerComponent* GetAudioListenerComponent();
		const AudioListenerComponent* GetAudioListenerComponent() const;

		//
		// Update the direction this should be after the rotation of the character has changed.
		// 
		// @param rDirection -- the new direction.
		//
		void UpdateAudioListenerDirection( const glm::vec3& rDirection );

		//
		// @returns if the audio listener is the primary one.
		//
		[[nodiscard]] bool IsPrimaryAudioListener();

	protected:
		//
		// This function is called during BeginPlay.
		// 
		// It allows the child class to setup any input bindings.
		//
		virtual void SetupInputBindings() {}

	protected:
		void MoveForward();
		void MoveBack();
		void MoveLeft();
		void MoveRight();

		void MoveForwardEnd();
		void MoveBackEnd();
		void MoveLeftEnd();
		void MoveRightEnd();

		void StartSprint();
		void EndSprint();

	protected:
		Ref<PlayerInputController> m_PlayerInputController = nullptr;

		SharedPtr<Entity> GetCameraEntity() { return m_CameraEntity; }
		const SharedPtr<Entity> GetCameraEntity() const { return m_CameraEntity; }

	protected:
		//////////////////////////////////////////////////////////////////////////
		// Movement

		//
		// Calculate the forward vector
		// based from the camera's
		// orientation.
		//
		glm::vec3 CalculateRight();

		//
		// Calculate the forward vector
		// based from the camera's
		// orientation.
		//
		glm::vec3 CalculateForward();

		float m_MovementSpeed = 0.0f;
		bool m_Sprinting = false;

	private:
		float m_MouseUpMovement = 0.0f;
		float m_MouseSensitivity = 0.0f;

		glm::vec2 m_MovementDirection{};
		glm::vec2 m_LastMousePos{};
		glm::vec3 m_LastMovement{};

	private:
		Mesh* m_Mesh = nullptr;
		AssetID m_MeshID = 0llu;

		SharedPtr<Entity> m_CameraEntity = nullptr;
	};
}
