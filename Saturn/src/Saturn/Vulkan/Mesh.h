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

#pragma once

#include "Shader.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Material.h"

#include "Saturn/Core/Base.h"
#include "Saturn/Core/AABB/AABB.h"
#include "Saturn/Core/Renderer/EditorCamera.h"

#include "Saturn/Asset/MemoryAssetDependency.h"
#include "Saturn/Asset/MaterialAsset.h"

#include "Saturn/Physics/PhysicsShapeTypes.h"

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

#if !defined(SAT_DIST)
#include "Saturn/ImGui/AssetImportPopupErrors.h"
#endif

#include <vector>
#include <string>
#include <utility>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>

#if !defined(SAT_DIST)
#include <assimp/Importer.hpp>
#endif

struct aiNode;
struct aiAnimation;
struct aiNodeAnim;
struct aiScene;

namespace Saturn {

	class Submesh
	{
	public:
		uint32_t BaseVertex = 0;
		uint32_t BaseIndex = 0;
		uint32_t MaterialIndex = 0;
		uint32_t IndexCount = 0; // NOT face count, for face count divide by 3 (number of ints)
		uint32_t VertexCount = 0;

		// Mesh-Space
		glm::mat4 Transform{};
		// Mesh-Space
		AABB BoundingBox;

		std::string NodeName, MeshName;
	public:
		bool operator==( const Submesh& other ) const
		{
			return BaseVertex == other.BaseVertex && BaseIndex == other.BaseIndex && MaterialIndex == other.MaterialIndex && IndexCount == other.IndexCount && VertexCount == other.VertexCount && NodeName == other.NodeName && MeshName == other.MeshName;
		}

	public:
		template<typename OStream>
		static void Serialise( const Submesh& rObject, OStream& rStream ) 
		{
			RawSerialisation::WriteObject( rObject.BaseVertex, rStream );
			RawSerialisation::WriteObject( rObject.BaseIndex, rStream );
			RawSerialisation::WriteObject( rObject.MaterialIndex, rStream );
			RawSerialisation::WriteObject( rObject.IndexCount, rStream );
			RawSerialisation::WriteObject( rObject.VertexCount, rStream );

			RawSerialisation::WriteMatrix4x4( rObject.Transform, rStream );
			RawSerialisation::WriteObject( rObject.BoundingBox, rStream );

			RawSerialisation::WriteString( rObject.NodeName, rStream );
			RawSerialisation::WriteString( rObject.MeshName, rStream );
		}

		template<typename IStream>
		static void Deserialise( Submesh& rObject, IStream& rStream )
		{
			RawSerialisation::ReadObject( rObject.BaseVertex, rStream );
			RawSerialisation::ReadObject( rObject.BaseIndex, rStream );
			RawSerialisation::ReadObject( rObject.MaterialIndex, rStream );
			RawSerialisation::ReadObject( rObject.IndexCount, rStream );
			RawSerialisation::ReadObject( rObject.VertexCount, rStream );

			RawSerialisation::ReadMatrix4x4( rObject.Transform, rStream );
			RawSerialisation::ReadObject( rObject.BoundingBox, rStream );

			rObject.NodeName = RawSerialisation::ReadString( rStream );
			rObject.MeshName = RawSerialisation::ReadString( rStream );
		}
	};
	
	struct SkeletalMeshBoneInfo
	{
		// The real index of this bone.
		// Bone Infos may be added to the skeleton in a different order than they should be from the DCC,
		// This may happen because if we have sub-meshes a bone that would come before another bone now may be added after.
		// To solve this BoneIndex provides the correct bone index into the Bone T,R,S and names map.
		// So, this must be used when getting the bone transforms!
		uint64_t BoneIndex = ~0u;

		// The bones' transform in Bone space, i.e. bring the vertices to the bones transform, bind point.
		glm::mat4 InverseBindPose{};

	public:
		template<typename OStream>
		static void Serialise( const SkeletalMeshBoneInfo& rObject, OStream& rStream )
		{
			RawSerialisation::WriteObjectChecked( rObject.BoneIndex, rStream );
			RawSerialisation::WriteMatrix4x4( rObject.InverseBindPose, rStream );
		}

