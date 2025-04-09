/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2024 BEAST                                                           *
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

	Frustum::Frustum( const glm::vec3& rPosition, const glm::vec3& rForward, const glm::vec3& rRight, const glm::vec3& rUp, float fov, float xnear, float xfar, float aspectRatio )
		: Position( rPosition ), Forward( rForward ), Right( rRight ), Up( rUp ), Fov( fov ), Near( xnear ), Far( xfar ), Aspect( aspectRatio )
	{
	}

	//////////////////////////////////////////////////////////////////////////
	// FRUSTUM
	
	float Frustum::PlaneSignedDistance( const glm::vec3& rPoint, const FrustumPlane& rPlane )
	{
		return glm::dot( rPlane.Normal, rPoint ) + rPlane.Distance;
	}

	bool Frustum::PlaneIntersectsSphere( const glm::vec3& rCenter, float radius, const FrustumPlane& rPlane )
	{
		float distance = PlaneSignedDistance( rCenter, rPlane );
		return distance <= radius;

		return PlaneSignedDistance( rCenter, rPlane ) > -radius;
	}

	bool Frustum::FrustumIntersectsSphere( const glm::vec3& rCenter, float radius )
	{
		for( int i = 0; i < 6; i++ )
		{
			if( !PlaneIntersectsSphere( rCenter, radius, Planes[ i ] ) )
				return false;
		}
		return true;
	}

	bool Frustum::PlaneIntersectsAABB( const glm::vec3& rExtent, const glm::vec3& rCenter, const FrustumPlane& rPlane )
	{
		float r = rExtent.x * glm::abs( rPlane.Normal.x ) +
			rExtent.y * glm::abs( rPlane.Normal.y ) +
			rExtent.z * glm::abs( rPlane.Normal.z );

		return -r <= PlaneSignedDistance( rCenter, rPlane );
	}

	bool Frustum::FrustumIntersectsAABB( const AABB& rBoundingBox )
	{
		for( const auto& rPlane : Planes )
		{
			auto posVec = rBoundingBox.Min;
			if( rPlane.Normal.x >= 0 )
			{
				posVec.x = rBoundingBox.Max.x;
			}
			if( rPlane.Normal.y >= 0 )
			{
				posVec.y = rBoundingBox.Max.y;
			}
			if( rPlane.Normal.z >= 0 )
			{
				posVec.z = rBoundingBox.Max.z;
			}

			// TODO: Do more testing and fix this...
			if( glm::dot( rPlane.Normal, posVec ) - rPlane.Distance > 0.0f )
			{
				return false;
			}
		}

		return true;
	}

	void Frustum::RenderDebug()
	{
		// Get the 8 corners of the frustum
		std::array<glm::vec3, 8> corners = GetFrustumCorners();

		glm::vec4 NEAR_COLOR = glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f );
		glm::vec4 FAR_COLOR = glm::vec4( 0.0f, 1.0, 0.0f, 1.0f );
		glm::vec4 CONNECT_COLOR = glm::vec4( 0.0F, 0.0f, 1.0f, 1.0f );

		// Near plane
		Renderer2D::Get().SubmitLine( corners[ 0 ], corners[ 1 ], NEAR_COLOR );
		Renderer2D::Get().SubmitLine( corners[ 1 ], corners[ 2 ], NEAR_COLOR );
		Renderer2D::Get().SubmitLine( corners[ 2 ], corners[ 3 ], NEAR_COLOR );
		Renderer2D::Get().SubmitLine( corners[ 3 ], corners[ 0 ], NEAR_COLOR );

		// Far plane
		Renderer2D::Get().SubmitLine( corners[ 4 ], corners[ 5 ], FAR_COLOR );
		Renderer2D::Get().SubmitLine( corners[ 5 ], corners[ 6 ], FAR_COLOR );
		Renderer2D::Get().SubmitLine( corners[ 6 ], corners[ 7 ], FAR_COLOR );
		Renderer2D::Get().SubmitLine( corners[ 7 ], corners[ 4 ], FAR_COLOR );

		// Connection corner plane
		Renderer2D::Get().SubmitLine( corners[ 0 ], corners[ 4 ], CONNECT_COLOR );
		Renderer2D::Get().SubmitLine( corners[ 1 ], corners[ 5 ], CONNECT_COLOR );
		Renderer2D::Get().SubmitLine( corners[ 2 ], corners[ 6 ], CONNECT_COLOR );
		Renderer2D::Get().SubmitLine( corners[ 3 ], corners[ 4 ], CONNECT_COLOR );
	}

	std::array<glm::vec3, 8> Frustum::GetFrustumCorners()
	{
		std::array<glm::vec3, 8> corners{};

		// Calculate the height and width of the near and far planes
		float tanFov = std::tan( Fov / 2.0f );

		float nearHeight = 2.0f * tanFov * Near;
		float nearWidth = nearHeight * Aspect;
		
		float farHeight = 2.0f * tanFov * Far;
		float farWidth = farHeight * Aspect;

		// Calculate the center positions of the near and far planes
		glm::vec3 nearCenter = Position + Forward * Near;
		glm::vec3 farCenter = Position + Forward * Far;

		corners[ 0 ] = nearCenter + Up * ( nearHeight / 2.0f ) - Right * ( nearWidth / 2.0f );
		corners[ 1 ] = nearCenter + Up * ( nearHeight / 2.0f ) + Right * ( nearWidth / 2.0f );
		corners[ 2 ] = nearCenter - Up * ( nearHeight / 2.0f ) + Right * ( nearWidth / 2.0f );
		corners[ 3 ] = nearCenter - Up * ( nearHeight / 2.0f ) - Right * ( nearCenter / 2.0f );

		corners[ 4 ] = farCenter + Up * ( farHeight / 2.0f ) - Right * ( farWidth / 2.0f );
		corners[ 5 ] = farCenter + Up * ( farHeight / 2.0f ) + Right * ( farWidth / 2.0f );
		corners[ 6 ] = farCenter - Up * ( farHeight / 2.0f ) + Right * ( farWidth / 2.0f );
		corners[ 7 ] = farCenter - Up * ( farHeight / 2.0f ) - Right * ( farWidth / 2.0f );

		return corners;
	}

	void Frustum::Update( const glm::vec3& rPosition, const glm::vec3& rForward, const glm::vec3& rRight, const glm::vec3& rUp, float fov, float xnear, float xfar, float aspectRatio, const glm::mat4& rViewProjection )
	{
		Position = rPosition;
		Forward = rForward;
		Right = rRight;
		Up = rUp;
		Fov = fov;
		Near = xnear;
		Far = xfar;
		Aspect = aspectRatio;

		Planes[ 0 ] = { rViewProjection[ 3 ][ 0 ] - rViewProjection[ 2 ][ 0 ], rViewProjection[ 3 ][ 1 ] - rViewProjection[ 2 ][ 1 ], rViewProjection[ 3 ][ 2 ] - rViewProjection[ 2 ][ 2 ], rViewProjection[ 3 ][ 3 ] - rViewProjection[ 2 ][ 3 ] };
		Planes[ 1 ] = { rViewProjection[ 3 ][ 0 ] + rViewProjection[ 2 ][ 0 ], rViewProjection[ 3 ][ 1 ] + rViewProjection[ 2 ][ 1 ], rViewProjection[ 3 ][ 2 ] + rViewProjection[ 2 ][ 2 ], rViewProjection[ 3 ][ 3 ] + rViewProjection[ 2 ][ 3 ] };
		Planes[ 2 ] = { rViewProjection[ 3 ][ 0 ] + rViewProjection[ 0 ][ 0 ], rViewProjection[ 3 ][ 1 ] + rViewProjection[ 0 ][ 1 ], rViewProjection[ 3 ][ 2 ] + rViewProjection[ 0 ][ 2 ], rViewProjection[ 3 ][ 3 ] + rViewProjection[ 0 ][ 3 ] };
		Planes[ 3 ] = { rViewProjection[ 3 ][ 0 ] - rViewProjection[ 0 ][ 0 ], rViewProjection[ 3 ][ 1 ] - rViewProjection[ 0 ][ 1 ], rViewProjection[ 3 ][ 2 ] - rViewProjection[ 0 ][ 2 ], rViewProjection[ 3 ][ 3 ] - rViewProjection[ 0 ][ 3 ] };
		Planes[ 4 ] = { rViewProjection[ 3 ][ 0 ] + rViewProjection[ 1 ][ 0 ], rViewProjection[ 3 ][ 1 ] + rViewProjection[ 1 ][ 1 ], rViewProjection[ 3 ][ 2 ] + rViewProjection[ 1 ][ 2 ], rViewProjection[ 3 ][ 3 ] + rViewProjection[ 1 ][ 3 ] };
		Planes[ 5 ] = { rViewProjection[ 3 ][ 0 ] - rViewProjection[ 1 ][ 0 ], rViewProjection[ 3 ][ 1 ] - rViewProjection[ 1 ][ 1 ], rViewProjection[ 3 ][ 2 ] - rViewProjection[ 1 ][ 2 ], rViewProjection[ 3 ][ 3 ] - rViewProjection[ 1 ][ 3 ] };
	}

	//////////////////////////////////////////////////////////////////////////
	// CAMERA

	Camera::Camera( const float Fov, const float Width, const float Height, const float NearPlane, const float FarPlane )
		: m_Projection( glm::perspectiveFov( glm::radians( Fov ), Width, Height, NearPlane, FarPlane ) ), 
		m_ViewportWidth( Width ), m_ViewportHeight( Height ), m_Fov( Fov ), m_NearPlane( NearPlane ), m_FarPlane( FarPlane )
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
}