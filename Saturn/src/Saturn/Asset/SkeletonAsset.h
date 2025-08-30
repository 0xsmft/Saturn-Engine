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

#include "Asset.h"

struct aiMesh;
struct aiNode;

namespace Saturn {

	struct SkeletalMeshBoneInfo;
	class Submesh;

	struct SkeletonAssetVertexSkin
	{
		uint32_t BoneIndices[ 4 ] = { 0, 0,0, 0 };
		float BoneWeights[ 4 ]{ 0.0f, 0.0f, 0.0f, 0.0f };

		inline void AddBoneData( uint32_t id, float weight )
		{
			for( size_t i = 0; i < 4; i++ )
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

	class SkeletonAsset : public Asset
	{
	public:
		SkeletonAsset();
		SkeletonAsset( const Ref<Asset>& rBase );
		virtual ~SkeletonAsset();

		void CreateFromMesh( const aiMesh* pMesh, const Submesh& rSubmesh );
		void BuildHierarchy( const aiNode* pNode, int parentIndex );
		
		void AddBoneInfo( const std::string& rName, int parentIndex, const glm::mat4& rOffsetMatrix, uint32_t boneIndex );
		void AddVertex( const SkeletonAssetVertexSkin& rSkin ) { m_Vertices.push_back( rSkin ); }

	public:
		const std::vector<SkeletalMeshBoneInfo>& GetBoneInfo() const { return m_BoneInfos; }
		const std::vector<SkeletonAssetVertexSkin>& GetVertices() const { return m_Vertices; }
		const std::unordered_map<std::string, uint32_t>& GetBoneMapping() const { m_BoneMapping; }

	private:
		std::vector<SkeletalMeshBoneInfo> m_BoneInfos;
		std::vector<SkeletonAssetVertexSkin> m_Vertices;
		std::unordered_map<std::string, uint32_t> m_BoneMapping;
	};	

}
