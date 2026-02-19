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
#include "NavigationSystem.h"

#include "NavBoundsEntity.h"

#include "Saturn/Core/Random.h"

#include "Saturn/Scene/Scene.h"

#include "RecastCore.h"

#include "Saturn/Vulkan/Renderer2D.h"

#include <Detour/DetourNavMeshQuery.h>
#include <glm/gtc/type_ptr.hpp>

namespace Saturn {

	void NavigationSystem::Initialise()
	{
		m_NavBoundsEntity = g_ActiveScene->GetNavBoundsEntity();

		if( SharedPtr<NavBoundsEntity> entity = m_NavBoundsEntity.Access() ) 
		{	
			const dtNavMesh* pNavMesh = entity->GetBuilder().GetNavMesh();

			m_pNavMeshQuery = dtAllocNavMeshQuery();
			m_pNavMeshQuery->init( pNavMesh, 1048 );

			m_Initialised = true;
		}
	}

	void NavigationSystem::ReleaseReferenceToNavBounds()
	{
		m_NavBoundsEntity.Reset();
	}

	void NavigationSystem::Terminate()
	{
		m_Paths.clear();

		dtFreeNavMeshQuery( m_pNavMeshQuery );
		m_pNavMeshQuery = nullptr;
	}

	NavigationSystem::~NavigationSystem()
	{
	}

	void NavigationSystem::DebugDraw( Renderer2D* pRenderer2D )
	{
		for( const auto& rPath : m_Paths )
		{
			const glm::vec4 pathColor = glm::vec4( 1.0f, 1.0f, 0.0f, 1.0f );

			const auto pathPoints = rPath->GetPoints();

			if( pathPoints.size() )
			{
				// Origin point.
				pRenderer2D->SubmitDiamond( pathPoints[ 0 ], 0.75f, glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f ) );

				for( size_t i = 0; i < pathPoints.size() - 1; i++ )
				{
					const glm::vec3& rThisPoint = pathPoints[ i ];
					const glm::vec3& rNextPoint = pathPoints[ i + 1 ];

					pRenderer2D->SubmitLine( rThisPoint, rNextPoint, pathColor );
					pRenderer2D->SubmitDiamond( rNextPoint, 0.75f, pathColor );
				}
			}
		}
	}

	StraightNavPath* NavigationSystem::CreateStraightPath( const glm::vec3& rStart, const glm::vec3& rEnd, uint32_t maxPaths /*= 256 */ )
	{
		Ref<StraightNavPath> path = Ref<StraightNavPath>::Create( rStart, rEnd, maxPaths );
		m_Paths.emplace_back( path );
	
		return path.Get();
	}

	void NavigationSystem::DestoryStraightPath( StraightNavPath* pPath )
	{
		if( const auto itr = std::find( m_Paths.begin(), m_Paths.end(), pPath ); itr != m_Paths.end() )
		{
			m_Paths.erase( itr );
		}
		else
		{
			// If you get this then you've either not registered the path with the Navigation system
			// or you have not called CreateStraightPath...
			// or the scene that you originally made it in has switched, i.e. using g_ActiveScene
			SAT_CORE_ASSERT( false, "Path is unknown to the Navigation system!" );
		}
	}

	//////////////////////////////////////////////////////////////////////////

	uint32_t NavigationSystem::FindNearestPoly( const glm::vec3& rPosition, float* pNearestPoint )
	{
		if( const auto bounds = m_NavBoundsEntity.Access(); bounds ) 
		{
			if( !bounds->GetBuilder().GetNavMesh() ) 
			{
				SAT_CORE_ERROR( "No navmesh to find a nearest poly in! (m_pNavMesh == nullptr)" );
				return SAT_DETOUR_NULLNAVNODE;
			}
		}

		dtQueryFilter filter;
		filter.setIncludeFlags( NavigationMeshPolyFlag_All ^ NavigationMeshPolyFlag_Disabled );
		filter.setExcludeFlags( 0 );
		float polyPickExt[ 3 ] = { 2.0f, 4.0f, 2.0f };

		dtPolyRef nearestPoly = SAT_DETOUR_NULLNAVNODE;
		m_pNavMeshQuery->findNearestPoly( glm::value_ptr( rPosition ), polyPickExt, &filter, &nearestPoly, pNearestPoint );

		return nearestPoly;
	}

	static float RcRandom()
	{
		return Random::RandomFloatInRange( 0.0f, 1.0f );
	}

	std::expected<glm::vec3, dtStatus> NavigationSystem::GetRandomPointInNavMesh( float maxRadius ) const
	{
		glm::vec3 dest{};

		dtQueryFilter filter;
		dtPolyRef randomRef = SAT_DETOUR_NULLNAVNODE;

		const auto status = m_pNavMeshQuery->findRandomPoint( &filter, RcRandom, &randomRef, glm::value_ptr( dest ) );
		if( status != DT_SUCCESS )
		{
			return std::unexpected( status );
		}

		if( randomRef == SAT_DETOUR_NULLNAVNODE )
		{
			SAT_CORE_WARN( "[NavigationSystem/GetRandomPointInNavMesh] Random poly is outside of the NavMesh!" );
			return std::unexpected( DT_FAILURE );
		}

		return dest;
	}

}
