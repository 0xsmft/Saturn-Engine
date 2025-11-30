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
#include "AnimationController.h"
#include "Animator.h"

#include "SkeletonAsset.h"

#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Core/App.h"
#include "Saturn/Core/Profiler.h"

#include <acl/decompression/decompress.h>

namespace Saturn {

	Animator::Animator()
	{
	}

	Animator::~Animator()
	{
		Destory();
	}

	void Animator::Destory()
	{
		m_SkeletalMesh = nullptr;
		m_SingleAnimationAsset = nullptr;
		m_AnimationControllerAsset = nullptr;

		m_Context.reset();

		delete m_pOutPose;
	}

	void Animator::InitAnimation( AssetID id, Ref<SkeletalMesh> sk, AnimatorType type )
	{
		m_SkeletalMesh = sk;
		m_CurrentID = id;

		switch( type )
		{
			case AnimatorType::Single:
				m_SingleAnimationAsset = AssetManager::Get().GetAssetAs<SkeletalAnimationAsset>( id );
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
				if( !m_Completed )
				{
					m_AnimationTime -= floorf( m_AnimationTime );
					m_AnimationTime += ts * 1.0f / m_SingleAnimationAsset->GetDuration();

					while( m_AnimationTime > 1.0f )
					{
						if( m_Looping )
						{
							m_AnimationTime -= 1.0f;
						}
						else
						{
							m_AnimationTime = 1.0f;
							m_Completed = true;
						}
					}
				}

				const float sampleTiming = m_AnimationTime * m_SingleAnimationAsset->GetDuration();
				m_Context.seek( sampleTiming, acl::sample_rounding_policy::none );
				m_Context.decompress_tracks( m_Writer );

				m_pOutPose->Timestamp = m_SingleAnimationAsset->GetDuration();
				m_pOutPose->Duration = m_AnimationTime;
			} break;
		}
	}

	void Animator::TickAnimation( Timestep ts )
	{
		SAT_PF_EVENT();

		switch( m_AnimatorType )
		{
			case AnimatorType::Single:
				TickSingleAnim( ts );
				break;

			case AnimatorType::AnimationControllerGraph:
				m_AnimationControllerAsset->Tick( ts );
				if( m_SingleAnimationAsset )
				{
					TickSingleAnim( ts );
				}
				break;

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

		m_StartTime = Application::Get().Time().Seconds();
		m_State = AnimationState::Playing;

		if( m_AnimatorType == AnimatorType::AnimationControllerGraph )
			m_AnimationControllerAsset->Initialise( this );

		m_pOutPose = new Pose();
		m_pOutPose->BonesUsed = m_SingleAnimationAsset->GetAnimationBones().size();

		m_Writer = PoseWriter( m_pOutPose );
		m_Context.initialize( *static_cast<const acl::compressed_tracks*>( m_SingleAnimationAsset->GetData() ) );

		TickSingleAnim( 0.0f );
	}

	void Animator::Clear()
	{
		m_AnimationTime = 0.0f;
		m_StartTime = 0.0f;
		m_SingleAnimationAsset = nullptr;
		m_AnimationControllerAsset = nullptr;
		m_CurrentID = 0llu;
		m_State = AnimationState::Inactive;
	}

	void Animator::QueueNewAnimation( AssetID id )
	{
		m_PendingAsset = id;
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
		const size_t count = m_SingleAnimationAsset->GetAnimationBones().size();
		std::vector<glm::mat4> ts( count );

		for( size_t i = 0; i < count; ++i )
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

			const uint32_t parent = m_SkeletalMesh->GetSkeletonAsset()->GetParentIndex( i );
			ts[ i ] = ( parent == ~0u ) ? local : ts[ parent ] * local;
		}

		return ts;
	}

	void Animator::ApplyBoneTransformations()
	{
		m_Context.seek( m_AnimationTime * m_SingleAnimationAsset->GetDuration(), acl::sample_rounding_policy::none );
		m_Context.decompress_tracks( m_Writer );

		/*
		const auto& rMeshBones = m_SkeletalMesh->GetSkeletonAsset()->GetBoneInfo();

		const auto& rBones = m_SingleAnimationAsset->GetAnimationBones();
		std::vector<glm::mat4> localTransforms( rMeshBones.size() );
		
		for( const auto& rBone : rBones )
		{
			if( rBone.Index == -1 )
			{
				continue;
			}

			const glm::vec3 pos = InterpolatePosition( rBone, m_AnimationTime );
			const glm::quat rot = InterpolateRotation( rBone, m_AnimationTime );
			const glm::vec3 scl = InterpolateScale( rBone, m_AnimationTime );

			const glm::mat4 translation = glm::translate( glm::mat4( 1.0f ), pos );
			const glm::mat4 rotation = glm::toMat4( rot );
			const glm::mat4 scaling = glm::scale( glm::mat4( 1.0f ), scl );

			const auto final = translation * rotation * scaling;
			localTransforms[ ( size_t ) rBone.Index ] = final;
		}

		// Root motion.
		if( m_SingleAnimationAsset->IsUsingRootMotion() )
		{
			const auto& rRootTransformation = localTransforms[ 0 ];
			const auto& rCurrentTranslation = glm::vec3( rRootTransformation[ 3 ] );
			const auto& rCurrentRotation = glm::quat_cast( rRootTransformation );

			const glm::vec3 deltaPos = rCurrentTranslation - m_LastRootTranslation;
			const glm::quat deltaRot = glm::conjugate( m_LastRootRotation ) * rCurrentRotation;

			// Temporary, would need to create a "fake" root motion bone if needed.
			localTransforms[ 0 ][ 3 ] = glm::vec4( 0.0f, 0.0f, 0.0f, 1.0f );

			m_LastRootTranslation = rCurrentTranslation;
			m_LastRootRotation = rCurrentRotation;
		}

		for( size_t i = 0; i < rMeshBones.size(); ++i )
		{
			if( rMeshBones[ i ].ParentIndex == -1 )
				UpdateBones( i, glm::mat4( 1.0f ), localTransforms );
		}
		*/
	}

}
