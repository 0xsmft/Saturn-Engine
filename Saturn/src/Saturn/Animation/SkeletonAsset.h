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

#include "BoneJoint.h"

#include "Saturn/Asset/Asset.h"
#include "Saturn/Vulkan/VertexBuffer.h"

struct aiMesh;
struct aiNode;

namespace Saturn {

	struct SkeletalMeshBoneInfo;
	class Submesh;
	class BoneJoint;

	struct SkeletonAssetVertexSkin
	{
		uint32_t BoneIndices[ 4 ] = { 0, 0,0, 0 };
		float BoneWeights[ 4 ]{ 0.0f, 0.0f, 0.0f, 0.0f };

		inline void AddBoneData( uint32_t id, float weight )
		{
			for( size_t i = 0; i < 4; ++i )
			{
				if( BoneWeights[ i ] == 0.0f )
				{
					BoneIndices[ i ] = id;
					BoneWeights[ i ] = weight;

					return;
				}
			}
		}
	};

	enum class SkeletonAssetVersion 
	{
		// Engine version: Alpha 0.2.3
		BeforeVersionWasAdded,
		// List of compatible meshes that can use a skeleton, introduced in Alpha 0.2.3
		CompatibilityInformationForMeshes,

		//^^^ only add new versions above here....
		Latest,
		Lowest = BeforeVersionWasAdded
	};

	class SkeletonAsset : public Asset
	{
	public:
		SkeletonAsset();
		SkeletonAsset( const Ref<Asset>& rBase );
		virtual ~SkeletonAsset();

	public:
		void AppendBonesFromMesh( const aiMesh* pMesh, uint32_t baseVertex );
		void BuildHierarchy( const aiNode* pNode, int parentIndex );
		void AddCompatibleMesh( UUID id );
		void AddBoneInfo( const std::string& rName, int parentIndex, const glm::mat4& rOffsetMatrix, uint32_t boneIndex );
		void AddVertex( const SkeletonAssetVertexSkin& rSkin ) { m_Vertices.push_back( rSkin ); }
		void MarkAsUncompatibleMesh( UUID meshID );

		BoneJoint& AddNewBoneJoint( const std::string& rBoneName, const std::string& rName );

	public:
		[[nodiscard]] uint32_t FindBoneIndex( const std::string& rName ) 
		{
			const auto itr = m_BoneMapping.find( rName );
			return itr == m_BoneMapping.end() ? -1 : itr->second;
		}

		[[nodiscard]] SkeletonAssetVersion GetLocalVersion() const { return m_LocalVersion; }

		BoneJoint& FindBoneJoint( const std::string& rBoneName );
		const BoneJoint& FindBoneJoint( const std::string& rBoneName ) const;

	public:
		const std::vector<SkeletalMeshBoneInfo>& GetBoneInfo() const { return m_BoneInfos; }
		const std::vector<SkeletonAssetVertexSkin>& GetVertices() const { return m_Vertices; }
		const std::unordered_map<std::string, uint32_t>& GetBoneMapping() const { m_BoneMapping; }
#if !defined(SAT_DIST)
		const std::vector<AssetID>& GetCompatibleMeshes() const { return m_CompatibleMeshes; }
#endif

		const std::vector<BoneJoint>& GetBoneJoints() const { return m_BoneJoints; }

		// TODO: We don't want to expose this function publicly.
		void PortToNewestVersion() { m_LocalVersion = SkeletonAssetVersion::Latest; }

	private:
		SkeletonAssetVersion m_LocalVersion = SkeletonAssetVersion::Lowest;

		std::vector<SkeletalMeshBoneInfo> m_BoneInfos;
		std::vector<SkeletonAssetVertexSkin> m_Vertices;
		//					NAME	-> INDEX
		std::unordered_map<std::string, uint32_t> m_BoneMapping;

#if !defined(SAT_DIST)
		std::vector<AssetID> m_CompatibleMeshes;
#endif
		std::vector<BoneJoint> m_BoneJoints;
	};	

}
