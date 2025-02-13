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
#include "Mesh.h"

#include "VulkanContext.h"
#include "Renderer.h"

#include "Saturn/Serialisation/AssetManagerSerialiser.h"
#include "Saturn/Serialisation/AssetSerialisers.h"

#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Asset/Asset.h"
#include "Saturn/Asset/MaterialAsset.h"
#include "Saturn/Asset/AssetImporter.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#if !defined(SAT_DIST)
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/LogStream.hpp>
#endif

#include <filesystem>

namespace Saturn {

#if !defined(SAT_DIST)
	static glm::mat4 Mat4FromAssimpMat4( const aiMatrix4x4& matrix )
	{
		glm::mat4 result;
		result[ 0 ][ 0 ] = matrix.a1; result[ 1 ][ 0 ] = matrix.a2; result[ 2 ][ 0 ] = matrix.a3; result[ 3 ][ 0 ] = matrix.a4;
		result[ 0 ][ 1 ] = matrix.b1; result[ 1 ][ 1 ] = matrix.b2; result[ 2 ][ 1 ] = matrix.b3; result[ 3 ][ 1 ] = matrix.b4;
		result[ 0 ][ 2 ] = matrix.c1; result[ 1 ][ 2 ] = matrix.c2; result[ 2 ][ 2 ] = matrix.c3; result[ 3 ][ 2 ] = matrix.c4;
		result[ 0 ][ 3 ] = matrix.d1; result[ 1 ][ 3 ] = matrix.d2; result[ 2 ][ 3 ] = matrix.d3; result[ 3 ][ 3 ] = matrix.d4;
		return result;
	}

	static constexpr uint32_t s_MeshImportFlags =
		aiProcess_CalcTangentSpace |        // Create binormals/tangents just in case
		aiProcess_Triangulate |             // Make sure we're triangles
		aiProcess_SortByPType |             // Split meshes by primitive type
		aiProcess_GenNormals |              // Make sure we have legit normals
		aiProcess_GenUVCoords |             // Convert UVs if required 
		aiProcess_OptimizeMeshes |          // Batch draws where possible
		aiProcess_JoinIdenticalVertices |
		aiProcess_ValidateDataStructure;    // Validation
	//aiProcess_GlobalScale |             // e.g. convert cm to m for fbx import (and other formats where cm is native)

