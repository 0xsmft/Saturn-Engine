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
#include "NavPath.h"

#include "Saturn/Scene/Scene.h"

#include "Saturn/Serialisation/RawSerialisation.h"

#include "Saturn/Vulkan/Renderer2D.h"

#include <Detour/DetourNavMeshQuery.h>
#include <glm/gtc/type_ptr.hpp>

#include "RecastCore.h"

#define DT_CHECK_AND_RETURN( x ) \
{\
unsigned int result = x; \
if( dtStatusFailed( (result) ) ) \
{ \
	std::string errorText = Auxiliary::DetourErrorToString( result ); \
	SAT_CORE_INFO( "[NavPath] Detour operation failed! Error code was DETOUR ERROR/{0}" ); \
	return false; \
}\
}

namespace Saturn {

	NavPath::NavPath( const glm::vec3& rTo, const glm::vec3& rFrom )
		: m_To( rTo ), m_From( rFrom )
	{
	}

	NavPath::~NavPath()
	{
	}

	bool NavPath::CreatePath()
	{
		auto* pNavMeshQuery = GActiveScene->GetNavMeshQuery();

		dtQueryFilter filter;
		dtPolyRef startPoly, endPoly;
		float polyPickExt[ 3 ] = { 2.0f, 2.0f, 2.0f }; // Extent of the poly pick.

		float outStartNearest[ 3 ], outEndNearest[ 3 ];
		DT_CHECK_AND_RETURN( pNavMeshQuery->findNearestPoly( glm::value_ptr( m_From ), polyPickExt, &filter, &startPoly, outStartNearest ) );

		DT_CHECK_AND_RETURN( pNavMeshQuery->findNearestPoly( glm::value_ptr( m_To ), polyPickExt, &filter, &endPoly, outEndNearest ) );

		// Found the polys, build the actual path
		dtPolyRef pathRefs[ 256 ];
		int pathCount = 0;

		DT_CHECK_AND_RETURN( pNavMeshQuery->findPath( startPoly, endPoly, outStartNearest, outEndNearest, &filter, pathRefs, &pathCount, 256 ) );

		float straightPath[ 256 * 3 ];
		unsigned char straightPathFlags[ 256 ];
		dtPolyRef straightPathPolys[ 256 ];
		int straightPathCount = 0;

		auto status = pNavMeshQuery->findStraightPath( outStartNearest, outEndNearest, pathRefs, pathCount, straightPath, straightPathFlags, straightPathPolys, &straightPathCount, 256 );

		if( dtStatusSucceed( status ) && straightPathCount > 0 )
		{
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

		return true;
	}

	void NavPath::InvalidatePath()
	{
		m_IsLive = false;
		m_PathPoints.clear();
		m_CurrentWaypoint = 0;
	}

	glm::vec3 NavPath::GetCurrentWaypoint()
	{
		return m_PathPoints.at( m_CurrentWaypoint );
	}

	bool NavPath::RetargetPath( const glm::vec3& rTo, const glm::vec3& rFrom )
	{
		m_To = rTo;
		m_From = rFrom;

		InvalidatePath();
		return CreatePath();
	}

	void NavPath::DebugDraw()
	{
		if( !m_IsLive )
			return;

		const glm::vec4 startColor = glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f );
		const glm::vec4 pathColor  = glm::vec4( 1.0f, 1.0f, 0.0f, 1.0f );
		const glm::vec4 endColor   = glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f );

		for( size_t i = 0; i < m_PathPoints.size() - 1; i++ )
		{
			const glm::vec3& rPathA = m_PathPoints[ i ];
			const glm::vec3& rPathB = m_PathPoints[ i + 1 ];

			glm::vec4 color = pathColor;
			if( i == 0 ) color = startColor;
			else if( i == m_PathPoints.size() - 1 ) color = endColor;

			Renderer2D::Get().SubmitLine( rPathA, rPathB, color );
			Renderer2D::Get().SubmitDiamond( rPathB, 0.75f, color );
		}
	}

	//////////////////////////////////////////////////////////////////////////

	void NavPath::Serialise( const NavPath& rObject, std::ofstream& rStream )
	{
		RawSerialisation::WriteVec3( rObject.m_To, rStream );
		RawSerialisation::WriteVec3( rObject.m_From, rStream );

		RawSerialisation::WriteVector( rObject.m_PathPoints, rStream );
	}

	void NavPath::Deserialise( NavPath& rObject, std::istream& rStream )
	{
		RawSerialisation::ReadVec3( rObject.m_To, rStream );
		RawSerialisation::ReadVec3( rObject.m_From, rStream );

		RawSerialisation::ReadVector( rObject.m_PathPoints, rStream );
	}

}
