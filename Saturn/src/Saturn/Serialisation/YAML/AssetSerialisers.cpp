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
#include "AssetSerialisers.h"

#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Asset/Prefab.h"
#include "Saturn/Physics/PhysicsMaterialAsset.h"
#include "Saturn/Asset/TextureSourceAsset.h"
#include "Saturn/Asset/MaterialAsset.h"
#include "Saturn/Animation/SkeletonAsset.h"
#include "Saturn/Animation/SkeletalAnimationAsset.h"
#include "Saturn/Audio/SoundSpecification.h"
#include "Saturn/Audio/GraphSound.h"
#include "Saturn/AI/BehaviourTree/BehaviourTreeMemorySpecification.h"
#include "Saturn/Alura/AluraFont.h"
#include "Saturn/Alura/AluraStylingProfile.h"

#include "Saturn/Vulkan/Renderer.h"

#include "Saturn/Project/Project.h"

#include "YamlAux.h"
#include "EntitySerialisation.h"

#include <glm/gtc/type_ptr.hpp>
#include <yaml-cpp/yaml.h>
#include <fstream>

namespace Saturn {

#define GetFilepathAbs( rPath ) Project::GetActiveProject()->FilepathAbs( rPath )

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
			std::filesystem::path relativePath = std::filesystem::relative( texture->GetPath(), Project::GetActiveProjectRootPath() );
			auto asset = AssetManager::Get().FindAsset( relativePath );

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

		if( AssetManager::Get().DoesAssetIDExist( albedoID ) )
		{
			Ref<Asset> rAsset = AssetManager::Get().FindAsset( albedoID );
			texture = Ref<Texture2D>::Create( Project::GetActiveProject()->FilepathAbs( rAsset->Path ), AddressingMode::Repeat );

			materialAsset->SetAlbeoMap( texture );
			AssetManager::Get().RegisterAssetDependency( rAsset->ID, albedoID );
		}

		auto useNormal = materialData[ "UseNormal" ].as<float>();
		auto normalID = materialData[ "NormalTexture" ].as<uint64_t>( 0 );

		materialAsset->UseNormalMap( useNormal );

		if( AssetManager::Get().DoesAssetIDExist( normalID ) )
		{
			Ref<Asset> rAsset = AssetManager::Get().FindAsset( normalID );
			texture = Ref<Texture2D>::Create( Project::GetActiveProject()->FilepathAbs( rAsset->Path ), AddressingMode::Repeat );

			materialAsset->SetNormalMap( texture );
			AssetManager::Get().RegisterAssetDependency( rAsset->ID, normalID );
		}

		auto metalness = materialData[ "Metalness" ].as<float>();
		auto metallicID = materialData[ "MetalnessTexture" ].as<uint64_t>( 0 );

		materialAsset->SetMetalness( metalness );

		if( AssetManager::Get().DoesAssetIDExist( metallicID ) )
		{
			Ref<Asset> rAsset = AssetManager::Get().FindAsset( metallicID );
			texture = Ref<Texture2D>::Create( Project::GetActiveProject()->FilepathAbs( rAsset->Path ), AddressingMode::Repeat );

			materialAsset->SetMetallicMap( texture );
			AssetManager::Get().RegisterAssetDependency( rAsset->ID, metallicID );
		}

		auto val = materialData[ "Roughness" ].as<float>();
		auto roughnessID = materialData[ "RoughnessTexture" ].as<uint64_t>( 0 );

		materialAsset->SetRoughness( val );

