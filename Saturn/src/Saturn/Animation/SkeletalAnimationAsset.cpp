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
#include "SkeletalAnimationAsset.h"

#if !defined(SAT_DIST)
#include "ACLCore.h"
#include "SkeletonAsset.h"

#include "Saturn/Asset/AssetManager.h"

#include <assimp/anim.h>

#include <acl/compression/compress.h>
#include <acl/compression/pre_process.h>
#include <acl/compression/track_array.h>
#include <acl/core/ansi_allocator.h>
#include <rtm/quatf.h>
#include <rtm/vector4f.h>
#endif

namespace Saturn {

	SkeletalAnimationAsset::SkeletalAnimationAsset( const Ref<Asset>& rBase )
		: Asset( rBase )
	{
	}

	SkeletalAnimationAsset::~SkeletalAnimationAsset()
	{
	}

#if !defined(SAT_DIST)
	static acl::error_result AclCompress(
		Ref<SkeletonAsset> sk,
		SkeletalAnimationAsset* animAsset,
		std::vector<AnimationChannel>& rChannels,
		float uncompressedDuration,
		float uncompressedTPS,
		acl::compressed_tracks*& outTracks )
	{
		acl::iallocator& rAllocator = ACLCore::GetAllocator();
		const uint32_t tracks = ( uint32_t ) animAsset->GetAnimationBones().size();
		const uint32_t samples = ( uint32_t ) animAsset->GetAnimationBones()[ 0 ].Positions.size();

		acl::track_array_qvvf trackList( rAllocator, tracks );

		const float fps = ( ( samples - 1 ) * uncompressedTPS ) / uncompressedDuration;

		for( uint32_t i = 0; i < tracks; ++i )
		{
			acl::track_desc_transformf desc;
			desc.output_index = i;
			desc.parent_index = ( i == 0 ) ? acl::k_invalid_track_index : sk->GetParentIndex( i );
			desc.precision = 0.0001f;
			desc.shell_distance = 3.0f;

			acl::track_qvvf track = acl::track_qvvf::make_reserve( desc, rAllocator, samples, fps );

			for( size_t j = 0; j < samples; ++j )
			{
				const auto& rPosition = rChannels[ i ].Positions[ j ].Value;
				const auto& rRotation = rChannels[ i ].Rotations[ j ].Value;
				const auto& rScale = rChannels[ i ].Scale[ j ].Value;

				track[ j ].rotation = rtm::quat_set( rRotation.x, rRotation.y, rRotation.z, rRotation.w );
				track[ j ].translation = rtm::vector_set( rPosition.x, rPosition.y, rPosition.z );
				track[ j ].scale = rtm::vector_set( rScale.x, rScale.y, rScale.z );
			}

			trackList[ i ] = std::move( track );
		}

		acl::pre_process_settings_t ppSettings;
		ppSettings.actions = acl::pre_process_actions::recommended;
		ppSettings.precision_policy = acl::pre_process_precision_policy::lossy;
		acl::qvvf_transform_error_metric error;
		ppSettings.error_metric = &error;

		acl::error_result result = acl::pre_process_track_list( rAllocator, ppSettings, trackList );
		if( result.any() )
		{
			return result;
		}

		acl::compression_settings compressionSettings = acl::get_default_compression_settings();
		compressionSettings.error_metric = &error;
		acl::output_stats stats{ acl::stat_logging::detailed };

		result = acl::compress_track_list( rAllocator, trackList, compressionSettings, outTracks, stats );

		animAsset->SetDuration( uncompressedDuration / uncompressedTPS );

		return result;
	}

