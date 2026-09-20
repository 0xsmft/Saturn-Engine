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
#include "Mesh.h"

#include "Saturn/Serialisation/YAML/AssetSerialisers.h"

#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Asset/Asset.h"
#include "Saturn/Asset/MaterialAsset.h"
#include "Saturn/Asset/TextureSourceAsset.h"

#include "Saturn/Animation/SkeletonAsset.h"
#include "Saturn/Animation/SkeletalAnimationAsset.h"

#include "Saturn/Project/Project.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <glm/gtx/quaternion.hpp>

#if !defined(SAT_DIST)
#include "Saturn/Core/Maths.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/LogStream.hpp>
#endif

#include <filesystem>

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

static constexpr uint32_t s_MeshImportFlags =
aiProcess_CalcTangentSpace |        // Create binormals/tangents just in case
aiProcess_Triangulate |             // Make sure we're triangles
aiProcess_SortByPType |             // Split meshes by primitive type
aiProcess_GenNormals |              // Make sure we have legit normals
aiProcess_GenUVCoords |             // Convert UVs if required 
aiProcess_OptimizeMeshes |          // Batch draws where possible
aiProcess_JoinIdenticalVertices |
aiProcess_LimitBoneWeights |
aiProcess_ValidateDataStructure;    // Validation
//aiProcess_GlobalScale;             // e.g. convert cm to m for fbx import (and other formats where cm is native)

#if defined(SAT_PLATFORM_WINDOWS) && defined(_MSC_VER)
static constexpr uint32_t s_DefaultLogStream = aiDefaultLogStream_DEBUGGER;
#else
static constexpr uint32_t s_DefaultLogStream = aiDefaultLogStream_STDOUT;
#endif

	struct AssimpLog : public Assimp::LogStream
	{
		static void Initialize()
		{
			if( Assimp::DefaultLogger::isNullLogger() )
			{
				Assimp::DefaultLogger::create( "", Assimp::Logger::VERBOSE, s_DefaultLogStream );
				Assimp::DefaultLogger::get()->attachStream( new AssimpLog, Assimp::Logger::Err | Assimp::Logger::Warn );
			}
		}

		virtual void write( const char* message ) override
		{
			SAT_CORE_WARN( "Assimp error: {0}", message );
		}
	};
#endif

	//////////////////////////////////////////////////////////////////////////
	// MESH BASE

	Mesh::Mesh( const std::filesystem::path& rFilepath )
		: m_FilePath( rFilepath )
	{
	}

	Mesh::Mesh( const std::vector<StaticVertex>& rVertices, const std::vector<Index>& rIndices, const glm::mat4& rTransform, const glm::mat4& rInverseTransform, uint32_t indicesCount, uint32_t verticesCount )
		: m_Vertices( rVertices ), m_Indices( rIndices ), m_Transform( rTransform ), m_InverseTransform( rInverseTransform ), m_IndicesCount( indicesCount ), m_VertexCount( verticesCount )
	{
	}

	Mesh::~Mesh()
	{
	}

	void Mesh::Import_InitMaterialRegistry()
	{
		if( !m_MaterialRegistry )
			m_MaterialRegistry = Ref<MaterialRegistry>::Create();
	}

	void Mesh::SetPhysicsMaterial( AssetID id )
	{
		m_PhysicsMaterial = id;
	}

	void Mesh::DeleteSourceModel() const
	{
		if( std::filesystem::exists( m_FilePath ) )
			std::filesystem::remove( m_FilePath );
	}

	//////////////////////////////////////////////////////////////////////////
	// STATIC MESH

	StaticMesh::StaticMesh( const Ref<Asset>& rBase, const std::filesystem::path& rFilepath )
		: Asset( rBase ), Mesh( rFilepath )
	{
#if !defined(SAT_DIST)
		Initialise();
#endif
	}

	StaticMesh::StaticMesh( const std::vector<StaticVertex>& rVertices, const std::vector<Index>& rIndices, const glm::mat4& rTransform )
		: Mesh( rVertices, rIndices, rTransform, glm::inverse( rTransform ), ( uint32_t ) rIndices.size(), ( uint32_t ) rVertices.size() )
	{
		Submesh submesh{};
		submesh.BaseVertex = 0;
		submesh.BaseIndex = 0;
		submesh.MaterialIndex = 0;
		submesh.VertexCount = m_VertexCount;
		submesh.IndexCount = m_IndicesCount * 3u;
		submesh.MeshName = "Default";
		submesh.Transform = m_Transform;
		m_Submeshes.push_back( submesh );

		m_MaterialRegistry = Ref<MaterialRegistry>::Create();

		m_VertexBuffer = Ref<VertexBuffer>::Create( m_Vertices.data(), m_Vertices.size() * sizeof( StaticVertex ) );
		m_IndexBuffer = Ref<IndexBuffer>::Create( m_Indices.data(), m_Indices.size() * sizeof( Index ) );
	}

#if !defined(SAT_DIST)
	void StaticMesh::Initialise()
	{
		AssimpLog::Initialize();

		if( !std::filesystem::exists( m_FilePath ) )
		{
			SAT_CORE_ERROR( "Failed to load mesh file (file does not exists): {0}", m_FilePath.string() );
			return;
		}
		else
			SAT_CORE_INFO( "Loading mesh: {0}", m_FilePath.string() );

		Assimp::Importer importer;

		// Check for global scale flag.
		auto flags = s_MeshImportFlags;
		if( ( m_MeshImportFlags & MeshImportBehaviour_GlobalScale ) )
			flags |= aiProcess_GlobalScale;

		const aiScene* scene = importer.ReadFile( m_FilePath.string(), flags );
		if( scene == nullptr || !scene->HasMeshes() )
		{
			SAT_CORE_ERROR( "Failed to load mesh file (does the file have meshes?): {0}", m_FilePath.string() );
			SAT_CORE_ERROR( "=== Debug Info ===" );
			const std::string hex = std::format( "{:08X}", ( uintptr_t ) scene );
			SAT_CORE_ERROR( " Scene Ptr=0x{0}", hex );
			SAT_CORE_ERROR( "=== [END] ===" );
			return;
		}

		m_Scene = scene;
		m_InverseTransform = glm::inverse( Auxiliary::Mat4FromAssimpMat4( m_Scene->mRootNode->mTransformation ) );
		m_Transform = Auxiliary::Mat4FromAssimpMat4( m_Scene->mRootNode->mTransformation );

		m_MaterialRegistry = Ref<MaterialRegistry>::Create();

		CreateVertices();
	}
#endif

	StaticMesh::~StaticMesh()
	{
		m_VertexBuffer = nullptr;
		m_IndexBuffer = nullptr;

		m_Vertices.clear();

		m_Submeshes.clear();

		m_MaterialRegistry = nullptr;
	}