		if( AssetManager::Get().DoesAssetIDExist( roughnessID ) )
		{
			Ref<Asset> rAsset = AssetManager::Get().FindAsset( roughnessID );
			texture = Ref<Texture2D>::Create( Project::GetActiveProject()->FilepathAbs( rAsset->Path ), AddressingMode::Repeat );

			materialAsset->SetRoughnessMap( texture );
			AssetManager::Get().RegisterAssetDependency( rAsset->ID, roughnessID );
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

			if( entity->GetChildren().size() > 0 )
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

		// Find root entity
		SharedPtr<Entity> RootEntity = nullptr;

		for( const auto& entity : view )
		{
			if( entity->GetComponent<RelationshipComponent>().Parent != 0 )
				continue;

			if( entity->GetChildren().size() > 0 )
				continue;

			RootEntity = entity;
		}

		prefabAsset->m_Entity = RootEntity;
		Scene::SetActiveScene( CurrentScene );

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

		std::wstring path = std::filesystem::relative( mesh->FilePath(), Project::GetActiveProject()->GetRootDir() );

		// On Windows we serialise the path as a Linux path for Linux support 
#if defined(SAT_PLATFORM_WINDOWS)
		std::replace( path.begin(), path.end(), L'\\', L'/' );

		out << YAML::Key << "Filepath" << YAML::Value << path;
#else
		out << YAML::Key << "Filepath" << YAML::Value << path;
#endif

		out << YAML::Key << "Attached Shape" << YAML::Value << (int)mesh->GetAttachedShape();

		out << YAML::Key << "Physics Material ID" << YAML::Value << (int)mesh->GetPhysicsMaterial();

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

		if( physicsMaterial )
			AssetManager::Get().RegisterAssetDependency( rAsset->ID, physicsMaterial );

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

					Ref<MaterialAsset> asset = AssetManager::Get().GetAssetAs<MaterialAsset>( id );

					if( id != 0 )
					{
						mesh->GetMaterialRegistry()->AddTargetMaterialAsset( i, id );

						AssetManager::Get().RegisterAssetDependency( rAsset->ID, id );
					}

					if( asset != nullptr )
					{
						mesh->GetMaterialRegistry()->AddAsset( asset );
					}
					else
					{
						auto defaultProjectAsset = AssetManager::Get().FindAsset( Project::GetActiveProject()->GetDefaultMaterialAsset() );

						if( defaultProjectAsset )
							mesh->GetMaterialRegistry()->AddAsset( AssetManager::Get().GetAssetAs<MaterialAsset>( defaultProjectAsset->ID ) );
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

		std::wstring path = std::filesystem::relative( mesh->FilePath(), Project::GetActiveProject()->GetRootDir() );

		// On Windows we serialise the path as a Linux path for Linux support 
#if defined(SAT_PLATFORM_WINDOWS)
		std::replace( path.begin(), path.end(), L'\\', L'/' );

		out << YAML::Key << "Filepath" << YAML::Value << path;
#else
		out << YAML::Key << "Filepath" << YAML::Value << path;
#endif

		out << YAML::Key << "Attached Shape" << YAML::Value << ( uint64_t ) mesh->GetAttachedShape();

		out << YAML::Key << "Physics Material ID" << YAML::Value << ( uint64_t ) mesh->GetPhysicsMaterial();

		out << YAML::Key << "Skeleton Asset ID" << YAML::Value << ( uint64_t ) mesh->GetSkeletonAsset()->ID;

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

		if( physicsMaterial )
			AssetManager::Get().RegisterAssetDependency( rAsset->ID, physicsMaterial );

		// Build master material registry
		const auto materialRegistry = meshData[ "MaterialRegistry" ];
		if( materialRegistry )
		{
			auto materials = materialRegistry[ "Materials" ];
			if( materials )
			{
				int i = 0;
				for( auto materialNode : materials )
				{
					auto id = materialNode[ i ].as<uint64_t>();

					Ref<MaterialAsset> asset = AssetManager::Get().GetAssetAs<MaterialAsset>( id );

					if( id != 0 )
					{
						mesh->GetMaterialRegistry()->AddTargetMaterialAsset( i, id );

						AssetManager::Get().RegisterAssetDependency( rAsset->ID, id );
					}

					if( asset != nullptr )
					{
						mesh->GetMaterialRegistry()->AddAsset( asset );
					}
					else
					{
						const auto defaultProjectAsset = AssetManager::Get().FindAsset( Project::GetActiveProject()->GetDefaultMaterialAsset() );

						if( defaultProjectAsset ) 
						{
							auto defAsset = AssetManager::Get().GetAssetAs<MaterialAsset>( defaultProjectAsset->ID );
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
			AssetManager::Get().RegisterAssetDependency( mesh->ID, mesh->GetSkeletonAsset()->ID );

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

		out << YAML::Key << "SourcePath" << YAML::Value << std::filesystem::relative( sound->SoundSourcePath, Project::GetActiveProject()->GetRootDir() );

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

		auto soundData = data[ "Sound" ];
		auto filepath = soundData[ "SourcePath" ].as<std::string>();
		auto importPath = soundData[ "ImportPath" ].as<std::string>();

		auto realPath = Project::GetActiveProject()->FilepathAbs( filepath );

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

		out << YAML::Key << "Static Friction" << YAML::Value << material->GetStaticFriction();

		out << YAML::Key << "Dynamic Friction" << YAML::Value << material->GetDynamicFriction();

		out << YAML::Key << "Restitution" << YAML::Value << material->GetRestitution();

		out << YAML::Key << "Flags" << YAML::Value << material->GetFlags();

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
		const auto dynamicFriction = materialData[ "Dynamic Friction" ].as<float>( 0.0f );
		const auto restitution = materialData[ "Restitution" ].as<float>( 0.0f );
		const auto flags = materialData[ "Flags" ].as<uint32_t>();

		auto material = Ref<PhysicsMaterialAsset>::Create( rAsset, staticFriction, dynamicFriction, restitution, (PhysicsMaterialFlags)flags );

		// Set rAsset reference to point to our new PhysicsMaterialAsset
		rAsset = material;

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// BehaviourTreeMemorySpec

	void BehaviourTreeMemorySpecAssetSerialiser::Serialise( const Ref<Asset>& rAsset ) const
	{
		const auto btMemorySpec = rAsset.As<BehaviourTreeMemorySpecification>();

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
			out << YAML::Key << "DataType" << YAML::Value << ( std::underlying_type_t<SPropertyType> )rData->DataType;
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

	bool BehaviourTreeMemorySpecAssetSerialiser::TryLoadData( Ref<Asset>& rAsset ) const
	{
		const auto absolutePath = GetFilepathAbs( rAsset->Path );
		std::ifstream FileIn( absolutePath );

		std::stringstream ss;
		ss << FileIn.rdbuf();

		YAML::Node data = YAML::Load( ss.str() );

		if( data.IsNull() )
			return false;

		auto specData = data[ "BehaviourTreeMemorySpecification" ];

		if( specData.IsNull() )
			return false;

		auto variables = specData[ "Variables" ];

		auto specAsset = Ref<BehaviourTreeMemorySpecification>::Create( rAsset );
	
		for( auto variable : variables )
		{
			auto name = variable[ "Name" ].as<std::string>();
			auto dataType = variable[ "DataType" ].as<std::underlying_type_t<SPropertyType>>();
			auto varID = variable[ "VariableID" ].as<uint64_t>( UUID() );

			specAsset->AddNew( name, ( SPropertyType ) dataType, varID );
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
		out << YAML::Key << "WindowPadding" << YAML::Value << rStyle.WindowPadding;
		out << YAML::Key << "ItemSpacing" << YAML::Value << rStyle.ItemSpacing;
		out << YAML::Key << "IndentSpacing" << YAML::Value << rStyle.IndentSpacing;
		out << YAML::Key << "WindowBorderSize" << YAML::Value << rStyle.WindowBorderSize;
		out << YAML::Key << "CurrentFontSize" << YAML::Value << rStyle.CurrentFontSize;

		out << YAML::Key << "Styles" << YAML::Value;
		out << YAML::BeginSeq;

		uint32_t index = 0;
		for( const auto& rColorVar : rStyle.Colors )
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
		rStyle.WindowPadding = stylingData[ "WindowPadding" ].as<glm::vec2>( glm::one<glm::vec2>() );
		rStyle.ItemSpacing = stylingData[ "ItemSpacing" ].as<glm::vec2>( glm::one<glm::vec2>() );
		rStyle.IndentSpacing = stylingData[ "IndentSpacing" ].as<float>( 1.0f );
		rStyle.WindowBorderSize = stylingData[ "WindowBorderSize" ].as<float>( 1.0f );
		rStyle.CurrentFontSize = stylingData[ "CurrentFontSize" ].as<float>( 1.0f );

		const auto colors = stylingData[ "Styles" ];
		for( const auto color : colors )
		{
			const auto index = color[ "Index" ].as<uint64_t>();
			const auto value = color[ "Value" ].as<glm::vec4>();

			rStyle.Colors[ index ] = value;
		}

		// Set rAsset reference to point to our new AluraStylingProfile
		rAsset = stylingProfAsset;

		return true;
	}

}
