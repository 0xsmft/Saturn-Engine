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
#include "DescriptorSet.h"

#include "Saturn/Serialisation/AssetRegistrySerialiser.h"
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
	glm::mat4 Mat4FromAssimpMat4( const aiMatrix4x4& matrix )
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

	StaticMesh::StaticMesh( const std::string& rFilepath )
		: m_FilePath( rFilepath )
	{
#if !defined(SAT_DIST)
		AssimpLog::Initialize();

		if( !std::filesystem::exists( m_FilePath ) )
			SAT_CORE_ERROR( "Failed to load mesh file (file does not exists): {0}", m_FilePath );
		else
			SAT_CORE_INFO( "Loading mesh: {0}", m_FilePath.c_str() );

		m_Importer = std::make_unique<Assimp::Importer>();

		const aiScene* scene = m_Importer->ReadFile( m_FilePath, s_MeshImportFlags );
		if( scene == nullptr || !scene->HasMeshes() )
			SAT_CORE_ERROR( "Failed to load mesh file (does the file have meshes?): {0}", m_FilePath );

		m_Scene = scene;
		// Shader new is the static mesh pbr shader.
		m_MeshShader = ShaderLibrary::Get().Find( "shader_new" );
		m_BaseMaterial = Ref< Material >::Create( m_MeshShader, "Base Material" );

		m_InverseTransform = glm::inverse( Mat4FromAssimpMat4( m_Scene->mRootNode->mTransformation ) );
		m_Transform = Mat4FromAssimpMat4( m_Scene->mRootNode->mTransformation );

		m_MaterialRegistry = Ref<MaterialRegistry>::Create();

		CreateVertices();
		CreateMaterials();
#endif
	}

	StaticMesh::~StaticMesh()
	{
		m_VertexBuffer = nullptr;
		m_IndexBuffer = nullptr;

		m_Vertices.clear();

		m_Submeshes.clear();

		m_MeshShader = nullptr;
		m_MaterialRegistry = nullptr;
		m_BaseMaterial = nullptr;

		m_MaterialsAssets.clear();
	}