#if !defined(SAT_DIST)
	void StaticMesh::OnDelete()
	{
		DeleteSourceModel();
	}

	void StaticMesh::OnAssetDependencyReplace( AssetID oldID, AssetID newID )
	{
		// Possible Assets that can be changed:
		// Material Assets
		// Physics Material Assets.

		Ref<Asset> oldAsset = AssetManager::Get()->FindAsset( oldID );
		switch( oldAsset->Type )
		{
			case AssetType::Material:
			{
				uint32_t index = 0u;
				for( auto& rMaterialAsset : m_MaterialRegistry->GetMaterialAssets() )
				{
					if( rMaterialAsset->ID == oldID )
					{
						m_MaterialRegistry->SetMaterialNoOvr( index, newID );
					}

					++index;
				}
			} break;

			case AssetType::PhysicsMaterial:
			{
				m_PhysicsMaterial = newID;

				// Register new dependency.
				if( m_PhysicsMaterial )
					AssetManager::Get()->RegisterAssetDependency( ID, newID );
			} break;

			default:
				break;
		}

		AssetManager::Get()->Save();
	}

	void StaticMesh::CreateVertices()
	{
		m_Submeshes.reserve( m_Scene->mNumMeshes );

		// Iterate over all meshes in the scene.
		for( unsigned int m = 0; m < m_Scene->mNumMeshes; ++m )
		{
			aiMesh* mesh = m_Scene->mMeshes[ m ];

			Submesh& submesh = m_Submeshes.emplace_back();
			submesh.BaseVertex = m_VertexCount;
			submesh.BaseIndex = m_IndicesCount;
			submesh.MaterialIndex = mesh->mMaterialIndex;
			submesh.VertexCount = mesh->mNumVertices;
			// Multiply by three because we don't care about the faces we want the number actual indices
			submesh.IndexCount = mesh->mNumFaces * 3;
			submesh.MeshName = mesh->mName.C_Str();

			auto& rAABB = submesh.BoundingBox;
			rAABB.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
			rAABB.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

			m_VertexCount += mesh->mNumVertices;
			// Add to the entire mesh index count
			m_IndicesCount += submesh.IndexCount;

			SAT_CORE_ASSERT( mesh->HasPositions(), "Meshes require positions." );
			SAT_CORE_ASSERT( mesh->HasNormals(), "Meshes require normals." );

			// Vertices
			m_Vertices.reserve( mesh->mNumVertices );

			for( size_t i = 0; i < mesh->mNumVertices; ++i )
			{
				StaticVertex vertex{};
				vertex.Position = { mesh->mVertices[ i ].x, mesh->mVertices[ i ].y, mesh->mVertices[ i ].z };
				vertex.Normal = { mesh->mNormals[ i ].x, mesh->mNormals[ i ].y, mesh->mNormals[ i ].z };

				rAABB.Min.x = glm::min( vertex.Position.x, rAABB.Min.x );
				rAABB.Min.y = glm::min( vertex.Position.y, rAABB.Min.y );
				rAABB.Min.z = glm::min( vertex.Position.z, rAABB.Min.z );

				rAABB.Max.x = glm::max( vertex.Position.x, rAABB.Max.x );
				rAABB.Max.y = glm::max( vertex.Position.y, rAABB.Max.y );
				rAABB.Max.z = glm::max( vertex.Position.z, rAABB.Max.z );

				if( mesh->HasTangentsAndBitangents() )
				{
					vertex.Tangent = { mesh->mTangents[ i ].x, mesh->mTangents[ i ].y, mesh->mTangents[ i ].z };
					vertex.Binormal = { mesh->mBitangents[ i ].x, mesh->mBitangents[ i ].y, mesh->mBitangents[ i ].z };
				}

				if( mesh->HasTextureCoords( 0 ) )
					vertex.Texcoord = { mesh->mTextureCoords[ 0 ][ i ].x, mesh->mTextureCoords[ 0 ][ i ].y };

				m_Vertices.push_back( vertex );
			}

			// Indices
			// Reserve for number of faces in the current submesh
			// We don't need to multiply by three because we are storing faces
			m_Indices.reserve( mesh->mNumFaces );

			for( size_t i = 0; i < mesh->mNumFaces; ++i )
			{
				SAT_CORE_ASSERT( mesh->mFaces[ i ].mNumIndices == 3, "Mesh must have 3 indices." );

				m_Indices.emplace_back( mesh->mFaces[ i ].mIndices[ 0 ], mesh->mFaces[ i ].mIndices[ 1 ], mesh->mFaces[ i ].mIndices[ 2 ] );
			}
		}

		m_VertexBuffer = Ref<VertexBuffer>::Create( m_Vertices.data(), ( uint32_t ) ( m_Vertices.size() * sizeof( StaticVertex ) ) );
		m_IndexBuffer = Ref<IndexBuffer>::Create( m_Indices.data(), m_Indices.size() * sizeof( Index ) );

		TraverseNodes( m_Scene->mRootNode );

		for( const auto& rSubmesh : m_Submeshes )
		{
			const AABB bb = rSubmesh.BoundingBox;
			const glm::vec3 min = glm::vec3( rSubmesh.Transform * glm::vec4( bb.Min, 1.0f ) );
			const glm::vec3 max = glm::vec3( rSubmesh.Transform * glm::vec4( bb.Max, 1.0f ) );

			m_BoundingBox.Min.x = glm::min( m_BoundingBox.Min.x, min.x );
			m_BoundingBox.Min.y = glm::min( m_BoundingBox.Min.y, min.y );
			m_BoundingBox.Min.z = glm::min( m_BoundingBox.Min.z, min.z );

			m_BoundingBox.Max.x = glm::max( m_BoundingBox.Max.x, max.x );
			m_BoundingBox.Max.y = glm::max( m_BoundingBox.Max.y, max.y );
			m_BoundingBox.Max.z = glm::max( m_BoundingBox.Max.z, max.z );
		}
	}

	void StaticMesh::TraverseNodes( aiNode* node, const glm::mat4& parentTransform /*= glm::mat4( 1.0f )*/, uint32_t level /*= 0 */ )
	{
		const glm::mat4 transform = parentTransform * Auxiliary::Mat4FromAssimpMat4( node->mTransformation );

		for( uint32_t i = 0; i < node->mNumMeshes; ++i )
		{
			uint32_t mesh = node->mMeshes[ i ];
			auto& submesh = m_Submeshes[ mesh ];
			submesh.NodeName = node->mName.C_Str();
			submesh.Transform = transform;
		}

		for( uint32_t i = 0; i < node->mNumChildren; ++i )
			TraverseNodes( node->mChildren[ i ], transform, level + 1 );
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// SERIALISATION/DESERIALISATION

	void StaticMesh::SerialiseData( std::ofstream& rStream ) const
	{
		RawSerialisation::WriteObject( m_VertexCount, rStream );
		RawSerialisation::WriteObject( m_IndicesCount, rStream );

		RawSerialisation::WriteVector( m_Indices, rStream );
		RawSerialisation::WriteVector( m_Vertices, rStream );
		RawSerialisation::WriteVector( m_Submeshes, rStream );

		RawSerialisation::WriteMatrix4x4( m_Transform, rStream );
		RawSerialisation::WriteMatrix4x4( m_InverseTransform, rStream );

		// Master material registry
		// Write asset material IDs
		// Matches with StaticMeshAssetSerialiser
		size_t materials = m_MaterialRegistry->GetMaterialAssets().size();
		rStream.write( reinterpret_cast< char* >( &materials ), sizeof( size_t ) );

		for( const auto& rMaterialAsset : m_MaterialRegistry->GetMaterialAssets() )
		{
			RawSerialisation::WriteObject( rMaterialAsset->ID, rStream );
		}
	}

	void StaticMesh::DeserialiseData( std::istream& rStream )
	{
		RawSerialisation::ReadObject( m_VertexCount, rStream );
		RawSerialisation::ReadObject( m_IndicesCount, rStream );

		RawSerialisation::ReadVector( m_Indices, rStream );
		RawSerialisation::ReadVector( m_Vertices, rStream );
		RawSerialisation::ReadVector( m_Submeshes, rStream );

		RawSerialisation::ReadMatrix4x4( m_Transform, rStream );
		RawSerialisation::ReadMatrix4x4( m_InverseTransform, rStream );

		m_VertexBuffer = Ref<VertexBuffer>::Create( m_Vertices.data(), ( uint32_t ) ( m_Vertices.size() * sizeof( StaticVertex ) ) );
		m_IndexBuffer = Ref<IndexBuffer>::Create( m_Indices.data(), m_Indices.size() * sizeof( Index ) );

		m_MaterialRegistry = Ref<MaterialRegistry>::Create();

		//////////////////////////////////////////////////////////////////////////
		// Read Master
		// Unable to call MaterialRegistry::Deserialise as Deserialise expects a map with the overrides
		// and because we are the master we don't care about overrides and thus don't have a map with overrides
		// So, manually read it back.
	
		// Read Materials
		size_t materials = 0;
		RawSerialisation::ReadObject( materials, rStream );

		m_MaterialRegistry->GetMaterialAssets().reserve( materials );

		for( size_t i = 0; i < materials; ++i )
		{
			UUID materialID = 0;
			RawSerialisation::ReadObject( materialID, rStream );

			// Try load material
			Ref<MaterialAsset> materialAsset = AssetManager::Get()->GetAssetAs<MaterialAsset>( materialID );

			// Failed to load material, create new and default it.
			if( materialAsset == nullptr )
			{
				// Safe to fall back to project defaults because in Dist project defaults must be set in order to package.
				materialAsset = AssetManager::Get()->GetAssetAs<MaterialAsset>( Project::GetActiveProject()->GetDefaultMaterialAsset() );
			}

			m_MaterialRegistry->AddAsset( materialAsset );
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// SKELETAL MESH

	SkeletalMesh::SkeletalMesh( const Ref<Asset>& rBase, const std::filesystem::path& rFilepath, AssetID skeletonID )
		: Asset( rBase ), Mesh( rFilepath )
	{
#if !defined(SAT_DIST)
		m_SkeletonAsset = AssetManager::Get()->GetAssetAs<SkeletonAsset>( skeletonID );
		SAT_CORE_ASSERT( m_SkeletonAsset );

		Initialise();
#endif
	}


	SkeletalMesh::~SkeletalMesh()
	{
		
	}

	Ref<SkeletonAsset> SkeletalMesh::GetSkeletonAsset() const
	{
		return m_SkeletonAsset;
	}

	const std::vector<glm::mat4>& SkeletalMesh::GetDefaultBoneTransforms()
	{
		return m_DefaultBoneTransforms;
	}

	const std::vector<glm::mat4>& SkeletalMesh::GetDefaultBoneTransforms() const
	{
		return m_DefaultBoneTransforms;
	}

	void SkeletalMesh::Import_InitSkeleton( AssetID id )
	{
		// TODO: Not the best way, a bit screwy
		if( !m_SkeletonAsset )
		{
			m_SkeletonAsset = Ref<SkeletonAsset>::Create( AssetManager::Get()->FindAsset( id ) );
		}
	}

#if !defined(SAT_DIST)
	void SkeletalMesh::Initialise()
	{
		AssimpLog::Initialize();

		if( !std::filesystem::exists( m_FilePath ) )
		{
			SAT_CORE_ERROR( "Failed to load mesh file (file does not exists): {0}", m_FilePath.string() );
			return;
		}
		else
			SAT_CORE_INFO( "Loading mesh: {0}", m_FilePath.string().c_str() );

		Assimp::Importer importer;
		importer.SetPropertyBool( AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false );

		// Check for global scale flag.
		auto flags = s_MeshImportFlags;
		if( ( m_MeshImportFlags & MeshImportBehaviour_GlobalScale ) )
			flags |= aiProcess_GlobalScale;

		const aiScene* scene = importer.ReadFile( m_FilePath.string(), flags );
		if( scene == nullptr || !scene->HasMeshes() )
		{
			SAT_CORE_ERROR( "Failed to load mesh file (does the file have meshes?): {0}", m_FilePath.string() );
			SAT_CORE_ERROR( "=== Debug Info ===" );
			const std::string hex = std::format( "{:08X}", ( uintptr_t ) scene );
			SAT_CORE_ERROR( " Scene Ptr=0x{0}", hex );
			SAT_CORE_ERROR( "=== [END] ===" );
			return;
		}

		m_Scene = scene;

		const glm::mat4 transform = Auxiliary::Mat4FromAssimpMat4( m_Scene->mRootNode->mTransformation );
		m_Transform = transform;
		m_InverseTransform = glm::inverse( m_Transform );

		m_MaterialRegistry = Ref<MaterialRegistry>::Create();

		CreateVertices();
	}

	void SkeletalMesh::CreateVertices()
	{
		m_Submeshes.reserve( m_Scene->mNumMeshes );

		// Iterate over all meshes in the scene.
		for( unsigned int m = 0; m < m_Scene->mNumMeshes; ++m )
		{
			const aiMesh* mesh = m_Scene->mMeshes[ m ];

			Submesh& submesh = m_Submeshes.emplace_back();
			submesh.BaseVertex = m_VertexCount;
			submesh.BaseIndex = m_IndicesCount;
			submesh.MaterialIndex = mesh->mMaterialIndex;
			submesh.VertexCount = mesh->mNumVertices;
			// Multiply by three because we don't care about the faces we want the number actual indices
			submesh.IndexCount = mesh->mNumFaces * 3;
			submesh.MeshName = mesh->mName.C_Str();

			auto& rAABB = submesh.BoundingBox;
			rAABB.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
			rAABB.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

			m_VertexCount += mesh->mNumVertices;
			// Add to the entire mesh index count
			m_IndicesCount += submesh.IndexCount;

			SAT_CORE_ASSERT( mesh->HasPositions(), "Meshes require positions." );
			SAT_CORE_ASSERT( mesh->HasNormals(), "Meshes require normals." );
			
			// Vertices
			m_Vertices.reserve( mesh->mNumVertices );
			m_BoneInfluences.resize( m_VertexCount );

			for( unsigned int i = 0; i < mesh->mNumVertices; ++i )
			{
				StaticVertex vertex{};
				vertex.Position = { mesh->mVertices[ i ].x, mesh->mVertices[ i ].y, mesh->mVertices[ i ].z };
				vertex.Normal = { mesh->mNormals[ i ].x, mesh->mNormals[ i ].y, mesh->mNormals[ i ].z };

				rAABB.Min.x = glm::min( vertex.Position.x, rAABB.Min.x );
				rAABB.Min.y = glm::min( vertex.Position.y, rAABB.Min.y );
				rAABB.Min.z = glm::min( vertex.Position.z, rAABB.Min.z );

				rAABB.Max.x = glm::max( vertex.Position.x, rAABB.Max.x );
				rAABB.Max.y = glm::max( vertex.Position.y, rAABB.Max.y );
				rAABB.Max.z = glm::max( vertex.Position.z, rAABB.Max.z );

				if( mesh->HasTangentsAndBitangents() )
				{
					vertex.Tangent = { mesh->mTangents[ i ].x, mesh->mTangents[ i ].y, mesh->mTangents[ i ].z };
					vertex.Binormal = { mesh->mBitangents[ i ].x, mesh->mBitangents[ i ].y, mesh->mBitangents[ i ].z };
				}

				if( mesh->HasTextureCoords( 0 ) )
					vertex.Texcoord = { mesh->mTextureCoords[ 0 ][ i ].x, mesh->mTextureCoords[ 0 ][ i ].y };

				m_Vertices.push_back( vertex );
			}

			// Indices
			// Reserve for number of faces in the current submesh
			// We don't need to multiply by three because we are storing faces
			m_Indices.reserve( mesh->mNumFaces );

			for( unsigned int i = 0; i < mesh->mNumFaces; ++i )
			{
				SAT_CORE_ASSERT( mesh->mFaces[ i ].mNumIndices == 3, "Mesh must have 3 indices." );

				m_Indices.emplace_back( mesh->mFaces[ i ].mIndices[ 0 ], mesh->mFaces[ i ].mIndices[ 1 ], mesh->mFaces[ i ].mIndices[ 2 ] );
			}

			for( unsigned int b = 0; b < mesh->mNumBones; ++b )
			{
				const aiBone* pBone = mesh->mBones[ b ];
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

				const uint32_t boneSkelIndex = m_SkeletonAsset->FindBoneIndex( boneName );

				uint32_t boneIndex = ~0;
				for( size_t i = 0; i < m_SkeletonAsset->m_BoneInfos.size(); ++i )
				{
					if( m_SkeletonAsset->m_BoneInfos[ i ].BoneIndex == boneSkelIndex )
					{
						boneIndex = ( uint32_t ) i;
						break;
					}
				}

				if( boneIndex == ~0 )
				{
					boneIndex = ( uint32_t ) m_SkeletonAsset->m_BoneInfos.size();

					SkeletalMeshBoneInfo bi{ .BoneIndex = boneSkelIndex, .InverseBindPose = Auxiliary::Mat4FromAssimpMat4( pBone->mOffsetMatrix ) };
					m_SkeletonAsset->m_BoneInfos.push_back( bi );

					SAT_CORE_INFO( "BoneInfo for bone '{0}'", pBone->mName.C_Str() );
					SAT_CORE_INFO( "  SubMeshIndex = {0}", m );
					SAT_CORE_INFO( "  BoneIndex = {0}", boneSkelIndex );

					glm::vec3 translation;
					glm::quat rotationQuat;
					glm::vec3 scale;
					Maths::DecomposeTransform( bi.InverseBindPose, translation, rotationQuat, scale );
					glm::vec3 rotation = glm::degrees( glm::eulerAngles( rotationQuat ) );
					SAT_CORE_INFO( "  Inverse Bind Pose = {" );
					SAT_CORE_INFO( "    translation: ({0:8.4f}, {1:8.4f}, {2:8.4f})", translation.x, translation.y, translation.z );
					SAT_CORE_INFO( "    rotation:    ({0:8.4f}, {1:8.4f}, {2:8.4f})", rotation.x, rotation.y, rotation.z );
					SAT_CORE_INFO( "    scale:       ({0:8.4f}, {1:8.4f}, {2:8.4f})", scale.x, scale.y, scale.z );
					SAT_CORE_INFO( "  }" );
				}

				for( unsigned int w = 0; w < pBone->mNumWeights; ++w )
				{
					const int vertID = submesh.BaseVertex + pBone->mWeights[ w ].mVertexId;
					const float weight = pBone->mWeights[ w ].mWeight;

					m_BoneInfluences[ vertID ].AddBoneData( boneIndex, weight );
				}
			}
		}

		for( auto& rInfluences : m_BoneInfluences )
		{
			rInfluences.NormaliseWeights();
		}

		m_VertexBuffer = Ref<VertexBuffer>::Create( m_Vertices.data(), ( uint32_t ) ( m_Vertices.size() * sizeof( StaticVertex ) ) );
		m_BoneVertexBuffer = Ref<VertexBuffer>::Create( m_BoneInfluences.data(), m_BoneInfluences.size() * sizeof( SkeletalBoneInfluence ) );
		m_IndexBuffer = Ref<IndexBuffer>::Create( m_Indices.data(), m_Indices.size() * sizeof( Index ) );

		const auto bones = m_SkeletonAsset->GetBoneNames().size();
		m_DefaultBoneTransforms.resize( bones );

		MeshNode& rRootNode = m_Nodes.emplace_back();
		TraverseNodes( m_Scene->mRootNode, 0 );

		for( const auto& rSubmesh : m_Submeshes )
		{
			const AABB bb = rSubmesh.BoundingBox;
			const glm::vec3 min = glm::vec3( rSubmesh.Transform * glm::vec4( bb.Min, 1.0f ) );
			const glm::vec3 max = glm::vec3( rSubmesh.Transform * glm::vec4( bb.Max, 1.0f ) );

			m_BoundingBox.Min.x = glm::min( m_BoundingBox.Min.x, min.x );
			m_BoundingBox.Min.y = glm::min( m_BoundingBox.Min.y, min.y );
			m_BoundingBox.Min.z = glm::min( m_BoundingBox.Min.z, min.z );

			m_BoundingBox.Max.x = glm::max( m_BoundingBox.Max.x, max.x );
			m_BoundingBox.Max.y = glm::max( m_BoundingBox.Max.y, max.y );
			m_BoundingBox.Max.z = glm::max( m_BoundingBox.Max.z, max.z );
		}

		for( size_t i = 0; i < bones; ++i )
		{
			const glm::mat4 local = glm::translate( glm::mat4( 1.0f ), m_SkeletonAsset->GetBonePositions().at( i ) )
				* glm::toMat4( m_SkeletonAsset->GetBoneRotations().at( i ) )
				* glm::scale( glm::mat4( 1.0f ), m_SkeletonAsset->GetBoneScales().at( i ) );

			const uint32_t parent = ( uint32_t ) m_SkeletonAsset->GetParentIndex( ( uint32_t ) i );
			m_DefaultBoneTransforms[ i ] = ( parent == ~0u ) ? local : m_DefaultBoneTransforms[ parent ] * local;
		}
	}

	void SkeletalMesh::TraverseNodes( aiNode* node, uint32_t index, const glm::mat4& parentTransform /*= glm::mat4( 1.0f )*/, uint32_t level /*= 0 */ )
	{
		MeshNode& rNode = m_Nodes[ index ];
		rNode.Name = node->mName.C_Str();
		rNode.LocalTransform = Auxiliary::Mat4FromAssimpMat4( node->mTransformation );

		const glm::mat4 transform = parentTransform * rNode.LocalTransform;
		for( uint32_t i = 0; i < node->mNumMeshes; ++i )
		{
			uint32_t mesh = node->mMeshes[ i ];
			auto& submesh = m_Submeshes[ mesh ];
			submesh.NodeName = node->mName.C_Str();
			submesh.Transform = transform;

			rNode.Submeshes.push_back( mesh );
		}

		uint32_t parentIndex = ( uint32_t ) ( m_Nodes.size() - 1 );
		rNode.Children.resize( node->mNumChildren );
		for( uint32_t i = 0; i < node->mNumChildren; ++i ) 
		{
			MeshNode& rChild = m_Nodes.emplace_back();
			uint32_t chiIndex = ( uint32_t ) ( m_Nodes.size() - 1 );
			rChild.Parent = parentIndex;
			m_Nodes[ index ].Children[ i ] = chiIndex;

			TraverseNodes( node->mChildren[ i ], chiIndex, transform, level + 1 );
		}
	}

#endif

#if defined(SAT_DIST)
	void SkeletalMesh::DistLoadSkeleton( AssetID skeletonID )
	{
		m_SkeletonAsset = AssetManager::Get()->GetAssetAs<SkeletonAsset>( skeletonID );
		SAT_CORE_VERIFY( m_SkeletonAsset, "Unable to load skeleton for this Skeletal Mesh!" );
	}
#endif

#if !defined(SAT_DIST)
	void SkeletalMesh::OnDelete()
	{
		DeleteSourceModel();
	}

	void SkeletalMesh::OnAssetDependencyReplace( AssetID oldID, AssetID newID )
	{
		// Possible Assets that can be changed:
		// Material Assets
		// Skeleton Assets
		// Physics Material Assets

		Ref<Asset> oldAsset = AssetManager::Get()->FindAsset( oldID );
		switch( oldAsset->Type )
		{
			case AssetType::Material:
			{
				uint32_t index = 0u;
				for( auto& rMaterialAsset : m_MaterialRegistry->GetMaterialAssets() )
				{
					if( rMaterialAsset->ID == oldID )
					{
						m_MaterialRegistry->SetMaterialNoOvr( index, newID );
					}

					++index;
				}
			} break;

			case AssetType::PhysicsMaterial:
			{
				m_PhysicsMaterial = newID;

				// Register new dependency.
				if( m_PhysicsMaterial )
					AssetManager::Get()->RegisterAssetDependency( ID, newID );
			} break;

			case AssetType::Skeleton:
			{
				SAT_CORE_ASSERT( false, "SkeletalMesh::OnAssetDependencyReplace - AssetType::Skeleton - not implmented yet." );
			} break;

			default:
				break;
		}

		AssetManager::Get()->Save();
	}
#endif

	void SkeletalMesh::SerialiseData( std::ofstream& rStream ) const
	{
		RawSerialisation::WriteObject( m_VertexCount, rStream );
		RawSerialisation::WriteObject( m_IndicesCount, rStream );

		RawSerialisation::WriteVector( m_Indices, rStream );
		RawSerialisation::WriteVector( m_Vertices, rStream );
		RawSerialisation::WriteVector( m_Submeshes, rStream );
		RawSerialisation::WriteVector( m_BoneInfluences, rStream );
		RawSerialisation::WriteVector( m_DefaultBoneTransforms, rStream );
		
		RawSerialisation::WriteMatrix4x4( m_Transform, rStream );
		RawSerialisation::WriteMatrix4x4( m_InverseTransform, rStream );

		// Master material registry
		// Write asset material IDs
		// Matches with StaticMeshAssetSerialiser
		const size_t materials = m_MaterialRegistry->GetMaterialAssets().size();
		rStream.write( reinterpret_cast< const char* >( &materials ), sizeof( size_t ) );

		for( const auto& rMaterialAsset : m_MaterialRegistry->GetMaterialAssets() )
		{
			RawSerialisation::WriteObject( rMaterialAsset->ID, rStream );
		}
	}

	void SkeletalMesh::DeserialiseData( std::istream& rStream )
	{
		RawSerialisation::ReadObject( m_VertexCount, rStream );
		RawSerialisation::ReadObject( m_IndicesCount, rStream );

		RawSerialisation::ReadVector( m_Indices, rStream );
		RawSerialisation::ReadVector( m_Vertices, rStream );
		RawSerialisation::ReadVector( m_Submeshes, rStream );

		RawSerialisation::ReadVector( m_BoneInfluences, rStream );
		RawSerialisation::ReadVector( m_DefaultBoneTransforms, rStream );
		
		RawSerialisation::ReadMatrix4x4( m_Transform, rStream );
		RawSerialisation::ReadMatrix4x4( m_InverseTransform, rStream );

		m_VertexBuffer     = Ref<VertexBuffer>::Create( m_Vertices.data(), ( uint32_t ) ( m_Vertices.size() * sizeof( StaticVertex ) ) );
		m_BoneVertexBuffer = Ref<VertexBuffer>::Create( m_BoneInfluences.data(), ( uint32_t ) ( m_BoneInfluences.size() * sizeof( SkeletalBoneInfluence ) ) );
		m_IndexBuffer      = Ref<IndexBuffer>::Create( m_Indices.data(), m_Indices.size() * sizeof( Index ) );

		m_MaterialRegistry = Ref<MaterialRegistry>::Create();

		//////////////////////////////////////////////////////////////////////////
		// Read Master
		// Unable to call MaterialRegistry::Deserialise as Deserialise expects a map with the overrides
		// and because we are the master we don't care about overrides and thus don't have a map with overrides
		// So, manually read it back.

		// Read Materials
		size_t materials = 0;
		RawSerialisation::ReadObject( materials, rStream );

		m_MaterialRegistry->GetMaterialAssets().reserve( materials );

		for( size_t i = 0; i < materials; ++i )
		{
			UUID materialID = 0;
			RawSerialisation::ReadObject( materialID, rStream );

			// Try load material
			Ref<MaterialAsset> materialAsset = AssetManager::Get()->GetAssetAs<MaterialAsset>( materialID );

			// Failed to load material, create new and default it.
			if( materialAsset == nullptr )
			{
				// Safe to fall back to project defaults because in Dist project defaults must be set in order to package.
				materialAsset = AssetManager::Get()->GetAssetAs<MaterialAsset>( Project::GetActiveProject()->GetDefaultMaterialAsset() );
			}

			m_MaterialRegistry->AddAsset( materialAsset );
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// MESH DETERMINER

	void MeshDeterminer::ImportAndDetermine( const std::filesystem::path& rPath )
	{
#if !defined(SAT_DIST)
		AssimpLog::Initialize();

		constexpr auto IMPORT_FLAGS = s_MeshImportFlags;

		auto importer = std::make_unique<Assimp::Importer>();
		importer->SetPropertyBool( AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false );

		const aiScene* scene = importer->ReadFile( rPath.string(), IMPORT_FLAGS );

		if( scene == nullptr )
		{
			SAT_CORE_ERROR( "Failed to load mesh file: {0}", rPath.string() );
			return;
		}

		if( scene->HasAnimations() )
			m_Result = MeshDeterminerResult_Animations;

		if( scene->HasMaterials() )
			m_Result |= MeshDeterminerResult_Materials;

		for( unsigned int m = 0; m < scene->mNumMeshes; m++ )
		{
			const aiMesh* pMesh = scene->mMeshes[ m ];

			if( pMesh->HasBones() )
			{
				m_Result |= MeshDeterminerResult_SkeletalMesh;

				// A Skeletal mesh can not contain any other meshes
//				break;
			}
			else
			{
				if( ( m_Result & MeshDeterminerResult_SkeletalMesh ) != 0 )
				{
//					m_Result = MeshDeterminerResult_Undetermined;
					SAT_CORE_ERROR( "A Skeletal mesh can not contain a static mesh, it can however, contain submeshes with the SAME armature!" );
				}
				else
				{
					m_Result |= MeshDeterminerResult_StaticMesh;
				}
			}
		}

		m_Ready.store( true );
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// MESH IMPORTER BASE
	
	MeshImporterBase::MeshImporterBase( const std::filesystem::path& rPath, const std::filesystem::path& rDstPath, MeshImportBehaviour importBehaviour )
#if !defined(SAT_DIST)
		: m_SourcePath( rPath ), m_DstPath( rDstPath ), m_ImportBehaviour( importBehaviour )
#endif
	{
	}

	MeshImporterBase::~MeshImporterBase()
	{
#if !defined(SAT_DIST)
		m_Importer.reset();
#endif
	}

#if !defined(SAT_DIST)
	bool MeshImporterBase::FindMaterials()
	{
		bool needToSaveAssetReg = false;
		m_MeshInformation.MaterialAssets.resize( m_Scene->mNumMaterials );

		if( m_Scene->mNumMaterials == 0 )
		{
			SAT_CORE_ERROR( "[MeshImporterBase] No materials exist in the Mesh. A mesh cannot have no materials but it can have no MaterialAssets. In your DCC tool ensure that you have created at least one material slot in the scene!" );
			return false;
		}

		// We need to get the material count, but we no longer need to search, we are done here.
		if( !m_NeedToFindMaterials )
			return true;

		for( size_t m = 0; m < m_Scene->mNumMaterials; m++ )
		{
			aiMaterial* material = m_Scene->mMaterials[ m ];

			aiString name;
			material->Get( AI_MATKEY_NAME, name );

			std::string MaterialName = std::string( name.C_Str() );

			if( MaterialName.empty() )
			{
				MaterialName = "Unnamed Material " + std::to_string( UUID() );

				if( ( m_ImportBehaviour & MeshImportBehaviour_AllowUnnamedMaterials ) == 0 )
				{
					SAT_CORE_ERROR( "Unnamed Material at INDEX/{0} was found. If you want to import materials with no name please use the \"AllowUnnamedMaterials\" flag!" );

					continue;
				}
			}

			Ref<MaterialAsset> materialAsset = nullptr;

			// Create a material asset if user wants to.
			if( ( m_ImportBehaviour & MeshImportBehaviour_CreateNoMaterials ) == 0 )
			{
				std::filesystem::path materialPath = m_DstPath;
				materialPath /= MaterialName;
				materialPath.replace_extension( ".smaterial" );

				Ref<Asset> asset = AssetManager::Get()->FindAsset( AssetManager::Get()->CreateAsset( AssetType::Material ) );
				asset->SetAbsolutePath( materialPath );

				materialAsset = Ref<MaterialAsset>::Create( asset, nullptr );
				materialAsset->SetName( MaterialName );

				needToSaveAssetReg = true;

				m_MeshInformation.MaterialAssets.at( m ) = ( uint64_t ) asset->ID;

				//////////////////////////////////////////////////////////////////////////

				// Set the material data (only for new materials).
				// Albedo Color
				aiColor3D color;
				if( material->Get( AI_MATKEY_COLOR_DIFFUSE, color ) == AI_SUCCESS )
					materialAsset->SetAlbeoColor( glm::vec3( color.r, color.g, color.b ) );

				float shininess, metalness;
				if( material->Get( AI_MATKEY_SHININESS, shininess ) != aiReturn_SUCCESS )
					shininess = 80.0f;

				if( material->Get( AI_MATKEY_REFLECTIVITY, metalness ) != aiReturn_SUCCESS )
					metalness = 0.0f;

				float roughness = 1.0f - glm::sqrt( shininess / 100.0f );

				materialAsset->SetRoughness( roughness );
				materialAsset->SetMetalness( metalness );
			}

			// Albedo Texture
			{
				aiString assimpAlbedoTexturePath;
				const bool HasAlbedoTexture = material->GetTexture( aiTextureType_DIFFUSE, 0, &assimpAlbedoTexturePath ) == AI_SUCCESS;

				if( HasAlbedoTexture && ( m_ImportBehaviour & MeshImportBehaviour_ExcludeTextures ) == 0 )
				{
					auto pp = m_SourcePath.parent_path();
					pp /= std::string( assimpAlbedoTexturePath.data );

					auto AlbedoTexturePath = pp.string();
					auto LocalPath = m_DstPath;

					LocalPath /= pp.filename();

					bool fileCopied = false;
					if( !std::filesystem::exists( LocalPath ) && std::filesystem::exists( AlbedoTexturePath ) )
						fileCopied = std::filesystem::copy_file( AlbedoTexturePath, LocalPath );

					if( materialAsset && fileCopied )
					{
						Ref<Asset> asset = AssetManager::Get()->FindAsset( AssetManager::Get()->CreateAsset( AssetType::Texture ) );

						auto stxPath = LocalPath;
						stxPath.replace_extension( ".stx" );
						
						asset->SetAbsolutePath( stxPath );

						Ref<TextureSourceAsset> srcAsset = Ref<TextureSourceAsset>::Create( asset, LocalPath );

						TextureSourceAssetSerialiser tsas;
						tsas.Serialise( srcAsset );

						auto texture = srcAsset->GetTexture();
						materialAsset->SetAlbeoMap( texture );
						asset->SetAbsolutePath( LocalPath );

						needToSaveAssetReg = true;
					}
				}
			}

			// Normal Texture
			{
				aiString TexturePath;
				bool HasTexture = material->GetTexture( aiTextureType_NORMALS, 0, &TexturePath ) == AI_SUCCESS;

				if( HasTexture && ( m_ImportBehaviour & MeshImportBehaviour_ExcludeTextures ) == 0 )
				{
					auto pp = m_SourcePath.parent_path();

					pp /= std::string( TexturePath.data );

					auto NormalTexturePath = pp.string();

					auto LocalPath = m_DstPath;

					LocalPath /= pp.filename();

					if( !std::filesystem::exists( LocalPath ) )
						std::filesystem::copy_file( NormalTexturePath, LocalPath );

					if( materialAsset )
					{
						Ref<Asset> asset = AssetManager::Get()->FindAsset( AssetManager::Get()->CreateAsset( AssetType::Texture ) );

						auto stxPath = LocalPath;
						stxPath.replace_extension( ".stx" );

						asset->SetAbsolutePath( stxPath );

						Ref<TextureSourceAsset> srcAsset = Ref<TextureSourceAsset>::Create( asset, LocalPath );

						TextureSourceAssetSerialiser tsas;
						tsas.Serialise( srcAsset );

						auto texture = srcAsset->GetTexture();

						materialAsset->SetNormalMap( texture );
						materialAsset->UseNormalMap( true );

						needToSaveAssetReg = true;
					}
				}
			}

			// Roughness Texture
			{
				aiString TexturePath;
				bool HasTexture = material->GetTexture( aiTextureType_SHININESS, 0, &TexturePath ) == AI_SUCCESS;

				if( HasTexture && ( m_ImportBehaviour & MeshImportBehaviour_ExcludeTextures ) == 0 )
				{
					auto pp = m_SourcePath.parent_path();

					pp /= std::string( TexturePath.data );

					auto RoughnessTexturePath = pp.string();

					auto LocalPath = m_DstPath;
					LocalPath /= pp.filename();

					if( !std::filesystem::exists( LocalPath ) && std::filesystem::exists( RoughnessTexturePath ) )
						std::filesystem::copy_file( RoughnessTexturePath, LocalPath );

					if( materialAsset && std::filesystem::exists( LocalPath ) )
					{
						Ref<Asset> asset = AssetManager::Get()->FindAsset( AssetManager::Get()->CreateAsset( AssetType::Texture ) );

						auto stxPath = LocalPath;
						stxPath.replace_extension( ".stx" );

						asset->SetAbsolutePath( stxPath );

						Ref<TextureSourceAsset> srcAsset = Ref<TextureSourceAsset>::Create( asset, LocalPath );

						TextureSourceAssetSerialiser tsas;
						tsas.Serialise( srcAsset );

						auto texture = srcAsset->GetTexture();
						materialAsset->SetRoughnessMap( texture );

						needToSaveAssetReg = true;
					}
				}
			}

			// Metalness
			{
				bool FoundMetalness = false;

				for( uint32_t i = 0; i < material->mNumProperties; ++i )
				{
					auto prop = material->mProperties[ i ];

					if( prop->mType == aiPTI_String )
					{
						uint32_t StringLen = *( uint32_t* ) prop->mData;
						std::string String( prop->mData + 4, StringLen );

						std::string Key = prop->mKey.data;
						if( Key == "$raw.ReflectionFactor|file" && ( m_ImportBehaviour & MeshImportBehaviour_ExcludeTextures ) == 0 )
						{
							auto pp = m_SourcePath.parent_path();

							pp /= String;

							auto TexturePath = pp.string();

							Ref< Texture2D > MetalnessTexture;

							auto localTexturePath = m_DstPath;

							localTexturePath /= pp.filename();

							if( !std::filesystem::exists( localTexturePath ) )
								std::filesystem::copy_file( TexturePath, localTexturePath );

							if( materialAsset )
							{
								Ref<Asset> asset = AssetManager::Get()->FindAsset( AssetManager::Get()->CreateAsset( AssetType::Texture ) );

								auto stxPath = localTexturePath;
								stxPath.replace_extension( ".stx" );

								asset->SetAbsolutePath( stxPath );

								Ref<TextureSourceAsset> srcAsset = Ref<TextureSourceAsset>::Create( asset, localTexturePath );

								TextureSourceAssetSerialiser tsas;
								tsas.Serialise( srcAsset );

								auto texture = srcAsset->GetTexture();
								materialAsset->SetMetallicMap( texture );

								needToSaveAssetReg = true;
							}

							break;
						}
					}
				}
			}

			if( materialAsset )
			{
				MaterialAssetSerialiser mas;
				mas.Serialise( materialAsset );
			}
		}

		if( needToSaveAssetReg )
		{
			AssetManager::Get()->Save();
		}

		return true;
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// MESH IMPORTER

	StaticMeshImporter::StaticMeshImporter( const std::filesystem::path& rPath, const std::filesystem::path& rDstPath, MeshImportBehaviour importBehaviour )
#if !defined(SAT_DIST)
		: MeshImporterBase( rPath, rDstPath, importBehaviour )
#endif
	{
	}

	StaticMeshImporter::~StaticMeshImporter()
	{
	}

#if !defined(SAT_DIST)
	AssetImportPopupError StaticMeshImporter::TryImport()
	{
		AssimpLog::Initialize();

		m_Importer = std::make_unique<Assimp::Importer>();
		m_Importer->SetPropertyBool( AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false );

		const aiScene* scene = m_Importer->ReadFile( m_SourcePath.string(), s_MeshImportFlags );

		if( scene == nullptr || !scene->HasMeshes() )
		{
			SAT_CORE_ERROR( "Failed to load mesh file: {0}", m_SourcePath.string() );
			return AssetImportPopupError::MeshAssimpInternalError;
		}

		m_Scene = scene;
		
		if( m_Scene->HasAnimations() ) SAT_CORE_WARN( "[StaticMeshImporter] Scene has animations, they will be ignored!" );

		if( ( m_ImportBehaviour & MeshImportBehaviour_ImportSubMeshAsAsset ) != 0 )
		{
			ImportSubmeshesSeperate();
		}
		
		return FindMaterials() ? AssetImportPopupError::None : AssetImportPopupError::MeshNoMaterials;
	}

	void StaticMeshImporter::ImportSubmeshesSeperate()
	{
	}

#endif

	//////////////////////////////////////////////////////////////////////////
	// SKELETAL MESH IMPORTER

	SkeletalMeshImporter::SkeletalMeshImporter( const std::filesystem::path& rPath, const std::filesystem::path& rDstPath, MeshImportBehaviour importBehaviour, AssetID existingSkeletonID )
#if !defined(SAT_DIST)
		: MeshImporterBase( rPath, rDstPath, importBehaviour ), m_SkeletonID( existingSkeletonID )
#endif
	{
	}

	SkeletalMeshImporter::~SkeletalMeshImporter()
	{
	}

#if !defined(SAT_DIST)
	AssetImportPopupError SkeletalMeshImporter::TryImport()
	{
		AssimpLog::Initialize();

		m_Importer = std::make_unique<Assimp::Importer>();
		m_Importer->SetPropertyBool( AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false );

		const aiScene* scene = m_Importer->ReadFile( m_SourcePath.string(), s_MeshImportFlags );

		if( scene == nullptr )
		{
			SAT_CORE_ERROR( "Failed to load mesh file: {0}", m_SourcePath.string() );
			return AssetImportPopupError::MeshAssimpInternalError;
		}

		m_Scene = scene;

		bool anyMaterialsFound = FindMaterials();
		CreateSkeletonIfNeeded();

		return anyMaterialsFound ? AssetImportPopupError::None : AssetImportPopupError::MeshNoMaterials;
	}

	void SkeletalMeshImporter::CreateSkeletonIfNeeded()
	{
		Ref<SkeletonAsset> skelAsset = nullptr;
		if( ( m_ImportBehaviour & MeshImportBehaviour_SK_MergeWithExistingSK ) == 0 )
		{
			auto asset = AssetManager::Get()->FindAsset( AssetManager::Get()->CreateAsset( AssetType::Skeleton ) );
			asset->Name = m_SourcePath.stem().string();

			auto path = m_DstPath / asset->Name;
			path.replace_extension( ".skel" );
			asset->SetAbsolutePath( path );

			skelAsset = Ref<SkeletonAsset>::Create( asset );
			m_SkeletonID = skelAsset->ID;

			SkeletonBoneHierarchy sbh( m_Scene, skelAsset.Get(), false );
			sbh.Build();

			SkeletonAssetSerialiser sas;
			skelAsset->PortToNewestVersion();
			sas.Serialise( skelAsset );
		}
		else
		{
			skelAsset = AssetManager::Get()->GetAssetAs<SkeletonAsset>( m_SkeletonID );
			SAT_CORE_ASSERT( skelAsset );
		}

		// Import animations
		ImportAnimations( skelAsset );

		for( unsigned int m = 0; m < m_Scene->mNumMeshes; m++ )
		{
			const aiMesh* pMesh = m_Scene->mMeshes[ m ];
			if( !pMesh->HasBones() ) continue;

			skelAsset->AppendBonesFromMesh( pMesh );
		}
	}

	void SkeletalMeshImporter::ImportAnimations( Ref<SkeletonAsset> sk )
	{
		SkeletonBoneHierarchy sbh( m_Scene, sk.Get(), true );
		sbh.Build();

		SkeletonAssetSerialiser sas;
		sas.Serialise( sk );

		for( unsigned int i = 0; i < m_Scene->mNumAnimations; ++i )
		{
			aiAnimation* pAnimation = m_Scene->mAnimations[ i ];

			std::string name( pAnimation->mName.C_Str() );

			// Some artists/websites (mixamo) will add | which is an illegal file name on Windows.
			std::replace( name.begin(), name.end(), '|', '-' );

			auto asset = AssetManager::Get()->FindAsset( AssetManager::Get()->CreateAsset( AssetType::SkeletalAnimation ) );
			asset->Name = name;

			auto path = m_DstPath / asset->Name;
			path.replace_extension( ".skanim" );
			asset->SetAbsolutePath( path );

			Ref<SkeletalAnimationAsset> animAsset = Ref<SkeletalAnimationAsset>::Create( asset );

			animAsset->SetSkeletonID( m_SkeletonID );
			animAsset->SetDuration( ( float ) pAnimation->mDuration );
			animAsset->SetTicks( ( float ) ( pAnimation->mTicksPerSecond == 0 ? 25.0f : pAnimation->mTicksPerSecond ) );

			std::unordered_map<std::string_view, uint32_t> boneIndices;
			for( uint32_t i = 0; i < sk->GetBonePositions().size(); ++i )
			{
				boneIndices.emplace( sk->GetBoneName( i ), i );
			}

			std::map<uint32_t, aiNodeAnim*> validChannels;
			for( uint32_t i = 0; i < pAnimation->mNumChannels; ++i )
			{
				aiNodeAnim* pAnim = pAnimation->mChannels[ i ];
				if( const auto itr = boneIndices.find( pAnim->mNodeName.C_Str() ); itr != boneIndices.end() )
				{
					validChannels.emplace( itr->second, pAnim );
				}
			}

			double firstFrameDelta = DBL_MAX;
			for( uint32_t boneIndex = 0; boneIndex < pAnimation->mNumChannels; ++boneIndex )
			{
				if( auto validChannel = validChannels.find( boneIndex ); validChannel != validChannels.end() )
				{
					auto nodeAnim = validChannel->second;
					if( nodeAnim->mNumPositionKeys > 0 )
						firstFrameDelta = std::min( firstFrameDelta, nodeAnim->mPositionKeys[ 0 ].mTime );

					if( nodeAnim->mNumRotationKeys > 0 )
						firstFrameDelta = std::min( firstFrameDelta, nodeAnim->mRotationKeys[ 0 ].mTime );

					if( nodeAnim->mNumScalingKeys > 0 )
						firstFrameDelta = std::min( firstFrameDelta, nodeAnim->mScalingKeys[ 0 ].mTime );
				}
			}

			for( unsigned int c = 0; c < pAnimation->mNumChannels; ++c )
			{
				AnimationChannel animBone;
				animBone.Index = c;
				
				if( auto validChannel = validChannels.find( c ); validChannel != validChannels.end() )
				{
					aiNodeAnim* pAnimNode = validChannel->second;
					animBone.Positions.reserve( pAnimNode->mNumPositionKeys );
					animBone.Rotations.reserve( pAnimNode->mNumRotationKeys );
					animBone.Scale.reserve( pAnimNode->mNumScalingKeys );

					for( unsigned int p = 0; p < pAnimNode->mNumPositionKeys; ++p )
					{
						const aiVectorKey key = pAnimNode->mPositionKeys[ p ];
						const float time = std::clamp( static_cast<float>( ( key.mTime - firstFrameDelta ) / pAnimation->mDuration ), 0.0f, 1.0f );

						if( ( p == 0 ) && ( time > 0.0f ) )
						{
							animBone.Positions.emplace_back( glm::vec3( ( float ) key.mValue.x, ( float ) key.mValue.y, ( float ) key.mValue.z ), 0.0f );
						}

						animBone.Positions.emplace_back( glm::vec3( key.mValue.x, key.mValue.y, key.mValue.z ), time );
					}
					
					if( animBone.Positions.empty() )
					{
						animBone.Positions.emplace_back( glm::vec3( 0.0f ), 0.0f );
						animBone.Positions.emplace_back( glm::vec3( 0.0f ), 1.0f );
					}
					else if( animBone.Positions.back().Timestamp < 1.0f )
					{
						animBone.Positions.emplace_back( animBone.Positions.back().Value, 1.0f );
					}

					for( unsigned int r = 0; r < pAnimNode->mNumRotationKeys; ++r )
					{
						const aiQuatKey key = pAnimNode->mRotationKeys[ r ];
						const float time = std::clamp( static_cast< float >( ( key.mTime - firstFrameDelta ) / pAnimation->mDuration ), 0.0f, 1.0f );

						if( ( r == 0 ) && ( time > 0.0f ) )
						{
							animBone.Rotations.emplace_back( glm::vec3( ( float ) key.mValue.x, ( float ) key.mValue.y, ( float ) key.mValue.z ), 0.0f );
						}

						animBone.Rotations.emplace_back( glm::quat( key.mValue.w, key.mValue.x, key.mValue.y, key.mValue.z ), time );
					}

					if( animBone.Rotations.empty() )
					{
						animBone.Rotations.emplace_back( glm::quat( 1.0f, 0.0f, 0.0f, 0.0f ), 0.0f );
						animBone.Rotations.emplace_back( glm::quat( 1.0f, 0.0f, 0.0f, 0.0f ), 1.0f );
					}
					else if( animBone.Rotations.back().Timestamp < 1.0f )
					{
						animBone.Rotations.emplace_back( animBone.Rotations.back().Value, 1.0f );
					}

					for( unsigned int s = 0; s < pAnimNode->mNumScalingKeys; ++s )
					{
						const aiVectorKey key = pAnimNode->mScalingKeys[ s ];
						const float time = std::clamp( static_cast< float >( ( key.mTime - firstFrameDelta ) / pAnimation->mDuration ), 0.0f, 1.0f );

						if( ( s == 0 ) && ( time > 0.0f ) )
						{
							animBone.Scale.emplace_back( glm::vec3( ( float ) key.mValue.x, ( float ) key.mValue.y, ( float ) key.mValue.z ), 0.0f );
						}

						animBone.Scale.emplace_back( glm::vec3( key.mValue.x, key.mValue.y, key.mValue.z ), time );
					}

					if( animBone.Scale.empty() )
					{
						animBone.Scale.emplace_back( glm::vec3( 1.0f ), 0.0f );
						animBone.Scale.emplace_back( glm::vec3( 1.0f ), 1.0f );
					}
					else if( animBone.Scale.back().Timestamp < 1.0f )
					{
						animBone.Scale.emplace_back( animBone.Scale.back().Value, 1.0f );
					}
				}
				else
				{
					const auto& p = sk->GetBonePositions().at( c );
					const auto& q = sk->GetBoneRotations().at( c );
					const auto& s = sk->GetBoneScales().at( c );

					animBone.Positions.emplace_back( p, 0.0f );
					animBone.Rotations.emplace_back( q, 0.0f );
					animBone.Scale.emplace_back( s, 0.0f );
				}

				animAsset->AddAnimBone( animBone );
			}

			animAsset->SetBoneCount( animAsset->GetAnimationBones().size() );

			animAsset->MakeUniformAndCompress( pAnimation );

			SkeletalAnimationAssetSerialiser saa;
			saa.Serialise( animAsset );
		}
	}
#endif

}
