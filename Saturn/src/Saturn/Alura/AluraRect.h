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

#include <glm/glm.hpp>

namespace Saturn {

	// Based from Dear ImGui's ImRect structure.
	class AluraRect
	{
	public:
		glm::vec2 Min;
		glm::vec2 Max;
	
		constexpr AluraRect() : Min( 0.0f, 0.0f ), Max( 0.0f, 0.0f ) {}
		constexpr AluraRect( const glm::vec2& min, const glm::vec2& max ) : Min( min ), Max( max ) {}
		constexpr AluraRect( const glm::vec4& v ) : Min( v.x, v.y ), Max( v.z, v.w ) {}
		constexpr AluraRect( float x1, float y1, float x2, float y2 ) : Min( x1, y1 ), Max( x2, y2 ) {}

		glm::vec2      GetCenter() const { return  glm::vec2( ( Min.x + Max.x ) * 0.5f, ( Min.y + Max.y ) * 0.5f ); }
		glm::vec2      GetSize() const { return  glm::vec2( Max.x - Min.x, Max.y - Min.y ); }
		float          GetWidth() const { return Max.x - Min.x; }
		float          GetHeight() const { return Max.y - Min.y; }
		float          GetArea() const { return ( Max.x - Min.x ) * ( Max.y - Min.y ); }
		glm::vec2      GetTL() const { return Min; }
		glm::vec2      GetTR() const { return  glm::vec2( Max.x, Min.y ); }
		glm::vec2      GetBL() const { return  glm::vec2( Min.x, Max.y ); }
		glm::vec2      GetBR() const { return Max; }
		bool           Contains( const glm::vec2& p ) const { return p.x >= Min.x && p.y >= Min.y && p.x < Max.x && p.y < Max.y; }
		bool           Contains( const AluraRect& r ) const { return r.Min.x >= Min.x && r.Min.y >= Min.y && r.Max.x <= Max.x && r.Max.y <= Max.y; }
		glm::vec4      ToVec4() const { return glm::vec4( Min.x, Min.y, Max.x, Max.y ); }

		// Shrink symmetrically on the X axis.
		void			ShrinkX( float x ) { Min.x += x; Max.x -= x; };
	};
	
}
