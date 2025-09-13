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

#pragma once

#include "SkeletalAnimationAsset.h"
#include "Saturn/Vulkan/Mesh.h"

namespace Saturn {

	enum class AnimationState 
	{
		NotInitialised, // InitAnimation not called
		Inactive, // InitAnimation called awaiting Play or Pause
		Playing,
		Paused
	};

	class Animator
	{
	public:
		Animator();
		~Animator();

		void InitAnimation( AssetID id, Ref<SkeletalMesh> sk );
		void TickAnimation( Timestep ts );
		void Pause();
		void Begin();
		void Clear();

		void QueueNewAnimation( AssetID id );

		AssetID GetCurrentID() const { return m_AnimationAsset != nullptr ? m_AnimationAsset->ID : AssetID( 0 ); }

		const std::vector<glm::mat4>& GetBoneTransforms() const { return m_BoneTransforms; }

		// An animator is consider active if an animation is playing or if it's paused
		// However, if it not initialised, meaning we've never had an animation, then it's not active
		bool IsActive() const { return m_State != AnimationState::Inactive && m_State != AnimationState::NotInitialised; }

	private:
		void ApplyBoneTransformations();
		void UpdateBones( size_t boneIndex, const glm::mat4& rParentTransform, const std::vector<glm::mat4>& rLocalTransforms );

	private:
		Ref<SkeletalAnimationAsset> m_AnimationAsset;
		Ref<SkeletalMesh> m_SkeletalMesh;

		AnimationState m_State = AnimationState::NotInitialised;

		float m_StartTime = 0.0f;
		float m_AnimationTime = 0.0f;

		AssetID m_PendingAsset = 0llu;

		std::vector<glm::mat4> m_BoneTransforms;
	};

}
