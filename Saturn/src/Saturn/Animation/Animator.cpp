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
#include "Animator.h"

#include "SkeletonAsset.h"

#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Core/App.h"

namespace Saturn {

	Animator::Animator()
	{
	}

	Animator::~Animator()
	{
	}

	void Animator::InitAnimation( AssetID id, Ref<SkeletalMesh> sk )
	{
		m_SkeletalMesh = sk;
		m_AnimationAsset = AssetManager::Get().GetAssetAs<SkeletalAnimationAsset>( id );
	
		m_State = AnimationState::Inactive;
	}

	void Animator::TickAnimation( Timestep ts )
	{
		if( m_PendingAsset )
		{
			m_AnimationAsset = AssetManager::Get().GetAssetAs<SkeletalAnimationAsset>( m_PendingAsset );
			m_PendingAsset = 0;

			m_AnimationTime = 0.0f;
			Begin();
		}

		switch( m_State )
		{
			default:
			case AnimationState::NotInitialised:
			case AnimationState::Inactive:
			case AnimationState::Paused:
				break;

			case AnimationState::Playing:
			{
				m_AnimationTime += ts * m_AnimationAsset->GetTicksPerSecond();
				m_AnimationTime = fmod( m_AnimationTime, m_AnimationAsset->GetDuration() );
				ApplyBoneTransformations();
			} break;
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
	}

	void Animator::Clear()
	{
		m_AnimationTime = 0.0f;
		m_StartTime = 0.0f;
		m_AnimationAsset = nullptr;
		m_BoneTransforms.clear();
		m_State = AnimationState::Inactive;
	}

	void Animator::QueueNewAnimation( AssetID id )
	{
		m_PendingAsset = id;
	}

	static uint32_t FindPositioning( const AnimationBone& rChannel, float time )
	{
		for( size_t i = 0; i < rChannel.Positions.size() - 1; i++ )
		{
			if( time < rChannel.Positions[ i + 1 ].TimeStamp )
				return i;
		}

		return 0;
	}

	static uint32_t FindRotation( const AnimationBone& rChannel, float time )
	{
		for( size_t i = 0; i < rChannel.Rotations.size() - 1; i++ )
		{
			if( time < rChannel.Rotations[ i + 1 ].TimeStamp )
				return i;
		}

		return 0;
	}

	static uint32_t FindScale( const AnimationBone& rChannel, float time )
	{
		for( size_t i = 0; i < rChannel.Scale.size() - 1; i++ )
		{
			if( time < rChannel.Scale[ i + 1 ].TimeStamp )
				return i;
		}

		return 0;
	}

	static glm::vec3 InterpolatePosition( const AnimationBone& channel, float time )
	{
		if( channel.Positions.size() == 1 )
			return channel.Positions[ 0 ].Value;

		const uint32_t positionIndex = FindPositioning( channel, time );
		const uint32_t nextPositionIndex = ( positionIndex + 1 );

		SAT_CORE_ASSERT( nextPositionIndex < channel.Positions.size() );

		const float deltaTime = ( float ) ( channel.Positions[ nextPositionIndex ].TimeStamp - channel.Positions[ positionIndex ].TimeStamp );
		float factor = ( time - ( float ) channel.Positions[ positionIndex ].TimeStamp ) / deltaTime;

		SAT_CORE_ASSERT( factor <= 1.0f, "Factor must be below 1.0f" );

		factor = glm::clamp( factor, 0.0f, 1.0f );
		const auto& rStart = channel.Positions[ positionIndex ].Value;
		const auto& rEnd = channel.Positions[ nextPositionIndex ].Value;

		const auto Delta = rEnd - rStart;
		return rStart + factor * Delta;
	}

	static glm::quat InterpolateRotation( const AnimationBone& channel, float time )
	{
		if( channel.Rotations.size() == 1 )
			return channel.Rotations[ 0 ].Value;

		const uint32_t positionIndex = FindRotation( channel, time );
		const uint32_t nextPositionIndex = ( positionIndex + 1 );

		SAT_CORE_ASSERT( nextPositionIndex < channel.Rotations.size() );

		const float deltaTime = ( float ) ( channel.Rotations[ nextPositionIndex ].TimeStamp - channel.Rotations[ positionIndex ].TimeStamp );
		float factor = ( time - ( float ) channel.Rotations[ positionIndex ].TimeStamp ) / deltaTime;

		SAT_CORE_ASSERT( factor <= 1.0f, "Factor must be below 1.0f" );

		factor = glm::clamp( factor, 0.0f, 1.0f );
		const auto& rStart = channel.Rotations[ positionIndex ].Value;
		const auto& rEnd = channel.Rotations[ nextPositionIndex ].Value;

		// We must use slerp and not mix. mix preforms linear interpolation while slerp does spherical
		glm::quat q = glm::slerp( rStart, rEnd, factor );
		
		return glm::normalize( q );
	}

	static glm::vec3 InterpolateScale( const AnimationBone& channel, float time )
	{
		if( channel.Scale.size() == 1 )
			return channel.Scale[ 0 ].Value;

		const uint32_t positionIndex = FindScale( channel, time );
		const uint32_t nextPositionIndex = ( positionIndex + 1 );

		SAT_CORE_ASSERT( nextPositionIndex < channel.Scale.size() );

		const float deltaTime = ( float ) ( channel.Scale[ nextPositionIndex ].TimeStamp - channel.Scale[ positionIndex ].TimeStamp );
		float factor = ( time - ( float ) channel.Scale[ positionIndex ].TimeStamp ) / deltaTime;

		SAT_CORE_ASSERT( factor <= 1.0f, "Factor must be below 1.0f" );

		factor = glm::clamp( factor, 0.0f, 1.0f );
		const auto& rStart = channel.Scale[ positionIndex ].Value;
		const auto& rEnd = channel.Scale[ nextPositionIndex ].Value;
		auto delta = rEnd - rStart;

		return rStart + factor * delta;
	}

	void Animator::ApplyBoneTransformations()
	{
		const auto& rMeshBones = m_SkeletalMesh->GetSkeletonAsset()->GetBoneInfo();

		const auto& rBones = m_AnimationAsset->GetAnimationBones();
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

		for( size_t i = 0; i < rMeshBones.size(); i++ )
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

		for( size_t i = 0; i < rMeshBones.size(); i++ )
		{
			if( rMeshBones[ i ].ParentIndex == boneIndex )
				UpdateBones( i, globalTransform, rLocalTransforms );
		}
	}

}
