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
#include "RecastNavigationMeshCache.h"

#include "RecastCore.h"

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

namespace Saturn {

	struct RecastNavMeshCacheFileHeader
	{
		// .SR[N]C (SaturnRecast[Navigation]Cache)
		const unsigned char Magic[ 4 ] = { 0x2E, 0x53, 0x52, 0x43 };

		RecastNavMeshCacheVersion Version = RecastNavMeshCacheVersion::Lowest;

		int TileCount = 0;
		dtNavMeshParams NavMeshParams{};
	};

	struct RecastNavMeshTileCacheFileHeader
	{
		dtTileRef TileReference = 0;
	};

	static void WriteHeader( const RecastNavMeshCacheFileHeader& rHeader, std::ofstream& rStream )
	{
		RawSerialisation::WriteObject( rHeader.Magic, rStream );
		RawSerialisation::WriteObject( rHeader.Version, rStream );
		RawSerialisation::WriteObject( rHeader.TileCount, rStream );
		RawSerialisation::WriteObject( rHeader.NavMeshParams, rStream );
	}

	static bool ReadHeader( RecastNavMeshCacheFileHeader& rHeader, std::ifstream& rStream )
	{
		char magic[ 4 ]{ 0 };
		RawSerialisation::ReadObject( magic, rStream );

		if( std::memcmp( rHeader.Magic, ".SRC", 4 ) != 0 )
		{
			SAT_CORE_ERROR( "[RecastNavigationMeshCache] Failed to read Recast Navigation Mesh Cache! File magic does not match" );
			return false;
		}

		RawSerialisation::ReadObject( rHeader.Version, rStream );
		RawSerialisation::ReadObject( rHeader.TileCount, rStream );
		RawSerialisation::ReadObject( rHeader.NavMeshParams, rStream );

		return true;
	}

	void RecastNavigationMeshCache::SaveNavMesh( const std::filesystem::path& rPath, const dtNavMesh* pMesh )
	{
		std::ofstream fout( rPath, std::ios::binary | std::ios::trunc );

		RecastNavMeshCacheFileHeader header;
		header.Version = RecastNavMeshCacheVersion::Latest;
		
		for( int i = 0; i < pMesh->getMaxTiles(); ++i )
		{
			const dtMeshTile* pTile = pMesh->getTile( i );
			if( !pTile || !pTile->header || !pTile->dataSize )
				continue;

			++header.TileCount;
		}

		std::memcpy( &header.NavMeshParams, pMesh->getParams(), sizeof( dtNavMeshParams ) );
		
		WriteHeader( header, fout );

		// Save tiles
		for( int i = 0; i < pMesh->getMaxTiles(); ++i )
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
		{
			SAT_CORE_ERROR( "[RecastNavigationMeshCache] Navigation cache: {0} does not exist!", rPath.string() );
			return nullptr;
		}

		std::ifstream stream( rPath, std::ios::binary | std::ios::in );

		RecastNavMeshCacheFileHeader header;

		if( !ReadHeader( header, stream ) )
			return nullptr;

		SAT_CORE_INFO( "Recast Navigation Mesh Cache:" );
		SAT_CORE_INFO( " Version: {0}", ( uint8_t ) header.Version );
		SAT_CORE_INFO( " Tile count: {0}", header.TileCount );
		SAT_CORE_INFO( "=====================================" );

		dtNavMesh* pNavMesh = dtAllocNavMesh();
		if( const auto result = pNavMesh->init( &header.NavMeshParams ); result != DT_SUCCESS ) 
		{
			std::string errorCode = Auxiliary::DetourErrorToString( result );
			SAT_CORE_INFO( "[Detour] Detour status check failed! STATUS/{0}", errorCode );
			return nullptr;
		}

		// Read tiles
		for( int i = 0; i < header.TileCount; ++i )
		{
			RecastNavMeshTileCacheFileHeader tileHeader{};
			RawSerialisation::ReadObject( tileHeader, stream );

			if( !tileHeader.TileReference )
				break;

			Buffer TemporaryBuffer;
			RawSerialisation::ReadSaturnBuffer( TemporaryBuffer, stream );

			// Pass on the data to Recast to assume ownership of.
			pNavMesh->addTile( TemporaryBuffer.Data, ( int ) TemporaryBuffer.Size, DT_TILE_FREE_DATA, tileHeader.TileReference, nullptr );
		}

		stream.close();

		return pNavMesh;
	}

}
