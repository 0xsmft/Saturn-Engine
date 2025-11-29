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

#include <set>

struct aiMesh;
struct aiNode;
struct aiScene;

namespace Saturn {

	struct SkeletalMeshBoneInfo;
	class Submesh;
	class BoneJoint;

	enum class SkeletonAssetVersion 
	{
		// Engine version: Alpha 0.2.3
		BeforeVersionWasAdded,
		// List of compatible meshes that can use a skeleton, introduced in Alpha 0.2.3
		CompatibilityInformationForMeshes,
		AttachmentPoints,

		//^^^ only add new versions above here....
		Latest,
		Lowest = BeforeVersionWasAdded
	};

	class SkeletonAsset;
	class SkeletonBoneHierarchy 
	{
	public:
		SkeletonBoneHierarchy( const aiScene* pScene, SkeletonAsset* pSk, bool append = false );

		void Build();

	private:
		void BuildHierarchy( const aiNode* pNode, const glm::mat4& rTransform = glm::mat4{ 1.0f } );
		void BuildHierarchyBone( const aiNode* pNode, uint32_t parentIndex );

	private:
		std::set<std::string> m_Names;
		bool m_Append = false;
		const aiScene* m_pScene = nullptr;
		SkeletonAsset* m_pSkeleton = nullptr;
	};

	class SkeletonAsset : public Asset
	{
	public:
		SkeletonAsset();
		SkeletonAsset( const Ref<Asset>& rBase );
		virtual ~SkeletonAsset();

	public:
		void AppendBonesFromMesh( const aiMesh* pMesh );
		void AddCompatibleMesh( UUID id );
		void MarkAsUncompatibleMesh( UUID meshID );

		void SetTransform( const glm::mat4& rTransform ) { m_Transform = rTransform; }

		BoneJoint& AddNewBoneJoint( const std::string& rBoneName, const std::string& rName );

		uint64_t SkAddBone( const std::string& rName, uint32_t parentIndex, const glm::mat4& rTransform );

	public:
		[[nodiscard]] uint32_t FindBoneIndex( const std::string& rName );

		const std::string& GetBoneName( uint64_t index ) 
		{
			SAT_CORE_ASSERT( index < m_BoneNames.size() );
			return m_BoneNames[ index ];
		}

		[[nodiscard]] SkeletonAssetVersion GetLocalVersion() const { return m_LocalVersion; }

		BoneJoint* FindBoneJoint( const std::string& rBoneName );
		const BoneJoint* FindBoneJoint( const std::string& rBoneName ) const;

	public:
		const std::vector<SkeletalMeshBoneInfo>& GetBoneInfo() const { return m_BoneInfos; }
#if !defined(SAT_DIST)
		const std::vector<AssetID>& GetCompatibleMeshes() const { return m_CompatibleMeshes; }
#endif

		std::vector<uint64_t>& GetParentIndices() { return m_ParentBoneIndices; }

		uint64_t GetParentIndex( uint32_t index ) { SAT_CORE_ASSERT( index < m_ParentBoneIndices.size() ); return m_ParentBoneIndices[ index ]; }

		const std::vector<BoneJoint>& GetBoneJoints() const { return m_BoneJoints; }
		std::vector<BoneJoint>& GetBoneJoints() { return m_BoneJoints; }

		std::vector<std::string>& GetBoneNames() { return m_BoneNames; }
		std::vector<glm::vec3>& GetBonePositions() { return m_BonePositions; }
		std::vector<glm::quat>& GetBoneRotations() { return m_BoneRotations; }
		std::vector<glm::vec3>& GetBoneScales() { return m_BoneScales; }

		const glm::mat4& GetTransform() const { return m_Transform; }

		// TODO: We don't want to expose this function publicly.
		void PortToNewestVersion() { m_LocalVersion = SkeletonAssetVersion::Latest; }

		void ClearAll();

	private:
		SkeletonAssetVersion m_LocalVersion = SkeletonAssetVersion::Lowest;

		std::vector<SkeletalMeshBoneInfo> m_BoneInfos;

		std::vector<uint64_t> m_ParentBoneIndices;
		std::vector<std::string> m_BoneNames;

		glm::mat4 m_Transform{};

#if !defined(SAT_DIST)
		std::vector<AssetID> m_CompatibleMeshes;
#endif
		std::vector<BoneJoint> m_BoneJoints;
		
		std::vector<glm::vec3> m_BonePositions;
		std::vector<glm::quat> m_BoneRotations;
		std::vector<glm::vec3> m_BoneScales;

	private:
		friend class SkeletonAssetSerialiser;
		friend class SkeletalMesh;
	};	

}
