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
		m_StartTime = Application::Get().Time().Seconds();
		m_Init = true;
	}

	void Animator::Play( Timestep ts )
	{
		if( m_StartTime == 0.0f )
			return;

		m_AnimationTime += ts * m_AnimationAsset->GetTicksPerSecond();
		m_AnimationTime = fmod( m_AnimationTime, m_AnimationAsset->GetDuration() );
		ApplyBoneTransformations();
	}

	void Animator::Clear()
	{
		m_AnimationTime = 0.0f;
		m_StartTime = 0.0f;
		m_AnimationAsset = nullptr;
		m_BoneTransforms.clear();
	}

	static uint32_t FindPositioning( const AnimationBone& channel, float time )
	{
		for( size_t i = 0; i < channel.Positions.size() - 1; i++ )
		{
			if( time < channel.Positions[ i + 1 ].TimeStamp )
				return i;
		}

		return 0;
	}

	static uint32_t FindRotation( const AnimationBone& channel, float time )
	{
		for( size_t i = 0; i < channel.Rotations.size() - 1; i++ )
		{
			if( time < channel.Rotations[ i + 1 ].TimeStamp )
				return i;
		}

		return 0;
	}

	static uint32_t FindScale( const AnimationBone& channel, float time )
	{
		for( size_t i = 0; i < channel.Scale.size() - 1; i++ )
		{
			if( time < channel.Scale[ i + 1 ].TimeStamp )
				return i;
		}

		return 0;
	}

	static glm::vec3 InterpolatePosition( const AnimationBone& channel, float time )
	{
		if( channel.Positions.size() == 1 )
			return channel.Positions[ 0 ].Value;

		uint32_t PositionIndex = FindPositioning( channel, time );
		uint32_t NextPositionIndex = ( PositionIndex + 1 );

		SAT_CORE_ASSERT( NextPositionIndex < channel.Positions.size() );

		float DeltaTime = ( float ) ( channel.Positions[ NextPositionIndex ].TimeStamp - channel.Positions[ PositionIndex ].TimeStamp );
		float Factor = ( time - ( float ) channel.Positions[ PositionIndex ].TimeStamp ) / DeltaTime;

		SAT_CORE_ASSERT( Factor <= 1.0f, "Factor must be below 1.0f" );

		Factor = glm::clamp( Factor, 0.0f, 1.0f );
		const auto& Start = channel.Positions[ PositionIndex ].Value;
		const auto& End = channel.Positions[ NextPositionIndex ].Value;
		auto Delta = End - Start;
		auto aiVec = Start + Factor * Delta;

		return aiVec;
	}

	static glm::quat InterpolateRotation( const AnimationBone& channel, float time )
	{
		if( channel.Rotations.size() == 1 )
			return channel.Rotations[ 0 ].Value;

		uint32_t PositionIndex = FindRotation( channel, time );
		uint32_t NextPositionIndex = ( PositionIndex + 1 );

		SAT_CORE_ASSERT( NextPositionIndex < channel.Rotations.size() );

		float DeltaTime = ( float ) ( channel.Rotations[ NextPositionIndex ].TimeStamp - channel.Rotations[ PositionIndex ].TimeStamp );
		float Factor = ( time - ( float ) channel.Rotations[ PositionIndex ].TimeStamp ) / DeltaTime;

		SAT_CORE_ASSERT( Factor <= 1.0f, "Factor must be below 1.0f" );

		Factor = glm::clamp( Factor, 0.0f, 1.0f );
		const auto& Start = channel.Rotations[ PositionIndex ].Value;
		const auto& End = channel.Rotations[ NextPositionIndex ].Value;

		glm::quat q{};
		// We must use slerp and not mix. mix preforms linear interpolation while slerp does spherical
		q = glm::slerp( Start, End, Factor );
		
		return glm::normalize( q );
	}

	static glm::vec3 InterpolateScale( const AnimationBone& channel, float time )
	{
		if( channel.Scale.size() == 1 )
			return channel.Scale[ 0 ].Value;

		uint32_t PositionIndex = FindScale( channel, time );
		uint32_t NextPositionIndex = ( PositionIndex + 1 );

		SAT_CORE_ASSERT( NextPositionIndex < channel.Scale.size() );

		float DeltaTime = ( float ) ( channel.Scale[ NextPositionIndex ].TimeStamp - channel.Scale[ PositionIndex ].TimeStamp );
		float Factor = ( time - ( float ) channel.Scale[ PositionIndex ].TimeStamp ) / DeltaTime;

		SAT_CORE_ASSERT( Factor <= 1.0f, "Factor must be below 1.0f" );

		Factor = glm::clamp( Factor, 0.0f, 1.0f );
		const auto& Start = channel.Scale[ PositionIndex ].Value;
		const auto& End = channel.Scale[ NextPositionIndex ].Value;
		auto Delta = End - Start;
		auto aiVec = Start + Factor * Delta;

		return aiVec;
	}

	void Animator::ApplyBoneTransformations()
	{
		const auto& rMeshBones = m_SkeletalMesh->GetSkeletonAsset()->GetBoneInfo();

		const auto& rBones = m_AnimationAsset->GetAnimationBones();
		std::vector<glm::mat4> localTransforms( rMeshBones.size() );
		
		for( size_t i = 0; i < rMeshBones.size(); i++ )
		{
			localTransforms[ i ] = rMeshBones[ i ].BoneOffset;
		}

		for( const auto& rBone : rBones )
		{
			auto index = m_SkeletalMesh->GetSkeletonAsset()->FindBoneIndex( rBone.Name );
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

			auto final = translation * rotation * scaling;

			localTransforms[ ( size_t ) index ] = final;
		}

		// Now build the global transforms
		m_BoneTransforms.clear();
		m_BoneTransforms.resize( localTransforms.size() );

		std::function<void( size_t, const glm::mat4& )> updateBones;

		updateBones = [ & ]( size_t boneIndex, const glm::mat4& rParentTransform )
		{
			const glm::mat4 globalTransform = rParentTransform * localTransforms[ boneIndex ];

			m_BoneTransforms[ boneIndex ] = m_SkeletalMesh->GetInverseTransform() * globalTransform * rMeshBones[ boneIndex ].BoneOffset; /* <- bone offset */

			for( size_t i = 0; i < rMeshBones.size(); i++ )
			{
				if( rMeshBones[ i ].ParentIndex == boneIndex )
					updateBones( i, globalTransform );
			}
		};

		for( size_t i = 0; i < rMeshBones.size(); i++ )
		{
			if( rMeshBones[ i ].ParentIndex == -1 )
				updateBones( i, glm::mat4( 1.0f ) );
		}
	}

}
