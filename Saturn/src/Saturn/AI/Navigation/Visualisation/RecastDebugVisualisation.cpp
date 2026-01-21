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
#include "RecastDebugVisualisation.h"

#include "Saturn/Vulkan/Renderer2D.h"

#include "Recast/Recast.h"

#include <glm/gtc/type_ptr.hpp>

namespace Saturn {
	
	RecastDebugVisualisation::RecastDebugVisualisation()
		: duDebugDraw()
	{
	}

	RecastDebugVisualisation::~RecastDebugVisualisation()
	{

	}

	void RecastDebugVisualisation::BeginRender( Renderer2D* pRenderer2D )
	{
		m_pRenderer2D = pRenderer2D;
	}

	void RecastDebugVisualisation::EndRender()
	{
		m_pRenderer2D = nullptr;
	}

	void RecastDebugVisualisation::depthMask( bool state )
	{
		// we only support drawing lines
	}

	void RecastDebugVisualisation::texture( bool state )
	{
	}

	void RecastDebugVisualisation::begin( duDebugDrawPrimitives prim, float size /*= 1.0f */ )
	{
		SAT_CORE_ASSERT( m_pRenderer2D, "[RecastDebugVisualisation] BeginRender must be called before Recast's begin funcition!" );

		m_CurrentPolygonMode = prim;
		m_Scale = size;
	}

	static glm::vec4 UnpackColor( unsigned int color ) 
	{
		const float r = ( color & 0xFF ) / 255.0f;
		const float g = ( ( color >> 8 ) & 0xFF ) / 255.0f;
		const float b = ( ( color >> 16 ) & 0xFF ) / 255.0f;
		const float a = ( ( color >> 24 ) & 0xFF ) / 255.0f;

		return glm::vec4( r, g, b, a );
	}

	void RecastDebugVisualisation::vertex( const float* pos, unsigned int color )
	{
		glm::vec3 start = glm::vec3( pos[ 0 ], pos[ 1 ], pos[ 2 ] );
		start.y += 1.0f;
		DrawInternal( start, UnpackColor( color ) );
	}

	void RecastDebugVisualisation::vertex( const float x, const float y, const float z, unsigned int color )
	{
		glm::vec3 start = glm::vec3( x, y, z );
		DrawInternal( start, UnpackColor( color ) );
	}

	void RecastDebugVisualisation::vertex( const float* pos, unsigned int color, const float* uv )
	{
		// We don't support textured lines
		glm::vec3 start = glm::vec3( pos[ 0 ], pos[ 1 ], pos[ 2 ] );
		DrawInternal( start, UnpackColor( color ) );
	}

	void RecastDebugVisualisation::vertex( const float x, const float y, const float z, unsigned int color, const float u, const float v )
	{
		// We don't support textured lines
		glm::vec3 start = glm::vec3( x, y, z );
		DrawInternal( start, UnpackColor( color ) );
	}

	void RecastDebugVisualisation::end()
	{
	}

	void RecastDebugVisualisation::DrawInternal( const glm::vec3& rPosition, const glm::vec4& rColor )
	{
		switch( m_CurrentPolygonMode )
		{
			case DU_DRAW_POINTS:
				break;

			case DU_DRAW_LINES:
				m_pRenderer2D->SubmitSingleLine( rPosition, rColor );
				break;

			case DU_DRAW_QUADS:
				m_pRenderer2D->SubmitQuad( rPosition, rColor, glm::vec2{ m_Scale } );
				break;

			case DU_DRAW_TRIS:
				m_pRenderer2D->SubmitVertex( rPosition, rColor );
				break;
		}
	}

}
