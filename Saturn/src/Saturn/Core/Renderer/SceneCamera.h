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

#include "Camera.h"

namespace Saturn {

	// SceneCamera
	// A SceneCamera does not calculate it's position, instead it's position is expected to be set from something else
	// For example if an entity has a SceneCamera, we set the camera's position to the entity's position.
	// Furthermore, SceneCamera does not contain the ability to move, it only has the ability to look around.
	class SceneCamera : public Camera
	{
	public:
		SceneCamera() = default;
		SceneCamera( const float fov, const float width, const float height );
		~SceneCamera() = default;

		void OnUpdate( Timestep ts );

		void SetViewportSize( uint32_t width, uint32_t height )
		{
			if( m_ViewportWidth == width || m_ViewportHeight == height )
				return;

			SetProjectionMatrix( m_Fov, ( float ) width, ( float ) height, m_NearPlane, m_FarPlane );
		}

		void SetPosition( const glm::vec3& pos ) { m_Position = pos; }

		const glm::mat4& ViewMatrix() const { return m_ViewMatrix; }
		glm::mat4 ViewProjection() const { return m_Projection * m_ViewMatrix; }

	private:
		void UpdateCameraView();
	
	private:
		glm::mat4 m_ViewMatrix{};
		glm::vec2 m_InitialMousePosition{};
	};
}