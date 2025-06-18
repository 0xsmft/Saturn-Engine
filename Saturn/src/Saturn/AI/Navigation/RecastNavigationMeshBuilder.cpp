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
#include "RecastNavigationMeshBuilder.h"

#include "Saturn/Vulkan/Renderer2D.h"

#include "RecastCore.h"
#include "RecastNavigationMeshCache.h"

#include <Detour/DetourNavMeshBuilder.h>
#include <Detour/DetourDebugDraw.h>

#include <Recast/RecastDebugDraw.h>
#include <Recast/RecastChunkyTriMesh.h>

#include <glm/gtc/type_ptr.hpp>

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// RecastContext

	RecastContext::RecastContext()
#if !defined(SAT_DIST)
		: rcContext( true )
	{
		for( auto& [label, timer] : m_Timers )
		{
			timer.Stop();
			timer.Reset();
		}
	}
#else
		: rcContext( false )
	{
	}
#endif

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

#if !defined(SAT_DIST)
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
#endif

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
		m_Config.walkableHeight = ( int ) glm::ceil( 2.0f / m_Config.ch );
		m_Config.walkableClimb = ( int ) glm::floor( 0.9f / m_Config.ch );
		m_Config.walkableRadius = ( int ) glm::ceil( 0.6f / m_Config.cs );
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

				RecastNavigationTileBuilderData tileData{ .LastTileMin = m_LastTileMin, .LastTileMax = m_LastTileMax, .KeepResults = false };
				RecastNavigationTileBuilder tb( tileData, x, y );

				if( tb.Build( m_pNavMesh, m_Context, rInputGeometry ) ) 
				{
					SAT_CORE_INFO( "[RecastNavMesh]: Tile ({0}, {1}) added!", x, y );
				}
			}
		}
	}

	bool RecastNavigationMeshBuilder::TryLoadFromCache( const std::filesystem::path& rPath )
	{
		if( m_pNavMesh )
		{
			dtFreeNavMesh( m_pNavMesh );
			m_pNavMesh = nullptr;
		}

		m_pNavMesh = RecastNavigationMeshCache::ReadNavMeshCache( rPath );

		return m_pNavMesh != nullptr;
	}

	void RecastNavigationMeshBuilder::DebugDraw()
	{
#if !defined(SAT_DIST)
		if( m_pNavMesh ) 
			duDebugDrawNavMesh( &m_DebugDrawer, *m_pNavMesh, 0 );
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// RecastNavigationTileBuilder

	RecastNavigationTileBuilder::RecastNavigationTileBuilder( RecastNavigationTileBuilderData& rData, int x, int y )
		: m_Data( rData ), m_X( x ), m_Y( y )
	{
	}

	void RecastNavigationTileBuilder::FreeAll()
	{
		if( m_pPolyMesh )
			rcFreePolyMesh( m_pPolyMesh );

		if( m_pPolyMeshDetail )
			rcFreePolyMeshDetail( m_pPolyMeshDetail );

		if( m_pSolidHeightfield )
			rcFreeHeightField( m_pSolidHeightfield );

		if( m_pCompactHeightfield )
			rcFreeCompactHeightfield( m_pCompactHeightfield );

		if( m_pContourSet )
			rcFreeContourSet( m_pContourSet );

		m_pPolyMesh = nullptr;
		m_pPolyMeshDetail = nullptr;
		m_pSolidHeightfield = nullptr;
		m_pCompactHeightfield = nullptr;
		m_pContourSet = nullptr;
	}

	RecastNavigationTileBuilder::~RecastNavigationTileBuilder()
	{
		FreeAll();
	}

	bool RecastNavigationTileBuilder::Build( dtNavMesh* pParentMesh, RecastContext& rContext, const RecastInputGeometry& rInputGeometry )
	{
		if( m_pPolyMesh )
		{
			FreeAll();
		}

		// Build new tile
		// Reset timers
		rContext.resetTimers();
		rContext.startTimer( RC_TIMER_TOTAL );

		// Setup local config for this tile
		// TODO: Copy the values from the master config as these values should not change
		rcConfig config{};
		config.cs = 0.3f;
		config.ch = 0.2f;
		config.walkableSlopeAngle = 45.0f;
		config.walkableHeight = ( int ) ceilf( 2.0f / config.ch );
		config.walkableClimb = ( int ) floorf( 0.9f / config.ch );
		config.walkableRadius = ( int ) ceilf( 0.6f / config.cs );
		config.maxEdgeLen = ( int ) ( 12.0f / config.cs );
		config.maxSimplificationError = 1.3f;
		config.minRegionArea = ( int ) rcSqr( 8 );
		config.mergeRegionArea = ( int ) rcSqr( 20 );
		config.maxVertsPerPoly = 3;
		config.tileSize = 32;
		config.borderSize = config.walkableRadius + 3;
		config.detailSampleDist = 6.0f * config.cs;
		config.detailSampleMaxError = config.detailSampleDist * config.ch;
		config.width = config.tileSize + config.borderSize * 2;
		config.height = config.tileSize + config.borderSize * 2;

		// Adjust for this tile
		rcVcopy( config.bmin, glm::value_ptr( m_Data.LastTileMin ) );
		rcVcopy( config.bmax, glm::value_ptr( m_Data.LastTileMax ) );

		config.bmin[ 0 ] -= config.borderSize * config.cs;
		config.bmin[ 2 ] -= config.borderSize * config.cs;

		config.bmax[ 0 ] += config.borderSize * config.cs;
		config.bmax[ 2 ] += config.borderSize * config.cs;

		rContext.log( RC_LOG_PROGRESS, "Building NavMesh" );
		rContext.log( RC_LOG_PROGRESS, " %d x %d cells", config.width, config.height );

		m_pSolidHeightfield = rcAllocHeightfield();
		RC_CHECK( rcCreateHeightfield(
			&rContext,
			*m_pSolidHeightfield,
			config.width,
			config.height,
			config.bmin,
			config.bmax,
			config.cs,
			config.ch ) );

		const rcChunkyTriMesh* pChunkyTriMesh = rInputGeometry.GetChunkyTriMesh();
		const auto& vb = rInputGeometry.GetVertexBuffer();
		const auto& ib = rInputGeometry.GetIndexBuffer();

		// indices count
		m_AreaFlags = new unsigned char[ pChunkyTriMesh->maxTrisPerChunk ];

		float tbmin[ 2 ], tbmax[ 2 ];
		tbmin[ 0 ] = config.bmin[ 0 ];
		tbmin[ 1 ] = config.bmin[ 2 ];
		tbmax[ 0 ] = config.bmax[ 0 ];
		tbmax[ 1 ] = config.bmax[ 2 ];

		int cid[ 512 ];
		const int ncid = rcGetChunksOverlappingRect( pChunkyTriMesh, tbmin, tbmax, cid, 512 );
		SAT_CORE_INFO( "Tile ({0}, {1}) has {2} overlapping chunks", m_X, m_Y, ncid );

		if( !ncid )
			return false;

		for( int i = 0; i < ncid; i++ )
		{
			const rcChunkyTriMeshNode& rNode = pChunkyTriMesh->nodes[ cid[ i ] ];
			const int* pTris = &pChunkyTriMesh->tris[ rNode.i * 3 ];
			const int nTri = rNode.n;

			memset( m_AreaFlags, 0, nTri * sizeof( unsigned char ) );
			rcMarkWalkableTriangles(
				&rContext, config.walkableSlopeAngle,
				vb.data(), ( int ) vb.size() / 3u,
				pTris, nTri,
				m_AreaFlags );

			RC_CHECK( rcRasterizeTriangles( &rContext,
				vb.data(), ( int ) vb.size() / 3u,
				pTris, m_AreaFlags, nTri, *m_pSolidHeightfield, config.walkableClimb ) );

			rContext.log( RC_LOG_PROGRESS, "Rasterised %d triangles into heightfield", nTri );
		}

		delete[] m_AreaFlags;
		m_AreaFlags = nullptr;

		rcFilterLowHangingWalkableObstacles( &rContext, config.walkableClimb, *m_pSolidHeightfield );
		rcFilterLedgeSpans( &rContext, config.walkableHeight, config.walkableClimb, *m_pSolidHeightfield );
		rcFilterWalkableLowHeightSpans( &rContext, config.walkableHeight, *m_pSolidHeightfield );

		m_pCompactHeightfield = rcAllocCompactHeightfield();
		RC_CHECK( rcBuildCompactHeightfield(
			&rContext,
			config.walkableHeight,
			config.walkableClimb,
			*m_pSolidHeightfield,
			*m_pCompactHeightfield ) );

		rContext.log( RC_LOG_PROGRESS, "Span count: %d", m_pCompactHeightfield->spanCount );

		rcFreeHeightField( m_pSolidHeightfield );
		m_pSolidHeightfield = nullptr;

		RC_CHECK( rcErodeWalkableArea( &rContext, config.walkableRadius, *m_pCompactHeightfield ) );

		// Watershed
		RC_CHECK( rcBuildDistanceField( &rContext, *m_pCompactHeightfield ) );
		RC_CHECK( rcBuildRegions( &rContext, *m_pCompactHeightfield, config.borderSize, config.minRegionArea, config.mergeRegionArea ) );

		m_pContourSet = rcAllocContourSet();
		RC_CHECK( rcBuildContours(
			&rContext,
			*m_pCompactHeightfield,
			config.maxSimplificationError,
			config.maxEdgeLen,
			*m_pContourSet ) );

		if( m_pContourSet->nconts == 0 )
		{
			rContext.log( RC_LOG_WARNING, "No contours, skipping!" );
			return false;
		}

		m_pPolyMesh = rcAllocPolyMesh();
		RC_CHECK( rcBuildPolyMesh( &rContext, *m_pContourSet, config.maxVertsPerPoly, *m_pPolyMesh ) );

		m_pPolyMeshDetail = rcAllocPolyMeshDetail();
		RC_CHECK( rcBuildPolyMeshDetail(
			&rContext,
			*m_pPolyMesh,
			*m_pCompactHeightfield,
			( float ) config.maxVertsPerPoly,
			config.maxSimplificationError,
			*m_pPolyMeshDetail ) );

		rcFreeCompactHeightfield( m_pCompactHeightfield );
		rcFreeContourSet( m_pContourSet );
		m_pCompactHeightfield = nullptr;
		m_pContourSet = nullptr;

		if( config.maxVertsPerPoly <= DT_VERTS_PER_POLYGON )
		{
			if( m_pPolyMesh->nverts >= 0xFFFF )
			{
				rContext.log( RC_LOG_ERROR, "Too many verts per tile! current/%d maximum/%d", m_pPolyMesh->nverts, 0xFFFF );
				SAT_CORE_ASSERT( false );
				return false;
			}

			// Update poly flags from areas.
			for( int i = 0; i < m_pPolyMesh->npolys; ++i )
			{
				if( m_pPolyMesh->areas[ i ] == RC_WALKABLE_AREA )
					m_pPolyMesh->areas[ i ] = (std::underlying_type_t<NavigationMeshArea> ) NavigationMeshArea::Ground;

				if( m_pPolyMesh->areas[ i ] == ( int ) NavigationMeshArea::Ground ||
					m_pPolyMesh->areas[ i ] == ( int ) NavigationMeshArea::Grass ||
					m_pPolyMesh->areas[ i ] == ( int ) NavigationMeshArea::Road )
				{
					m_pPolyMesh->flags[ i ] = NavigationMeshPolyFlag_Walk;
				}
				else if( m_pPolyMesh->areas[ i ] == ( std::underlying_type_t<NavigationMeshArea> )NavigationMeshArea::Water )
				{
					m_pPolyMesh->flags[ i ] = NavigationMeshPolyFlag_Swim;
				}
				else if( m_pPolyMesh->areas[ i ] == ( std::underlying_type_t<NavigationMeshArea> )NavigationMeshArea::Door )
				{
					m_pPolyMesh->flags[ i ] = NavigationMeshPolyFlag_Walk | NavigationMeshPolyFlag_Door;
				}
			}

			dtNavMeshCreateParams params{};
			params.verts = m_pPolyMesh->verts;
			params.vertCount = m_pPolyMesh->nverts;
			params.polys = m_pPolyMesh->polys;
			params.polyAreas = m_pPolyMesh->areas;
			params.polyFlags = m_pPolyMesh->flags;
			params.polyCount = m_pPolyMesh->npolys;
			params.nvp = m_pPolyMesh->nvp;

			params.detailMeshes = m_pPolyMeshDetail->meshes;
			params.detailVerts = m_pPolyMeshDetail->verts;
			params.detailVertsCount = m_pPolyMeshDetail->nverts;
			params.detailTris = m_pPolyMeshDetail->tris;
			params.detailTriCount = m_pPolyMeshDetail->ntris;

			params.walkableHeight = config.walkableHeight * config.ch;
			params.walkableRadius = config.walkableRadius * config.cs;
			params.walkableClimb = config.walkableClimb * config.ch;

			params.tileX = m_X;
			params.tileY = m_Y;

			rcVcopy( params.bmin, m_pPolyMesh->bmin );
			rcVcopy( params.bmax, m_pPolyMesh->bmax );

			params.cs = config.cs;
			params.ch = config.ch;
			params.buildBvTree = true;

			rContext.log( RC_LOG_PROGRESS, "NavMesh bounds: min(%.2f, %.2f, %.2f) max(%.2f, %.2f, %.2f)",
				params.bmin[ 0 ], params.bmin[ 1 ], params.bmin[ 2 ],
				params.bmax[ 0 ], params.bmax[ 1 ], params.bmax[ 2 ] );

			unsigned char* pNavData = nullptr;
			int navDataSize = 0;
			RC_CHECK( dtCreateNavMeshData( &params, &pNavData, &navDataSize ) );

			if( pNavData )
			{
				pParentMesh->removeTile( pParentMesh->getTileRefAt( m_X, m_Y, 0 ), nullptr, 0 );

				// Using DT_TILE_FREE_DATA means that Recast will assume ownership of the data.
				auto status = pParentMesh->addTile( pNavData, navDataSize, DT_TILE_FREE_DATA, 0, 0 );
			
				if( dtStatusFailed( status ) )
					dtFree( pNavData );

				MarkDirty();
			}
		}

		rContext.stopTimer( RC_TIMER_TOTAL );
		rContext.log( RC_LOG_PROGRESS, "Building took: %i ms", rContext.getAccumulatedTime( RC_TIMER_TOTAL ) );
		rContext.log( RC_LOG_PROGRESS, "Tile mesh: %i verts %i ploys", m_pPolyMesh->nverts, m_pPolyMesh->npolys );

		return true;
	}

	void RecastNavigationTileBuilder::RemoveTile( dtNavMesh* pParentMesh )
	{
		if( dtStatusSucceed( pParentMesh->removeTile( pParentMesh->getTileRefAt( m_X, m_Y, 0 ), nullptr, 0 ) ) )
		{
			FreeAll();
			CleanDirty();
		}
	}

}
