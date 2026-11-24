/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2026 0xsmft https://0xsmft.dev                                       *
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
#include "AnimationController.h"
#include "Animator.h"

#include "SkeletonAsset.h"

#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Core/Profiler.h"

#include <acl/decompression/decompress.h>

namespace Saturn {

	//
	// Writes a single track (bone) into a local transform, 
	// used to sample one bone at times other than the current time.
	//
	class SingleBoneWriter : public acl::track_writer
	{
	public:
		SingleBoneWriter( AnimatorLocalTransfrom* pTransform ) : m_pTransform( pTransform ) {}

		// Root motion does not use scale.
		static constexpr bool skip_all_scales() { return true; }

		void RTM_SIMD_CALL write_rotation( uint32_t /*index*/, rtm::quatf_arg0 value )
		{
			m_pTransform->Rotation = glm::quat(
				rtm::quat_get_w( value ),
				rtm::quat_get_x( value ),
				rtm::quat_get_y( value ),
				rtm::quat_get_z( value ) );
		}

		void RTM_SIMD_CALL write_translation( uint32_t /*index*/, rtm::vector4f_arg0 value )
		{
			rtm::vector_store3( value, glm::value_ptr( m_pTransform->Position ) );
		}

		void RTM_SIMD_CALL write_scale( uint32_t /*index*/, rtm::vector4f_arg0 /*value*/ ) {}

	private:
		AnimatorLocalTransfrom* m_pTransform = nullptr;
	};

	//////////////////////////////////////////////////////////////////////////
	// ANIMATOR

	Animator::Animator()
	{
	}

	Animator::~Animator()
	{
		Destroy();
	}

	void Animator::Destroy()
	{
		m_SkeletalMesh = nullptr;
		m_SingleAnimationAsset = nullptr;
		m_AnimationControllerAsset = nullptr;

		delete m_pOutPose;
		m_pOutPose = nullptr;
	}

	void Animator::InitAnimation( AssetID id, Ref<SkeletalMesh> sk, AnimatorType type )
	{
		m_SkeletalMesh = sk;

		switch( type )
		{
			case AnimatorType::Single:
				m_SingleAnimationAsset = AssetManager::Get()->GetAssetAs<SkeletalAnimationAsset>( id );
				break;
		
			case AnimatorType::AnimationControllerGraph:
				m_AnimationControllerAsset = Ref<AnimationController>::Create( id );
				break;
		
			default: break;
		}

		m_AnimatorType = type;
		m_State = AnimationState::Inactive;
	}

	void Animator::TickSingleAnim( Timestep ts ) 
	{
		if( !m_SingleAnimationAsset )
			return;

		switch( m_State )
		{
			default:
			case AnimationState::NotInitialised:
			case AnimationState::Inactive:
			case AnimationState::Paused: 
				break;

			case AnimationState::Playing:
			{
				float previousTime = m_AnimationTime;
				uint32_t loopCount = 0u;

				if( !m_Completed )
				{
					m_AnimationTime -= floorf( m_AnimationTime );
					previousTime = m_AnimationTime;

					m_AnimationTime += ts * m_PlaybackSpeed / m_SingleAnimationAsset->GetDuration();

					while( m_AnimationTime > 1.0f )
					{
						if( m_Looping )
						{
							m_AnimationTime -= 1.0f;
							++loopCount;
						}
						else
						{
							m_AnimationTime = 1.0f;
							m_Completed = true;

							// Reset back
							if( m_QuickFireAnimPlaying )
							{
								m_AnimatorType = AnimatorType::AnimationControllerGraph;
								m_Looping = m_BackupOfLooping;
								m_BackupOfLooping = false;
								m_QuickFireAnimPlaying = false;
							}
						}
					}
				}

				const float sampleTiming = m_AnimationTime * m_SingleAnimationAsset->GetDuration();
				m_Context.seek( sampleTiming, acl::sample_rounding_policy::none );
				m_Context.decompress_tracks( m_Writer );

				if( m_RootMotionEnabled )
					ExtractRootMotion( previousTime, loopCount );

				m_pOutPose->Timestamp = m_SingleAnimationAsset->GetDuration();
			} break;
		}
	}

