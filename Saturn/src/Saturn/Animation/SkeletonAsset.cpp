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
#include "SkeletonAsset.h"

#include "Saturn/Vulkan/Mesh.h"

#include "Saturn/Core/Maths.h"

#if !defined(SAT_DIST)
#include <assimp/scene.h>
#endif

namespace Saturn {

#if !defined(SAT_DIST)
namespace Auxiliary {
	static glm::mat4 Mat4FromAssimpMat4( const aiMatrix4x4& matrix )
	{
		glm::mat4 result;
		result[ 0 ][ 0 ] = matrix.a1; result[ 1 ][ 0 ] = matrix.a2; result[ 2 ][ 0 ] = matrix.a3; result[ 3 ][ 0 ] = matrix.a4;
		result[ 0 ][ 1 ] = matrix.b1; result[ 1 ][ 1 ] = matrix.b2; result[ 2 ][ 1 ] = matrix.b3; result[ 3 ][ 1 ] = matrix.b4;
		result[ 0 ][ 2 ] = matrix.c1; result[ 1 ][ 2 ] = matrix.c2; result[ 2 ][ 2 ] = matrix.c3; result[ 3 ][ 2 ] = matrix.c4;
		result[ 0 ][ 3 ] = matrix.d1; result[ 1 ][ 3 ] = matrix.d2; result[ 2 ][ 3 ] = matrix.d3; result[ 3 ][ 3 ] = matrix.d4;
		return result;
	}
}
#endif

	//////////////////////////////////////////////////////////////////////////

	SkeletonBoneHierarchy::SkeletonBoneHierarchy( const aiScene* pScene, SkeletonAsset* pSk )
		: m_pScene( pScene ), m_pSkeleton( pSk )
	{
	}

	void SkeletonBoneHierarchy::InitSkeleton()
	{
		m_pSkeleton->ClearAll();

		for( uint32_t i = 0; i < m_pScene->mNumMeshes; ++i )
		{
			const aiMesh* pMesh = m_pScene->mMeshes[ i ];
			for( uint32_t j = 0; j < pMesh->mNumBones; ++j )
			{
				m_Names.emplace( pMesh->mBones[ j ]->mName.C_Str() );
			}
		}

		for( uint32_t i = 0; i < m_pScene->mNumAnimations; ++i )
		{
			const aiAnimation* pMesh = m_pScene->mAnimations[ i ];
			for( uint32_t j = 0; j < pMesh->mNumChannels; ++j )
			{
				const aiNodeAnim* pAnimNode = pMesh->mChannels[ j ];
				m_Names.emplace( pAnimNode->mNodeName.C_Str() );
			}
		}

		BuildHierarchy( m_pScene->mRootNode );
	}

	void SkeletonBoneHierarchy::BuildHierarchy( const aiNode* pNode, const glm::mat4& rTransform )
	{
		const std::string boneName( pNode->mName.C_Str() );
		if( m_Names.find( boneName ) != m_Names.end() )
		{
			m_pSkeleton->SetTransform( rTransform );
			BuildHierarchyBone( pNode, ~0 );
		}
		else
		{
			for( uint32_t i = 0; i < pNode->mNumChildren; ++i )
			{
				const auto ts = rTransform * Auxiliary::Mat4FromAssimpMat4( pNode->mTransformation );
				BuildHierarchy( pNode->mChildren[ i ], ts );
			}
		}
	}

