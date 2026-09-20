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
#include "AssetSerialisers.h"

#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Asset/Prefab.h"
#include "Saturn/Physics/PhysicsMaterialAsset.h"
#include "Saturn/Physics/PhysicsSurfaceRegistryAsset.h"
#include "Saturn/Asset/TextureSourceAsset.h"
#include "Saturn/Asset/MaterialAsset.h"
#include "Saturn/Animation/SkeletonAsset.h"
#include "Saturn/Animation/SkeletalAnimationAsset.h"
#include "Saturn/Audio/SoundSpecification.h"
#include "Saturn/Audio/GraphSound.h"
#include "Saturn/AI/BehaviourTree/BlackboardSpecificationAsset.h"
#include "Saturn/Alura/AluraFont.h"
#include "Saturn/Alura/AluraStylingProfile.h"

#include "Saturn/Project/Project.h"

#include "YamlAux.h"
#include "EntitySerialisation.h"
#include "Saturn/Scene/Scene.h"

#include <glm/gtc/type_ptr.hpp>
#include <yaml-cpp/yaml.h>
#include <fstream>

namespace Saturn {

#define GetFilepathAbs( rPath ) Project::GetActiveProject()->FilepathAbs( rPath )

	//////////////////////////////////////////////////////////////////////////
	// TEXTURE

	void TextureSourceAssetSerialiser::Serialise( const Ref<Asset>& rAsset ) const
	{
#if !defined(SAT_DIST)
		const auto& basePath = rAsset->Path;
		const auto fullPath = GetFilepathAbs( basePath );

		auto textureSource = rAsset.As<TextureSourceAsset>();

		YAML::Emitter out;

		out << YAML::BeginMap;

		out << YAML::Key << "Texture" << YAML::Value;

		out << YAML::BeginMap;

		out << YAML::Key << "Width"    << YAML::Value <<			  textureSource->Width();
		out << YAML::Key << "Height"   << YAML::Value <<			  textureSource->Height();
		out << YAML::Key << "Channels" << YAML::Value <<			  textureSource->Channels();
		out << YAML::Key << "Hdr"      << YAML::Value <<			  textureSource->IsHdr();
		out << YAML::Key << "Flags"    << YAML::Value << ( uint32_t ) textureSource->GetLoadFlags();
		out << YAML::Key << "FilteringFlags" << YAML::Value << ( uint32_t ) textureSource->GetFilteringFlags();

		auto path = std::filesystem::relative( textureSource->GetTextureAbsolutePath(), Project::GetActiveProjectRootPath() );

		// On Windows we serialise the path as a Linux path for Linux support 
#if defined(SAT_PLATFORM_WINDOWS)
		auto wstr = path.wstring();
		std::replace( wstr.begin(), wstr.end(), L'\\', L'/' );
		out << YAML::Key << "Source Path" << YAML::Value << wstr;
#else
		out << YAML::Key << "Source Path" << YAML::Value << path;
#endif

		out << YAML::EndMap;

		out << YAML::EndMap;

		std::ofstream file( fullPath );
		file << out.c_str();
#endif
	}