	void Animator::AnimGraph_SetSingleAnim( Ref<SkeletalAnimationAsset> anim, bool loop )
	{
		Loop( loop );

		// Reset anim time
		m_AnimationTime = 0.0f;
		m_Completed = false;

		m_SingleAnimationAsset = anim;
		m_Context.initialize( *static_cast< const acl::compressed_tracks* >( anim->GetData() ) );

		InitialiseRootMotion();
	}

	//////////////////////////////////////////////////////////////////////////
	// Root motion

	void Animator::InitialiseRootMotion()
	{
		m_RootMotionEnabled = false;
		m_RootMotionBone = 0;

		if( !m_SingleAnimationAsset || !m_SingleAnimationAsset->IsUsingRootMotion() || !m_SkeletalMesh || !m_Context.is_initialized() )
			return;

		// Skip if we do not have any root motion set.
		const RootMotionSettings& rSettings = m_SingleAnimationAsset->GetRootMotionSettings();
		if( !rSettings.ExtractsAnything() )
			return;

		Ref<SkeletonAsset> skeleton = m_SkeletalMesh->GetSkeletonAsset();
		if( !skeleton )
			return;

		const std::vector<uint64_t>& rParents = skeleton->GetParentIndices();
		const uint32_t boneCount = glm::min( m_Context.get_compressed_tracks()->get_num_tracks(), ( uint32_t ) rParents.size() );
		if( boneCount == 0 || boneCount > SK_MAX_BONES )
			return;

		m_RootMotionBone = FindRootMotionBone( rParents, boneCount );

		// Root motion has to be worked out in the entity's local space, 
		// which is how the mesh gets rendered:
		//   entity * submesh transform * skeleton transform * parent bones * root motion bone
		// 
		// We take the parents from the start of the animation, 
		// normally they do not move (otherwise they would have been picked).
		Pose startPose;
		SamplePose( 0.0f, startPose );

		std::vector<uint32_t> parentChain;
		for( uint64_t parent = rParents[ m_RootMotionBone ]; parent < boneCount && parentChain.size() < boneCount; parent = rParents[ parent ] )
			parentChain.push_back( ( uint32_t ) parent );

		// NOTE: The renderer applies each submesh's node transform on top of the skin, 
		// every submesh of a skinned mesh normally shares the same one.
		glm::mat4 submeshTransform( 1.0f );
		if( !m_SkeletalMesh->Submeshes().empty() )
		{
			const glm::mat4& rTransform = m_SkeletalMesh->Submeshes()[ 0 ].Transform;
			if( glm::abs( glm::determinant( glm::mat3( rTransform ) ) ) > 1e-12f )
				submeshTransform = rTransform;
		}

		glm::mat4 parentTransform = submeshTransform * skeleton->GetTransform();
		for( auto itr = parentChain.rbegin(); itr != parentChain.rend(); ++itr )
		{
			const AnimatorLocalTransfrom& rLocal = startPose.LocalTransforms[ *itr ];
			parentTransform = parentTransform
				* glm::translate( glm::mat4( 1.0f ), rLocal.Position )
				* glm::toMat4( rLocal.Rotation )
				* glm::scale( glm::mat4( 1.0f ), rLocal.Scale );
		}

		const AnimatorLocalTransfrom& rStart = startPose.LocalTransforms[ m_RootMotionBone ];
		const AnimatorLocalTransfrom end = SampleBone( m_RootMotionBone, 1.0f );

		m_RootMotion.Initialise( rSettings, parentTransform, rStart.Position, rStart.Rotation, end.Position, end.Rotation );
		m_RootMotionEnabled = true;

#if !defined(SAT_DIST)
		SAT_CORE_INFO( "[Animator]: Root motion: Anim: '{0}' will use bone '{1}' as the root bone",
			m_SingleAnimationAsset->Name, skeleton->GetBoneName( m_RootMotionBone ) );
#endif
	}

