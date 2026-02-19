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

#include "sppch.h"
#include "StraightNavPath.h"

#include "Saturn/Scene/Scene.h"

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

#include "Saturn/Vulkan/Renderer2D.h"

#include <Detour/DetourCommon.h>
#include <Detour/DetourNavMeshQuery.h>
#include <glm/gtc/type_ptr.hpp>

#include "RecastCore.h"
#include "RecastNavigationMeshBuilder.h"

namespace Saturn {

	StraightNavPath::StraightNavPath( const glm::vec3& rStart, const glm::vec3& rEnd, uint32_t maxPaths )
		: m_StartingCoord( rStart ), m_EndCoord( rEnd ), m_MaxPaths( maxPaths )
	{
	}

	StraightNavPath::~StraightNavPath()
	{
	}

	bool StraightNavPath::CreatePath()
	{
		auto& rNavSystem = g_ActiveScene->GetNavigationSystem();
		auto* pNavMeshQuery = g_ActiveScene->GetNavigationSystem().GetNavMeshQuery();

		float outStartNearest[ 3 ];
		dtPolyRef startPoly = startPoly = rNavSystem.FindNearestPoly( m_StartingCoord, outStartNearest );
		if( startPoly == SAT_DETOUR_NULLNAVNODE )
		{
			SAT_CORE_WARN( "[NavPath] Starting poly is not on the nav mesh!" );
			return false;
		}

		float outEndNearest[ 3 ];
		dtPolyRef endPoly = rNavSystem.FindNearestPoly( m_EndCoord, outEndNearest );
		if( endPoly == SAT_DETOUR_NULLNAVNODE )
		{
			SAT_CORE_WARN( "[NavPath] Ending poly is not on the nav mesh!" );
			return false;
		}

		// Found the polys, build the actual path
		dtPolyRef pathRefs[ 256 ];
		int pathCount = 0;

		// TEMP: filter
		dtQueryFilter filter;
		filter.setIncludeFlags( NavigationMeshPolyFlag_All ^ NavigationMeshPolyFlag_Disabled );
		filter.setExcludeFlags( 0 );
		pNavMeshQuery->findPath( startPoly, endPoly, outStartNearest, outEndNearest, &filter, pathRefs, &pathCount, 256 );

		if( pathCount != 0 )
		{
			float straightPath[ 256 * 3 ];
			unsigned char straightPathFlags[ 256 ];
			dtPolyRef straightPathPolys[ 256 ];
			int straightPathCount = 0;

			float epos[ 3 ];
			dtVcopy( epos, outEndNearest );
			if( pathRefs[ 255 ] != endPoly )
			{
				pNavMeshQuery->closestPointOnPoly( pathRefs[ 255 ], outEndNearest, epos, 0 );
			}

			auto status = pNavMeshQuery->findStraightPath( outStartNearest, epos, pathRefs, pathCount, straightPath, straightPathFlags, straightPathPolys, &straightPathCount, 256, DT_STRAIGHTPATH_AREA_CROSSINGS );

			if( dtStatusSucceed( status ) && straightPathCount > 0 )
			{
				m_PathPoints.clear();

				m_PathPoints.reserve( straightPathCount );

				for( size_t i = 0; i < straightPathCount; i++ )
				{
					float* pPath = &straightPath[ i * 3 ];
					m_PathPoints.emplace_back( pPath[ 0 ], pPath[ 1 ], pPath[ 2 ] );
				}

				m_IsLive = true;
			}
			else
				return false;
		}
		else 
			return false;

		return true;
	}

	void StraightNavPath::InvalidatePath()
	{
		m_IsLive = false;
		m_PathPoints.clear();
		m_CurrentWaypoint = 0;
	}

	glm::vec3 StraightNavPath::GetCurrentWaypoint()
	{
		return m_PathPoints.at( m_CurrentWaypoint );
	}

	bool StraightNavPath::RetargetPath( const glm::vec3& rStart, const glm::vec3& rEnd )
	{
		m_StartingCoord = rStart;
		m_EndCoord = rEnd;

		InvalidatePath();
		return CreatePath();
	}

	//////////////////////////////////////////////////////////////////////////

	void StraightNavPath::Serialise( const StraightNavPath& rObject, std::ofstream& rStream )
	{
		RawSerialisation::WriteVec3( rObject.m_StartingCoord, rStream );
		RawSerialisation::WriteVec3( rObject.m_EndCoord, rStream );

		RawSerialisation::WriteVector( rObject.m_PathPoints, rStream );
	}

	void StraightNavPath::Deserialise( StraightNavPath& rObject, std::istream& rStream )
	{
		RawSerialisation::ReadVec3( rObject.m_StartingCoord, rStream );
		RawSerialisation::ReadVec3( rObject.m_EndCoord, rStream );

		RawSerialisation::ReadVector( rObject.m_PathPoints, rStream );
	}

}
