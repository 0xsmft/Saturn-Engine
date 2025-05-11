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

#include "Saturn/Vulkan/Renderer2D.h"

#include "RecastCore.h"

#include <Detour/DetourNavMeshBuilder.h>
#include <Detour/DetourDebugDraw.h>

#include <Recast/RecastDebugDraw.h>

#include <glm/gtc/type_ptr.hpp>

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// RecastContext
	
	RecastContext::RecastContext()
		: rcContext( false )
	{
		enableLog( true );
		enableTimer( true );

		for( auto& [label, timer] : m_Timers )
		{
			timer.Stop();
			timer.Reset();
		}
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

	void RecastContext::doResetTimers()
	{
		for( auto& [label, timer] : m_Timers )
		{
			timer.Reset();
		}
	}

	void RecastContext::doStartTimer( const rcTimerLabel label )
	{
		auto itr = m_Timers.find( label );

		if( itr != m_Timers.end() )
		{
			m_Timers[ label ].Reset();
		}
	}

	void RecastContext::doStopTimer( const rcTimerLabel label )
	{
		m_Timers[ label ].Stop();
	}

	int RecastContext::doGetAccumulatedTime( const rcTimerLabel label ) const
	{
		float ms = 0;
		for( auto& [label, timer] : m_Timers )
		{
			ms += timer.ElapsedMilliseconds();
		}

		return ( int ) ms;
	}

	//////////////////////////////////////////////////////////////////////////
	// RecastNavigationMeshBuilder

	RecastNavigationMeshBuilder::RecastNavigationMeshBuilder()
	{
	}

	RecastNavigationMeshBuilder::~RecastNavigationMeshBuilder()
	{
		dtFreeNavMesh( m_pNavMesh );
	}

	void RecastNavigationMeshBuilder::Init() 
	{
		m_Config.cs = 0.3f;
		m_Config.ch = 0.2f;
		m_Config.walkableSlopeAngle = 45.0f;
		m_Config.walkableHeight = ( int ) ceilf( 2.0f / m_Config.ch );
		m_Config.walkableClimb = ( int ) floorf( 0.9f / m_Config.ch );
		m_Config.walkableRadius = ( int ) ceilf( 0.6f / m_Config.cs );
		m_Config.maxEdgeLen = ( int ) ( 12.0f / m_Config.cs );
		m_Config.maxSimplificationError = 1.3f;
		m_Config.minRegionArea = ( int ) rcSqr( 8 );
		m_Config.mergeRegionArea = ( int ) rcSqr( 20 );
		m_Config.maxVertsPerPoly = 3;
		m_Config.tileSize = 32;
		m_Config.borderSize = 5;
		m_Config.detailSampleDist = 6.0f * m_Config.cs;
		m_Config.detailSampleMaxError = m_Config.detailSampleDist * m_Config.ch;
	}

	void RecastNavigationMeshBuilder::Build( const RecastInputGeometry& rInputGeometry )
	{
		float bmin[3], bmax[3];
		rcVcopy( bmin, glm::value_ptr( rInputGeometry.GetMinBounds() ) );
		rcVcopy( bmax, glm::value_ptr( rInputGeometry.GetMaxBounds() ) );

		int xw{}, xh{};
		rcCalcGridSize( bmin, bmax, m_Config.cs, &xw, &xh );

		const int tileWidth  = ( xw + m_Config.tileSize - 1 ) / m_Config.tileSize;
		const int tileHeight = ( xh + m_Config.tileSize - 1 ) / m_Config.tileSize;
		const float tileCellSize = m_Config.tileSize * m_Config.cs;

		dtNavMeshParams params{};
		rcVcopy( params.orig, bmin );
		params.tileWidth = m_Config.tileSize * m_Config.cs;
		params.tileHeight = m_Config.tileSize * m_Config.cs;
		params.maxTiles = tileWidth * tileHeight;
		params.maxPolys = 2048;

		m_pNavMesh = dtAllocNavMesh();
		m_pNavMesh->init( &params );

		for( int y = 0; y < tileHeight; y++ )
		{
			for( int x = 0; x < tileWidth; x++ )
			{
				m_LastTileMin.x = bmin[ 0 ] + x * tileCellSize;
				m_LastTileMin.y = bmin[ 1 ];
				m_LastTileMin.z = bmin[ 2 ] + y * tileCellSize;

				m_LastTileMax.x = bmin[ 0 ] + ( x + 1 ) * tileCellSize;
				m_LastTileMax.y = bmax[ 1 ];
				m_LastTileMax.z = bmin[ 2 ] + ( y + 1 ) * tileCellSize;

				int dataSize = 0;
				unsigned char* pData = NavBuildTile( x, y, rInputGeometry, dataSize );

				if( pData )
				{
					m_pNavMesh->removeTile( m_pNavMesh->getTileRefAt( x, y, 0 ), nullptr, 0 );

					auto status = m_pNavMesh->addTile( pData, dataSize, DT_TILE_FREE_DATA, 0, 0 );
					if( dtStatusFailed( status ) )
						dtFree( pData );
				}
			}
		}
	}

	unsigned char* RecastNavigationMeshBuilder::NavBuildTile( int x, int y, const RecastInputGeometry& rInputGeometry, int& rOutDataSize )
	{
		if( m_PolyMesh )
		{
			rcFreePolyMesh( m_PolyMesh );
			m_PolyMesh = nullptr;

			rcFreePolyMeshDetail( m_PolyMeshDetail );
			m_PolyMeshDetail = nullptr;

			delete[] m_AreaFlags;
			m_AreaFlags = nullptr;
		}

		rcVcopy( m_Config.bmin, glm::value_ptr( m_LastTileMin ) );
		rcVcopy( m_Config.bmax, glm::value_ptr( m_LastTileMax ) );

		m_Context.resetTimers();

		m_Context.startTimer( RC_TIMER_TOTAL );
		
		m_Config.borderSize = m_Config.walkableRadius + 3;
		m_Config.width = m_Config.tileSize + m_Config.borderSize * 2;
		m_Config.height = m_Config.tileSize + m_Config.borderSize * 2;

		m_Config.bmin[ 0 ] -= m_Config.borderSize * m_Config.cs;
		m_Config.bmin[ 2 ] -= m_Config.borderSize * m_Config.cs;

		m_Config.bmax[ 0 ] += m_Config.borderSize * m_Config.cs;
		m_Config.bmax[ 2 ] += m_Config.borderSize * m_Config.cs;

		m_Context.log( RC_LOG_PROGRESS, "Building NavMesh" );
		m_Context.log( RC_LOG_PROGRESS, " %d x %d cells", m_Config.width, m_Config.height );

		rcHeightfield* pHeightfield = rcAllocHeightfield();
		RC_CHECK( rcCreateHeightfield(
			&m_Context,
			*pHeightfield,
			m_Config.width,
			m_Config.height,
			m_Config.bmin,
			m_Config.bmax,
			m_Config.cs,
			m_Config.ch ) );

		const rcChunkyTriMesh* pChunkyTriMesh = rInputGeometry.GetChunkyTriMesh();
		const auto& vb = rInputGeometry.GetVertexBuffer();
		const auto& ib = rInputGeometry.GetIndexBuffer();

		// indices count
		m_AreaFlags = new unsigned char[ pChunkyTriMesh->maxTrisPerChunk ];

		float tbmin[ 2 ], tbmax[ 2 ];
		tbmin[ 0 ] = m_Config.bmin[ 0 ];
		tbmin[ 1 ] = m_Config.bmin[ 2 ];
		tbmax[ 0 ] = m_Config.bmax[ 0 ];
		tbmax[ 1 ] = m_Config.bmax[ 2 ];

		int cid[ 512 ];
		const int ncid = rcGetChunksOverlappingRect( pChunkyTriMesh, tbmin, tbmax, cid, 512 );
		SAT_CORE_INFO( "Tile ({0}, {1}) has {2} overlapping chunks", x, y, ncid );

		if( !ncid )
			return nullptr;

		for( int i = 0; i < ncid; i++ )
		{
			const rcChunkyTriMeshNode& rNode = pChunkyTriMesh->nodes[ cid[ i ] ];
			const int* pTris = &pChunkyTriMesh->tris[ rNode.i * 3 ];
			const int nTri = rNode.n;

			memset( m_AreaFlags, 0, nTri * sizeof( unsigned char ) );
			rcMarkWalkableTriangles( 
				&m_Context, m_Config.walkableSlopeAngle,
				vb.data(), vb.size() / 3,
				pTris, nTri, 
				m_AreaFlags );

			RC_CHECK( rcRasterizeTriangles( &m_Context,
				vb.data(), vb.size() / 3,
				pTris, m_AreaFlags, nTri, *pHeightfield, m_Config.walkableClimb ) );
			
			m_Context.log( RC_LOG_PROGRESS, "Rasterised %d triangles into heightfield", nTri );
		}

		delete[] m_AreaFlags; 
		m_AreaFlags = nullptr;

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

		if( pContourSet->nconts == 0 ) 
		{
			return nullptr;
		}

		m_PolyMesh = rcAllocPolyMesh();
		RC_CHECK( rcBuildPolyMesh( &m_Context, *pContourSet, m_Config.maxVertsPerPoly, *m_PolyMesh ) );

		m_PolyMeshDetail = rcAllocPolyMeshDetail();
		RC_CHECK( rcBuildPolyMeshDetail(
			&m_Context,
			*m_PolyMesh,
			*pCompactHeightfield,
			m_Config.maxVertsPerPoly,
			m_Config.maxSimplificationError,
			*m_PolyMeshDetail ) );

		rcFreeCompactHeightfield( pCompactHeightfield );
		rcFreeContourSet( pContourSet );
		pCompactHeightfield = nullptr;
		pContourSet = nullptr;

		unsigned char* pNavData = nullptr;
		int navDataSize = 0;

		if( m_Config.maxVertsPerPoly <= DT_VERTS_PER_POLYGON )
		{
			if( m_PolyMesh->nverts >= 0xFFFF )  
			{
				m_Context.log( RC_LOG_ERROR, "Too many verts per tile! current/%d maximum/%d", m_PolyMesh->nverts, 0xFFFF );
				SAT_CORE_ASSERT( false );
			}

			dtNavMeshCreateParams params{};
			params.verts = m_PolyMesh->verts;
			params.vertCount = m_PolyMesh->nverts;
			params.polys = m_PolyMesh->polys;
			params.polyAreas = m_PolyMesh->areas;
			params.polyFlags = m_PolyMesh->flags;
			params.polyCount = m_PolyMesh->npolys;
			params.nvp = m_PolyMesh->nvp;

			params.detailMeshes = m_PolyMeshDetail->meshes;
			params.detailVerts = m_PolyMeshDetail->verts;
			params.detailVertsCount = m_PolyMeshDetail->nverts;
			params.detailTris = m_PolyMeshDetail->tris;
			params.detailTriCount = m_PolyMeshDetail->ntris;

			params.walkableHeight = m_Config.walkableHeight * m_Config.ch;
			params.walkableRadius = m_Config.walkableRadius * m_Config.cs;
			params.walkableClimb = m_Config.walkableClimb * m_Config.ch;

			params.tileX = x;
			params.tileY = y;

			rcVcopy( params.bmin, m_PolyMesh->bmin );
			rcVcopy( params.bmax, m_PolyMesh->bmax );

			params.cs = m_Config.cs;
			params.ch = m_Config.ch;
			params.buildBvTree = true;

			m_Context.log( RC_LOG_PROGRESS, "NavMesh bounds: min(%.2f, %.2f, %.2f) max(%.2f, %.2f, %.2f)",
				params.bmin[ 0 ], params.bmin[ 1 ], params.bmin[ 2 ],
				params.bmax[ 0 ], params.bmax[ 1 ], params.bmax[ 2 ] );

			RC_CHECK( dtCreateNavMeshData( &params, &pNavData, &navDataSize ) );
		}

		m_Context.stopTimer( RC_TIMER_TOTAL );
		m_Context.log( RC_LOG_PROGRESS, "Building took: %i ms", m_Context.getAccumulatedTime( RC_TIMER_TOTAL ) );
		m_Context.log( RC_LOG_PROGRESS, "Tile mesh: %i verts %i ploys", m_PolyMesh->nverts, m_PolyMesh->npolys );

		rOutDataSize = navDataSize;
		return pNavData;
	}

	void RecastNavigationMeshBuilder::DebugDraw()
	{
		duDebugDrawNavMesh( &m_DebugDrawer, *m_pNavMesh, DU_DRAWNAVMESH_COLOR_TILES );
	}

}