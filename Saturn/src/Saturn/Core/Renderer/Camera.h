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

#include "Saturn/Core/AABB/AABB.h"

#include <stdint.h>

#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <array>

namespace Saturn {

	struct FrustumPlane
	{
		FrustumPlane() = default;

		FrustumPlane( float a, float b, float c, float d )
		{
			const float inverseLength = 1.0f / std::hypot( a, b, c );
			Normal = glm::vec3( a * inverseLength, b * inverseLength, c * inverseLength );
			Distance = d * inverseLength;
		}

		glm::vec3 Normal{};
		float Distance = 0.0F;
	};

	class Renderer2D;

	class Camera
	{
	public:
		Camera() = default;
		Camera( const float Fov, const float Width, const float Height, const float NearPlane, const float FarPlane );

		virtual ~Camera() = default;

	public:
		const glm::mat4& ProjectionMatrix() const { return m_Projection; }
		
		void SetProjectionMatrix( const float Fov, const float Width, const float Height, const float NearPlane, const float FarPlane ) 
		{
			m_Projection = glm::perspectiveFov( glm::radians( Fov ), Width, Height, NearPlane, FarPlane ); 
		
			m_ViewportWidth = ( uint32_t ) Width;
			m_ViewportHeight = ( uint32_t ) Height;
			m_NearPlane = NearPlane;
			m_FarPlane = FarPlane;
		}
		
		const glm::vec3& GetPosition() const { return m_Position; }

		glm::vec3 GetUpDirection() const;
		glm::vec3 GetRightDirection() const;
		glm::vec3 GetForwardDirection() const;
		glm::quat GetOrientation() const;

		glm::vec3 GetRotation() const { return m_Rotation; }

		uint32_t GetViewportWidth() const { return m_ViewportWidth; }
		uint32_t GetViewportHeight() const { return m_ViewportHeight; }
		uint32_t GetAspectRatio() const { return m_ViewportWidth / m_ViewportHeight; }

		float GetNearPlane() const { return m_NearPlane; }
		float GetFarPlane()  const { return m_FarPlane; }
		float GetFov()       const { return m_Fov; }

		float GetYaw()		 const { return m_Yaw; }
		float GetPitch()	 const { return m_Pitch; }

		bool IsActive() const { return m_IsActive; }
		void SetActive( bool active ) { m_IsActive = active; }
	
		bool CameraFrustumIntersectsAABB( const AABB& rBoundingBox );
		void RenderDebugFrustum( Ref<Renderer2D>& sceneRenderer ) const;
		std::array<glm::vec3, 8> GetFrustumCorners() const;

	protected:
		void UpdateFrustum( const glm::mat4& rViewProjection );

	protected:
		glm::mat4 m_Projection = glm::mat4( 1.0f );
		glm::vec3 m_Position = glm::vec3( 0.0f );
		glm::vec3 m_RightDirection{};
		glm::vec3 m_Rotation{};
		glm::vec3 m_PositionDelta{};

		uint32_t m_ViewportWidth{};
		uint32_t m_ViewportHeight{};

		float m_Pitch{}, m_Yaw{};
		float m_PitchDelta{}, m_YawDelta{};

		float m_NearPlane = 0.1f;
		float m_FarPlane = 1000.0f;
		// Fov in degrees
		float m_Fov = 45.0f;
		bool m_IsActive = false;

		std::array<FrustumPlane, 6> m_CameraFrustumPlanes;
	};

	// The current camera's information to be sent to a renderer (SceneRenderer/Renderer2D)
	struct RendererCamera
	{
		Camera* pCamera = nullptr;

		// Stored separately as some camera don't have a view matrix such as the SceneCamera whose view matrix is calculated in the Active Scene and is relative to the entity who owns it.
		glm::mat4 ViewMatrix{};
	};
}
