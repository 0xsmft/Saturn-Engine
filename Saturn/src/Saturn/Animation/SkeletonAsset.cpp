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

#if !defined(SAT_DIST)
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
			/*
			aiNode* pParent = pNode->mParent;
			while( pParent != nullptr )
			{
				pParent->mTransformation = aiMatrix4x4();
				pParent = pParent->mParent;
			}
			*/

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
		for( uint32_t i = 0; i < pNode->mNumChildren; ++i )
		{
			if( m_Names.find( pNode->mChildren[ i ]->mName.C_Str() ) != m_Names.end() )
			{
				BuildHierarchyBone( pNode->mChildren[ i ], index );
			}
		}
	}
#endif

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

#if !defined(SAT_DIST)
	void SkeletonAsset::AppendBonesFromMesh( const aiMesh* pMesh )
	{
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
	}
#endif

	void SkeletonAsset::ClearAll()
	{
//		m_BoneInfos.clear();
		m_BoneNames.clear();
		m_ParentBoneIndices.clear();
		m_BonePositions.clear();
		m_BoneRotations.clear();
		m_BoneScales.clear();
	}

#if !defined(SAT_DIST)
	void SkeletonAsset::AddCompatibleMesh( UUID id )
	{
		m_CompatibleMeshes.push_back( id );
	}

	void SkeletonAsset::MarkAsUncompatibleMesh( UUID meshID )
	{
		m_CompatibleMeshes.erase( std::remove( m_CompatibleMeshes.begin(), m_CompatibleMeshes.end(), meshID ), m_CompatibleMeshes.end() );
	}
#endif

	struct SkeletonAssetFileHeader
	{
		// .SK
		const unsigned char Magic[ 3 ] = { 0x2E, 0x53, 0x4B };
	};

	static void SerialiseBoneJoints( const std::vector<BoneJoint>& rBoneJoints, std::ofstream& rStream ) 
	{
		RawSerialisation::WriteObject( rBoneJoints.size(), rStream );

		for( const auto& rJoint : rBoneJoints )
		{
			RawSerialisation::WriteObject( rJoint.GetBoneIndex(), rStream );
			RawSerialisation::WriteString( rJoint.GetBoneName(), rStream );
			RawSerialisation::WriteString( rJoint.GetName(), rStream );
			RawSerialisation::WriteVec3( rJoint.GetRelativePosition(), rStream );
			RawSerialisation::WriteObject( rJoint.GetRelativeRotation(), rStream );
			RawSerialisation::WriteVec3( rJoint.GetRelativeScale(), rStream );
		}
	}

	static void DeserialiseBoneJoints( std::vector<BoneJoint>& rBoneJoints, std::ifstream& rStream )
	{
		size_t size = 0llu;
		RawSerialisation::ReadObject( size, rStream );

		rBoneJoints.reserve( size );

		for( size_t i = 0; i < size; ++i )
		{
			uint64_t boneIndex = 0llu;
			RawSerialisation::ReadObject( boneIndex, rStream );
			const auto boneName = RawSerialisation::ReadString( rStream );
			const auto jointName = RawSerialisation::ReadString( rStream );

			auto& rBoneJoint = rBoneJoints.emplace_back( boneIndex, boneName, jointName );

			glm::vec3 pos{}, scale{};
			glm::quat rot{};

			RawSerialisation::ReadVec3( pos, rStream );
			RawSerialisation::ReadObject( rot, rStream );
			RawSerialisation::ReadVec3( scale, rStream );

			rBoneJoint.SetRelativePosition( pos );
			rBoneJoint.SetRelativeRotation( rot );
			rBoneJoint.SetRelativeScale( scale );
		}
	}

	void SkeletonAsset::Serialise( const std::filesystem::path& rPath ) const
	{
		std::ofstream fout( rPath, std::ios::binary | std::ios::trunc );

		SkeletonAssetFileHeader header;
		RawSerialisation::WriteObject( header, fout );

		RawSerialisation::WriteObject( GetLocalVersion(), fout );

		RawSerialisation::WriteVector( m_BoneInfos, fout );
		RawSerialisation::WriteVector( m_ParentBoneIndices, fout );
		RawSerialisation::WriteVector( m_BoneNames, fout );
		RawSerialisation::WriteObject( m_Transform, fout );
#if !defined(SAT_DIST)
		RawSerialisation::WriteVector( m_CompatibleMeshes, fout );
#endif
		SerialiseBoneJoints( m_BoneJoints, fout );

		RawSerialisation::WriteVector( m_BonePositions, fout );
		RawSerialisation::WriteVector( m_BoneRotations, fout );
		RawSerialisation::WriteVector( m_BoneScales, fout );

		fout.close();
	}

	void SkeletonAsset::Deserialise( std::filesystem::path& rPath )
	{
		std::ifstream FileIn( rPath, std::ios::binary | std::ios::in );

		SkeletonAssetFileHeader header;
		RawSerialisation::ReadObject( header, FileIn );

		SkeletonAssetVersion skVersion = SkeletonAssetVersion::Lowest;
		RawSerialisation::ReadObject( skVersion, FileIn );

		RawSerialisation::ReadVector( m_BoneInfos, FileIn );
		RawSerialisation::ReadVector( m_ParentBoneIndices, FileIn );
		RawSerialisation::ReadVector( m_BoneNames, FileIn );
		RawSerialisation::ReadObject( m_Transform, FileIn );
#if !defined(SAT_DIST)
		RawSerialisation::ReadVector( m_CompatibleMeshes, FileIn );
#endif
		if( skVersion >= SkeletonAssetVersion::AttachmentPointsV2 )
		{
			DeserialiseBoneJoints( m_BoneJoints, FileIn );
		}

		RawSerialisation::ReadVector( m_BonePositions, FileIn );
		RawSerialisation::ReadVector( m_BoneRotations, FileIn );
		RawSerialisation::ReadVector( m_BoneScales, FileIn );

		m_LocalVersion = skVersion;

		FileIn.close();
	}

	BoneJoint& SkeletonAsset::AddNewBoneJoint( 
		const uint64_t boneIndex, 
		const std::string& rBoneName, 
		const std::string& rName )
	{
		return m_BoneJoints.emplace_back( boneIndex, rBoneName, rName );
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
		m_ParentBoneIndices.push_back( ( uint64_t )parentIndex );

		m_BonePositions.emplace_back();
		m_BoneRotations.emplace_back();
		m_BoneScales.emplace_back();

		Maths::DecomposeTransform( rTransform, m_BonePositions.back(), m_BoneRotations.back(), m_BoneScales.back() );

		return index;
	}

	uint32_t SkeletonAsset::FindBoneIndex( const std::string& rName )
	{
		for( size_t i = 0; i < m_BoneNames.size(); ++i )
		{
			if( m_BoneNames[ i ] == rName )
			{
				return ( uint32_t ) i;
			}
		}

		return ~0u;
	}

	BoneJoint* SkeletonAsset::FindBoneJoint( const std::string& rBoneName )
	{
		auto itr = std::find_if( m_BoneJoints.begin(), m_BoneJoints.end(),
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
