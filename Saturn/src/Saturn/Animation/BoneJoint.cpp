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
#include "BoneJoint.h"

#include "Animator.h"
#include "SkeletonAsset.h"

#include "Saturn/Vulkan/Mesh.h"

namespace Saturn {

	BoneJoint::BoneJoint()
	{
	}

	BoneJoint::BoneJoint( const std::string& rBoneName, const std::string& rName )
		: m_BoneName( rBoneName ), m_Name( rName )
	{
	}

	BoneJoint::~BoneJoint()
	{
	}

	glm::mat4 BoneJoint::GetBoneMatrix( Ref<Animator> animator ) const
	{
		const int boneIndex = animator->GetSkeletalMesh()->GetSkeletonAsset()->FindBoneIndex( m_BoneName );
		if( boneIndex != -1 )
		{
			const auto& boneTransform = animator->GetBoneTransforms().at( boneIndex );
			const auto ts = glm::translate( glm::mat4( 1.0f ), m_Position )
				* glm::toMat4( m_Rotation );

			return ts * boneTransform;
		}

		return glm::one<glm::mat4>();
	}

	glm::mat4 BoneJoint::GetBoneMatrixPreview( Ref<class SkeletalMesh> animator ) const
	{
		const int boneIndex = animator->GetSkeletonAsset()->FindBoneIndex( m_BoneName );
		if( boneIndex != -1 )
		{
			const auto& boneTransform = animator->GetDefaultBoneTransforms().at( boneIndex );
			const auto ts = glm::translate( glm::mat4( 1.0f ), m_Position )
				* glm::toMat4( m_Rotation );

			return ts * boneTransform;
		}

		return glm::one<glm::mat4>();
	}

}