#if !defined(SAT_DIST)
	void StaticMesh::CreateVertices()
	{
		m_Submeshes.reserve( m_Scene->mNumMeshes );

		// Iterate over all meshes in the scene.
		for( unsigned m = 0; m < m_Scene->mNumMeshes; m++ )
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
				StaticVertex vertex;
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

	void StaticMesh::CreateMaterials()
	{
		m_MaterialsAssets.resize( m_Scene->mNumMaterials );

		for( size_t m = 0; m < m_Scene->mNumMaterials; m++ )
		{
			aiMaterial* material = m_Scene->mMaterials[ m ];

			aiString name;
			material->Get( AI_MATKEY_NAME, name );

			std::string MaterialName = std::string( name.C_Str() );

			if( MaterialName.empty() ) 
			{
				MaterialName = std::format( "Unnamed Material {0}", std::to_string( UUID() ) );
			}

			Ref<Texture2D> PinkTexture = Renderer::Get().GetPinkTexture();

			// This is more of a hack, as we use parent_path just so we can add the material on to it.
			auto assetPath = std::filesystem::path( m_FilePath ).parent_path();
			assetPath /= MaterialName;
			assetPath += ".smaterial";

			auto realPath = std::filesystem::relative( assetPath, Project::GetActiveProject()->GetRootDir() );

			Ref<Asset> asset = AssetManager::Get().FindAsset( realPath );
			Ref<MaterialAsset> materialAsset;

			if( !asset ) 
			{
				// Create the new asset
				asset = AssetManager::Get().FindAsset( AssetManager::Get().CreateAsset( AssetType::Material ) );
				asset->SetAbsolutePath( assetPath );

				// Does not exists, mesh source did not copy it?
				if( !std::filesystem::exists( assetPath ) )
				{
					materialAsset = Ref<MaterialAsset>::Create( nullptr );
				}
				else
				{
					materialAsset = AssetManager::Get().GetAssetAs<MaterialAsset>( asset->GetAssetID() );
				}

				// Material is still null but exists, likely a new material.
				if( materialAsset == nullptr ) 
				{
					materialAsset = Ref<MaterialAsset>::Create( nullptr );
				}

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

				// Save the material in it's most default state.
				// So when we write the actual information in the asset will already be loaded.
				// Because at this point this asset only exists in memory.
				MaterialAssetSerialiser mas;
				mas.Serialise( materialAsset );

				materialAsset->SetName( MaterialName );

				m_MaterialsAssets[ m ] = materialAsset;
			}
			else
			{
				// Asset was already loaded.
				materialAsset = AssetManager::Get().GetAssetAs<MaterialAsset>( asset->GetAssetID() );
				m_MaterialsAssets[ m ] = materialAsset;

				m_MaterialRegistry->AddAsset( materialAsset );

				continue;
			}

			m_MaterialRegistry->AddAsset( materialAsset );

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

			// Albedo Texture
			{
				aiString AlbedoTexturePath;
				bool HasAlbedoTexture = material->GetTexture( aiTextureType_DIFFUSE, 0, &AlbedoTexturePath ) == AI_SUCCESS;

				if( HasAlbedoTexture )
				{
					std::filesystem::path AlbedoPath = m_FilePath;
					auto pp = AlbedoPath.parent_path();

					pp /= std::string( AlbedoTexturePath.data );

					auto AlbedoTexturePath = pp.string();

					Ref< Texture2D > AlbedoTexture;

					SAT_CORE_INFO( " Albedo Map texture {0}", AlbedoTexturePath );

					auto localTexturePath = assetPath.parent_path();

					localTexturePath /= pp.filename();

					if( !std::filesystem::exists( localTexturePath ) )
						std::filesystem::copy_file( AlbedoTexturePath, localTexturePath );

					if( std::filesystem::exists( localTexturePath ) )
						AlbedoTexture = Ref< Texture2D >::Create( localTexturePath, AddressingMode::Repeat, false );

					if( AlbedoTexture )
					{
						materialAsset->SetAlbeoMap( AlbedoTexture );
					}
					else
					{
						materialAsset->SetAlbeoMap( PinkTexture );
					}
				}
				else
				{
					materialAsset->SetAlbeoMap( PinkTexture );
				}
			}

			// Normal Texture
			{
				aiString NormalTexturePath;
				bool HasNormalTexture = material->GetTexture( aiTextureType_NORMALS, 0, &NormalTexturePath ) == AI_SUCCESS;

				if( HasNormalTexture )
				{
					std::filesystem::path Path = m_FilePath;
					auto pp = Path.parent_path();

					pp /= std::string( NormalTexturePath.data );

					auto NormalTexturePath = pp.string();

					Ref< Texture2D > NormalTexture;

					SAT_CORE_INFO( " Normal Map texture {0}", NormalTexturePath );

					auto localTexturePath = assetPath.parent_path();

					localTexturePath /= pp.filename();

					if( !std::filesystem::exists( localTexturePath ) )
						std::filesystem::copy_file( NormalTexturePath, localTexturePath );

					if( std::filesystem::exists( localTexturePath ) )
						NormalTexture = Ref< Texture2D >::Create( localTexturePath, AddressingMode::Repeat, false );

					if( NormalTexture )
					{
						materialAsset->SetNormalMap( NormalTexture );
						materialAsset->UseNormalMap( 1.0f );
					}
					else
					{
						materialAsset->SetNormalMap( PinkTexture );
						materialAsset->UseNormalMap( 0.0f );
					}
				}
				else
				{
					materialAsset->SetNormalMap( PinkTexture );
					materialAsset->UseNormalMap( 0.0f );
				}
			}

			// Roughness texture
			{
				aiString RoughnessTexturePath;
				bool HasRoughnessTexture = material->GetTexture( aiTextureType_SHININESS, 0, &RoughnessTexturePath ) == AI_SUCCESS;

				materialAsset->Set( "u_Materials.Roughness", roughness );

				materialAsset->SetRoughness( roughness );

				if( HasRoughnessTexture )
				{
					std::filesystem::path Path = m_FilePath;
					auto pp = Path.parent_path();

					pp /= std::string( RoughnessTexturePath.data );

					auto TexturePath = pp.string();

					Ref< Texture2D > RoughnessTexture;

					auto localTexturePath = assetPath.parent_path();

					localTexturePath /= pp.filename();

					if( !std::filesystem::exists( localTexturePath ) )
						std::filesystem::copy_file( TexturePath, localTexturePath );

					if( std::filesystem::exists( localTexturePath ) )
						RoughnessTexture = Ref< Texture2D >::Create( localTexturePath, AddressingMode::Repeat, false );

					if( RoughnessTexture )
					{
						materialAsset->SetRoughnessMap( RoughnessTexture );
					}
					else
					{
						materialAsset->SetRoughnessMap( PinkTexture );
					}
				}
				else
				{
					materialAsset->SetRoughnessMap( PinkTexture );
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
						if( Key == "$raw.ReflectionFactor|file" )
						{
							std::filesystem::path Path = m_FilePath;
							auto pp = Path.parent_path();

							pp /= String;

							auto TexturePath = pp.string();

							Ref< Texture2D > MetalnessTexture;

							auto localTexturePath = assetPath.parent_path();

							localTexturePath /= pp.filename();

							if( !std::filesystem::exists( localTexturePath ) )
								std::filesystem::copy_file( TexturePath, localTexturePath );

							if( std::filesystem::exists( localTexturePath ) )
								MetalnessTexture = Ref< Texture2D >::Create( localTexturePath, AddressingMode::Repeat, false );

							if( MetalnessTexture )
							{
								FoundMetalness = true;

								materialAsset->SetMetallicMap( MetalnessTexture );
							}
							else
							{
								materialAsset->SetMetallicMap( PinkTexture );
							}

							materialAsset->SetMetalness( 1.0f );

							break;
						}
					}
				}

				if( !FoundMetalness )
				{
					materialAsset->SetMetallicMap( PinkTexture );
					materialAsset->SetMetalness( metalness );
				}

				MaterialAssetSerialiser mas;
				mas.Serialise( materialAsset );
			}
		}

		// Serialise the asset registry to save any new materials.
		AssetManager::Get().Save();
	}
