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

#include "Saturn/Asset/Asset.h"

namespace Saturn {

	template<typename Ty>
	struct AnimationKey
	{
		AnimationKey() = default;

		AnimationKey( const Ty& rValue, double ts )
			: Value( rValue ), TimeStamp( ts )
		{
		}

		Ty Value = Ty{};
		// The time stamp when our value should be applied to the armature.
		double TimeStamp = 0.0;
	};

	struct AnimationBone
	{
		std::string Name;
		std::vector<AnimationKey<glm::vec3>> Positions;
		std::vector<AnimationKey<glm::quat>> Rotations;
		std::vector<AnimationKey<glm::vec3>> Scale;
	};

	enum class SkeletalAnimationAssetVersion
	{
		// Engine version: Alpha 0.2.3
		BeforeVersionWasAdded,
		// Root motion support added, introduced in Alpha 0.2.3
		RootMotion,

		//^^^ only add new versions above here....
		Latest,
		Lowest = BeforeVersionWasAdded
	};

	class SkeletalAnimationAsset : public Asset
	{
	public:
		SkeletalAnimationAsset() = default;
		SkeletalAnimationAsset( const Ref<Asset>& rBase );

		virtual ~SkeletalAnimationAsset();

		[[nodiscard]] AssetID GetSkeletonID()     const { return m_SkeletonAssetID; }
		[[nodiscard]] double  GetDuration()       const { return m_Duration; }
		[[nodiscard]] double  GetTicksPerSecond() const { return m_TicksPerSecond; }
		[[nodiscard]] bool    IsUsingRootMotion() const { return m_UseRootMotion; }
		[[nodiscard]] SkeletalAnimationAssetVersion GetLocalAssetVersion() const { return m_LocalVersion; }

		const std::vector<AnimationBone>& GetAnimationBones() const { return m_Bones; }

	public:
		void SetDuration( double duration ) { m_Duration = duration; }
		void SetTicks( double ticks ) { m_TicksPerSecond = ticks; }
		void SetSkeletonID( AssetID id ) { m_SkeletonAssetID = id; }
		void UseRootMotion( bool val ) { m_UseRootMotion = val; }
		void AddAnimBone( const AnimationBone& bone ) { m_Bones.push_back( bone ); }
		void PortToNewestVersion() { m_LocalVersion = SkeletalAnimationAssetVersion::Latest; }

	private:
		SkeletalAnimationAssetVersion m_LocalVersion = SkeletalAnimationAssetVersion::Lowest;
		bool m_UseRootMotion = false;

		AssetID m_SkeletonAssetID;

		// The duration of this animation in seconds
		double m_Duration = 0.0;
		double m_TicksPerSecond = 0.0;

		std::vector<AnimationBone> m_Bones;
	};

}