		template<typename IStream>
		static void Deserialise( SkeletalMeshBoneInfo& rObject, IStream& rStream )
		{
			RawSerialisation::ReadObjectChecked( rObject.BoneIndex, rStream );
			RawSerialisation::ReadMatrix4x4( rObject.InverseBindPose, rStream );
		}
	};

	struct SkeletalBoneInfluence
	{
		// Relative to the m_BoneInfos map
		uint32_t BoneIndices[ 4 ] = { 0, 0, 0, 0 };
		float BoneWeights[ 4 ]{ 0.0f, 0.0f, 0.0f, 0.0f };

		inline void AddBoneData( uint32_t id, float weight )
		{
			if( weight < 0.0f || weight > 1.0f )
			{
				SAT_CORE_WARN( "Vertex bone weight is out of range. We will clamp it to [0, 1] (BoneID={0}, Weight={1})", id, weight );
				weight = std::clamp( weight, 0.0f, 1.0f );
			}
			if( weight > 0.0f )
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

				// Note: when importing from assimp we are passing aiProcess_LimitBoneWeights which automatically keeps only the top N (where N defaults to 4)
				//       bone weights (and normalizes the sum to 1), which is exactly what we want.
				//       So, we should never get here.
				SAT_CORE_WARN( "Vertex has more than four bones affecting it, extra bone influences will be discarded (BoneID={0}, Weight={1})", id, weight );
			}
		}

		inline void NormaliseWeights()
		{
			float sumWeights = 0.0f;
			for( size_t i = 0; i < 4; ++i )
			{
				sumWeights += BoneWeights[ i ];
			}
			if( sumWeights > 0.0f )
			{
				for( size_t i = 0; i < 4; ++i )
				{
					BoneWeights[ i ] /= sumWeights;
				}
			}
		}

	public:
		template<typename OStream>
		static void Serialise( const SkeletalBoneInfluence& rObject, OStream& rStream )
		{
			// Indices
			RawSerialisation::WriteObject( rObject.BoneIndices[ 0 ], rStream );
			RawSerialisation::WriteObject( rObject.BoneIndices[ 1 ], rStream );
			RawSerialisation::WriteObject( rObject.BoneIndices[ 2 ], rStream );
			RawSerialisation::WriteObject( rObject.BoneIndices[ 3 ], rStream );

			// Weights
			RawSerialisation::WriteObject( rObject.BoneWeights[ 0 ], rStream );
			RawSerialisation::WriteObject( rObject.BoneWeights[ 1 ], rStream );
			RawSerialisation::WriteObject( rObject.BoneWeights[ 2 ], rStream );
			RawSerialisation::WriteObject( rObject.BoneWeights[ 3 ], rStream );
		}

		template<typename IStream>
		static void Deserialise( SkeletalBoneInfluence& rObject, IStream& rStream )
		{
			// Indices
			RawSerialisation::ReadObject( rObject.BoneIndices[ 0 ], rStream );
			RawSerialisation::ReadObject( rObject.BoneIndices[ 1 ], rStream );
			RawSerialisation::ReadObject( rObject.BoneIndices[ 2 ], rStream );
			RawSerialisation::ReadObject( rObject.BoneIndices[ 3 ], rStream );

			// Weights
			RawSerialisation::ReadObject( rObject.BoneWeights[ 0 ], rStream );
			RawSerialisation::ReadObject( rObject.BoneWeights[ 1 ], rStream );
			RawSerialisation::ReadObject( rObject.BoneWeights[ 2 ], rStream );
			RawSerialisation::ReadObject( rObject.BoneWeights[ 3 ], rStream );
		}
	};

	struct MeshNode
	{
		uint32_t Parent = 0xFFFFFFFF;
		std::vector<uint32_t> Children;
		std::vector<uint32_t> Submeshes;

		std::string Name;
		glm::mat4 LocalTransform;

		inline bool IsRoot() const { return Parent == 0xFFFFFFFF; }
	};
}

