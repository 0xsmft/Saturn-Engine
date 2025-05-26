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

#include <RecastShared/DebugDraw.h>

namespace Saturn {

	// Recast and detour debug drawing
	class RecastDebugVisualisation : public duDebugDraw
	{
	public:
		RecastDebugVisualisation();
		virtual ~RecastDebugVisualisation();

	public:
		void depthMask( bool state ) override;
		void texture( bool state ) override;

		void begin( duDebugDrawPrimitives prim, float size = 1.0f ) override;

		void vertex( const float* pos, unsigned int color ) override;
		void vertex( const float x, const float y, const float z, unsigned int color ) override;
		void vertex( const float* pos, unsigned int color, const float* uv ) override;
		void vertex( const float x, const float y, const float z, unsigned int color, const float u, const float v ) override;

		void end() override;

	private:
		void DrawInternal( const glm::vec3& rPosition, const glm::vec4& rColor );

	private:
		duDebugDrawPrimitives m_CurrentPolygonMode = DU_DRAW_LINES;
		float m_Scale = 1.0f;
	};
	
}
