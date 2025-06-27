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
#include "RecastNavigationMeshCache.h"

#include "RecastCore.h"

#include "Saturn/Serialisation/RawSerialisation.h"

namespace Saturn {

	// .SR[N]C (SaturnRecast[Navigation]Cache)
	constexpr char SRNC_HEADER_MAGIC[ 4 ] = { '.', 'S', 'R', 'C' };

	struct RecastNavMeshCacheFileHeader
	{
		const char Magic[ 5 ] = ".SRC";
		uint32_t Version = 0;
		int TileCount = 0;
		dtNavMeshParams NavMeshParams;
	};

	struct RecastNavMeshTileCacheFileHeader
	{
		dtTileRef TileReference = 0;
	};

	void RecastNavigationMeshCache::SaveNavMesh( const std::filesystem::path& rPath, const dtNavMesh* pMesh )
	{
		std::ofstream fout( rPath, std::ios::binary | std::ios::trunc );

		RecastNavMeshCacheFileHeader header;
		header.Version = SAT_CURRENT_VERSION;
		
		for( int i = 0; i < pMesh->getMaxTiles(); i++ )
		{
			const dtMeshTile* pTile = pMesh->getTile( i );
			if( !pTile || !pTile->header || !pTile->dataSize ) 
				continue;

			header.TileCount++;
		}

		std::memcpy( &header.NavMeshParams, pMesh->getParams(), sizeof( dtNavMeshParams ) );
		
		RawSerialisation::WriteObject( header, fout );

		// Save tiles
		for( int i = 0; i < pMesh->getMaxTiles(); i++ )
		{
			const dtMeshTile* pTile = pMesh->getTile( i );
			if( !pTile || !pTile->header || !pTile->dataSize )
				continue;

			RecastNavMeshTileCacheFileHeader tileHeader{};
			tileHeader.TileReference = pMesh->getTileRef( pTile );

			RawSerialisation::WriteObject( tileHeader, fout );

			Buffer dataBuffer( pTile->dataSize, pTile->data );
			RawSerialisation::WriteSaturnBuffer( dataBuffer, fout );
		}

		fout.close();
	}

	dtNavMesh* RecastNavigationMeshCache::ReadNavMeshCache( const std::filesystem::path& rPath )
	{
		if( !std::filesystem::exists( rPath ) )
			return nullptr;

		std::ifstream stream( rPath, std::ios::binary | std::ios::in );

		RecastNavMeshCacheFileHeader header;
		RawSerialisation::ReadObject( header, stream );

		if( std::memcmp( header.Magic, ".SRC", 4 ) != 0 )
		{
			SAT_CORE_ERROR( "[RecastNavigationMeshCache] Failed to read Recast Navigation Mesh Cache! File magic does not match" );
			return nullptr;
		}

		if( header.Version != SAT_CURRENT_VERSION )
		{
			// Warning for now, until we make a major breaking change.
			SAT_CORE_WARN( "[RecastNavigationMeshCache] File version mismatch! continuing read!" );
		}

		SAT_CORE_INFO( "Recast Navigation Mesh Cache:" );
		SAT_CORE_INFO( " Version: {0}", header.Version );
		SAT_CORE_INFO( " Tile count: {0}", header.TileCount );
		SAT_CORE_INFO( "=====================================" );

		dtNavMesh* pNavMesh = dtAllocNavMesh();
		DT_CHECK( pNavMesh->init( &header.NavMeshParams ) );

		// Read tiles
		for( int i = 0; i < header.TileCount; i++ )
		{
			RecastNavMeshTileCacheFileHeader tileHeader{};
			RawSerialisation::ReadObject( tileHeader, stream );

			if( !tileHeader.TileReference )
				break;

			Buffer TemporaryBuffer;
			RawSerialisation::ReadSaturnBuffer( TemporaryBuffer, stream );

			// Pass on the data to Recast to assume ownership of.
			pNavMesh->addTile( TemporaryBuffer.Data, (int)TemporaryBuffer.Size, DT_TILE_FREE_DATA, tileHeader.TileReference, nullptr );
		}

		stream.close();

		return pNavMesh;
	}

}