namespace std {

	template<>
	struct hash< Saturn::Submesh >
	{
		size_t operator()( const Saturn::Submesh& rOther ) const
		{
			return hash< std::string >()( rOther.NodeName );
		}
	};

}

namespace Saturn {

	// This class acts as a base class to StaticMesh and SkeletalMesh, 
	// The usage of this class is limited because it does not store
	// vertices as static meshes have a different vertex layout to it's Skeletal counterpart.
	// This means that generally you want to avoid using this and instead use StaticMesh/SkeletalMesh, only use this class if you know that may deal with both meshes.
	class Mesh
	{
	public:
		Mesh() = default;
		Mesh( const std::filesystem::path& rFilepath );
		Mesh( const std::vector<StaticVertex>& rVertices, const std::vector<Index>& rIndices, const glm::mat4& rTransform, const glm::mat4& rInverseTransform, uint32_t indicesCount, uint32_t verticesCount );

		virtual ~Mesh();

	public:
		// Import (Internal functions)
		void Import_InitMaterialRegistry();

	public:
		void SetFilepath( const std::filesystem::path& rFilepath ) { m_FilePath = rFilepath; }
		const std::filesystem::path FilePath() const { return m_FilePath; }

		glm::mat4 GetInverseTransform() const { return m_InverseTransform; }
		glm::mat4 GetTransform() const { return m_Transform; }

		std::vector< Ref< MaterialAsset > >& GetMaterialAssets() { return m_MaterialRegistry->GetMaterialAssets(); }
		const std::vector< Ref< MaterialAsset > >& GetMaterialAssets() const { return m_MaterialRegistry->GetMaterialAssets(); }

		std::vector<Submesh>& Submeshes() { return m_Submeshes; }
		const std::vector<Submesh>& Submeshes() const { return m_Submeshes; }

		Ref<VertexBuffer> GetVertexBuffer() { return m_VertexBuffer; }
		Ref<IndexBuffer> GetIndexBuffer() { return m_IndexBuffer; }

		std::vector<Index>& Indices() { return m_Indices; }
		const std::vector<Index>& Indices() const { return m_Indices; }

		void SetAttachedShape( PhysicsShapeType type ) { m_AttachedPhysicsShape = type; }
		const PhysicsShapeType GetAttachedShape() const { return m_AttachedPhysicsShape; }

		void SetPhysicsMaterial( AssetID id ) { m_PhysicsMaterial = id; }
		const AssetID GetPhysicsMaterial() const { return m_PhysicsMaterial; }

		Ref<MaterialRegistry>& GetMaterialRegistry() { return m_MaterialRegistry; }
		const Ref<MaterialRegistry>& GetMaterialRegistry() const { return m_MaterialRegistry; }

		AABB& GetBoundingBox() { return m_BoundingBox; }
		const AABB& GetBoundingBox() const { return m_BoundingBox; }

		// Return the number of indices that make up a single face
		uint32_t GetIndexCount() const { return m_IndicesCount; }

		// Return the number of faces
		size_t GetFaceCount() const { return m_Indices.size(); }

	protected:
		void DeleteSourceModel();

	protected:
		Ref<VertexBuffer> m_VertexBuffer;
		Ref<IndexBuffer> m_IndexBuffer;

		std::vector<StaticVertex> m_Vertices;
		std::vector<Index> m_Indices;
		std::vector<Submesh> m_Submeshes;
		std::vector<MeshNode> m_Nodes;

		std::filesystem::path m_FilePath;

		glm::mat4 m_InverseTransform = {};
		glm::mat4 m_Transform = {};

		uint32_t m_IndicesCount = 0;
		uint32_t m_VertexCount = 0;

		PhysicsShapeType m_AttachedPhysicsShape = PhysicsShapeType::Unknown;
		MemoryAssetDependency<AssetType::PhysicsMaterial> m_PhysicsMaterial;

		// Materials
		Ref<MaterialRegistry> m_MaterialRegistry;

