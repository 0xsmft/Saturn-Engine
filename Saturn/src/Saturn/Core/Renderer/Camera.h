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

#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>

namespace Saturn {

	class Camera
	{
	public:
		Camera() = default;

		Camera( const float Fov, const float Width, const float Height, const float NearPlane, const float FarPlane )
			: m_Projection( glm::perspectiveFov( glm::radians( Fov ), Width, Height, NearPlane, FarPlane ) )
		{
		}

		virtual ~Camera() = default;

		const glm::mat4& ProjectionMatrix() const { return m_Projection; }
		void SetProjectionMatrix( const float Fov, const float Width, const float Height, const float NearPlane, const float FarPlane ) { m_Projection = glm::perspectiveFov( glm::radians( Fov ), Width, Height, NearPlane, FarPlane ); }
		
	protected:
		glm::mat4 m_Projection = glm::mat4( 1.0f );
	};

	// The current camera's information to be sent to a renderer (SceneRenderer/Renderer2D)
	struct RendererCamera
	{
		Camera Camera;
		glm::mat4 ViewMatrix{};
	};
}