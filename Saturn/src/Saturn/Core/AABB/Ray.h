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

#include "AABB.h"

namespace Saturn {

	struct Ray
	{
		glm::vec3 Origin;
		glm::vec3 Direction;

		inline bool IntersectsAABB( const AABB& rBB, float& t ) const
		{
			glm::vec3 dirfrac{};
			// r.dir is unit direction vector of ray
			dirfrac.x = 1.0f / Direction.x;
			dirfrac.y = 1.0f / Direction.y;
			dirfrac.z = 1.0f / Direction.z;
			// lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
			// r.org is origin of ray
			const glm::vec3& lb = rBB.Min;
			const glm::vec3& rt = rBB.Max;
			const float t1 = ( lb.x - Origin.x ) * dirfrac.x;
			const float t2 = ( rt.x - Origin.x ) * dirfrac.x;
			const float t3 = ( lb.y - Origin.y ) * dirfrac.y;
			const float t4 = ( rt.y - Origin.y ) * dirfrac.y;
			const float t5 = ( lb.z - Origin.z ) * dirfrac.z;
			const float t6 = ( rt.z - Origin.z ) * dirfrac.z;

			const float tmin = glm::max( glm::max( glm::min( t1, t2 ), glm::min( t3, t4 ) ), glm::min( t5, t6 ) );
			const float tmax = glm::min( glm::min( glm::max( t1, t2 ), glm::max( t3, t4 ) ), glm::max( t5, t6 ) );

			// if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us
			if( tmax < 0 )
			{
				t = tmax;
				return false;
			}

			// if tmin > tmax, ray doesn't intersect AABB
			if( tmin > tmax )
			{
				t = tmax;
				return false;
			}

			t = tmin;
			return true;
		}

		bool IntersectsTri( const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, float& t ) const
		{
			const glm::vec3 E1 = b - a;
			const glm::vec3 E2 = c - a;
			const glm::vec3 N = cross( E1, E2 );
			const float det = -glm::dot( Direction, N );
			const float invdet = 1.f / det;
			const glm::vec3 AO = Origin - a;
			const glm::vec3 DAO = glm::cross( AO, Direction );
			const float u = glm::dot( E2, DAO ) * invdet;
			const float v = -glm::dot( E1, DAO ) * invdet;
			t = glm::dot( AO, N ) * invdet;
			return ( det >= 1e-6f && t >= 0.0f && u >= 0.0f && v >= 0.0f && ( u + v ) <= 1.0f );
		}
	};
	
}