		AABB m_BoundingBox{};

#if !defined(SAT_DIST)
		const aiScene* m_Scene = nullptr;
#endif
	};

	// StaticMeshAsset
	class StaticMesh : public Asset, public Mesh
	{
	public:
		StaticMesh() = default;
		StaticMesh( const Ref<Asset>& rBase, const std::filesystem::path& rFilepath );
		StaticMesh( const std::vector<StaticVertex>& rVertices, const std::vector<Index>& rIndices, const glm::mat4& rTransform );
		
		virtual ~StaticMesh();
		
	public:
		// Asset
		virtual void OnDelete() override;

	public:
		std::vector<StaticVertex>& Vertices() { return m_Vertices; }
		const std::vector<StaticVertex>& Vertices() const { return m_Vertices; }

	public:
		void SerialiseData( std::ofstream& rStream ) const;
		void DeserialiseData( std::istream& rStream );

	private:
#if !defined(SAT_DIST)
		void Initialise();
		
		void TraverseNodes( aiNode* node, const glm::mat4& parentTransform = glm::mat4( 1.0f ), uint32_t level = 0 );
		void CreateVertices();
#endif
	};

	class SkeletonAsset;

	// SkeletalMeshAsset
	class SkeletalMesh : public Asset, public Mesh
	{
	public:
		SkeletalMesh() = default;
		SkeletalMesh( const Ref<Asset>& rBase, const std::filesystem::path& rFilepath, AssetID skeletonID );
		~SkeletalMesh() = default;

		std::vector<Submesh>& Submeshes() { return m_Submeshes; }
		const std::vector<Submesh>& Submeshes() const { return m_Submeshes; }

		Ref<SkeletonAsset> GetSkeletonAsset() const;

		const std::vector<glm::mat4> GetDefaultBoneTransforms();

		Ref<VertexBuffer> GetBoneVertexBuffer() { return m_BoneVertexBuffer; }

#if defined(SAT_DIST)
	public:
		void DistLoadSkeleton( AssetID skeletonID );
#endif

	public:
		// Asset
		virtual void OnDelete() override;

	public:
		void SerialiseData( std::ofstream& rStream ) const;
		void DeserialiseData( std::istream& rStream );

	public:
		void Import_InitSkeleton( AssetID id );

	private:
#if !defined(SAT_DIST)
		void Initialise();

		void TraverseNodes( aiNode* node, uint32_t index, const glm::mat4& parentTransform = glm::mat4( 1.0f ), uint32_t level = 0 );
		void CreateVertices();
#endif

	private:
		std::vector<SkeletalBoneInfluence> m_BoneInfluences;
		
		std::vector<glm::mat4> m_DefaultBoneTransforms;

		Ref<SkeletonAsset> m_SkeletonAsset;
		Ref<VertexBuffer> m_BoneVertexBuffer;

	private:
		friend class SkeletalMeshAssetSerialiser;
	};

	struct MeshInformation
	{
		uint32_t TriangleCount = 0; // not used yet.
		uint32_t IndicesCount = 0;  // not used yet.
		uint32_t VerticesCount = 0; // not used yet.
		uint32_t Submeshes = 0;     // not used yet.

		std::vector<uint64_t> MaterialAssets;
	};

	enum MeshImportBehaviour_ : uint32_t
	{
		// Specify whether to import materials from the mesh asset
		MeshImportBehaviour_CreateNoMaterials     = 1 << 0,

		// Specify whether to not create textures from the mesh asset
		MeshImportBehaviour_ExcludeTextures       = 1 << 1,

		// If this is enabled then materials that have no names in the mesh file will be automatically generated with a given name
		MeshImportBehaviour_AllowUnnamedMaterials = 1 << 2,

		// Import mesh
		MeshImportBehaviour_SK_ImportMesh         = 1 << 3,

		// Skeletal mesh only, used to specify whether to only import an animation or a skeleton without any mesh data.
		MeshImportBehaviour_SK_NoAnimations       = 1 << 4,

