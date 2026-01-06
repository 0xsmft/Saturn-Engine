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

	void NavigationSystem::Terminate()
	{
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

			for( size_t i = 0; i < pathPoints.size() - 1; i++ )
			{
				const glm::vec3& rThisPoint = pathPoints[ i ];
				const glm::vec3& rNextPoint = pathPoints[ i + 1 ];

				pRenderer2D->SubmitLine( rThisPoint, rNextPoint, pathColor );
				pRenderer2D->SubmitDiamond( rNextPoint, 0.75f, pathColor );
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////

	static float RcRandom()
	{
		return Random::RandomFloatInRange( 0.0f, 1.0f );
	}

	std::expected<glm::vec3, dtStatus> NavigationSystem::GetRandomPointInNavMesh( const glm::vec3& rOrigin, float maxRadius ) const
	{
		glm::vec3 dest{};

		dtQueryFilter filter;
		dtPolyRef randomRef;

		const auto status = m_pNavMeshQuery->findRandomPoint( &filter, RcRandom, &randomRef, glm::value_ptr( dest ) );
		if( status != DT_SUCCESS )
		{
			return std::unexpected( status );
		}

		return dest;
	}

}
