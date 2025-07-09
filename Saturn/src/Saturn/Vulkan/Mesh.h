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

#include "Saturn/Core/Base.h"
#include "Saturn/Core/AABB/AABB.h"
#include "Saturn/Core/Renderer/EditorCamera.h"
#include "Shader.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Material.h"

#include "Saturn/Asset/MemoryAssetDependency.h"
#include "Saturn/Asset/MaterialAsset.h"

#include "Saturn/Physics/PhysicsShapeTypes.h"

#include "Saturn/Serialisation/RawSerialisation.h"

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
		uint32_t BaseVertex;
		uint32_t BaseIndex;
		uint32_t MaterialIndex;
		uint32_t IndexCount; // NOT face count, for face count divide by 3 (number of ints)
		uint32_t VertexCount;

		// Mesh-Space
		glm::mat4 Transform;
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

	// StaticMeshAsset
	class StaticMesh : public Asset
	{
	public:
		StaticMesh() = default;
		StaticMesh( const Ref<Asset>& rBase, const std::filesystem::path& rFilepath );
		StaticMesh( const std::vector<StaticVertex>& rVertices, const std::vector<Index>& rIndices, const glm::mat4& rTransform );
		
		virtual ~StaticMesh();
		
		void SetFilepath( const std::filesystem::path& rFilepath ) { m_FilePath = rFilepath; }
		
		std::filesystem::path FilePath() { return m_FilePath; }
		const std::filesystem::path FilePath() const { return m_FilePath; }

		glm::mat4 GetInverseTransform() const { return m_InverseTransform; }
		glm::mat4 GetTransform() const { return m_Transform; }

		std::vector< Ref< MaterialAsset > >& GetMaterialAssets() { return m_MaterialRegistry->GetMaterialAssets(); }
		const std::vector< Ref< MaterialAsset > >& GetMaterialAssets() const { return m_MaterialRegistry->GetMaterialAssets(); }

		std::vector<Submesh>& Submeshes() { return m_Submeshes; }
		const std::vector<Submesh>& Submeshes() const { return m_Submeshes; }

		Ref<VertexBuffer> GetVertexBuffer() { return m_VertexBuffer; }
		Ref<IndexBuffer> GetIndexBuffer() { return m_IndexBuffer; }

		std::vector<StaticVertex>& Vertices() { return m_Vertices; }
		const std::vector<StaticVertex>& Vertices() const { return m_Vertices; }

		std::vector<Index>& Indices() { return m_Indices; }
		const std::vector<Index>& Indices() const { return m_Indices; }

		void SetAttachedShape( ShapeType type ) { m_AttachedPhysicsShape = type; }
		const ShapeType GetAttachedShape() const { return m_AttachedPhysicsShape; }

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

	public:
		// Import (Internal functions)
		void Import_InitMaterialRegistry();
		void Import_AddMaterialID( uint64_t index, AssetID assetID );

	public:
		void SerialiseData( std::ofstream& rStream );
		void DeserialiseData( std::istream& rStream );

	private:
#if !defined(SAT_DIST)
		void Initialise();
		
		void TraverseNodes( aiNode* node, const glm::mat4& parentTransform = glm::mat4( 1.0f ), uint32_t level = 0 );
		void CreateVertices();
#endif
	private:
		Ref<VertexBuffer> m_VertexBuffer;
		Ref<IndexBuffer> m_IndexBuffer;

		std::vector<StaticVertex> m_Vertices;
		std::vector<Index> m_Indices;
		std::vector<Submesh> m_Submeshes;

		std::filesystem::path m_FilePath;

		glm::mat4 m_InverseTransform = {};
		glm::mat4 m_Transform = {};

		uint32_t m_IndicesCount = 0;
		uint32_t m_VertexCount = 0;

		ShapeType m_AttachedPhysicsShape = ShapeType::Unknown;
		MemoryAssetDependency<AssetType::PhysicsMaterial> m_PhysicsMaterial;

		// Materials
		Ref<MaterialRegistry> m_MaterialRegistry;

		AABB m_BoundingBox{};

#if !defined(SAT_DIST)
		std::unique_ptr<Assimp::Importer> m_Importer;
		const aiScene* m_Scene = nullptr;
#endif
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
		MeshImportBehaviour_NoMaterials = 1,
		MeshImportBehaviour_ExcludeTextures = 2,
		MeshImportBehaviour_AllowUnnamedMaterials = 4,
		MeshImportBehaviour_Default = MeshImportBehaviour_AllowUnnamedMaterials | MeshImportBehaviour_NoMaterials,
	};

	// enum MeshImportBehaviour_
	typedef uint32_t MeshImportBehaviour;

	// A mesh cloner class only exists to get information about a mesh, use the mesh class to render meshes.
	class MeshImporter : public RefTarget
	{
	public:
		MeshImporter( const std::filesystem::path& rPath, const std::filesystem::path& rDstPath, MeshImportBehaviour importBehaviour );
		~MeshImporter();

#if !defined(SAT_DIST)
		const MeshInformation& GetMeshInformation()       { return m_MeshInformation; }
		const MeshInformation& GetMeshInformation() const { return m_MeshInformation; }

		MeshImportBehaviour GetImportBehaviour() const { return m_ImportBehaviour; }

	private:
		void FindMaterials();

	private:
		std::unique_ptr<Assimp::Importer> m_Importer;
		const aiScene* m_Scene;
#endif
	private:
		std::filesystem::path m_SourcePath;
		std::filesystem::path m_DstPath;
		MeshImportBehaviour m_ImportBehaviour;

		MeshInformation m_MeshInformation;
	};
}