	struct AssimpLog : public Assimp::LogStream
	{
		static void Initialize()
		{
			if( Assimp::DefaultLogger::isNullLogger() )
			{
				Assimp::DefaultLogger::create( "", Assimp::Logger::VERBOSE );
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

	StaticMesh::StaticMesh( const Ref<Asset>& rBase, const std::filesystem::path& rFilepath )
		: Asset( rBase ), m_FilePath( rFilepath )
	{
#if !defined(SAT_DIST)
		AssimpLog::Initialize();

		if( !std::filesystem::exists( m_FilePath ) ) 
		{
			SAT_CORE_ERROR( "Failed to load mesh file (file does not exists): {0}", m_FilePath );
			return;
		}
		else
			SAT_CORE_INFO( "Loading mesh: {0}", m_FilePath.string().c_str() );

		m_Importer = std::make_unique<Assimp::Importer>();

		const aiScene* scene = m_Importer->ReadFile( m_FilePath.string(), s_MeshImportFlags );
		if( scene == nullptr || !scene->HasMeshes() ) 
		{
			SAT_CORE_ERROR( "Failed to load mesh file (does the file have meshes?): {0}", m_FilePath );
			return;
		}

		m_Scene = scene;
		m_InverseTransform = glm::inverse( Mat4FromAssimpMat4( m_Scene->mRootNode->mTransformation ) );
		m_Transform = Mat4FromAssimpMat4( m_Scene->mRootNode->mTransformation );

		m_MaterialRegistry = Ref<MaterialRegistry>::Create();
		m_MaterialRegistry->SetMesh( this );

		CreateVertices();
#endif
	}

	StaticMesh::~StaticMesh()
	{
		m_VertexBuffer = nullptr;
		m_IndexBuffer = nullptr;

		m_Vertices.clear();

		m_Submeshes.clear();

		m_MaterialRegistry = nullptr;

#if !defined(SAT_DIST)
		m_Importer.release();
#endif
	}

#if !defined(SAT_DIST)
	void StaticMesh::CreateVertices()
	{
		m_Submeshes.reserve( m_Scene->mNumMeshes );

		// Iterate over all meshes in the scene.
		for( unsigned int m = 0; m < m_Scene->mNumMeshes; m++ )
		{
			aiMesh* mesh = m_Scene->mMeshes[ m ];

			Submesh& submesh = m_Submeshes.emplace_back();
			submesh.BaseVertex = m_VertexCount;
			submesh.BaseIndex = m_IndicesCount;
			submesh.MaterialIndex = mesh->mMaterialIndex;
			submesh.VertexCount = mesh->mNumVertices;
			submesh.IndexCount = mesh->mNumFaces * 3;
			submesh.MeshName = mesh->mName.C_Str();

			auto& rAABB = submesh.BoundingBox;
			rAABB.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
			rAABB.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

			m_VertexCount += mesh->mNumVertices;
			m_IndicesCount += submesh.IndexCount;

			SAT_CORE_ASSERT( mesh->HasPositions(), "Meshes require positions." );
			SAT_CORE_ASSERT( mesh->HasNormals(), "Meshes require normals." );

			// Vertices
			m_Vertices.reserve( mesh->mNumVertices );

			for( size_t i = 0; i < mesh->mNumVertices; i++ )
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
			m_Indices.reserve( mesh->mNumFaces );

			for( size_t i = 0; i < mesh->mNumFaces; i++ )
			{
				SAT_CORE_ASSERT( mesh->mFaces[ i ].mNumIndices == 3, "Mesh must have 3 indices." );

				m_Indices.emplace_back( mesh->mFaces[ i ].mIndices[ 0 ], mesh->mFaces[ i ].mIndices[ 1 ], mesh->mFaces[ i ].mIndices[ 2 ] );
			}
		}

		m_VertexBuffer = Ref<VertexBuffer>::Create( m_Vertices.data(), ( uint32_t ) ( m_Vertices.size() * sizeof( StaticVertex ) ) );
		m_IndexBuffer = Ref<IndexBuffer>::Create( m_Indices.data(), m_Indices.size() * sizeof( Index ) );

		TraverseNodes( m_Scene->mRootNode );
	}

	void StaticMesh::TraverseNodes( aiNode* node, const glm::mat4& parentTransform /*= glm::mat4( 1.0f )*/, uint32_t level /*= 0 */ )
	{
		glm::mat4 transform = parentTransform * Mat4FromAssimpMat4( node->mTransformation );

		for( uint32_t i = 0; i < node->mNumMeshes; i++ )
		{
			uint32_t mesh = node->mMeshes[ i ];
			auto& submesh = m_Submeshes[ mesh ];
			submesh.NodeName = node->mName.C_Str();
			submesh.Transform = transform;
		}

		for( uint32_t i = 0; i < node->mNumChildren; i++ )
			TraverseNodes( node->mChildren[ i ], transform, level + 1 );
	}
#endif

	void StaticMesh::Import_InitMaterialRegistry()
	{
		if( m_MaterialRegistry == nullptr )
			m_MaterialRegistry = Ref<MaterialRegistry>::Create();
	}

	void StaticMesh::Import_AddMaterialID( uint64_t index, AssetID assetID )
	{
	}

	//////////////////////////////////////////////////////////////////////////
	// SERIALISATION/DESERIALISATION

	void StaticMesh::SerialiseData( std::ofstream& rStream )
	{
		RawSerialisation::WriteObject( m_VertexCount, rStream );
		RawSerialisation::WriteObject( m_IndicesCount, rStream );

		RawSerialisation::WriteVector( m_Indices, rStream );
		RawSerialisation::WriteVector( m_Vertices, rStream );
		RawSerialisation::WriteVector( m_Submeshes, rStream );

		RawSerialisation::WriteMatrix4x4( m_Transform, rStream );
		RawSerialisation::WriteMatrix4x4( m_InverseTransform, rStream );

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
		// and because we are the master we don't care about overrides
		// So manual read it back.
	
		// Read Materials
		size_t materials = 0;
		RawSerialisation::ReadObject( materials, rStream );

		m_MaterialRegistry->GetMaterialAssets().reserve( materials );

		for( size_t i = 0; i < materials; i++ )
		{
			UUID materialID = 0;
			RawSerialisation::ReadObject( materialID, rStream );

			if( materialID )
				m_MaterialRegistry->AddTargetMaterialAsset( materialID );

			// Try load material
			Ref<MaterialAsset> materialAsset = AssetManager::Get().GetAssetAs<MaterialAsset>( materialID );

			// Failed to load material, create new and default it.
			if( materialAsset == nullptr )
			{
				// Safe to fall back to project defaults because in Dist project defaults must be set in order to package.
				materialAsset = AssetManager::Get().GetAssetAs<MaterialAsset>( Project::GetActiveProject()->GetDefaultMaterialAsset() );
			}

			m_MaterialRegistry->AddAsset( materialAsset );
		}

		m_MaterialRegistry->SetMesh( this );
	}

	//////////////////////////////////////////////////////////////////////////
	// MESH IMPORTER

	MeshImporter::MeshImporter( const std::filesystem::path& rPath, const std::filesystem::path& rDstPath, MeshImportBehaviour importBehaviour )
		: m_SourcePath( rPath ), m_DstPath( rDstPath ), m_ImportBehaviour( importBehaviour )
	{
#if !defined(SAT_DIST)
		AssimpLog::Initialize();

		m_Importer = std::make_unique<Assimp::Importer>();

		const aiScene* scene = m_Importer->ReadFile( rPath.string(), s_MeshImportFlags );

		if( scene == nullptr || !scene->HasMeshes() )
			SAT_CORE_ERROR( "Failed to load mesh file: {0}", rPath.string() );

		m_Scene = scene;

		FindMaterials();
#endif
	}

#if !defined(SAT_DIST)
	void MeshImporter::FindMaterials()
	{
		bool needToSaveAssetReg = false;
		m_MeshInformation.MaterialAssets.resize( m_Scene->mNumMaterials );

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
			if( ( m_ImportBehaviour & MeshImportBehaviour_NoMaterials ) == 0 )
			{
				std::filesystem::path materialPath = m_DstPath;
				materialPath /= MaterialName;
				materialPath.replace_extension( ".smaterial" );

				Ref<Asset> asset = AssetManager::Get().FindAsset( AssetManager::Get().CreateAsset( AssetType::Material ) );
				asset->SetAbsolutePath( materialPath );

				materialAsset = Ref<MaterialAsset>::Create( nullptr );
				materialAsset->SetName( MaterialName );

				needToSaveAssetReg = true;

				m_MeshInformation.MaterialAssets.at( m ) = (uint64_t)asset->ID;

				// Write to disk, create file.
				// TODO: (Asset) Fix this.
				struct
				{
					UUID ID;
					AssetType Type;
					uint32_t Flags;
					std::filesystem::path Path;
					std::string Name;
				} OldAssetData = {};

				OldAssetData.ID = asset->ID;
				OldAssetData.Type = asset->Type;
				OldAssetData.Flags = asset->Flags;
				OldAssetData.Path = asset->Path;
				OldAssetData.Name = asset->Name;

				asset = materialAsset;
				asset->ID = OldAssetData.ID;
				asset->Type = OldAssetData.Type;
				asset->Flags = OldAssetData.Flags;
				asset->Path = OldAssetData.Path;
				asset->Name = OldAssetData.Name;

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
				aiString AlbedoTexturePath;
				bool HasAlbedoTexture = material->GetTexture( aiTextureType_DIFFUSE, 0, &AlbedoTexturePath ) == AI_SUCCESS;

				if( HasAlbedoTexture && ( m_ImportBehaviour & MeshImportBehaviour_ExcludeTextures ) == 0 )
				{
					auto pp = m_SourcePath.parent_path();

					pp /= std::string( AlbedoTexturePath.data );

					auto AlbedoTexturePath = pp.string();
					auto LocalPath = m_DstPath;

					LocalPath /= pp.filename();

					if( !std::filesystem::exists( LocalPath ) )
						std::filesystem::copy_file( AlbedoTexturePath, LocalPath );

					if( materialAsset )
					{
						auto texture = Ref<Texture2D>::Create( LocalPath, AddressingMode::Repeat, false );
						materialAsset->SetAlbeoMap( texture );

						Ref<Asset> asset = AssetManager::Get().FindAsset( AssetManager::Get().CreateAsset( AssetType::Texture ) );
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
						auto texture = Ref<Texture2D>::Create( LocalPath, AddressingMode::Repeat, false );
						materialAsset->SetNormalMap( texture );
						materialAsset->UseNormalMap( true );

						Ref<Asset> asset = AssetManager::Get().FindAsset( AssetManager::Get().CreateAsset( AssetType::Texture ) );
						asset->SetAbsolutePath( LocalPath );
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

					if( !std::filesystem::exists( LocalPath ) )
						std::filesystem::copy_file( RoughnessTexturePath, LocalPath );

					if( materialAsset )
					{
						auto texture = Ref<Texture2D>::Create( LocalPath, AddressingMode::Repeat, false );
						materialAsset->SetRoughnessMap( texture );

						Ref<Asset> asset = AssetManager::Get().FindAsset( AssetManager::Get().CreateAsset( AssetType::Texture ) );
						asset->SetAbsolutePath( LocalPath );
						needToSaveAssetReg = true;
					}
				}
			}

			// Metalness
			{
				bool FoundMetalness = false;

				for( uint32_t i = 0; i < material->mNumProperties; i++ )
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
								auto texture = Ref<Texture2D>::Create( localTexturePath, AddressingMode::Repeat, false );
								materialAsset->SetMetallicMap( texture );

								Ref<Asset> asset = AssetManager::Get().FindAsset( AssetManager::Get().CreateAsset( AssetType::Texture ) );
								asset->SetAbsolutePath( localTexturePath );

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
			AssetManager::Get().Save();
		}
	}
#endif

	MeshImporter::~MeshImporter()
	{
#if !defined(SAT_DIST)
		m_Importer.release();
		delete m_Scene;
		m_Scene = nullptr;
#endif
	}
}