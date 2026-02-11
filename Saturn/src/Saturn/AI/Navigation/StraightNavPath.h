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

#include "Saturn/Core/Ref.h"

#include <glm/glm.hpp>
#include <vector>

namespace Saturn {

	//
	// StraightNavPath - A navigation path from one poly to another
	// 
	// In order to create a path call CreateStraightPath from the navigation system.
	// To destroy a path make sure to call DestoryStraightPath
	// You must you use the functions listed above because Path objects are owned by the navigation system.
	// 
	// The class does not preform any modifications to positions!
	// It simply creates the path, and provides a way to get the current point and move onto the next path
	//
	// StraightNavPath essentially wraps a detour straight path
	// StraightNavPaths have the ability to be Serialised, allowing for pre-baked paths.
	//
	class StraightNavPath : public RefTarget
	{
	public:
		StraightNavPath() = default;
		StraightNavPath( const glm::vec3& rStart, const glm::vec3& rEnd, uint32_t maxPaths = 256 );
		~StraightNavPath();

		[[nodiscard]] bool CreatePath();
		void InvalidatePath();

		[[nodiscard]] uint32_t GetCurrentWaypointIndex() const { return m_CurrentWaypoint; }
		[[nodiscard]] uint32_t GetNextWaypointIndex() const { return m_CurrentWaypoint + 1; }
		[[nodiscard]] bool IsLive() const { return m_IsLive; }
		[[nodiscard]] bool IsCompleted() const { return m_CurrentWaypoint == m_PathPoints.size(); }
		[[nodiscard]] glm::vec3 GetCurrentWaypoint();
		[[nodiscard]] const std::vector<glm::vec3> GetPoints() const { return m_PathPoints; }
		[[nodiscard]] glm::vec3 GetStartingPoint() const { return m_StartingCoord; }
		[[nodiscard]] glm::vec3 GetEndPoint() const { return m_EndCoord; }

		// Move onto the next waypoint index.
		inline void NextWaypoint()
		{
			++m_CurrentWaypoint;

			if( m_CurrentWaypoint >= m_PathPoints.size() )
			{
				InvalidatePath();
			}
		}

		// Change the path to a completely different start and end point.
		[[nodiscard]] bool RetargetPath( const glm::vec3& rStart, const glm::vec3& rEnd );

	public:
		static void Serialise( const StraightNavPath& rObject, std::ofstream& rStream );
		static void Deserialise(     StraightNavPath& rObject, std::istream& rStream );

	private:
		glm::vec3 m_StartingCoord{};
		glm::vec3 m_EndCoord{};
		
		uint32_t m_CurrentWaypoint = 0;
		bool m_IsLive = false;
		uint32_t m_MaxPaths = 256;

		std::vector<glm::vec3> m_PathPoints;
	};
	
}