	void SkeletalAnimationAsset::MakeUniformAndCompress( aiAnimation* pAnimation )
	{
		uint64_t highestSample = 2;
		for( const auto& rChannel : m_Bones )
		{
			// Pick highest.
			highestSample = glm::max( highestSample, rChannel.Positions.size() );
			highestSample = glm::max( highestSample, rChannel.Rotations.size() );
			highestSample = glm::max( highestSample, rChannel.Scale.size() );
		}

		const float interval = 1.0f / ( highestSample - 1 );

		for( auto& rChannel : m_Bones )
		{
			AnimationChannel ab;

			// POSITION
			uint64_t tsIndex = 1;
			ab.Positions.reserve( highestSample );
			ab.Positions.emplace_back( rChannel.Positions[ 0 ] );

			for( uint32_t i = 1; i < highestSample - 1; ++i )
			{
				float ts = i * interval;
				while( ( tsIndex < rChannel.Positions.size() ) && ( rChannel.Positions[ tsIndex ].Timestamp < ts ) )
				{
					++tsIndex;
				}

				const float newT = ( ts - rChannel.Positions[ tsIndex - 1 ].Timestamp ) / ( rChannel.Positions[ tsIndex ].Timestamp - rChannel.Positions[ tsIndex - 1 ].Timestamp );
				ab.Positions.emplace_back( glm::mix( rChannel.Positions[ tsIndex - 1 ].Value, rChannel.Positions[ tsIndex ].Value, newT ), ts );
			}
			ab.Positions.emplace_back( ab.Positions.back() );

			// ROTATION
			uint64_t rotIndex = 1;
			ab.Rotations.reserve( highestSample );
			ab.Rotations.emplace_back( rChannel.Rotations[ 0 ] );

			for( uint32_t i = 1; i < highestSample - 1; ++i )
			{
				float ts = i * interval;
				while( ( rotIndex < rChannel.Rotations.size() ) && ( rChannel.Rotations[ rotIndex ].Timestamp < ts ) )
				{
					++rotIndex;
				}

				const float newT = ( ts - rChannel.Rotations[ rotIndex - 1 ].Timestamp ) / ( rChannel.Rotations[ rotIndex ].Timestamp - rChannel.Rotations[ rotIndex - 1 ].Timestamp );

				ab.Rotations.emplace_back( glm::slerp( rChannel.Rotations[ rotIndex - 1 ].Value, rChannel.Rotations[ rotIndex ].Value, newT ), ts );
			}
			ab.Rotations.emplace_back( ab.Rotations.back() );

			// SCALING
			uint64_t scaleIndex = 1;
			ab.Scale.reserve( highestSample );
			ab.Scale.emplace_back( rChannel.Scale[ 0 ] );

			for( uint32_t i = 1; i < highestSample - 1; ++i )
			{
				float ts = i * interval;
				while( ( scaleIndex < rChannel.Scale.size() ) && ( rChannel.Scale[ scaleIndex ].Timestamp < ts ) )
				{
					++scaleIndex;
				}

				const float newT = ( ts - rChannel.Scale[ scaleIndex - 1 ].Timestamp ) / ( rChannel.Scale[ scaleIndex ].Timestamp - rChannel.Scale[ scaleIndex - 1 ].Timestamp );
				ab.Scale.emplace_back( glm::mix( rChannel.Scale[ scaleIndex - 1 ].Value, rChannel.Scale[ scaleIndex ].Value, newT ), ts );
			}
			ab.Scale.emplace_back( ab.Scale.back() );

			rChannel.Positions = std::move( ab.Positions );
			rChannel.Rotations = std::move( ab.Rotations );
			rChannel.Scale = std::move( ab.Scale );
		}

		m_UncompressedDuration = pAnimation->mDuration;
		m_UncompressedTPS = pAnimation->mTicksPerSecond;

		Compress();
	}

	void SkeletalAnimationAsset::Compress()
	{
		Ref<SkeletonAsset> sk = AssetManager::Get().GetAssetAs<SkeletonAsset>( m_SkeletonAssetID );

		acl::compressed_tracks* pTracks = nullptr;
		const auto error = AclCompress( sk, this, m_Bones, m_UncompressedDuration, m_UncompressedTPS, pTracks );
		if( error.any() )
		{
			SAT_CORE_ERROR( "Unable to compress animation!" );
			SAT_CORE_ASSERT( false );
		}

		SetACLData( pTracks );
	}

	void SkeletalAnimationAsset::SerialiseAclData( std::ofstream& rStream ) const
	{
		// Write header.
		acl::compressed_tracks* pTracks = reinterpret_cast< acl::compressed_tracks* >( m_pData );
		rStream.write( reinterpret_cast< const char* >( pTracks ), sizeof( acl::compressed_tracks ) );

		// Now write the actual compressed data
		const uint8_t* pData = reinterpret_cast<const uint8_t*>( pTracks ) + sizeof( acl::compressed_tracks );
		
		// Note according to Acl get_size() includes the acl::compressed_tracks size (so 16 bytes).
		rStream.write( reinterpret_cast< const char* >( pData ), pTracks->get_size() - sizeof( acl::compressed_tracks ) );
	}

#endif

#if !defined( SAT_DIST )
	void SkeletalAnimationAsset::DeserialiseAclData( std::ifstream& rStream )
#else	
	void SkeletalAnimationAsset::DeserialiseAclData( std::istream& rStream )
#endif
	{
		uint8_t headerBuffer[ sizeof( acl::compressed_tracks ) ];
		rStream.read( reinterpret_cast< char* >( headerBuffer ), sizeof( headerBuffer ) );

		const acl::compressed_tracks* pHeaderView = reinterpret_cast< const acl::compressed_tracks* >( headerBuffer );

		const uint32_t headerSize = pHeaderView->get_size();
		const uint32_t animDataSize = headerSize - sizeof( acl::compressed_tracks );

		acl::iallocator& rAllocator = ACLCore::GetAllocator();
		uint8_t* pBuffer = reinterpret_cast< uint8_t* >( rAllocator.allocate( headerSize ) );

		memcpy( pBuffer, headerBuffer, sizeof( headerBuffer ) );

		rStream.read( reinterpret_cast< char* >( pBuffer + sizeof( acl::compressed_tracks ) ), animDataSize );

		SetACLData( acl::make_compressed_tracks( pBuffer ) );

		SetDuration( m_UncompressedDuration / m_UncompressedTPS );
	}

	void SkeletalAnimationAsset::SetACLData( void* pData )
	{
		m_pData = pData;
	}

}
