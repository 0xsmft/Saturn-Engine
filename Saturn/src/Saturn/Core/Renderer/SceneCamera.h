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

	class SceneCamera : public Camera
	{
	public:
		SceneCamera() = default;
		SceneCamera( float fov, float width, float height ) : Camera( fov, width, height, m_Near, m_Far ), m_Fov( fov ), m_Width( ( uint32_t ) width ), m_Height( ( uint32_t ) height ) {}
		~SceneCamera() = default;

		void SetViewportSize( uint32_t width, uint32_t height )
		{
			if( m_Width == width || m_Height == height )
				return;

			m_Width = width;
			m_Height = height;

			m_Projection = glm::perspectiveFov( m_Fov, ( float ) width, ( float ) height, m_Near, m_Far );
		}

		void SetFOV( float fov ) { m_Fov = fov; }

	private:
		float m_Fov = 45.0f;
		float m_Far = 1000.0f;
		float m_Near = 0.1f;
	
		uint32_t m_Width;
		uint32_t m_Height;
	};
}