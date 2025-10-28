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
		m_BoneTransforms.clear();
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
		if( m_PendingAsset )
		{
			m_SingleAnimationAsset = AssetManager::Get().GetAssetAs<SkeletalAnimationAsset>( m_PendingAsset );
			m_CurrentID = m_PendingAsset;
			m_PendingAsset = 0;

			m_AnimationTime = 0.0f;
			Begin();
		}

		switch( m_State )
		{
			default:
			case AnimationState::NotInitialised:
			case AnimationState::Inactive:
				break;
			case AnimationState::Paused: 
			{
				if( m_PendingStepTime != -1.0f )
				{
					m_AnimationTime = m_PendingStepTime;
					m_PendingStepTime = -1.0f;

					ApplyBoneTransformations();
				}
			} break;

			case AnimationState::Playing:
			{
				m_AnimationTime += ts * m_SingleAnimationAsset->GetTicksPerSecond();

				if( m_Looping )
				{
					const float dur = ( float ) m_SingleAnimationAsset->GetDuration();
					if( m_AnimationTime > dur )
					{
						// Allow systems to react to when the anim is done
						m_Completed = true;
					}
					else
						m_Completed = false;

					m_AnimationTime = fmod( m_AnimationTime, dur );
				}
				else
				{
					const float dur = ( float ) m_SingleAnimationAsset->GetDuration();
					if( m_AnimationTime > dur )
					{
						// Stop at last frame.
						m_AnimationTime = 0.0f;
						m_Completed = true;
					}
				}

				if( m_PendingStepTime != -1.0f )
				{
					m_AnimationTime = m_PendingStepTime;
					m_PendingStepTime = -1.0f;
				}

				ApplyBoneTransformations();
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

		m_BoneTransforms.resize( m_SkeletalMesh->GetSkeletonAsset()->GetBoneInfo().size() );

		if( m_AnimatorType == AnimatorType::AnimationControllerGraph )
			m_AnimationControllerAsset->Initialise( this );
	}

	void Animator::Clear()
	{
		m_PendingStepTime = -1.0f;
		m_AnimationTime = 0.0f;
		m_StartTime = 0.0f;
		m_SingleAnimationAsset = nullptr;
		m_AnimationControllerAsset = nullptr;
		m_CurrentID = 0llu;
		m_BoneTransforms.clear();
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

	static uint32_t FindPositioning( const AnimationBone& rChannel, float time )
	{
		for( size_t i = 0; i < rChannel.Positions.size() - 1; ++i )
		{
			if( time < rChannel.Positions[ i + 1 ].TimeStamp )
				return ( uint32_t ) i;
		}

		return 0u;
	}

	static uint32_t FindRotation( const AnimationBone& rChannel, float time )
	{
		for( size_t i = 0; i < rChannel.Rotations.size() - 1; ++i )
		{
			if( time < rChannel.Rotations[ i + 1 ].TimeStamp )
				return ( uint32_t ) i;
		}

		return 0u;
	}

	static uint32_t FindScale( const AnimationBone& rChannel, float time )
	{
		for( size_t i = 0; i < rChannel.Scale.size() - 1; ++i )
		{
			if( time < rChannel.Scale[ i + 1 ].TimeStamp )
				return ( uint32_t ) i;
		}

		return 0u;
	}

	static glm::vec3 InterpolatePosition( const AnimationBone& rChannel, float time )
	{
		if( rChannel.Positions.size() == 1 )
			return rChannel.Positions[ 0 ].Value;

		const uint32_t positionIndex = FindPositioning( rChannel, time );
		const uint32_t nextPositionIndex = ( positionIndex + 1 );

		SAT_CORE_ASSERT( nextPositionIndex < rChannel.Positions.size() );

		const float deltaTime = ( float ) ( rChannel.Positions[ nextPositionIndex ].TimeStamp - rChannel.Positions[ positionIndex ].TimeStamp );
		float factor = ( time - ( float ) rChannel.Positions[ positionIndex ].TimeStamp ) / deltaTime;

		SAT_CORE_ASSERT( factor <= 1.0f, "Factor must be below 1.0f" );

		factor = glm::clamp( factor, 0.0f, 1.0f );
		const auto& rStart = rChannel.Positions[ positionIndex ].Value;
		const auto& rEnd = rChannel.Positions[ nextPositionIndex ].Value;

		const auto Delta = rEnd - rStart;
		return rStart + factor * Delta;
	}

	static glm::quat InterpolateRotation( const AnimationBone& rChannel, float time )
	{
		if( rChannel.Rotations.size() == 1 )
			return rChannel.Rotations[ 0 ].Value;

		const uint32_t positionIndex = FindRotation( rChannel, time );
		const uint32_t nextPositionIndex = ( positionIndex + 1 );

		SAT_CORE_ASSERT( nextPositionIndex < rChannel.Rotations.size() );

		const float deltaTime = ( float ) ( rChannel.Rotations[ nextPositionIndex ].TimeStamp - rChannel.Rotations[ positionIndex ].TimeStamp );
		float factor = ( time - ( float ) rChannel.Rotations[ positionIndex ].TimeStamp ) / deltaTime;

		SAT_CORE_ASSERT( factor <= 1.0f, "Factor must be below 1.0f" );

		factor = glm::clamp( factor, 0.0f, 1.0f );
		const auto& rStart = rChannel.Rotations[ positionIndex ].Value;
		const auto& rEnd = rChannel.Rotations[ nextPositionIndex ].Value;

		// We must use slerp and not mix. mix preforms linear interpolation while slerp does spherical
		glm::quat q = glm::slerp( rStart, rEnd, factor );
		
		return glm::normalize( q );
	}

	static glm::vec3 InterpolateScale( const AnimationBone& rChannel, float time )
	{
		if( rChannel.Scale.size() == 1 )
			return rChannel.Scale[ 0 ].Value;

		const uint32_t positionIndex = FindScale( rChannel, time );
		const uint32_t nextPositionIndex = ( positionIndex + 1 );

		SAT_CORE_ASSERT( nextPositionIndex < rChannel.Scale.size() );

		const float deltaTime = ( float ) ( rChannel.Scale[ nextPositionIndex ].TimeStamp - rChannel.Scale[ positionIndex ].TimeStamp );
		float factor = ( time - ( float ) rChannel.Scale[ positionIndex ].TimeStamp ) / deltaTime;

		SAT_CORE_ASSERT( factor <= 1.0f, "Factor must be below 1.0f" );

		factor = glm::clamp( factor, 0.0f, 1.0f );
		const auto& rStart = rChannel.Scale[ positionIndex ].Value;
		const auto& rEnd = rChannel.Scale[ nextPositionIndex ].Value;
		auto delta = rEnd - rStart;

		return rStart + factor * delta;
	}

	void Animator::ApplyBoneTransformations()
	{
		const auto& rMeshBones = m_SkeletalMesh->GetSkeletonAsset()->GetBoneInfo();

		const auto& rBones = m_SingleAnimationAsset->GetAnimationBones();
		std::vector<glm::mat4> localTransforms( rMeshBones.size() );
		
		for( const auto& rBone : rBones )
		{
			const auto index = m_SkeletalMesh->GetSkeletonAsset()->FindBoneIndex( rBone.Name );
			if( index == -1 )
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
			localTransforms[ ( size_t ) index ] = final;
		}

		// Root motion
		if( m_SingleAnimationAsset->IsUsingRootMotion() )
		{
			const auto& rRootTransformation = localTransforms[ 0 ];
			const auto& rCurrentTranslation = glm::vec3( rRootTransformation[ 3 ] );
			const auto& rCurrentRotation = glm::quat_cast( rRootTransformation );

			const glm::vec3 deltaPos = rCurrentTranslation - m_LastRootTranslation;
			const glm::quat deltaRot = glm::conjugate( m_LastRootRotation ) * rCurrentRotation;

			// Temporary, would need to create a "fake" root motion bone if needed
			localTransforms[ 0 ][ 3 ] = glm::vec4( 0.0f, 0.0f, 0.0f, 1.0f );

			m_LastRootTranslation = rCurrentTranslation;
			m_LastRootRotation = rCurrentRotation;
		}

		for( size_t i = 0; i < rMeshBones.size(); ++i )
		{
			if( rMeshBones[ i ].ParentIndex == -1 )
				UpdateBones( i, glm::mat4( 1.0f ), localTransforms );
		}
	}

	void Animator::UpdateBones( size_t boneIndex, const glm::mat4& rParentTransform, const std::vector<glm::mat4>& rLocalTransforms )
	{
		const auto& rMeshBones = m_SkeletalMesh->GetSkeletonAsset()->GetBoneInfo();

		const glm::mat4 globalTransform = rParentTransform * rLocalTransforms[ boneIndex ];

		m_BoneTransforms[ boneIndex ] = m_SkeletalMesh->GetInverseTransform() * globalTransform * rMeshBones[ boneIndex ].BoneOffset; /* <- bone offset */

		for( size_t i = 0; i < rMeshBones.size(); ++i )
		{
			if( rMeshBones[ i ].ParentIndex == boneIndex )
				UpdateBones( i, globalTransform, rLocalTransforms );
		}
	}

}
