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

#pragma once

#include "AnimatorType.h"
#include "SkeletalAnimationAsset.h"

#include "Saturn/Vulkan/Mesh.h"

#include "PoseWriter.h"

#include <acl/decompression/decompress.h>

namespace Saturn {

	enum class AnimationState : uint8_t
	{
		NotInitialised, // InitAnimation not called
		Inactive, // InitAnimation called awaiting Play or Pause
		Playing,
		Paused
	};

	class AnimationController;

	//
	// The animator control the skeletal animation
	// system in an Entity.
	// 
	// The animator is responsible for Ticking any animation graphs
	// and single animation assets.
	// 
	// Animator has two animation modes, 
	// Single and AnimationControllerGraph.
	//
	class Animator : public RefTarget
	{
		SAT_DISABLE_COPY( Animator );
	public:
		Animator();
		virtual ~Animator();

		void Destroy();
		void InitAnimation( AssetID id, Ref<SkeletalMesh> sk, AnimatorType type );
		void TickAnimation( Timestep ts );
		void Pause();
		void Begin();
		void Clear();
		void StepTo( float time ) { m_AnimationTime = time; }
		void PlayFromStart();

		void SetPlaybackSpeed( float playbackSpeed ) { m_PlaybackSpeed = playbackSpeed; }
		void Loop( bool shouldLoop ) { m_Looping = shouldLoop; }

	public:
		Ref<Asset> GetCurrentAnimation() const;

		Ref<SkeletalMesh> GetSkeletalMesh() const { return m_SkeletalMesh; }

		std::vector<glm::mat4> GetBoneTransforms();

		// An animator is consider active if an animation is playing or if it's paused
		// However, if it not initialised, meaning we've never had an animation, then it's not active
		bool IsActive() const { return m_State != AnimationState::Inactive && m_State != AnimationState::NotInitialised; }

		bool IsCompleted() const { return m_Completed; }
		bool IsLooping() const { return m_Looping; }
		bool IsPlaying() const { return m_State == AnimationState::Playing && m_SingleAnimationAsset; }
		bool IsPaused() const { return m_State == AnimationState::Paused; }

		Pose* GetFinalOutPose() { return m_pOutPose; }

		float GetPlaybackSpeed() const { return m_PlaybackSpeed; }
		float GetCurrentAnimTime() const { return m_AnimationTime; }
		AnimationState GetAnimationState() const { return m_State; }

		//
		// NB: May return null if no anim graph or AnimatorType is not
		// AnimationControllerGraph
		//
		Ref<AnimationController> GetAnimGraph() const;

		void ChangeType( AnimatorType type );

		//
		// Play an animation and return back to the animation graph.
		//
		// NB: This will cause the AnimGraph to rest and start back from
		// the beginning.
		//
		void QuickFireAnimation( AssetID id );
		void QuickFireAnimation( const std::string& rName );

	private:
		void TickSingleAnim( Timestep ts );
		void AnimGraph_SetSingleAnim( Ref<SkeletalAnimationAsset> anim, bool loop );

	private:
		float m_AnimationTime = 0.0f;
		float m_PlaybackSpeed = 1.0f;
		AnimationState m_State = AnimationState::NotInitialised;
		AnimatorType m_AnimatorType = AnimatorType::Single;
		bool m_Looping = false;
		bool m_Completed = false;
		bool m_QuickFireAnimPlaying = false;
		bool m_BackupOfLooping = false;

		Ref<SkeletalAnimationAsset> m_SingleAnimationAsset;
		Ref<AnimationController> m_AnimationControllerAsset;
		Ref<SkeletalMesh> m_SkeletalMesh;

		Pose* m_pOutPose = nullptr;
		PoseWriter m_Writer;
		acl::decompression_context<acl::default_transform_decompression_settings> m_Context;

	private:
		friend class AnimGraphPlayAnimTask;
	};

}
