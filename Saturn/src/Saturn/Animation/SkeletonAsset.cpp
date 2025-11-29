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

	SkeletonBoneHierarchy::SkeletonBoneHierarchy( const aiScene* pScene, SkeletonAsset* pSk, bool append /*=false*/ )
		: m_pScene( pScene ), m_pSkeleton( pSk ), m_Append( append )
	{
	}

	void SkeletonBoneHierarchy::Build()
	{
		if( !m_Append )
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
			BuildHierarchyBone( pNode, ~0u );
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

	void SkeletonBoneHierarchy::BuildHierarchyBone( const aiNode* pNode, uint32_t parentIndex )
	{
		const auto index = m_pSkeleton->SkAddBone( pNode->mName.C_Str(), parentIndex, Auxiliary::Mat4FromAssimpMat4( pNode->mTransformation )  );
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

	void SkeletonAsset::AppendBonesFromMesh( const aiMesh* pMesh )
	{
#if !defined(SAT_DIST)
		for( unsigned int b = 0; b < pMesh->mNumBones; ++b )
		{
			const aiBone* pBone = pMesh->mBones[ b ];
			std::string boneName( pBone->mName.data );

			bool hasNonZeroWeight = false;
			for( size_t j = 0; j < pBone->mNumWeights; ++j )
			{
				if( pBone->mWeights[ j ].mWeight > 0.000001f )
				{
					hasNonZeroWeight = true;
				}
			}
			if( !hasNonZeroWeight )
				continue;

			uint32_t boneSkelIndex = FindBoneIndex( boneName );

			uint32_t boneIndex = ~0;
			for( size_t i = 0; i < m_BoneInfos.size(); ++i )
			{
				if( m_BoneInfos[ i ].BoneIndex == boneSkelIndex )
				{
					boneIndex = i;
					break;
				}
			}
			
			if( boneIndex == ~0 )
			{
				boneIndex = m_BoneInfos.size();

				SkeletalMeshBoneInfo bi{ .BoneIndex = boneSkelIndex, .InverseBindPose = Auxiliary::Mat4FromAssimpMat4( pBone->mOffsetMatrix ) };
				m_BoneInfos.push_back( bi );
			}

			/*
			for( unsigned int w = 0; w < pBone->mNumWeights; ++w )
			{
				const int vertID = rSubmesh.BaseVertex + pBone->mWeights[ w ].mVertexId;
				const float weight = pBone->mWeights[ w ].mWeight;

				rInfluences[ vertID ].AddBoneData( boneIndex, weight );
			}
			*/
		}
#endif
	}

	void SkeletonAsset::ClearAll()
	{
//		m_BoneInfos.clear();
		m_BoneNames.clear();
		m_ParentBoneIndices.clear();
		m_BonePositions.clear();
		m_BoneRotations.clear();
		m_BoneScales.clear();
	}

	void SkeletonAsset::AddCompatibleMesh( UUID id )
	{
		m_CompatibleMeshes.push_back( id );
	}

	void SkeletonAsset::MarkAsUncompatibleMesh( UUID meshID )
	{
		m_CompatibleMeshes.erase( std::remove( m_CompatibleMeshes.begin(), m_CompatibleMeshes.end(), meshID ), m_CompatibleMeshes.end() );
	}

	BoneJoint& SkeletonAsset::AddNewBoneJoint( const std::string& rBoneName, const std::string& rName )
	{
		return m_BoneJoints.emplace_back( rBoneName, rName );
	}

	uint64_t SkeletonAsset::SkAddBone( const std::string& rName, uint32_t parentIndex, const glm::mat4& rTransform )
	{
		const auto itr = std::find( m_BoneNames.begin(), m_BoneNames.end(), rName );
		if( itr != m_BoneNames.end() )
		{
			return std::distance( m_BoneNames.begin(), itr );
		}

		const auto index = m_BoneNames.size();
		m_BoneNames.emplace_back( rName );
		m_ParentBoneIndices.push_back( parentIndex );

		m_BonePositions.emplace_back();
		m_BoneRotations.emplace_back();
		m_BoneScales.emplace_back();

		Maths::DecomposeTransform( rTransform, m_BonePositions.back(), m_BoneRotations.back(), m_BoneScales.back() );

		return index;
	}

	uint32_t SkeletonAsset::FindBoneIndex( const std::string& rName )
	{
		for( size_t i = 0; i < m_BoneNames.size(); i++ )
		{
			if( m_BoneNames[ i ] == rName )
			{
				return ( uint32_t ) i;
			}
		}

		return ~0;
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
