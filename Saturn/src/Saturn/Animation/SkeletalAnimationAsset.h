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

struct aiAnimation;

namespace Saturn {

	template<typename Ty>
	struct AnimationKey
	{
		AnimationKey() = default;

		AnimationKey( const Ty& rValue, float ts )
			: Value( rValue ), Timestamp( ts )
		{
		}

		Ty Value = Ty{};
		// The time stamp when our value should be applied to the armature.
		float Timestamp = 0.0f;
	};

	// NOTE: This used to be called AnimationBone, hence why you may see "ab" or animBone in the codebase.
	struct AnimationChannel
	{
		uint32_t Index = ~0u;
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
		[[nodiscard]] float   GetDuration()       const { return m_Duration; }
		[[nodiscard]] float   GetTicksPerSecond() const { return m_TicksPerSecond; }
		[[nodiscard]] bool    IsUsingRootMotion() const { return m_UseRootMotion; }
		[[nodiscard]] size_t  GetBoneCount()      const { return m_BoneCount; }
		[[nodiscard]] SkeletalAnimationAssetVersion GetLocalAssetVersion() const { return m_LocalVersion; }
		[[nodiscard]] void*   GetData()           const { return m_pData; }

#if !defined(SAT_DIST)
		const std::vector<AnimationChannel>& GetAnimationBones() const { return m_Bones; }
		std::vector<AnimationChannel>& GetAnimationBones() { return m_Bones; }
#endif

	public:
		void SetDuration( float duration )               { m_Duration = duration; }
		void SetTicks( float ticks )                     { m_TicksPerSecond = ticks; }
		void SetSkeletonID( AssetID id )                 { m_SkeletonAssetID = id; }
		void UseRootMotion( bool val )                   { m_UseRootMotion = val; }
		void SetUncompressedDuration( float duration )   { m_UncompressedDuration = duration; }
		void SetUncompressedTicks( float ticks )         { m_UncompressedTPS = ticks; }
		void SetBoneCount( size_t boneCount )			 { m_BoneCount = boneCount; }
		void PortToNewestVersion()                       { m_LocalVersion = SkeletalAnimationAssetVersion::Latest; }

		void SetACLData( void* pData );

#if !defined(SAT_DIST)
		void AddAnimBone( const AnimationChannel& bone ) { m_Bones.push_back( bone ); }

		void MakeUniformAndCompress( aiAnimation* pAnimation );
		void Compress();

		void Serialise( std::ofstream& rStream ) const;
		void Deserialise( std::ifstream& rStream );
#endif

	private:
#if !defined(SAT_DIST)
		void SerialiseAclData( std::ofstream& rStream ) const;
		void DeserialiseAclData( std::ifstream& rStream );
#else
		void DeserialiseAclData( std::istream& rStream );
#endif

	private:
		SkeletalAnimationAssetVersion m_LocalVersion = SkeletalAnimationAssetVersion::Lowest;
		bool m_UseRootMotion = false;

		void* m_pData = nullptr;

		AssetID m_SkeletonAssetID = 0llu;

		size_t m_BoneCount = 0;

		// The duration of this animation in seconds
		float m_Duration = 0.0f;
		float m_TicksPerSecond = 0.0f;

		float m_UncompressedDuration = 0.0f;
		float m_UncompressedTPS = 0.0f;

#if !defined(SAT_DIST)
		std::vector<AnimationChannel> m_Bones;
#endif

	private:
		friend class RawSkeletalAnimationSerialiser;
	};

}
