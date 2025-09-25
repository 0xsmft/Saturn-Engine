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
		m_Vertices.resize( (size_t) ( baseVertex + pMesh->mNumVertices ) );

		for( unsigned int b = 0; b < pMesh->mNumBones; b++ )
		{
			const aiBone* pBone = pMesh->mBones[ b ];
			std::string boneName( pBone->mName.data );

			int index = 0;
			if( m_BoneMapping.find( boneName ) == m_BoneMapping.end() )
			{
				// Create new bone
				index = m_BoneInfos.size();

				SkeletalMeshBoneInfo bi{ .BoneName = boneName, .BoneOffset = Auxiliary::Mat4FromAssimpMat4( pBone->mOffsetMatrix ) };

				m_BoneInfos.push_back( bi );
				m_BoneMapping[ boneName ] = index;
			}
			else
			{
				index = m_BoneMapping[ boneName ];
			}

			// Weight calculation
			for( unsigned int w = 0; w < pBone->mNumWeights; w++ )
			{
				const int vertID = baseVertex + pBone->mWeights[ w ].mVertexId;
				const float weight = pBone->mWeights[ w ].mWeight;

				m_Vertices[ vertID ].AddBoneData( index, weight );
			}
		}
#endif
	}

	void SkeletonAsset::BuildHierarchy( const aiNode* pNode, int parentIndex )
	{
#if !defined(SAT_DIST)
		const std::string nodeName( pNode->mName.data );

		int boneIndex = -1;
		if( m_BoneMapping.find( nodeName ) != m_BoneMapping.end() ) 
		{
			boneIndex = m_BoneMapping[ nodeName ];
			m_BoneInfos[ boneIndex ].ParentIndex = parentIndex;
		}

		for( unsigned int i = 0; i < pNode->mNumChildren; ++i ) 
		{
			BuildHierarchy( pNode->mChildren[ i ], boneIndex );
		}
#endif
	}

	void SkeletonAsset::AddBoneInfo( const std::string& rName, int parentIndex, const glm::mat4& rOffsetMatrix, uint32_t boneIndex )
	{
		if( m_BoneMapping.find( rName ) == m_BoneMapping.end() )
		{
			SkeletalMeshBoneInfo bi{ .BoneName = rName, .ParentIndex = parentIndex, .BoneOffset = rOffsetMatrix };

			m_BoneInfos.push_back( bi );
			m_BoneMapping[ rName ] = boneIndex;
		}
	}

}
