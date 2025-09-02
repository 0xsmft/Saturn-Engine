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

	struct AnimationKeyVec3
	{
		AnimationKeyVec3() = default;

		AnimationKeyVec3( const glm::vec3& rValue, double ts ) 
			: Value( rValue ), TimeStamp( ts )
		{
		}

		glm::vec3 Value{ 0.0f };
		// The time stamp when our value should be applied to the armature.
		double TimeStamp = 0.0;
	};

	struct AnimationKeyQuat
	{
		AnimationKeyQuat() = default;

		AnimationKeyQuat( const glm::quat& rValue, double ts )
			: Value( rValue ), TimeStamp( ts )
		{
		}

		glm::quat Value{ 0.0f, 0.0f, 0.0f, 0.0f };
		// The time stamp when our value should be applied to the armature.
		double TimeStamp = 0.0;
	};

	struct AnimationBone
	{
		std::string Name;
		std::vector<AnimationKeyVec3> Positions;
		std::vector<AnimationKeyQuat> Rotations;
		std::vector<AnimationKeyVec3> Scale;
	};

	class SkeletalAnimationAsset : public Asset
	{
	public:
		SkeletalAnimationAsset() = default;
		SkeletalAnimationAsset( const Ref<Asset>& rBase );

		virtual ~SkeletalAnimationAsset();

		void SetDuration( double duration ) { m_Duration = duration; }
		void SetTicks( double ticks ) { m_TicksPerSecond = ticks; }
		void SetSkeletonID( AssetID id ) { m_SkeletonAssetID = id; }
		void AddAnimBone( AnimationBone bone ) { m_Bones.push_back( bone ); }

		[[nodiscard]] AssetID GetSkeletonID() const { return m_SkeletonAssetID; }
		[[nodiscard]] double  GetDuration() const   { return m_Duration; }
		[[nodiscard]] double  GetTicksPerSecond() const { return m_TicksPerSecond; }

		const std::vector<AnimationBone>& GetAnimationBones() const { return m_Bones; }

	private:
		AssetID m_SkeletonAssetID;

		// The duration of this animation in seconds
		double m_Duration = 0.0;
		double m_TicksPerSecond = 0.0;

		std::vector<AnimationBone> m_Bones;
	};

}
