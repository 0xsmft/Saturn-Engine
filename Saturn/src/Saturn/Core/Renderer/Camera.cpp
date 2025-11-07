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
#include "Camera.h"

#include "Saturn/Vulkan/Renderer2D.h"

namespace Saturn {
	
	//////////////////////////////////////////////////////////////////////////
	// CAMERA

	Camera::Camera( const float Fov, const float Width, const float Height, const float NearPlane, const float FarPlane )
		: m_Projection( glm::perspectiveFov( glm::radians( Fov ), Width, Height, NearPlane, FarPlane ) ), 
		m_ViewportWidth( (uint32_t)Width ), m_ViewportHeight( (uint32_t)Height ), m_Fov( Fov ), m_NearPlane( NearPlane ), m_FarPlane( FarPlane )
	{
	}

	glm::vec3 Camera::GetUpDirection() const
	{
		return glm::rotate( GetOrientation(), glm::vec3( 0.0f, 1.0f, 0.0f ) );
	}

	glm::vec3 Camera::GetRightDirection() const
	{
		return glm::rotate( GetOrientation(), glm::vec3( 1.f, 0.f, 0.f ) );
	}

	glm::vec3 Camera::GetForwardDirection() const
	{
		return glm::rotate( GetOrientation(), glm::vec3( 0.0f, 0.0f, -1.0f ) );
	}

	glm::quat Camera::GetOrientation() const
	{
		return glm::quat( glm::vec3( -m_Pitch - m_PitchDelta, -m_Yaw - m_YawDelta, 0.0f ) );
	}

	bool Camera::CameraFrustumIntersectsAABB( const AABB& rBoundingBox )
	{
		for( const auto& rPlane : m_CameraFrustumPlanes )
		{
			// Find most positive vector (p-vertex)
			glm::vec3 posVec = rBoundingBox.Min;
			if( rPlane.Normal.x >= 0 ) posVec.x = rBoundingBox.Max.x;
			if( rPlane.Normal.y >= 0 ) posVec.y = rBoundingBox.Max.y;
			if( rPlane.Normal.z >= 0 ) posVec.z = rBoundingBox.Max.z;

			// n.p + d < 0
			if( glm::dot( rPlane.Normal, posVec ) + rPlane.Distance < 0.0f )
			{
				return false;
			}
		}

		return true;
	}

	void Camera::RenderDebugFrustum( Ref<Renderer2D>& renderer2d ) const
	{
		// Get the 8 corners of the frustum
		std::array<glm::vec3, 8> corners = GetFrustumCorners();

		const glm::vec4 NEAR_COLOR = glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f );
		const glm::vec4 FAR_COLOR = glm::vec4( 0.0f, 1.0, 0.0f, 1.0f );
		const glm::vec4 CONNECT_COLOR = glm::vec4( 0.0F, 0.0f, 1.0f, 1.0f );

		// Near plane
		renderer2d->SubmitLine( corners[ 0 ], corners[ 1 ], NEAR_COLOR );
		renderer2d->SubmitLine( corners[ 1 ], corners[ 2 ], NEAR_COLOR );
		renderer2d->SubmitLine( corners[ 2 ], corners[ 3 ], NEAR_COLOR );
		renderer2d->SubmitLine( corners[ 3 ], corners[ 0 ], NEAR_COLOR );

		// Far plane
		renderer2d->SubmitLine( corners[ 4 ], corners[ 5 ], FAR_COLOR );
		renderer2d->SubmitLine( corners[ 5 ], corners[ 6 ], FAR_COLOR );
		renderer2d->SubmitLine( corners[ 6 ], corners[ 7 ], FAR_COLOR );
		renderer2d->SubmitLine( corners[ 7 ], corners[ 4 ], FAR_COLOR );