	bool TextureSourceAssetSerialiser::TryLoadData( Ref<Asset>& rAsset ) const
	{
		const auto absolutePath = GetFilepathAbs( rAsset->Path );
		std::ifstream FileIn( absolutePath );

		std::stringstream ss;
		ss << FileIn.rdbuf();

		YAML::Node data = YAML::Load( ss.str() );

		if( data.IsNull() )
			return false;

		// Create new Material Asset with rAsset being the base Asset
		auto textureSrcAsset = Ref<TextureSourceAsset>::Create( rAsset );

		const auto textureData = data[ "Texture" ];

		const auto width = textureData[ "Width" ].as<uint32_t>();
		const auto height = textureData[ "Height" ].as<uint32_t>();
		const auto channel = textureData[ "Channels" ].as<uint32_t>();
		const auto hdr = textureData[ "Hdr" ].as<bool>();
		const auto flags = textureData[ "Flags" ].as<uint32_t>();
		const auto filteringFlags = textureData[ "FilteringFlags" ].as<uint32_t>( 0 );
	
		auto path = textureData[ "Source Path" ].as<std::filesystem::path>();

#if defined( SAT_PLATFORM_WINDOWS )
		std::wstring wstr = path.wstring();
		std::replace( wstr.begin(), wstr.end(), L'/', L'\\' );
		path = wstr;
#endif

		textureSrcAsset->m_Width = width;
		textureSrcAsset->m_Height = height;
		textureSrcAsset->m_Channels = channel;
		textureSrcAsset->m_HDR = hdr;
		textureSrcAsset->m_LoadFlags = ( TextureLoadFlags ) flags;
		textureSrcAsset->m_SamplerFliteringFlags = ( TextureFilteringFlags ) filteringFlags;
#if !defined(SAT_DIST)
		textureSrcAsset->m_AbsolutePath = GetFilepathAbs( path );
#endif

		textureSrcAsset->LoadOnMostSuitableThread();

		// Set rAsset reference to point to our new MaterialAsset
		rAsset = textureSrcAsset;

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// MATERIAL

	void MaterialAssetSerialiser::Serialise( const Ref<Asset>& rAsset ) const
	{
		const auto& basePath = rAsset->Path;
		const auto fullPath = GetFilepathAbs( basePath );

		auto materialAsset = rAsset.As<MaterialAsset>();

		YAML::Emitter out;

		out << YAML::BeginMap;

		out << YAML::Key << "Material" << YAML::Value;

		out << YAML::BeginMap;

		out << YAML::Key << "AlbedoColor" << YAML::Value << materialAsset->GetAlbeoColor();

		auto writeTexture = [&](const char* key, Ref<Texture2D> texture ) 
		{
			auto asset = AssetManager::Get()->FindAsset( texture->GetSourceAssetID() );

			if( asset )
				out << YAML::Key << key << YAML::Value << asset->ID;
			else
				out << YAML::Key << key << YAML::Value << 0;
		};

		writeTexture( "AlbedoTexture", materialAsset->GetAlbeoMap() );

		out << YAML::Key << "UseNormal" << YAML::Value << materialAsset->IsUsingNormalMap();

		writeTexture( "NormalTexture", materialAsset->GetNormalMap() );

		out << YAML::Key << "Metalness" << YAML::Value << materialAsset->GetMetalness();

		writeTexture( "MetalnessTexture", materialAsset->GetMetallicMap() );

		out << YAML::Key << "Roughness" << YAML::Value << materialAsset->GetRoughness();

		writeTexture( "RoughnessTexture", materialAsset->GetRoughnessMap() );

		out << YAML::Key << "Emissive" << YAML::Value << materialAsset->GetEmissive();

		out << YAML::EndMap;

		out << YAML::EndMap;

		std::ofstream file( fullPath );
		file << out.c_str();
	}

	bool MaterialAssetSerialiser::TryLoadData( Ref<Asset>& rAsset ) const
	{
		const auto absolutePath = GetFilepathAbs( rAsset->Path );
		std::ifstream FileIn( absolutePath );

		std::stringstream ss;
		ss << FileIn.rdbuf();

		YAML::Node data = YAML::Load( ss.str() );

		if( data.IsNull() )
			return false;

		// Create new Material Asset with rAsset being the base Asset
		auto materialAsset = Ref<MaterialAsset>::Create( rAsset, nullptr );

		auto materialData = data[ "Material" ];

		auto albedoColor = materialData[ "AlbedoColor" ].as<glm::vec3>();
		auto albedoID = materialData[ "AlbedoTexture" ].as<uint64_t>( 0 );

		materialAsset->SetAlbeoColor( albedoColor );

		Ref<Texture2D> texture = nullptr;

		if( AssetManager::Get()->DoesAssetIDExist( albedoID ) )
		{
			Ref<TextureSourceAsset> textureAsset = AssetManager::Get()->GetAssetAs<TextureSourceAsset>( albedoID );

			materialAsset->SetAlbeoMap( textureAsset );
			AssetManager::Get()->RegisterAssetDependency( rAsset->ID, albedoID );
		}

		auto useNormal = materialData[ "UseNormal" ].as<bool>( false );
		auto normalID = materialData[ "NormalTexture" ].as<uint64_t>( 0 );

		materialAsset->UseNormalMap( useNormal );

		if( AssetManager::Get()->DoesAssetIDExist( normalID ) )
		{
			Ref<TextureSourceAsset> textureAsset = AssetManager::Get()->GetAssetAs<TextureSourceAsset>( normalID );

			materialAsset->SetNormalMap( textureAsset );
			AssetManager::Get()->RegisterAssetDependency( rAsset->ID, normalID );
		}

		auto metalness = materialData[ "Metalness" ].as<float>();
		auto metallicID = materialData[ "MetalnessTexture" ].as<uint64_t>( 0 );

		materialAsset->SetMetalness( metalness );

		if( AssetManager::Get()->DoesAssetIDExist( metallicID ) )
		{
			Ref<TextureSourceAsset> textureAsset = AssetManager::Get()->GetAssetAs<TextureSourceAsset>( metallicID );

			materialAsset->SetMetallicMap( textureAsset );
			AssetManager::Get()->RegisterAssetDependency( rAsset->ID, metallicID );
		}

		auto val = materialData[ "Roughness" ].as<float>();
		auto roughnessID = materialData[ "RoughnessTexture" ].as<uint64_t>( 0 );

		materialAsset->SetRoughness( val );

		if( AssetManager::Get()->DoesAssetIDExist( roughnessID ) )
		{
			Ref<TextureSourceAsset> textureAsset = AssetManager::Get()->GetAssetAs<TextureSourceAsset>( roughnessID );

			materialAsset->SetRoughnessMap( textureAsset );
			AssetManager::Get()->RegisterAssetDependency( rAsset->ID, roughnessID );
		}

		auto emissive = materialData[ "Emissive" ].as<float>( 0.0f );
		materialAsset->SetEmissive( emissive );

		// We may not always need to do this because most of the time this material will be bound meaning will change the textures.
		// However, we don't always know if it will ever be bound, for instance if we open a material in the material asset viewer, the material will not bound.
		// Meaning that the textures will not be updated.
		materialAsset->ForceUpdate();
		materialAsset->SetName( rAsset->Name );

		// Set rAsset reference to point to our new MaterialAsset
		rAsset = materialAsset;

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// PREFAB

	void PrefabSerialiser::Serialise( const Ref<Asset>& rAsset ) const
	{
		auto prefabAsset = rAsset.As<Prefab>();

		const auto& basePath = rAsset->Path;
		const auto fullPath = GetFilepathAbs( basePath );

		YAML::Emitter out;

		out << YAML::Comment( "WARNING, MODIFICATIONS TO THIS FILE WILL NOT UPDATE GLOBALLY IN EDITOR!" );

		out << YAML::BeginMap;

		out << YAML::Key << "Prefab" << YAML::Value << prefabAsset->ID;

		// Find root entity
		SharedPtr<Entity> RootEntity = nullptr;

		const auto view = prefabAsset->m_Scene->GetAllEntitiesWith<RelationshipComponent>();
		for( const auto& entity : view )
		{
			if( entity->GetComponent<RelationshipComponent>().Parent != 0 )
				continue;

			RootEntity = entity;
		}

		out << YAML::Key << "Root Entity" << YAML::Value << RootEntity->GetUUID();
		
		out << YAML::Key << "Entities";

		out << YAML::BeginSeq;

		prefabAsset->m_Scene->Each( [&]( SharedPtr<Entity> entity ) 
			{
				EntitySerialisation::SerialiseEntity( out, entity );
			} );

		out << YAML::EndSeq;

		out << YAML::Key << "ComponentCache";
		out << YAML::BeginSeq;

		for( const auto& [entityID, rHashes] : prefabAsset->GetComponentMap() )
		{
			out << YAML::BeginMap;
			out << YAML::Key << "EntityID" << YAML::Value << entityID;

			out << YAML::Key << "Count" << YAML::Value << rHashes.size();
			
			out << YAML::Key << "Hashes";
			out << YAML::Flow;
			out << YAML::BeginSeq;
			for( const auto& rHash : rHashes )
			{
				out << rHash;
			}
			out << YAML::EndSeq;
			out << YAML::EndMap;
		}
		out << YAML::EndSeq;

		out << YAML::EndMap;

		std::ofstream fout( fullPath );
		fout << out.c_str();
	}

	bool PrefabSerialiser::TryLoadData( Ref<Asset>& rAsset ) const
	{
		auto prefabAsset = Ref<Prefab>::Create( rAsset );

		const auto absolutePath = GetFilepathAbs( rAsset->Path );
		std::ifstream FileIn( absolutePath );

		std::stringstream ss;
		ss << FileIn.rdbuf();

		const YAML::Node data = YAML::Load( ss.str() );

		if( data.IsNull() )
			return false;

		const auto entities = data[ "Entities" ];

		prefabAsset->m_Scene = Ref<Scene>::Create();
		Scene* CurrentScene = g_ActiveScene;

		Scene::SetActiveScene( prefabAsset->m_Scene.Get() );

		for( const auto entityNode : entities )
		{
			EntitySerialisation::DeserialiseEntity( entityNode, prefabAsset->m_Scene );
		}

		const auto view = prefabAsset->m_Scene->GetAllEntitiesWith<RelationshipComponent>();

		// Root entity.
		const auto rootEntityID = data[ "Root Entity" ].as<uint64_t>( 0 );

		// Find root entity
		SharedPtr<Entity> RootEntity = prefabAsset->m_Scene->FindEntityByID( rootEntityID );

		// Preform manual search if not found.
		if( !RootEntity )
		{
			for( const auto& entity : view )
			{
				if( entity->GetComponent<RelationshipComponent>().Parent != 0 )
					continue;

				if( entity->GetChildren().size() > 0 )
					continue;

				RootEntity = entity;
			}
		}

		prefabAsset->m_Entity = RootEntity;
		Scene::SetActiveScene( CurrentScene );

		// Component cache
		const auto componentCacheNode = data[ "ComponentCache" ];
		if( componentCacheNode )
		{
			// TODO
//			const auto numberOfCaches = componentCacheNode[ "Count" ].as<size_t>( 0llu );
//			prefabAsset->m_ComponentCaches.reserve( numberOfCaches );

			for( const auto& rEntityNode : componentCacheNode )
			{
				const auto entityID = rEntityNode[ "EntityID" ].as<uint64_t>( 0llu );
				const auto cacheCount = rEntityNode[ "Count" ].as<size_t>( 0llu );
				const auto hashesNode = rEntityNode[ "Hashes" ];

				auto& rCacheList = prefabAsset->m_ComponentCaches[ entityID ];
				rCacheList.reserve( cacheCount );

				for( const auto& rHash : hashesNode )
				{
					rCacheList.push_back( rHash.as<entt::id_type>( 0u ) );
				}
			}
		}

		rAsset = prefabAsset;

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// STATIC MESH

	void StaticMeshAssetSerialiser::Serialise( const Ref<Asset>& rAsset ) const
	{
		auto mesh = rAsset.As<StaticMesh>();

		YAML::Emitter out;

		out << YAML::BeginMap;

		out << YAML::Key << "StaticMesh" << YAML::Value;

		out << YAML::BeginMap;

		std::filesystem::path path = std::filesystem::relative( mesh->FilePath(), Project::GetActiveProject()->GetRootDir() );

		// On Windows we serialise the path as a Linux path for Linux support 
#if defined(SAT_PLATFORM_WINDOWS)
		std::wstring wpath = path.wstring();
		std::replace( wpath.begin(), wpath.end(), L'\\', L'/' );

		out << YAML::Key << "Filepath" << YAML::Value << wpath;
#else
		out << YAML::Key << "Filepath" << YAML::Value << path;
#endif

		out << YAML::Key << "Attached Shape" << YAML::Value << (int)mesh->GetAttachedShape();

		out << YAML::Key << "Physics Material ID" << YAML::Value << ( uint64_t ) mesh->GetPhysicsMaterial();

#if !defined(SAT_DIST)
		out << YAML::Key << "Import Behaviour" << YAML::Value << ( uint32_t ) mesh->GetImportBehaviour();
#endif

		out << YAML::Key << "MaterialRegistry";
		out << YAML::BeginMap;

		out << YAML::Key << "Materials";
		out << YAML::BeginSeq;

		if( mesh->GetMaterialRegistry() )
		{
			int i = 0;
			for( const auto& material : mesh->GetMaterialRegistry()->GetMaterialAssets() )
			{
				out << YAML::BeginMap;

				if( material )
					out << YAML::Key << i << YAML::Value << material->ID;
				else
					out << YAML::Key << i << YAML::Value << 0;

				out << YAML::EndMap;

				++i;
			}
		}

		out << YAML::EndSeq;
		out << YAML::EndMap;

		out << YAML::EndMap;

		out << YAML::EndMap;

		auto& basePath = rAsset->Path;
		auto fullPath = GetFilepathAbs( basePath );

		std::ofstream fout( fullPath );
		fout << out.c_str();
	}

	bool StaticMeshAssetSerialiser::TryLoadData( Ref<Asset>& rAsset ) const
	{
		auto absolutePath = GetFilepathAbs( rAsset->Path );
		std::ifstream FileIn( absolutePath );

		std::stringstream ss;
		ss << FileIn.rdbuf();

		YAML::Node data = YAML::Load( ss.str() );

		if( data.IsNull() )
			return false;

		const auto meshData = data[ "StaticMesh" ];
		const auto shapeType = meshData[ "Attached Shape" ].as<int>( 0 );
		const auto physicsMaterial = meshData[ "Physics Material ID" ].as<uint64_t>( 0 );
		const auto importBehaviour = meshData[ "Import Behaviour" ].as<uint32_t>( MeshImportBehaviour_Default );

		std::filesystem::path filepath = meshData[ "Filepath" ].as<std::string>();

#if defined(SAT_PLATFORM_WINDOWS)
		std::wstring windowsPath = filepath.wstring();
		std::replace( windowsPath.begin(), windowsPath.end(), L'/', L'\\' );
		filepath = windowsPath;
#endif

		const auto realMeshPath = Project::GetActiveProject()->FilepathAbs( filepath );
		auto mesh = Ref<StaticMesh>::Create( rAsset, realMeshPath.string() );

		mesh->SetAttachedShape( (PhysicsShapeType)shapeType );
		mesh->SetPhysicsMaterial( physicsMaterial );
		
#if !defined(SAT_DIST)
		mesh->Import_SetImportBehaviour( importBehaviour );
#endif

		if( physicsMaterial )
			AssetManager::Get()->RegisterAssetDependency( rAsset->ID, physicsMaterial );

		// Build master material registry
		auto materialRegistry = meshData[ "MaterialRegistry" ];
		if( materialRegistry )
		{
			auto materials = materialRegistry[ "Materials" ];
			if( materials )
			{
				int i = 0;
				for( auto materialNode : materials )
				{
					auto id = materialNode[ i ].as<uint64_t>();

					Ref<MaterialAsset> asset = AssetManager::Get()->GetAssetAs<MaterialAsset>( id );

					if( id != 0 )
					{
						AssetManager::Get()->RegisterAssetDependency( rAsset->ID, id );
					}

					if( asset != nullptr )
					{
						mesh->GetMaterialRegistry()->AddAsset( asset );
					}
					else
					{
						auto defaultProjectAsset = AssetManager::Get()->FindAsset( Project::GetActiveProject()->GetDefaultMaterialAsset() );

						if( defaultProjectAsset )
							mesh->GetMaterialRegistry()->AddAsset( AssetManager::Get()->GetAssetAs<MaterialAsset>( defaultProjectAsset->ID ) );
						else
							mesh->GetMaterialRegistry()->AddAsset( Ref<MaterialAsset>::Create( nullptr ) );
					}

					++i;
				}
			}
		}
		
		// Set rAsset reference to point to our new StaticMesh
		rAsset = mesh;

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// SKELETAL MESH

	void SkeletalMeshAssetSerialiser::Serialise( const Ref<Asset>& rAsset ) const
	{
		const auto mesh = rAsset.As<SkeletalMesh>();

		YAML::Emitter out;

		out << YAML::BeginMap;

		out << YAML::Key << "SkeletalMesh" << YAML::Value;

		out << YAML::BeginMap;

		std::filesystem::path path = std::filesystem::relative( mesh->FilePath(), Project::GetActiveProject()->GetRootDir() );

		// On Windows we serialise the path as a Linux path for Linux support 
#if defined(SAT_PLATFORM_WINDOWS)
		std::wstring wPath = path.wstring();
		std::replace( wPath.begin(), wPath.end(), L'\\', L'/' );

		out << YAML::Key << "Filepath" << YAML::Value << wPath;
#else
		out << YAML::Key << "Filepath" << YAML::Value << path;
#endif

		out << YAML::Key << "Attached Shape" << YAML::Value << ( uint64_t ) mesh->GetAttachedShape();

		out << YAML::Key << "Physics Material ID" << YAML::Value << ( uint64_t ) mesh->GetPhysicsMaterial();

		out << YAML::Key << "Skeleton Asset ID" << YAML::Value << ( uint64_t ) mesh->GetSkeletonAsset()->ID;

#if !defined(SAT_DIST)
		out << YAML::Key << "Import Behaviour" << YAML::Value << ( uint32_t ) mesh->GetImportBehaviour();
#endif

		out << YAML::Key << "MaterialRegistry";
		out << YAML::BeginMap;

		out << YAML::Key << "Materials";
		out << YAML::BeginSeq;

		if( mesh->GetMaterialRegistry() )
		{
			int i = 0;
			for( const auto& material : mesh->GetMaterialRegistry()->GetMaterialAssets() )
			{
				out << YAML::BeginMap;

				if( material )
					out << YAML::Key << i << YAML::Value << material->ID;
				else
					out << YAML::Key << i << YAML::Value << 0;

				out << YAML::EndMap;

				++i;
			}
		}

		out << YAML::EndSeq;
		out << YAML::EndMap;

		out << YAML::EndMap;

		out << YAML::EndMap;

		auto& basePath = rAsset->Path;
		auto fullPath = GetFilepathAbs( basePath );

		std::ofstream fout( fullPath );
		fout << out.c_str();
	}

	bool SkeletalMeshAssetSerialiser::TryLoadData( Ref<Asset>& rAsset ) const
	{
		auto absolutePath = GetFilepathAbs( rAsset->Path );
		std::ifstream FileIn( absolutePath );

		std::stringstream ss;
		ss << FileIn.rdbuf();

		YAML::Node data = YAML::Load( ss.str() );

		if( data.IsNull() )
			return false;

		const auto meshData        = data[ "SkeletalMesh" ];
		const auto shapeType       = meshData[ "Attached Shape" ].as<int>( 0 );
		const auto physicsMaterial = meshData[ "Physics Material ID" ].as<uint64_t>( 0 );
		const auto skeletonAsset   = meshData[ "Skeleton Asset ID" ].as<uint64_t>( 0 );
		const auto importBehaviour = meshData[ "Import Behaviour" ].as<uint32_t>( 0 );

		std::filesystem::path filepath = meshData[ "Filepath" ].as<std::string>();

#if defined(SAT_PLATFORM_WINDOWS)
		std::wstring windowsPath = filepath.wstring();
		std::replace( windowsPath.begin(), windowsPath.end(), L'/', L'\\' );
		filepath = windowsPath;
#endif

		const auto realMeshPath = Project::GetActiveProject()->FilepathAbs( filepath );
		auto mesh = Ref<SkeletalMesh>::Create( rAsset, realMeshPath.string(), skeletonAsset );

		mesh->SetAttachedShape( ( PhysicsShapeType ) shapeType );
		mesh->SetPhysicsMaterial( physicsMaterial );
		
#if !defined(SAT_DIST)
		mesh->Import_SetImportBehaviour( importBehaviour );
#endif

		if( physicsMaterial )
			AssetManager::Get()->RegisterAssetDependency( rAsset->ID, physicsMaterial );

		// Build master material registry
		const auto materialRegistry = meshData[ "MaterialRegistry" ];
		if( materialRegistry )
		{
			auto materials = materialRegistry[ "Materials" ];
			if( materials )
			{
				size_t i = 0llu;
				for( auto materialNode : materials )
				{
					auto id = materialNode[ i ].as<uint64_t>();

					Ref<MaterialAsset> asset = AssetManager::Get()->GetAssetAs<MaterialAsset>( id );

					if( id != 0 )
					{
						AssetManager::Get()->RegisterAssetDependency( rAsset->ID, id );
					}

					if( asset != nullptr )
					{
						mesh->GetMaterialRegistry()->AddAsset( asset );
					}
					else
					{
						const auto defaultProjectAsset = AssetManager::Get()->FindAsset( Project::GetActiveProject()->GetDefaultMaterialAsset() );

						if( defaultProjectAsset ) 
						{
							auto defAsset = AssetManager::Get()->GetAssetAs<MaterialAsset>( defaultProjectAsset->ID );
							mesh->GetMaterialRegistry()->AddAsset( defAsset );
						}
						else 
						{
							auto nullAsset = Ref<MaterialAsset>::Create( nullptr );

							mesh->GetMaterialRegistry()->AddAsset( nullAsset );
						}
					}

					++i;
				}
			}
		}

		if( mesh->GetSkeletonAsset() )
			AssetManager::Get()->RegisterAssetDependency( mesh->ID, mesh->GetSkeletonAsset()->ID );

		// Set rAsset reference to point to our new SkeletalMesh
		rAsset = mesh;

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// SOUND

	void SoundSpecificationAssetSerialiser::Serialise( const Ref<Asset>& rAsset ) const
	{
		auto sound = rAsset.As<SoundSpecification>();

		YAML::Emitter out;

		out << YAML::BeginMap;

		out << YAML::Key << "Sound" << YAML::Value;

		out << YAML::BeginMap;

		auto sourcePath = std::filesystem::relative( sound->SoundSourcePath, Project::GetActiveProject()->GetRootDir() );

#if defined( SAT_PLATFORM_WINDOWS )
		std::wstring wstr = sourcePath.wstring();
		std::replace( wstr.begin(), wstr.end(), L'\\', L'/' );
		sourcePath = wstr;
#endif

		out << YAML::Key << "SourcePath" << YAML::Value << sourcePath;

		out << YAML::Key << "ImportPath" << YAML::Value << sound->OriginalImportPath;
		
#if !defined(SAT_DIST)
		out << YAML::Key << "LastWriteTime" << YAML::Value << sound->LastWriteTime;
#endif

		out << YAML::EndMap;

		out << YAML::EndMap;

		auto& basePath = rAsset->Path;
		auto fullPath = GetFilepathAbs( basePath );

		std::ofstream fout( fullPath );
		fout << out.c_str();
	}

	bool SoundSpecificationAssetSerialiser::TryLoadData( Ref<Asset>& rAsset ) const
	{
		auto absolutePath = GetFilepathAbs( rAsset->Path );
		std::ifstream FileIn( absolutePath );

		std::stringstream ss;
		ss << FileIn.rdbuf();

		YAML::Node data = YAML::Load( ss.str() );

		if( data.IsNull() )
			return false;

		const auto soundData = data[ "Sound" ];
		const auto filepath = soundData[ "SourcePath" ].as<std::string>();
		const auto importPath = soundData[ "ImportPath" ].as<std::string>();

		auto realPath = Project::GetActiveProject()->FilepathAbs( filepath );
#if defined( SAT_PLATFORM_WINDOWS )
		std::wstring wstr = realPath.wstring();
		std::replace( wstr.begin(), wstr.end(), L'/', L'\\' );
		realPath = wstr;
#endif

		auto soundSpec = Ref<SoundSpecification>::Create( rAsset );
		soundSpec->SoundSourcePath = realPath;
		soundSpec->OriginalImportPath = importPath;

#if !defined(SAT_DIST)
		auto lastWriteTime = soundData[ "LastWriteTime" ].as<std::string>();
		soundSpec->LastWriteTime = lastWriteTime;
#endif

		// Set rAsset reference to point to our new SoundSpecification
		rAsset = soundSpec;

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// PhysicsMaterial

	void PhysicsMaterialAssetSerialiser::Serialise( const Ref<Asset>& rAsset ) const
	{
		auto material = rAsset.As<PhysicsMaterialAsset>();

		YAML::Emitter out;

		out << YAML::BeginMap;

		out << YAML::Key << "PhysicsMaterial" << YAML::Value;

		out << YAML::BeginMap;

		out << YAML::Key << "Static Friction" << YAML::Value << material->GetFriction();
		out << YAML::Key << "Restitution" << YAML::Value << material->GetRestitution();
		out << YAML::Key << "SurfaceName" << YAML::Value << material->GetSurfaceName();

		out << YAML::EndMap;

		out << YAML::EndMap;

		auto& basePath = rAsset->Path;
		auto fullPath = GetFilepathAbs( basePath );

		std::ofstream fout( fullPath );
		fout << out.c_str();
	}

	bool PhysicsMaterialAssetSerialiser::TryLoadData( Ref<Asset>& rAsset ) const
	{
		auto absolutePath = GetFilepathAbs( rAsset->Path );
		std::ifstream FileIn( absolutePath );

		std::stringstream ss;
		ss << FileIn.rdbuf();

		YAML::Node data = YAML::Load( ss.str() );

		if( data.IsNull() )
			return false;

		const auto materialData = data[ "PhysicsMaterial" ];

		if( materialData.IsNull() )
			return false;

		const auto staticFriction = materialData[ "Static Friction" ].as<float>( 0.0f );
		const auto restitution = materialData[ "Restitution" ].as<float>( 0.0f );
		const auto surfaceName = materialData[ "SurfaceName" ].as<std::string>( std::string{} );

		auto material = Ref<PhysicsMaterialAsset>::Create( rAsset, staticFriction, restitution );
		
		if( surfaceName.empty() )
			material->SetSurfaceName( surfaceName );

		// Set rAsset reference to point to our new PhysicsMaterialAsset
		rAsset = material;

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// BehaviourTreeMemorySpec

	void BlackboardAssetSerialiser::Serialise( const Ref<Asset>& rAsset ) const
	{
		const auto btMemorySpec = rAsset.As<BlackboardSpecificationAsset>();

		YAML::Emitter out;

		out << YAML::BeginMap;

		out << YAML::Key << "BehaviourTreeMemorySpecification" << YAML::Value;
		out << YAML::BeginMap;

		out << YAML::Key << "Variables" << YAML::Value;
		out << YAML::BeginSeq;

		for( const auto& rData : btMemorySpec->m_SpecificationData )
		{
			out << YAML::BeginMap;

			out << YAML::Key << "Name" << YAML::Value << rData->Name;
			out << YAML::Key << "DataType" << YAML::Value << ( uint16_t )rData->DataType;
			out << YAML::Key << "VariableID" << YAML::Value << ( uint64_t )rData->VariableID;

			out << YAML::EndMap;
		}

		out << YAML::EndSeq;

		out << YAML::EndMap;

		out << YAML::EndMap;

		auto& basePath = rAsset->Path;
		auto fullPath = GetFilepathAbs( basePath );

		std::ofstream fout( fullPath );
		fout << out.c_str();
	}

	bool BlackboardAssetSerialiser::TryLoadData( Ref<Asset>& rAsset ) const
	{
		const auto absolutePath = GetFilepathAbs( rAsset->Path );
		std::ifstream FileIn( absolutePath );

		std::stringstream ss;
		ss << FileIn.rdbuf();

		YAML::Node data = YAML::Load( ss.str() );

		if( data.IsNull() )
			return false;

		const auto specData = data[ "BehaviourTreeMemorySpecification" ];
		if( specData.IsNull() )
			return false;

		const auto variables = specData[ "Variables" ];
	
		auto specAsset = Ref<BlackboardSpecificationAsset>::Create( rAsset );
		for( const auto variable : variables )
		{
			const auto name = variable[ "Name" ].as<std::string>();
			const auto dataType = variable[ "DataType" ].as<std::underlying_type_t<NodeEditorVariableDataType>>();
			const auto varID = variable[ "VariableID" ].as<uint64_t>( UUID() );

			specAsset->AddNew( name, ( NodeEditorVariableDataType ) dataType, varID );
		}

		// Set rAsset reference to point to our new BehaviourTreeMemorySpecification
		rAsset = specAsset;

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// SkeletonAssetSerialiser

	void SkeletonAssetSerialiser::Serialise( const Ref<Asset>& rAsset ) const
	{
		const auto skelAsset = rAsset.As<SkeletonAsset>();

		auto& basePath = rAsset->Path;
		auto fullPath = GetFilepathAbs( basePath );

		// TRANSITION: Binary
		skelAsset->Serialise( fullPath );
	}

	bool SkeletonAssetSerialiser::TryLoadData( Ref<Asset>& rAsset ) const
	{
		auto skeletonAsset = Ref<SkeletonAsset>::Create( rAsset );

		auto absolutePath = GetFilepathAbs( rAsset->Path );

		// TRANSITION: Binary
		skeletonAsset->Deserialise( absolutePath );

		// Set rAsset reference to point to our new Skeleton
		rAsset = skeletonAsset;

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// SkeletalAnimationAssetSerialiser

	void SkeletalAnimationAssetSerialiser::Serialise( const Ref<Asset>& rAsset ) const
	{
#if !defined(SAT_DIST)
		const auto animAsset = rAsset.As<SkeletalAnimationAsset>();

		auto& basePath = rAsset->Path;
		auto fullPath = GetFilepathAbs( basePath );
		std::ofstream fout( fullPath, std::ios::binary | std::ios::trunc );

		// TRANSITION: Binary
		animAsset->Serialise( fout );

		fout.close();
#endif
	}

	bool SkeletalAnimationAssetSerialiser::TryLoadData( Ref<Asset>& rAsset ) const
	{
#if !defined(SAT_DIST)
		const auto absolutePath = GetFilepathAbs( rAsset->Path );
		auto animAsset = Ref<SkeletalAnimationAsset>::Create( rAsset );

		std::ifstream FileIn( absolutePath, std::ios::binary | std::ios::in );
		// TRANSITION: Binary
		animAsset->Deserialise( FileIn );
		FileIn.close();

		// Set rAsset reference to point to our new SkeletalAnimation
		rAsset = animAsset;

		return true;
#else
		return false;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// AluraFontAssetSerialiser

	void AluraFontAssetSerialiser::Serialise( const Ref<Asset>& rAsset ) const
	{
		SAT_CORE_ASSERT( false, "AluraFontAssetSerialiser::Serialise is not to be called!, use the Serialise function on the object itself" );
	}

	bool AluraFontAssetSerialiser::TryLoadData( Ref<Asset>& rAsset ) const
	{
#if !defined(SAT_DIST)
		auto fontAsset = Ref<AluraFont>::Create( rAsset );

		const auto absolutePath = GetFilepathAbs( rAsset->Path );
		std::ifstream FileIn( absolutePath, std::ios::binary | std::ios::in );

		// TRANSITION: Binary
		fontAsset->Deserialise( FileIn );

		FileIn.close();

		// Set rAsset reference to point to our new AluraFont
		rAsset = fontAsset;

		return true;
#else
		return false;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// AluraStylingProfileAssetSerialiser

	void AluraStylingProfileAssetSerialiser::Serialise( const Ref<Asset>& rAsset ) const
	{
		const auto stylingProf = rAsset.As<AluraStylingProfile>();

		YAML::Emitter out;

		out << YAML::BeginMap;

		out << YAML::Key << "AluraStylingProfile" << YAML::Value;
		out << YAML::BeginMap;

		const auto& rStyle = stylingProf->GetStyle();

		out << YAML::Key << "Alpha" << YAML::Value << rStyle.Alpha;
		out << YAML::Key << "DisabledAlpha" << YAML::Value << rStyle.DisabledAlpha;
		out << YAML::Key << "RegionRounding" << YAML::Value << rStyle.RegionRounding;
		out << YAML::Key << "WindowPadding" << YAML::Value << rStyle.WindowPadding;
		out << YAML::Key << "ItemSpacing" << YAML::Value << rStyle.ItemSpacing;
		out << YAML::Key << "InnerItemSpacing" << YAML::Value << rStyle.ItemInnerSpacing;
		out << YAML::Key << "IndentSpacing" << YAML::Value << rStyle.IndentSpacing;
		out << YAML::Key << "WindowBorderSize" << YAML::Value << rStyle.WindowBorderSize;
		out << YAML::Key << "CurrentFontSize" << YAML::Value << rStyle.CurrentFontSize;

		out << YAML::Key << "Styles" << YAML::Value;
		out << YAML::BeginSeq;

		uint32_t index = 0;
		for( const auto& rColorVar : rStyle.Colours )
		{
			out << YAML::BeginMap;

			out << YAML::Key << "Index" << YAML::Value << index;
			out << YAML::Key << "Value" << YAML::Value << rColorVar;

			out << YAML::EndMap; // StyleVar
			++index;
		}

		out << YAML::EndSeq; // Styles

		out << YAML::EndMap; // Asset

		out << YAML::EndMap; // Root

		auto& basePath = rAsset->Path;
		auto fullPath = GetFilepathAbs( basePath );

		std::ofstream fout( fullPath );
		fout << out.c_str();
	}

	bool AluraStylingProfileAssetSerialiser::TryLoadData( Ref<Asset>& rAsset ) const
	{
		const auto absolutePath = GetFilepathAbs( rAsset->Path );
		std::ifstream FileIn( absolutePath );

		std::stringstream ss;
		ss << FileIn.rdbuf();

		YAML::Node data = YAML::Load( ss.str() );

		if( data.IsNull() )
			return false;

		const auto stylingData = data[ "AluraStylingProfile" ];

		if( stylingData.IsNull() )
			return false;

		auto stylingProfAsset = Ref<AluraStylingProfile>::Create( rAsset );

		auto& rStyle = stylingProfAsset->GetStyle();
		rStyle.Alpha = stylingData[ "Alpha" ].as<float>( 1.0f );
		rStyle.DisabledAlpha = stylingData[ "DisabledAlpha" ].as<float>( 1.0f );
		rStyle.RegionRounding = stylingData[ "RegionRounding" ].as<float>( 0.0f );
		rStyle.WindowPadding = stylingData[ "WindowPadding" ].as<glm::vec2>( glm::one<glm::vec2>() );
		rStyle.ItemSpacing = stylingData[ "ItemSpacing" ].as<glm::vec2>( glm::one<glm::vec2>() );
		rStyle.ItemInnerSpacing = stylingData[ "InnerItemSpacing" ].as<glm::vec2>( glm::one<glm::vec2>() );
		rStyle.IndentSpacing = stylingData[ "IndentSpacing" ].as<float>( 1.0f );
		rStyle.WindowBorderSize = stylingData[ "WindowBorderSize" ].as<float>( 1.0f );
		rStyle.CurrentFontSize = stylingData[ "CurrentFontSize" ].as<float>( 1.0f );

		const auto colors = stylingData[ "Styles" ];
		for( const auto color : colors )
		{
			const auto index = color[ "Index" ].as<uint64_t>();
			const auto value = color[ "Value" ].as<glm::vec4>();

			rStyle.Colours[ index ] = value;
		}

		// Set rAsset reference to point to our new AluraStylingProfile
		rAsset = stylingProfAsset;

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// PhysicsSurfaceRegistryAssetSerialiser

	void PhysicsSurfaceRegistryAssetSerialiser::Serialise( const Ref<Asset>& rAsset ) const
	{
		const auto physSurfaceReg = rAsset.As<PhysicsSurfaceRegistryAsset>();

		YAML::Emitter out;

		out << YAML::BeginMap;

		out << YAML::Key << "PhysicsSurfaceRegistry" << YAML::Value;
		out << YAML::BeginMap;

		out << YAML::Key << "Count" << YAML::Value << physSurfaceReg->GetNamesList().size();

		out << YAML::Key << "Names" << YAML::Value;
		out << YAML::BeginSeq;

#if defined( SAT_DIST )
		for( const auto& [rName] : physSurfaceReg->GetNamesList() )
#else
		for( const auto& [rName, rId] : physSurfaceReg->GetNamesList() )
#endif
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Name" << YAML::Value << rName;
			out << YAML::EndMap; // Name
		}

		out << YAML::EndSeq; // Names

		out << YAML::EndMap; // Asset

		out << YAML::EndMap; // Root

		const auto& basePath = rAsset->Path;
		const auto fullPath = GetFilepathAbs( basePath );

		std::ofstream fout( fullPath );
		fout << out.c_str();
	}

	bool PhysicsSurfaceRegistryAssetSerialiser::TryLoadData( Ref<Asset>& rAsset ) const
	{
		const auto absolutePath = GetFilepathAbs( rAsset->Path );
		std::ifstream FileIn( absolutePath );

		std::stringstream ss;
		ss << FileIn.rdbuf();

		YAML::Node data = YAML::Load( ss.str() );

		if( data.IsNull() )
			return false;

		const auto regData = data[ "PhysicsSurfaceRegistry" ];

		if( regData.IsNull() )
			return false;

		auto physSurfaceReg = Ref<PhysicsSurfaceRegistryAsset>::Create( rAsset );

		const auto namesCount = regData[ "Count" ].as<size_t>( 0llu );
		physSurfaceReg->m_Surfaces.reserve( namesCount );

		const auto names = regData[ "Names" ];
		for( const auto nameNode : names )
		{
			const auto name = nameNode[ "Name" ].as<std::string>();
			physSurfaceReg->m_Surfaces.emplace_back( name );
		}

		// Set rAsset reference to point to our new PhysicsSurfaceRegistryAsset
		rAsset = physSurfaceReg;

		return true;
	}

}
