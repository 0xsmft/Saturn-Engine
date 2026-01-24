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

	class SkeletonAsset;

#if !defined(SAT_DIST)
	//
	// Helper class to build the hierarchy of a skeleton.
	//
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

		// Use raw pointer here, no need for us to really hold a smart pointer reference.
		SkeletonAsset* m_pSkeleton = nullptr;
	};
#endif

	enum class SkeletonAssetVersion
	{
		// Engine version: Alpha 0.2.3
		BeforeVersionWasAdded,
		
		// List of compatible meshes that can use a skeleton, introduced in Alpha 0.2.3
		CompatibilityInformationForMeshes,

		// AttachmentPoints (BoneJoints)
		AttachmentPoints,

		//^^^ only add new versions above here....
		Latest,
		Lowest = BeforeVersionWasAdded
	};

	struct SkeletalMeshBoneInfo;
	class Submesh;
	class BoneJoint;

	class SkeletonAsset : public Asset
	{
	public:
		SkeletonAsset();
		SkeletonAsset( const Ref<Asset>& rBase );
		virtual ~SkeletonAsset();

	public:
#if !defined(SAT_DIST)
		void AppendBonesFromMesh( const aiMesh* pMesh );
		void AddCompatibleMesh( UUID id );
		void MarkAsUncompatibleMesh( UUID meshID );

		void SetTransform( const glm::mat4& rTransform ) { m_Transform = rTransform; }
#endif
	public:
		void Serialise( const std::filesystem::path& rPath ) const;
		void Deserialise( std::filesystem::path& rPath );

	public:
		BoneJoint& AddNewBoneJoint( const std::string& rBoneName, const std::string& rName );

		// INTERNAL, Adds a bone to the skeleton.
		uint64_t SkAddBone( const std::string& rName, uint32_t parentIndex, const glm::mat4& rTransform );

	public:
		[[nodiscard]] uint32_t FindBoneIndex( const std::string& rName );

		// 
		// @brief Returns the bone name of the specified bone at an index.
		// 
		// @note Asserts on invalid index.
		// 
		const std::string& GetBoneName( uint64_t index ) 
		{
			SAT_CORE_ASSERT( index < m_BoneNames.size() );
			return m_BoneNames[ index ];
		}

		BoneJoint* FindBoneJoint( const std::string& rBoneName );
		const BoneJoint* FindBoneJoint( const std::string& rBoneName ) const;

	public:
		const std::vector<SkeletalMeshBoneInfo>& GetBoneInfo() const { return m_BoneInfos; }
#if !defined(SAT_DIST)
		const std::vector<AssetID>& GetCompatibleMeshes() const { return m_CompatibleMeshes; }
#endif
		[[nodiscard]] SkeletonAssetVersion GetLocalVersion() const { return m_LocalVersion; }

		std::vector<uint64_t>& GetParentIndices() { return m_ParentBoneIndices; }

		// 
		// @brief Returns the parent index of the specified bone at an index.
		// 
		// @note Asserts on invalid index.
		// 
		uint64_t GetParentIndex( uint32_t index ) { SAT_CORE_ASSERT( index < m_ParentBoneIndices.size() ); return m_ParentBoneIndices[ index ]; }

		const std::vector<BoneJoint>& GetBoneJoints() const { return m_BoneJoints; }
		std::vector<BoneJoint>& GetBoneJoints() { return m_BoneJoints; }

		std::vector<std::string>& GetBoneNames() { return m_BoneNames; }

		// 
		// All bone positions are in bone space.
		// 
		std::vector<glm::vec3>& GetBonePositions() { return m_BonePositions; }

		// 
		// All bone rotations are in bone space. Rotations are returned as a quaternion.
		// 
		std::vector<glm::quat>& GetBoneRotations() { return m_BoneRotations; }
		
		// 
		// All bone scales are in bone space.
		// 
		std::vector<glm::vec3>& GetBoneScales() { return m_BoneScales; }

		//
		// The global transform for this skeleton.
		//
		const glm::mat4& GetTransform() const { return m_Transform; }

		void ClearAll();
		
	public:
		// TODO: We don't want to expose this function publicly.
		void PortToNewestVersion() { m_LocalVersion = SkeletonAssetVersion::Latest; }

	private:
		SkeletonAssetVersion m_LocalVersion = SkeletonAssetVersion::Lowest;

		std::vector<SkeletalMeshBoneInfo> m_BoneInfos;

		// Linear map of bone parent indices
		// Will look like
		// 0 : ~0u
		// 1 : 0
		// 2 : 0
		// 3 : 1
		// ...
		std::vector<uint64_t> m_ParentBoneIndices;
		std::vector<std::string> m_BoneNames;

		glm::mat4 m_Transform{};

#if !defined(SAT_DIST)
		std::vector<AssetID> m_CompatibleMeshes;
#endif

		std::vector<BoneJoint> m_BoneJoints;
		
		// In bone space, each local to the bone it self.
		std::vector<glm::vec3> m_BonePositions;
		std::vector<glm::quat> m_BoneRotations;
		std::vector<glm::vec3> m_BoneScales;

	private:
		friend class SkeletonAssetSerialiser;
		friend class RawSkeletonAssetSerialiser;
		friend class SkeletalMesh;
	};	

}