	uint32_t Animator::FindRootMotionBone( const std::vector<uint64_t>& rParents, uint32_t boneCount )
	{
		// Start at the root and walk down while the bone does not move and only has one child.
		// But, this isn't great, because a lot of downloaded animations do not have a proper
		// root boot at 0,0. So can we please ensure that artists create a proper skeleton!!!!
		uint32_t bone = 0;

		for( uint32_t depth = 0; depth < boneCount; ++depth )
		{
			uint32_t child = 0, childCount = 0;
			for( uint32_t i = 0; i < boneCount; ++i )
			{
				if( rParents[ i ] == bone )
				{
					child = i;
					++childCount;
				}
			}

			if( childCount != 1 )
				break;

			const AnimatorLocalTransfrom start = SampleBone( bone, 0.0f );

			bool moves = false;
			for( uint32_t s = 1; s <= 9 && !moves; ++s )
			{
				const AnimatorLocalTransfrom sample = SampleBone( bone, ( float ) s / 9.0f );

				moves = glm::length( sample.Position - start.Position ) > 1e-4f
					|| glm::abs( glm::dot( sample.Rotation, start.Rotation ) ) < 1.0f - 1e-6f;
			}

			if( moves )
				break;

			bone = child;
		}

		return bone;
	}

	AnimatorLocalTransfrom Animator::SampleBone( uint32_t boneIndex, float normalisedTime )
	{
		AnimatorLocalTransfrom result;
		SingleBoneWriter writer( &result );

		m_Context.seek( normalisedTime * m_SingleAnimationAsset->GetDuration(), acl::sample_rounding_policy::none );
		m_Context.decompress_track( boneIndex, writer );

		return result;
	}

	void Animator::SamplePose( float normalisedTime, Pose& rPose )
	{
		PoseWriter writer( &rPose );

		m_Context.seek( normalisedTime * m_SingleAnimationAsset->GetDuration(), acl::sample_rounding_policy::none );
		m_Context.decompress_tracks( writer );
	}

	void Animator::ExtractRootMotion( float previousTime, uint32_t loopCount )
	{
		AnimatorLocalTransfrom& rRoot = m_pOutPose->LocalTransforms[ m_RootMotionBone ];
		const RootMotionTransform current = m_RootMotion.CalculateFrame( rRoot.Position, rRoot.Rotation );

		RootMotionTransform delta;
		if( previousTime != m_AnimationTime || loopCount > 0 )
		{
			// NB: This seeks the context, that is fine because the pose has already been decompressed for this tick.
			const AnimatorLocalTransfrom previousRoot = SampleBone( m_RootMotionBone, previousTime );
			const RootMotionTransform previous = m_RootMotion.CalculateFrame( previousRoot.Position, previousRoot.Rotation );

			delta = m_RootMotion.CalculateDelta( previous, current, loopCount );
		}

		m_pOutPose->RootMotion = delta;
		m_PendingRootMotion *= delta;

		// Take the extracted motion out of the root bone so the mesh plays in place.
		m_RootMotion.RemoveFromRoot( current, rRoot.Position, rRoot.Rotation );
	}

	RootMotionTransform Animator::ConsumeRootMotion()
	{
		const RootMotionTransform result = m_PendingRootMotion;
		m_PendingRootMotion = {};

		return result;
	}

	//////////////////////////////////////////////////////////////////////////

	void Animator::TickAnimation( Timestep ts )
	{
		SAT_PF_EVENT();

		switch( m_AnimatorType )
		{
			case AnimatorType::Single:
				TickSingleAnim( ts );
				break;

			case AnimatorType::AnimationControllerGraph: 
			{
				// Tick AnimGraph
				m_AnimationControllerAsset->Tick( ts );
				
				// Tick single anim asset... a bit screwy :(
				if( m_SingleAnimationAsset )
				{
					TickSingleAnim( ts );
				}
			} break;

			default: break;
		}
	}

	void Animator::Pause()
	{
		m_State = AnimationState::Paused;
	}

