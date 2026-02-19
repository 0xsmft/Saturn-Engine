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

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

#include <glm/glm.hpp>

namespace Saturn {

	// Axis aligned bounding box
	class AABB
	{
	public:
		glm::vec3 Min, Max;

	public:
		AABB()
			: Min( 0.0f ), Max( 0.0f )
		{
		}

		AABB( const glm::vec3& min, const glm::vec3& max )
			: Min( min ), Max( max )
		{
		}

	public:
		glm::vec3 Center() const
		{
			return ( Min + Max ) * 0.5f;
		}

		glm::vec3 HalfExtent() const
		{
			return ( Max - Min ) * 0.5f;
		}

		glm::vec3 Extent() const
		{
			return ( Max - Min );
		}

		float Volume() const 
		{
			glm::vec3 size = Extent();
			return size.x * size.y * size.z;
		}

		// Check if a point is inside the AABB
		bool Contains( const glm::vec3& point ) const
		{
			return (
				point.x >= Min.x && point.x <= Max.x &&
				point.y >= Min.y && point.y <= Max.y &&
				point.z >= Min.z && point.z <= Max.z );
		}

		// Check if another AABB is inside this AABB
		bool Contains( const AABB& rOther ) const
		{
			return ( rOther.Min.x >= Min.x && rOther.Max.x <= Max.x &&
				rOther.Min.y >= Min.y && rOther.Max.y <= Max.y &&
				rOther.Min.z >= Min.z && rOther.Max.z <= Max.z );
		}

		bool Intersects( const AABB& rOther ) const
		{
			return ( Min.x <= rOther.Max.x && Max.x >= rOther.Min.x ) &&
				( Min.y <= rOther.Max.y && Max.y >= rOther.Min.y ) &&
				( Min.z <= rOther.Max.z && Max.z >= rOther.Min.z );
		}

	public:
		static void Serialise( const AABB& rObject, std::ofstream& rStream )
		{
			RawSerialisation::WriteVec3( rObject.Min, rStream );
			RawSerialisation::WriteVec3( rObject.Max, rStream );
		}

		static void Deserialise( AABB& rObject, std::ifstream& rStream )
		{
			RawSerialisation::ReadVec3( rObject.Min, rStream );
			RawSerialisation::ReadVec3( rObject.Max, rStream );
		}
	};

}