	void SkeletonBoneHierarchy::BuildHierarchyBone( const aiNode* pNode, int parentIndex )
	{
		const auto index = m_pSkeleton->SkAddBone( pNode->mName.C_Str(), parentIndex, Auxiliary::Mat4FromAssimpMat4( pNode->mTransformation ) );
		for( uint32_t i = 0; i < pNode->mNumChildren; i++ )
		{
			if( m_Names.find( pNode->mChildren[ i ]->mName.C_Str() ) != m_Names.end() )
			{
				BuildHierarchyBone( pNode->mChildren[ i ], index );
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////

	SkeletonAsset::SkeletonAsset()
	{
	}

	SkeletonAsset::SkeletonAsset( const Ref<Asset>& rBase )
		: Asset( rBase )
	{
	}

	SkeletonAsset::~SkeletonAsset()
	{
	}

	void SkeletonAsset::AppendBonesFromMesh( const aiMesh* pMesh, uint32_t baseVertex )
	{
#if !defined(SAT_DIST)
		for( unsigned int b = 0; b < pMesh->mNumBones; ++b )
		{
			const aiBone* pBone = pMesh->mBones[ b ];
			std::string boneName( pBone->mName.data );

			bool hasNonZeroWeight = false;
			for( size_t j = 0; j < pBone->mNumWeights; j++ )
			{
				if( pBone->mWeights[ j ].mWeight > 0.000001f )
				{
					hasNonZeroWeight = true;
				}
			}
			if( !hasNonZeroWeight )
				continue;

			auto index = m_BoneNames.size();

			SkeletalMeshBoneInfo bi{ .BoneIndex = index, .InverseBindPose = Auxiliary::Mat4FromAssimpMat4( pBone->mOffsetMatrix ) };
			m_BoneInfos.push_back( bi );
			m_BoneNames.push_back( boneName );
		}
#endif
	}

	void SkeletonAsset::BuildHierarchy( const aiNode* pNode, int parentIndex, const glm::mat4& rTransform )
	{
#if !defined(SAT_DIST)
		const std::string nodeName( pNode->mName.data );

		if( const auto itr = std::find( m_BoneNames.begin(), m_BoneNames.end(), nodeName ); itr != m_BoneNames.end() )
		{
			if( parentIndex == -1 )
				m_Transform = rTransform;

			const auto ts = rTransform * Auxiliary::Mat4FromAssimpMat4( pNode->mTransformation );
			for( unsigned int i = 0; i < pNode->mNumChildren; ++i )
			{
				const auto index = std::distance( m_BoneNames.begin(), itr );

				m_BoneInfos[ ( uint64_t ) index ].ParentIndex = parentIndex;

				m_BonePositions.emplace_back();
				m_BoneRotations.emplace_back();
				m_BoneScales.emplace_back();

				Maths::DecomposeTransform( ts, m_BonePositions.back(), m_BoneRotations.back(), m_BoneScales.back() );

				BuildHierarchy( pNode->mChildren[ i ], parentIndex + 1, ts );
			}
		}
		else
		{
			const auto ts = rTransform * Auxiliary::Mat4FromAssimpMat4( pNode->mTransformation );
			for( unsigned int i = 0; i < pNode->mNumChildren; ++i )
			{
				BuildHierarchy( pNode->mChildren[ i ], parentIndex, ts );
			}
		}
#endif
	}

	void SkeletonAsset::ClearAll()
	{
		m_BoneInfos.clear();
	}


		for( unsigned int i = 0; i < pNode->mNumChildren; ++i ) 
		{
			BuildHierarchy( pNode->mChildren[ i ], boneIndex );
		}
#endif
	}

	void SkeletonAsset::AddCompatibleMesh( UUID id )
	{
		m_CompatibleMeshes.push_back( id );
	}

	void SkeletonAsset::AddBoneInfo( const std::string& rName, int parentIndex, const glm::mat4& rOffsetMatrix, uint32_t boneIndex )
	{
	}

	void SkeletonAsset::MarkAsUncompatibleMesh( UUID meshID )
	{
		m_CompatibleMeshes.erase( std::remove( m_CompatibleMeshes.begin(), m_CompatibleMeshes.end(), meshID ), m_CompatibleMeshes.end() );
	}

	BoneJoint& SkeletonAsset::AddNewBoneJoint( const std::string& rBoneName, const std::string& rName )
	{
		return m_BoneJoints.emplace_back( rBoneName, rName );
	}

	BoneJoint* SkeletonAsset::FindBoneJoint( const std::string& rBoneName )
	{
		auto itr = std::find_if( m_BoneJoints.begin(), m_BoneJoints.end(), 
			[rBoneName](const auto& rItem) 
		{
			return rItem.GetBoneName() == rBoneName;
		} );

		if( itr == m_BoneJoints.end() )
		{
			return nullptr;
		}

		return &*itr;
	}

	const BoneJoint* SkeletonAsset::FindBoneJoint( const std::string& rBoneName ) const
	{
		const auto itr = std::find_if( m_BoneJoints.begin(), m_BoneJoints.end(),
			[ rBoneName ]( const auto& rItem )
		{
			return rItem.GetBoneName() == rBoneName;
		} );

		if( itr == m_BoneJoints.end() )
		{
			return nullptr;
		}

		return &*itr;
	}

}