	void Animator::Begin()
	{
		if( m_State == AnimationState::NotInitialised )
			return;

		m_State = AnimationState::Playing;

		m_pOutPose = new Pose();
		m_pOutPose->Duration = m_AnimationTime;

		m_Writer = PoseWriter( m_pOutPose );

		switch( m_AnimatorType )
		{
			case AnimatorType::Single:
			{
				if( m_SingleAnimationAsset )
				{
					m_pOutPose->BonesUsed = m_SingleAnimationAsset->GetBoneCount();
					m_Context.initialize( *static_cast< const acl::compressed_tracks* >( m_SingleAnimationAsset->GetData() ) );

					InitialiseRootMotion();

					TickSingleAnim( 0.0f );
				}
			} break;

			case AnimatorType::AnimationControllerGraph:
				m_AnimationControllerAsset->Initialise( this );
				break;
			
			default: break;
		}
	}

	void Animator::Clear()
	{
		m_AnimationTime = 0.0f;
		m_SingleAnimationAsset = nullptr;
		m_AnimationControllerAsset = nullptr;
		m_State = AnimationState::Inactive;

		m_RootMotionEnabled = false;
		m_PendingRootMotion = {};
	}

	void Animator::PlayFromStart()
	{
		m_AnimationTime = 0.0f;
		m_Completed = false;
	}

	Ref<Asset> Animator::GetCurrentAnimation() const
	{
		switch( m_AnimatorType )
		{
			case AnimatorType::Single:
				return m_SingleAnimationAsset;

			case AnimatorType::AnimationControllerGraph:
				return m_AnimationControllerAsset->GetAsset();
			default:
				break;
		}

		return nullptr;
	}

	std::vector<glm::mat4> Animator::GetBoneTransforms()
	{
		const size_t count = m_SingleAnimationAsset->GetBoneCount();
		std::vector<glm::mat4> ts( count );

		for( uint32_t i = 0; i < ( uint32_t ) count; ++i )
		{
			const auto& lts = m_pOutPose->LocalTransforms[ i ];
			const glm::mat4 local = glm::translate( glm::mat4( 1.0f ), lts.Position ) 
				* glm::toMat4( lts.Rotation ) 
				* glm::scale( glm::mat4( 1.0f ), lts.Scale );

			/*
			SAT_CORE_INFO( "BONE TRS: {0}", i );
			SAT_CORE_INFO( " T:{0}", lts.Position );
			SAT_CORE_INFO( " R:{0}", glm::degrees( glm::eulerAngles( lts.Rotation ) ) );
			SAT_CORE_INFO( " S:{0}", lts.Scale );
			*/

			const uint32_t parent = ( uint32_t ) m_SkeletalMesh->GetSkeletonAsset()->GetParentIndex( i );
			ts[ i ] = ( parent == ~0u ) ? local : ts[ parent ] * local;
		}

		return ts;
	}

	Ref<AnimationController> Animator::GetAnimGraph() const
	{
		return m_AnimationControllerAsset;
	}

	void Animator::ChangeType( AnimatorType type )
	{
		// Ignore sentinel value.
		if( ( ( uint8_t ) type & ( uint8_t ) AnimatorType::Unknown ) != 0 )
			m_AnimatorType = type;
	}

	void Animator::QuickFireAnimation( AssetID id )
	{
		if( id == 0llu )
			return;

		m_SingleAnimationAsset = AssetManager::Get()->GetAssetAs<SkeletalAnimationAsset>( id );
		m_AnimatorType = AnimatorType::Single;
		m_AnimationTime = 0.0f;
		m_Completed = false;

		m_Context.initialize( *static_cast< const acl::compressed_tracks* >( m_SingleAnimationAsset->GetData() ) );
		InitialiseRootMotion();

		m_BackupOfLooping = m_Looping;
		m_Looping = false;

		m_QuickFireAnimPlaying = true;
	}

	void Animator::QuickFireAnimation( const std::string& rName )
	{
		QuickFireAnimation( AssetManager::Get()->FindAsset( rName, AssetType::SkeletalAnimation )->ID );
	}

}