		// Connection corner plane
		renderer2d->SubmitLine( corners[ 0 ], corners[ 4 ], CONNECT_COLOR );
		renderer2d->SubmitLine( corners[ 1 ], corners[ 5 ], CONNECT_COLOR );
		renderer2d->SubmitLine( corners[ 2 ], corners[ 6 ], CONNECT_COLOR );
		renderer2d->SubmitLine( corners[ 3 ], corners[ 7 ], CONNECT_COLOR );
	}

	std::array<glm::vec3, 8> Camera::GetFrustumCorners() const
	{
		std::array<glm::vec3, 8> corners{};

		float fovRad = glm::radians( m_Fov );

		// Calculate the height and width of the near and far planes
		float tanFov = std::tan( fovRad * 0.5f );

		float nearHeight = 2.0f * tanFov * m_NearPlane;
		float nearWidth = nearHeight * GetAspectRatio();

		float farHeight = 2.0f * tanFov * m_FarPlane;
		float farWidth = farHeight * GetAspectRatio();

//		const glm::vec3 forward = glm::normalize( Forward );
//		const glm::vec3 right = glm::normalize( glm::cross( forward, Up ) );
//		const glm::vec3 up = glm::normalize( glm::cross( right, forward ) );

		// Calculate the center positions of the near and far planes
		glm::vec3 nearCenter = m_Position + m_Rotation * m_NearPlane;
		glm::vec3 farCenter = m_Position + m_Rotation * m_FarPlane;

		glm::vec3 up = GetUpDirection();

		corners[ 0 ] = nearCenter + up * ( nearHeight / 2.0f ) - m_RightDirection * ( nearWidth / 2.0f );
		corners[ 1 ] = nearCenter + up * ( nearHeight / 2.0f ) + m_RightDirection * ( nearWidth / 2.0f );
		corners[ 2 ] = nearCenter - up * ( nearHeight / 2.0f ) + m_RightDirection * ( nearWidth / 2.0f );
		corners[ 3 ] = nearCenter - up * ( nearHeight / 2.0f ) - m_RightDirection * ( nearWidth / 2.0f );

		corners[ 4 ] = farCenter + up * ( farHeight / 2.0f ) - m_RightDirection * ( farWidth / 2.0f );
		corners[ 5 ] = farCenter + up * ( farHeight / 2.0f ) + m_RightDirection * ( farWidth / 2.0f );
		corners[ 6 ] = farCenter - up * ( farHeight / 2.0f ) + m_RightDirection * ( farWidth / 2.0f );
		corners[ 7 ] = farCenter - up * ( farHeight / 2.0f ) - m_RightDirection * ( farWidth / 2.0f );

		return corners;
	}

	void Camera::UpdateFrustum( const glm::mat4& rViewProjection )
	{
		/*
		m_CameraFrustumPlanes[ 0 ] = { 
			rViewProjection[ 3 ][ 0 ] - rViewProjection[ 2 ][ 0 ], 
			rViewProjection[ 3 ][ 1 ] - rViewProjection[ 2 ][ 1 ], 
			rViewProjection[ 3 ][ 2 ] - rViewProjection[ 2 ][ 2 ], 
			rViewProjection[ 3 ][ 3 ] - rViewProjection[ 2 ][ 3 ] };

		m_CameraFrustumPlanes[ 1 ] = { 
			rViewProjection[ 3 ][ 0 ] + rViewProjection[ 2 ][ 0 ], 
			rViewProjection[ 3 ][ 1 ] + rViewProjection[ 2 ][ 1 ], 
			rViewProjection[ 3 ][ 2 ] + rViewProjection[ 2 ][ 2 ], 
			rViewProjection[ 3 ][ 3 ] + rViewProjection[ 2 ][ 3 ] };

		m_CameraFrustumPlanes[ 2 ] = { 
			rViewProjection[ 3 ][ 0 ] + rViewProjection[ 0 ][ 0 ], 
			rViewProjection[ 3 ][ 1 ] + rViewProjection[ 0 ][ 1 ], 
			rViewProjection[ 3 ][ 2 ] + rViewProjection[ 0 ][ 2 ], 
			rViewProjection[ 3 ][ 3 ] + rViewProjection[ 0 ][ 3 ] };
		
		m_CameraFrustumPlanes[ 3 ] = { 
			rViewProjection[ 3 ][ 0 ] - rViewProjection[ 0 ][ 0 ], 
			rViewProjection[ 3 ][ 1 ] - rViewProjection[ 0 ][ 1 ], 
			rViewProjection[ 3 ][ 2 ] - rViewProjection[ 0 ][ 2 ], 
			rViewProjection[ 3 ][ 3 ] - rViewProjection[ 0 ][ 3 ] };
		
		m_CameraFrustumPlanes[ 4 ] = { 
			rViewProjection[ 3 ][ 0 ] + rViewProjection[ 1 ][ 0 ], 
			rViewProjection[ 3 ][ 1 ] + rViewProjection[ 1 ][ 1 ], 
			rViewProjection[ 3 ][ 2 ] + rViewProjection[ 1 ][ 2 ], 
			rViewProjection[ 3 ][ 3 ] + rViewProjection[ 1 ][ 3 ] };
		
		m_CameraFrustumPlanes[ 5 ] = { 
			rViewProjection[ 3 ][ 0 ] - rViewProjection[ 1 ][ 0 ], 
			rViewProjection[ 3 ][ 1 ] - rViewProjection[ 1 ][ 1 ], 
			rViewProjection[ 3 ][ 2 ] - rViewProjection[ 1 ][ 2 ], 
			rViewProjection[ 3 ][ 3 ] - rViewProjection[ 1 ][ 3 ] };
			*/

		// Left
		m_CameraFrustumPlanes[ 0 ].Normal.x = rViewProjection[ 0 ][ 3 ] + rViewProjection[ 0 ][ 0 ];
		m_CameraFrustumPlanes[ 0 ].Normal.y = rViewProjection[ 1 ][ 3 ] + rViewProjection[ 1 ][ 0 ];
		m_CameraFrustumPlanes[ 0 ].Normal.z = rViewProjection[ 2 ][ 3 ] + rViewProjection[ 2 ][ 0 ];
		m_CameraFrustumPlanes[ 0 ].Distance = rViewProjection[ 3 ][ 3 ] + rViewProjection[ 3 ][ 0 ];

		// Right
		m_CameraFrustumPlanes[ 1 ].Normal.x = rViewProjection[ 0 ][ 3 ] - rViewProjection[ 0 ][ 0 ];
		m_CameraFrustumPlanes[ 1 ].Normal.y = rViewProjection[ 1 ][ 3 ] - rViewProjection[ 1 ][ 0 ];
		m_CameraFrustumPlanes[ 1 ].Normal.z = rViewProjection[ 2 ][ 3 ] - rViewProjection[ 2 ][ 0 ];
		m_CameraFrustumPlanes[ 1 ].Distance = rViewProjection[ 3 ][ 3 ] - rViewProjection[ 3 ][ 0 ];

		// Bottom
		m_CameraFrustumPlanes[ 2 ].Normal.x = rViewProjection[ 0 ][ 3 ] + rViewProjection[ 0 ][ 1 ];
		m_CameraFrustumPlanes[ 2 ].Normal.y = rViewProjection[ 1 ][ 3 ] + rViewProjection[ 1 ][ 1 ];
		m_CameraFrustumPlanes[ 2 ].Normal.z = rViewProjection[ 2 ][ 3 ] + rViewProjection[ 2 ][ 1 ];
		m_CameraFrustumPlanes[ 2 ].Distance = rViewProjection[ 3 ][ 3 ] + rViewProjection[ 3 ][ 1 ];

		// Top
		m_CameraFrustumPlanes[ 3 ].Normal.x = rViewProjection[ 0 ][ 3 ] - rViewProjection[ 0 ][ 1 ];
		m_CameraFrustumPlanes[ 3 ].Normal.y = rViewProjection[ 1 ][ 3 ] - rViewProjection[ 1 ][ 1 ];
		m_CameraFrustumPlanes[ 3 ].Normal.z = rViewProjection[ 2 ][ 3 ] - rViewProjection[ 2 ][ 1 ];
		m_CameraFrustumPlanes[ 3 ].Distance = rViewProjection[ 3 ][ 3 ] - rViewProjection[ 3 ][ 1 ];

		// Near
		m_CameraFrustumPlanes[ 4 ].Normal.x = rViewProjection[ 0 ][ 3 ] + rViewProjection[ 0 ][ 2 ];
		m_CameraFrustumPlanes[ 4 ].Normal.y = rViewProjection[ 1 ][ 3 ] + rViewProjection[ 1 ][ 2 ];
		m_CameraFrustumPlanes[ 4 ].Normal.z = rViewProjection[ 2 ][ 3 ] + rViewProjection[ 2 ][ 2 ];
		m_CameraFrustumPlanes[ 4 ].Distance = rViewProjection[ 3 ][ 3 ] + rViewProjection[ 3 ][ 2 ];

		// Far
		m_CameraFrustumPlanes[ 5 ].Normal.x = rViewProjection[ 0 ][ 3 ] - rViewProjection[ 0 ][ 2 ];
		m_CameraFrustumPlanes[ 5 ].Normal.y = rViewProjection[ 1 ][ 3 ] - rViewProjection[ 1 ][ 2 ];
		m_CameraFrustumPlanes[ 5 ].Normal.z = rViewProjection[ 2 ][ 3 ] - rViewProjection[ 2 ][ 2 ];
		m_CameraFrustumPlanes[ 5 ].Distance = rViewProjection[ 3 ][ 3 ] - rViewProjection[ 3 ][ 2 ];
	}

}