		// INTERNAL FLAG! Should we create a skeleton, or try to merge with an existing one?
		MeshImportBehaviour_SK_MergeWithExistingSK = 1 << 5,

		// Default behaviour for Static Meshes
		MeshImportBehaviour_Default = MeshImportBehaviour_AllowUnnamedMaterials | MeshImportBehaviour_CreateNoMaterials,
		// Default behaviour for Skeletal Meshes
		MeshImportBehaviour_SK_Default = MeshImportBehaviour_AllowUnnamedMaterials | MeshImportBehaviour_CreateNoMaterials | MeshImportBehaviour_SK_ImportMesh
	};

	// enum MeshImportBehaviour_
	typedef uint32_t MeshImportBehaviour;

	enum MeshDeterminerResult_ 
	{
		MeshDeterminerResult_Undetermined = 0,
		MeshDeterminerResult_StaticMesh   = 1 << 0,
		MeshDeterminerResult_SkeletalMesh = 1 << 1,
		MeshDeterminerResult_Materials    = 1 << 2,
		MeshDeterminerResult_Animations   = 1 << 3,
	};

	// enum MeshDeterminerResult_
	typedef uint32_t MeshDeterminerResult;

	// Imports a mesh and returns what type it is
	class MeshDeterminer 
	{
	public:
		void ImportAndDetermine( const std::filesystem::path& rPath );

		[[nodiscard]] bool IsReady() const { return m_Ready.load(); }
		MeshDeterminerResult GetResult() const { return m_Result; }
		[[nodiscard]] bool CheckResult( MeshDeterminerResult flag ) const { return ( m_Result & ( uint32_t ) flag ) != 0; }

	private:
		MeshDeterminerResult m_Result = MeshDeterminerResult_Undetermined;

		std::atomic_bool m_Ready{ false };
	};

	//////////////////////////////////////////////////////////////////////////

	class MeshImporterBase
	{
	public:
		MeshImporterBase( const std::filesystem::path& rPath, const std::filesystem::path& rDstPath, MeshImportBehaviour importBehaviour );
		virtual ~MeshImporterBase();

#if !defined(SAT_DIST)
		virtual AssetImportPopupError TryImport() = 0;

	public:
		MeshImportBehaviour GetImportBehaviour() const { return m_ImportBehaviour; }
		const MeshInformation& GetMeshInformation() { return m_MeshInformation; }
		const MeshInformation& GetMeshInformation() const { return m_MeshInformation; }

	protected:
		[[nodiscard]] bool FindMaterials();

	protected:
		std::filesystem::path m_SourcePath;
		std::filesystem::path m_DstPath;
		MeshImportBehaviour m_ImportBehaviour;
		MeshInformation m_MeshInformation;

		std::unique_ptr<Assimp::Importer> m_Importer;
		const aiScene* m_Scene = nullptr;
#else
	protected:
		MeshImporterBase() = default;
#endif
	};

	// A mesh cloner class only exists to get information about a mesh, use the mesh class to render meshes.
	class StaticMeshImporter : public MeshImporterBase
	{
	public:
		StaticMeshImporter( const std::filesystem::path& rPath, const std::filesystem::path& rDstPath, MeshImportBehaviour importBehaviour );
		~StaticMeshImporter();

#if !defined(SAT_DIST)
		virtual AssetImportPopupError TryImport() override;
#endif
	};

	class SkeletalMeshImporter : public MeshImporterBase
	{
	public:
		SkeletalMeshImporter( const std::filesystem::path& rPath, const std::filesystem::path& rDstPath, MeshImportBehaviour importBehaviour, AssetID existingSkeletonID = 0llu );
		~SkeletalMeshImporter();

#if !defined(SAT_DIST)
		virtual AssetImportPopupError TryImport() override;

		[[nodiscard]] AssetID GetCreatedSkeletonID() const { return m_SkeletonID; }

	private:
		void CreateSkeletonIfNeeded();
		void ImportAnimations( Ref<SkeletonAsset> sk );
	private:
		AssetID m_SkeletonID = 0llu;
#endif
	};

}
