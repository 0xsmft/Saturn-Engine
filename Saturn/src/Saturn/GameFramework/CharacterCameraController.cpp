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
#include "CharacterCameraController.h"

#include "Saturn/Scene/Scene.h"
#include "Saturn/Scene/Entity.h"
#include "Saturn/Core/Input.h"

namespace Saturn {

	CharacterCameraController::CharacterCameraController()
	{
	}

	void CharacterCameraController::Setup( Ref<class Camera> camera, SharedPtr<class Entity> camEntity, CharacterCameraControllerMode desiredMode )
	{
		SAT_CORE_ASSERT( camera );
		SAT_CORE_ASSERT( camEntity );

		m_Camera = camera;
		m_CameraEntity = camEntity;
		m_CameraMode = desiredMode;

		switch( desiredMode )
		{
			default: break;
			
			case CharacterCameraControllerMode::ThirdPerson: 
			{
				m_PlayerEntity = m_CameraEntity->GetScene()->FindEntityByID( m_CameraEntity->GetParent() );

				SAT_CORE_ASSERT( m_PlayerEntity );

				CreateEntityParameters params{};
				params.Parent = m_PlayerEntity;
				params.Tag = "Pivot";
				params.pClass = Entity::StaticClass();

				m_PivotEntity = m_CameraEntity->GetScene()->CreateEntity( params );

				m_CameraEntity->ChangeToNewParent( m_PivotEntity );
			} break;
		}
	}

	CharacterCameraController::~CharacterCameraController()
	{
	}

	void CharacterCameraController::Update()
	{
		UpdateMouseState();

		switch( m_CameraMode )
		{
			case CharacterCameraControllerMode::FirstPerson:
				Update_ForFP();
				break;
		
			case CharacterCameraControllerMode::ThirdPerson:
				Update_ForTP();
				break;

			default: break;
		}
	}

	void CharacterCameraController::UpdateMouseState()
	{
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

	void CharacterCameraController::Update_ForTP()
	{
		const auto& rCamera = m_CameraEntity->GetComponent<CameraComponent>().Camera;
		auto rot = m_PivotEntity->GetLocalRotation();

		// TODO: This is inverted and is really annoying!!!
		rot.x = rCamera->GetPitch();
		rot.y = glm::pi<float>() - rCamera->GetYaw();

		m_PivotEntity->SetRotation( rot );
	}

	void CharacterCameraController::Update_ForFP()
	{
		const auto& camera = m_CameraEntity->GetComponent<CameraComponent>().Camera;
		auto rotation = m_CameraEntity->GetLocalRotation();
		rotation.x = camera->GetPitch();
		rotation.y = glm::pi<float>() - camera->GetYaw();

		m_CameraEntity->SetRotation( rotation );
	}

	glm::vec3 CharacterCameraController::GetRight() const
	{
		SAT_CORE_ASSERT( m_Camera );

		return m_Camera->GetRightDirection();
	}

	glm::vec3 CharacterCameraController::GetForward() const
	{
		return m_Camera->GetForwardDirection();
	}

}
