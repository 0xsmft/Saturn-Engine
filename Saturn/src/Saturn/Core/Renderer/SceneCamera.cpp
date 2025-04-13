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
#include "SceneCamera.h"

#include "Saturn/Core/Input.h"

#include <glm/glm.hpp>

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

constexpr auto M_PI = glm::pi<float>();

namespace Saturn {

	SceneCamera::SceneCamera( const float fov, const float width, const float height )
		: Camera( fov, width, width, 0.1f, 1000.0f )
	{
		m_Yaw = 3.0f * ( float ) M_PI / 4.0f;
		m_Pitch = M_PI / 4.0f;

		const glm::quat orientation = GetOrientation();
		m_Rotation = glm::eulerAngles( orientation ) * ( 180.0f / ( float ) M_PI );
		m_ViewMatrix = glm::translate( glm::mat4( 1.0f ), m_Position ) * glm::toMat4( orientation );
		m_ViewMatrix = glm::inverse( m_ViewMatrix );

		UpdateFrustum( ViewProjection() );
	}

	static void DisableMouse()
	{
		Input::Get().SetCursorMode( RubyCursorMode::Locked );
	}

	static void EnableMouse()
	{
		Input::Get().SetCursorMode( RubyCursorMode::Normal );
	}

	void SceneCamera::OnUpdate( Timestep ts )
	{
		const glm::vec2& mouse = Input::Get().MousePosition();
		const glm::vec2 delta = ( mouse - m_InitialMousePosition ) * 0.002f;

		if( !m_IsActive )
		{
			EnableMouse();

			// Extra step to counteract the camera's position moving but we aren't active anymore.
			const float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
			const glm::vec3 lookAt = m_Position + GetForwardDirection();
			m_ViewMatrix = glm::lookAt( m_Position, lookAt, glm::vec3( 0.0f, yawSign, 0.0f ) );

			return;
		}
		else
		{
			DisableMouse();

			const float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;

			constexpr float maxRate{ 0.12f };
			m_YawDelta += glm::clamp( yawSign * delta.x, -maxRate, maxRate );
			m_PitchDelta += glm::clamp( delta.y, -maxRate, maxRate );

			m_RightDirection = glm::cross( m_Rotation, glm::vec3{ 0.f, yawSign, 0.f } );
			m_Rotation = glm::rotate( glm::normalize( glm::cross( glm::angleAxis( -m_PitchDelta, m_RightDirection ),
				glm::angleAxis( -m_YawDelta, glm::vec3{ 0.f, yawSign, 0.f } ) ) ), m_Rotation );
		}

		m_InitialMousePosition = mouse;

		m_Yaw += m_YawDelta;
		m_Pitch += m_PitchDelta;

		m_Pitch = glm::clamp( m_Pitch, glm::radians( -88.0f ), glm::radians( 88.0f ) );

		UpdateCameraView();
	}

	void SceneCamera::UpdateCameraView()
	{
		const float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;

		// Extra step to handle the problem when the camera direction is the same as the up vector
		const float cosAngle = glm::dot( GetForwardDirection(), GetUpDirection() );
		if( cosAngle * yawSign > 0.99f )
			m_PitchDelta = 0.f;

		const glm::vec3 lookAt = m_Position + GetForwardDirection();
		m_Rotation = glm::normalize( lookAt - m_Position );
		m_ViewMatrix = glm::lookAt( m_Position, lookAt, glm::vec3( 0.0f, yawSign, 0.0f ) );

		// Damping for smooth camera
		m_YawDelta *= 0.6f;
		m_PitchDelta *= 0.6f;

		UpdateFrustum( ViewProjection() );
	}
}