#endif

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

		// Write materials
		size_t materials = m_MaterialsAssets.size();
		rStream.write( reinterpret_cast< char* >( &materials ), sizeof( size_t ) );

		for( const auto& rMaterialAsset : m_MaterialsAssets )
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

		m_MeshShader = ShaderLibrary::Get().Find( "shader_new" );
		m_BaseMaterial = Ref< Material >::Create( m_MeshShader, "Base Material" );
		m_MaterialRegistry = Ref<MaterialRegistry>::Create();

		// Read Materials
		size_t materials = 0;
		RawSerialisation::ReadObject( materials, rStream );

		m_MaterialsAssets.resize( materials );

		for( size_t i = 0; i < materials; i++ )
		{
			UUID materialID = 0;
			RawSerialisation::ReadObject( materialID, rStream );

			// Try load material
			Ref<MaterialAsset> materialAsset = AssetManager::Get().GetAssetAs<MaterialAsset>( materialID );

			// Failed to load material, create new and default it.
			if( materialAsset == nullptr )
			{
				Ref<MaterialAsset> defaultMat = Ref<MaterialAsset>::Create( nullptr );
				m_MaterialsAssets[ i ] = defaultMat;
			}
			else
			{
				m_MaterialsAssets[ i ] = materialAsset;
			}

			m_MaterialRegistry->AddAsset( m_MaterialsAssets[ i ] );
		}

		m_MaterialRegistry->SetMesh( this );
	}

	//////////////////////////////////////////////////////////////////////////

	MeshSource::MeshSource( const std::filesystem::path& rPath, const std::filesystem::path& rDstPath )
	{
#if !defined(SAT_DIST)
		AssimpLog::Initialize();

		m_Importer = std::make_unique<Assimp::Importer>();

		const aiScene* scene = m_Importer->ReadFile( rPath.string(), s_MeshImportFlags );

		if( scene == nullptr || !scene->HasMeshes() )
			SAT_CORE_ERROR( "Failed to load mesh file: {0}", rPath.string() );

		m_Scene = scene;

		for( size_t m = 0; m < m_Scene->mNumMaterials; m++ )
		{
			aiMaterial* material = m_Scene->mMaterials[ m ];

			aiString name;
			material->Get( AI_MATKEY_NAME, name );

			std::string MaterialName = std::string( name.C_Str() );

			if( MaterialName.empty() )
			{
				MaterialName = "Unnamed Material " + std::to_string( rand() );
			}

			// Albedo Texture
			{
				aiString AlbedoTexturePath;
				bool HasAlbedoTexture = material->GetTexture( aiTextureType_DIFFUSE, 0, &AlbedoTexturePath ) == AI_SUCCESS;

				if( HasAlbedoTexture )
				{
					auto pp = rPath.parent_path();

					pp /= std::string( AlbedoTexturePath.data );

					auto AlbedoTexturePath = pp.string();
					auto LocalPath = rDstPath;


					LocalPath /= pp.filename();

					if( !std::filesystem::exists( LocalPath ) )
						std::filesystem::copy_file( AlbedoTexturePath, LocalPath );
				}
			}

			// Normal Texture
			{
				aiString TexturePath;
				bool HasTexture = material->GetTexture( aiTextureType_NORMALS, 0, &TexturePath ) == AI_SUCCESS;

				if( HasTexture )
				{
					auto pp = rPath.parent_path();

					pp /= std::string( TexturePath.data );

					auto NormalTexturePath = pp.string();

					auto LocalPath = rDstPath;

					LocalPath /= pp.filename();

					if( !std::filesystem::exists( LocalPath ) )
						std::filesystem::copy_file( NormalTexturePath, LocalPath );
				}
			}

			// Normal Texture
			{
				aiString TexturePath;
				bool HasTexture = material->GetTexture( aiTextureType_SHININESS, 0, &TexturePath ) == AI_SUCCESS;

				if( HasTexture )
				{
					auto pp = rPath.parent_path();

					pp /= std::string( TexturePath.data );

					auto RoughnessTexturePath = pp.string();

					auto LocalPath = rDstPath;

					LocalPath /= pp.filename();

					if( !std::filesystem::exists( LocalPath ) )
						std::filesystem::copy_file( RoughnessTexturePath, LocalPath );
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
						if( Key == "$raw.ReflectionFactor|file" )
						{
							auto pp = rPath.parent_path();

							pp /= String;

							auto TexturePath = pp.string();

							Ref< Texture2D > MetalnessTexture;

							auto localTexturePath = rDstPath;

							localTexturePath /= pp.filename();

							if( !std::filesystem::exists( localTexturePath ) )
								std::filesystem::copy_file( TexturePath, localTexturePath );

							break;
						}
					}
				}
			}
		}
#endif
	}

	MeshSource::~MeshSource()
	{
	}

#if !defined(SAT_DIST)
	void MeshSource::TraverseNodes( aiNode* node, const glm::mat4& parentTransform /*= glm::mat4( 1.0f )*/, uint32_t level /*= 0 */ )
	{
	}
#endif
}