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
#include "RecastNavigationMesh.h"

#include "RecastCore.h"

#include "Saturn/Vulkan/Renderer2D.h"

#include "DetourNavMeshBuilder.h"

#include <glm/gtc/type_ptr.hpp>

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// RecastContext
	
	RecastContext::RecastContext()
		: rcContext( false )
	{
		enableLog( true );
		enableTimer( true );
	}

	void RecastContext::doLog( const rcLogCategory category, const char* pMessage, const int len )
	{
		switch( category )
		{
			case RC_LOG_ERROR:
				SAT_CORE_ERROR( "Recast Navigation: {0}", pMessage );
				break;

			case RC_LOG_WARNING:
				SAT_CORE_WARN( "Recast Navigation: {0}", pMessage );
				break;

			case RC_LOG_PROGRESS:
				SAT_CORE_INFO( "Recast Navigation: {0}", pMessage );
				break;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// RecastNavigationMeshBuilder

	RecastNavigationMeshBuilder::RecastNavigationMeshBuilder( AABB bounds )
		: m_Bounds( bounds )
	{
	}

	RecastNavigationMeshBuilder::RecastNavigationMeshBuilder()
	{
	}

	RecastNavigationMeshBuilder::~RecastNavigationMeshBuilder()
	{
		dtFreeNavMesh( m_pDMesh );
	}

	void RecastNavigationMeshBuilder::SetBounds( AABB bounds )
	{
		m_Bounds = bounds;
	}

	void RecastNavigationMeshBuilder::Init() 
	{
		m_Config.cs = 0.3f;
		m_Config.ch = 0.2f;
		m_Config.walkableSlopeAngle = 45.0f;
		m_Config.walkableHeight = ( int ) glm::ceil( 2.0f / m_Config.ch );
		m_Config.walkableClimb = ( int ) glm::floor( 0.9f / m_Config.ch );
		m_Config.walkableRadius = ( int ) glm::ceil( 0.6f / m_Config.cs );
		m_Config.maxEdgeLen = ( int ) ( 12.0f / m_Config.cs );
		m_Config.maxSimplificationError = 1.3f;
		m_Config.minRegionArea = ( int ) glm::sqrt( 8 );
		m_Config.mergeRegionArea = ( int ) glm::sqrt( 20 );
		m_Config.maxVertsPerPoly = 3;
		m_Config.tileSize = 48;
		m_Config.borderSize = 3;
	}

	void RecastNavigationMeshBuilder::Build( const RecastInputGeometry& rInputGeometry )
	{
		rcVcopy( m_Config.bmin, glm::value_ptr( rInputGeometry.GetMinBounds() ) );
		rcVcopy( m_Config.bmax, glm::value_ptr( rInputGeometry.GetMaxBounds() ) );

		int xw{}, xh{};
		rcCalcGridSize( m_Config.bmin, m_Config.bmax, m_Config.cs, &xw, &xh );

		const int tileWidth  = ( xw + m_Config.tileSize - 1 ) / m_Config.tileSize;
		const int tileHeight = ( xh + m_Config.tileSize - 1 ) / m_Config.tileSize;
		const float tileCellSize = m_Config.tileSize * m_Config.cs;

		dtNavMeshParams params{};
		rcVcopy( params.orig, m_Config.bmin );
		params.tileWidth = m_Config.tileSize * m_Config.cs;
		params.tileHeight = m_Config.tileSize * m_Config.cs;
		params.maxTiles = tileWidth * tileHeight;
		params.maxPolys = 2048;

		m_pDMesh = dtAllocNavMesh();
		m_pDMesh->init( &params );

		for( int y = 0; y < tileHeight; y++ )
		{
			for( int x = 0; x < tileWidth; x++ )
			{
				m_LastTileMin.x = m_Config.bmin[ 0 ] + x * tileCellSize;
				m_LastTileMin.y = m_Config.bmin[ 1 ];
				m_LastTileMin.z = m_Config.bmin[ 2 ] + y * tileCellSize;

				m_LastTileMax.x = m_Config.bmin[ 0 ] + ( x + 1 ) * tileCellSize;
				m_LastTileMax.y = m_Config.bmax[ 1 ];
				m_LastTileMax.z = m_Config.bmin[ 2 ] + ( y + 1 ) * tileCellSize;

				NavBuildTile( x, y, rInputGeometry );
			}
		}
	}

	void RecastNavigationMeshBuilder::NavBuildTile( int x, int y, const RecastInputGeometry& rInputGeometry )
	{
		float bmin[ 3 ], bmax[ 3 ];
		rcVcopy( bmin, glm::value_ptr( m_LastTileMin ) );
		rcVcopy( bmax, glm::value_ptr( m_LastTileMax ) );

		m_Context.resetTimers();

		m_Context.startTimer( RC_TIMER_TOTAL );
		
		m_Config.borderSize = m_Config.walkableRadius + 3;
		m_Config.width = m_Config.tileSize + m_Config.borderSize * 2;
		m_Config.height = m_Config.tileSize + m_Config.borderSize * 2;

		bmin[ 0 ] -= m_Config.borderSize * m_Config.cs;
		bmin[ 2 ] -= m_Config.borderSize * m_Config.cs;

		bmax[ 0 ] += m_Config.borderSize * m_Config.cs;
		bmax[ 2 ] += m_Config.borderSize * m_Config.cs;

		m_Context.log( RC_LOG_PROGRESS, "Building NavMesh" );
		m_Context.log( RC_LOG_PROGRESS, " %d x %d cells", m_Config.width, m_Config.height );

		rcHeightfield* pHeightfield = rcAllocHeightfield();
		RC_CHECK( rcCreateHeightfield(
			&m_Context,
			*pHeightfield,
			m_Config.width,
			m_Config.height,
			bmin,
			bmax,
			m_Config.cs,
			m_Config.ch ) );

		const rcChunkyTriMesh* pChunkyTriMesh = rInputGeometry.GetChunkyTriMesh();
		const auto& vb = rInputGeometry.GetVertexBuffer();
		const auto& ib = rInputGeometry.GetIndexBuffer();

		// indices count
		size_t triCount = pChunkyTriMesh->maxTrisPerChunk;
		auto areaFlags = new unsigned char[ triCount ];

		float tbmin[ 2 ], tbmax[ 2 ];
		tbmin[ 0 ] = bmin[ 0 ];
		tbmin[ 1 ] = bmin[ 2 ];
		tbmax[ 0 ] = bmax[ 0 ];
		tbmax[ 1 ] = bmax[ 2 ];

		int cid[ 512 ];
		const int ncid = rcGetChunksOverlappingRect( pChunkyTriMesh, tbmin, tbmax, cid, 512 );
		SAT_CORE_INFO( "Tile ({0}, {1}) has {2} overlapping chunks", x, y, ncid );

		for( int i = 0; i < ncid; i++ )
		{
			const rcChunkyTriMeshNode& rNode = pChunkyTriMesh->nodes[ cid[ i ] ];
			const int* pTris = &pChunkyTriMesh->tris[ rNode.i * 3 ];
			const int nTri = rNode.n;

			rcMarkWalkableTriangles( 
				&m_Context, m_Config.walkableSlopeAngle,
				vb.data(), vb.size() / 3,
				pTris, nTri, 
				areaFlags );

			RC_CHECK( rcRasterizeTriangles( &m_Context,
				vb.data(), vb.size() / 3,
				pTris, areaFlags, nTri, *pHeightfield, m_Config.walkableClimb ) );
			
			m_Context.log( RC_LOG_PROGRESS, "Rasterised %d triangles into heightfield", nTri );
		}

		delete[] areaFlags; 
		areaFlags = nullptr;

		rcFilterLowHangingWalkableObstacles( &m_Context, m_Config.walkableClimb, *pHeightfield );
		rcFilterLedgeSpans( &m_Context, m_Config.walkableHeight, m_Config.walkableClimb, *pHeightfield );
		rcFilterWalkableLowHeightSpans( &m_Context, m_Config.walkableHeight, *pHeightfield );

		rcCompactHeightfield* pCompactHeightfield = rcAllocCompactHeightfield();
		RC_CHECK( rcBuildCompactHeightfield(
			&m_Context,
			m_Config.walkableHeight,
			m_Config.walkableClimb,
			*pHeightfield,
			*pCompactHeightfield ) );

		m_Context.log( RC_LOG_PROGRESS, "Span count: %d", pCompactHeightfield->spanCount );

		rcFreeHeightField( pHeightfield );
		pHeightfield = nullptr;

		RC_CHECK( rcErodeWalkableArea( &m_Context, m_Config.walkableRadius, *pCompactHeightfield ) );

		// Watershed
		RC_CHECK( rcBuildDistanceField( &m_Context, *pCompactHeightfield ) );
		RC_CHECK( rcBuildRegions( &m_Context, *pCompactHeightfield, m_Config.borderSize, m_Config.minRegionArea, m_Config.mergeRegionArea ) );

		rcContourSet* pContourSet = rcAllocContourSet();
		RC_CHECK( rcBuildContours(
			&m_Context,
			*pCompactHeightfield,
			m_Config.maxSimplificationError,
			m_Config.maxEdgeLen,
			*pContourSet ) );

		rcPolyMesh* polyMesh = rcAllocPolyMesh();
		RC_CHECK( rcBuildPolyMesh( &m_Context, *pContourSet, m_Config.maxVertsPerPoly, *polyMesh ) );

		rcPolyMeshDetail* pPolyMeshDetail = rcAllocPolyMeshDetail();
		RC_CHECK( rcBuildPolyMeshDetail(
			&m_Context,
			*polyMesh,
			*pCompactHeightfield,
			m_Config.maxVertsPerPoly,
			m_Config.maxSimplificationError,
			*pPolyMeshDetail ) );

		rcFreeCompactHeightfield( pCompactHeightfield );
		rcFreeContourSet( pContourSet );
		pCompactHeightfield = nullptr;
		pContourSet = nullptr;

		if( m_Config.maxVertsPerPoly <= DT_VERTS_PER_POLYGON )
		{
			if( polyMesh->nverts >= 0xFFFF )  
			{
				m_Context.log( RC_LOG_ERROR, "Too many verts per tile! current/%d maximum/%d", polyMesh->nverts, 0xFFFF );
				SAT_CORE_ASSERT( false );
			}

			// TODO: Fix why this happends, most likely due to out IA
			if( polyMesh->nverts == 0 )
			{
				SAT_CORE_INFO( "polyMesh->nverts == 0" );
				rcFreePolyMesh( polyMesh );
				rcFreePolyMeshDetail( pPolyMeshDetail );
				return;
			}

			dtNavMeshCreateParams params{};
			params.verts = polyMesh->verts;
			params.vertCount = polyMesh->nverts;
			params.polys = polyMesh->polys;
			params.polyAreas = polyMesh->areas;
			params.polyFlags = polyMesh->flags;
			params.polyCount = polyMesh->npolys;
			params.nvp = polyMesh->nvp;

			params.detailMeshes = pPolyMeshDetail->meshes;
			params.detailVerts = pPolyMeshDetail->verts;
			params.detailVertsCount = pPolyMeshDetail->nverts;
			params.detailTris = pPolyMeshDetail->tris;
			params.detailTriCount = pPolyMeshDetail->ntris;

			params.walkableHeight = m_Config.walkableHeight * m_Config.ch;
			params.walkableRadius = m_Config.walkableRadius * m_Config.cs;
			params.walkableClimb = m_Config.walkableClimb * m_Config.ch;

			params.tileX = x;
			params.tileY = y;

			rcVcopy( params.bmin, polyMesh->bmin );
			rcVcopy( params.bmax, polyMesh->bmax );

			params.cs = m_Config.cs;
			params.ch = m_Config.ch;
			params.buildBvTree = true;

			unsigned char* pNavData = nullptr;
			int navDataSize = 0;

			m_Context.log( RC_LOG_PROGRESS, "NavMesh bounds: min(%.2f, %.2f, %.2f) max(%.2f, %.2f, %.2f)",
				params.bmin[ 0 ], params.bmin[ 1 ], params.bmin[ 2 ],
				params.bmax[ 0 ], params.bmax[ 1 ], params.bmax[ 2 ] );

			RC_CHECK( dtCreateNavMeshData( &params, &pNavData, &navDataSize ) );

			if( pNavData )
			{
				DT_CHECK( m_pDMesh->addTile( pNavData, navDataSize, DT_TILE_FREE_DATA, 0, nullptr ) );

				SAT_CORE_INFO( "[RecastNavigationMesh] Added Tile: X/{0} Y/{1}", x, y );
			}
		}

		m_Context.stopTimer( RC_TIMER_TOTAL );
		m_Context.log( RC_LOG_PROGRESS, "Building took: %i", m_Context.getAccumulatedTime( RC_TIMER_TOTAL ) );
		m_Context.log( RC_LOG_PROGRESS, "Tile mesh: %i verts %i ploys", polyMesh->nverts, polyMesh->npolys );

		rcFreePolyMesh( polyMesh );
		rcFreePolyMeshDetail( pPolyMeshDetail );
	}

	void RecastNavigationMeshBuilder::DebugDraw()
	{
		const auto* pMesh = m_pDMesh;

		for( int i = 0; i < m_pDMesh->getMaxTiles(); i++ )
		{
			const auto* pTile = pMesh->getTile( i );
			if( !pTile || !pTile->header ) continue;

			for( int j = 0; j < pTile->header->polyCount; j++ )
			{
				const dtPoly* pPoly = &pTile->polys[ j ];
				if( pPoly->getType() != DT_POLYTYPE_GROUND ) continue;

				std::vector<glm::vec3> polyVerts;
				for( int j = 0; j < pPoly->vertCount; ++j )
				{
					const unsigned short vi = pPoly->verts[ j ];
					const float* v = &pTile->verts[ vi * 3 ];

					polyVerts.push_back( glm::vec3( v[ 0 ], v[ 1 ], v[ 2 ] ) );
				}

				for( size_t j = 0; j < polyVerts.size(); ++j )
				{
					const glm::vec3& v0 = polyVerts[ j ];
					const glm::vec3& v1 = polyVerts[ ( j + 1 ) % polyVerts.size() ];

					Renderer2D::Get().SubmitLine( v0, v1, glm::vec4{ 1.0f } );
				}
			}
		}
	}

}