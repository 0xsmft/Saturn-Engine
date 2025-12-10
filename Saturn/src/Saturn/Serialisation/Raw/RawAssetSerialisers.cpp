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
#include "RawAssetSerialisers.h"

#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Asset/Prefab.h"
#include "Saturn/Asset/PhysicsMaterialAsset.h"
#include "Saturn/Asset/TextureSourceAsset.h"
#include "Saturn/Asset/MaterialAsset.h"

#include "Saturn/Audio/SoundSpecification.h"
#include "Saturn/Audio/AudioSystem.h"

#include "Saturn/AI/BehaviourTree/BehaviourTreeMemorySpecification.h"

#include "Saturn/Core/VirtualFS.h"
#include "Saturn/Core/MemoryStream.h"

#include "Saturn/Physics/PhysicsShapeTypes.h"

#include "Saturn/Vulkan/Renderer.h"

#include "Saturn/Project/Project.h"

#include "RawSerialisation.h"

// #NOTE
// NOTES WHEN ADDING NEW FIELDS TO SERIALISE:
// First, make sure to add the field in both Raw and YAML serialisers
// Next, add it to TryLoadData.
// Finally, for VFS Assets, make sure to load it and write into the VFile!

namespace Saturn {

	bool RawMaterialAssetSerialiser::TryLoadData( Ref<Asset>& rAsset ) const
	{
		const std::string& rMountBase = Project::GetActiveConfig().Name;
		Ref<VFile> file = VirtualFS::Get().FindFile( rMountBase, rAsset->Path );

		if( !file )
			return false;

		PakFileMemoryBuffer membuf( file->FileContent );

		std::istream stream( &membuf );

		/////////////////////////////////////

		Ref<MaterialAsset> materialAsset = Ref<MaterialAsset>::Create( rAsset, nullptr );
		AssetID currentTextureID = 0;

		// ALBEO COLOR

		glm::vec3 albeoColor = glm::vec3( 0.0f );
		RawSerialisation::ReadVec3( albeoColor, stream );

		materialAsset->SetAlbeoColor( albeoColor );

		// ALBEO MAP
		RawSerialisation::ReadObject( currentTextureID, stream );
		
		materialAsset->SetAlbeoMap( currentTextureID );

		// IS USING NORMAL MAPS
		bool isUsingNormalMaps = false;
		RawSerialisation::ReadObject( isUsingNormalMaps, stream );
		materialAsset->UseNormalMap( isUsingNormalMaps );

		// NORMAL MAP
		RawSerialisation::ReadObject( currentTextureID, stream );
		
		materialAsset->SetNormalMap( currentTextureID );

		// METALNESS
		float metalness = 0.0f;

		RawSerialisation::ReadObject( metalness, stream );

		materialAsset->SetMetalness( metalness );

		// METALLIC MAP
		RawSerialisation::ReadObject( currentTextureID, stream );
		materialAsset->SetMetallicMap( currentTextureID );

		// ROUGHNESS
		float roughness = 0.0f;

		RawSerialisation::ReadObject( roughness, stream );

		materialAsset->SetRoughness( roughness );

		// ROUGHNESS MAP
		RawSerialisation::ReadObject( currentTextureID, stream );
		materialAsset->SetRoughnessMap( currentTextureID );

		// EMISSIVE
		float emissive = 0.0f;
		RawSerialisation::ReadObject( emissive, stream );
		
		materialAsset->SetEmissive( emissive );

		materialAsset->ForceUpdate();

		// Set rAsset reference to point to our new MaterialAsset.
		rAsset = materialAsset;

		return true;
	}

	bool RawMaterialAssetSerialiser::DumpAndWriteToVFS( const Ref<Asset>& rAsset ) const
	{
		auto materialAsset = rAsset.As<MaterialAsset>();

		if( !materialAsset )
			return false;

		std::filesystem::path out = Project::GetActiveProject()->GetTempDir();
		out /= std::to_string( rAsset->ID );
		out.replace_extension( ".vfs" );

		std::ofstream fout( out, std::ios::binary | std::ios::trunc );

		/////////////////////////////////////

		RawSerialisation::WriteVec3( materialAsset->GetAlbeoColor(), fout );
		
		auto writeTextureID = []( const Ref<Texture2D> targetTexture, std::ofstream& rStream ) 
			{
				AssetID textureID = 0;
				if( targetTexture ) 
				{
					auto path = std::filesystem::relative( targetTexture->GetPath(), Project::GetActiveProject()->GetRootDir() );

					SAT_CORE_INFO( "texture relative path: {0} for texture: {1}", path, targetTexture->GetPath() );

					// We are fine to use the main asset registry here, we are only looking for an asset.
					const Ref<Asset> textureSourceAsset = AssetManager::Get().FindAsset( path );
					if( textureSourceAsset )
						textureID = textureSourceAsset->ID;
				}

				RawSerialisation::WriteObject( textureID, rStream );
			};

		// ALBEO
		writeTextureID( materialAsset->GetAlbeoMap(), fout );

		// NORMAL MAP
		bool isUsingNormalMaps = materialAsset->IsUsingNormalMap();
		RawSerialisation::WriteObject( isUsingNormalMaps, fout );

		writeTextureID( materialAsset->GetNormalMap(), fout );

		// METALLIC MAP
		RawSerialisation::WriteObject( materialAsset->GetMetalness(), fout );

		writeTextureID( materialAsset->GetMetallicMap(), fout );

		// ROUGHNESS MAP
		RawSerialisation::WriteObject( materialAsset->GetRoughness(), fout );

		writeTextureID( materialAsset->GetRoughnessMap(), fout );

		RawSerialisation::WriteObject( materialAsset->GetEmissive(), fout );
		
		fout.close();

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// PREFAB

	bool RawPrefabSerialiser::TryLoadData( Ref<Asset>& rAsset ) const
	{
		auto prefabAsset = Ref<Prefab>::Create();

//		prefabAsset->DeserialisePrefab();

		// TODO: (Asset) Fix this.
		rAsset = prefabAsset;

		return true;
	}

	bool RawPrefabSerialiser::DumpAndWriteToVFS( const Ref<Asset>& rAsset ) const
	{
		auto prefabAsset = rAsset.As<Prefab>();

		std::filesystem::path out = Project::GetActiveProject()->GetTempDir();
		out /= std::to_string( rAsset->ID );
		out.replace_extension( ".vfs" );

		std::ofstream fout( out, std::ios::binary | std::ios::trunc );

		/////////////////////////////////////

		prefabAsset->SerialisePrefab( fout );
		
		fout.close();

		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	// STATIC MESH

	bool RawStaticMeshAssetSerialiser::DumpAndWriteToVFS( const Ref<Asset>& rAsset ) const
	{
		const auto staticMeshAsset = rAsset.As<StaticMesh>();

		std::filesystem::path out = Project::GetActiveProject()->GetTempDir();
		out /= std::to_string( rAsset->ID );
		out.replace_extension( ".vfs" );

		std::ofstream fout( out, std::ios::binary | std::ios::trunc );

		/////////////////////////////////////

		RawSerialisation::WriteObject( staticMeshAsset->GetAttachedShape(), fout );
		RawSerialisation::WriteObject( staticMeshAsset->GetPhysicsMaterial(), fout );

		staticMeshAsset->SerialiseData( fout );

		fout.close();

		return true;
	}

	bool RawStaticMeshAssetSerialiser::TryLoadData( Ref<Asset>& rAsset ) const
	{
		const std::string& rMountBase = Project::GetActiveConfig().Name;
		Ref<VFile> file = VirtualFS::Get().FindFile( rMountBase, rAsset->Path );

		if( !file )
			return false;

		PakFileMemoryBuffer membuf( file->FileContent );
		std::istream stream( &membuf );

		/////////////////////////////////////
		auto staticMeshAsset = Ref<StaticMesh>::Create( rAsset );

		PhysicsShapeType shapeType = PhysicsShapeType::Unknown;
		AssetID physicsMaterial = 0;

		RawSerialisation::ReadObject( shapeType, stream );
		RawSerialisation::ReadObject( physicsMaterial, stream );

		staticMeshAsset->SetAttachedShape( shapeType );
		staticMeshAsset->SetPhysicsMaterial( physicsMaterial );

		staticMeshAsset->DeserialiseData( stream );

		// Set rAsset reference to point to our new StaticMesh.
		rAsset = staticMeshAsset;

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// SKELETAL MESH

	bool RawSkeletalMeshAssetSerialiser::TryLoadData( Ref<Asset>& rAsset ) const
	{
		const std::string& rMountBase = Project::GetActiveConfig().Name;
		Ref<VFile> file = VirtualFS::Get().FindFile( rMountBase, rAsset->Path );

		if( !file )
			return false;

		PakFileMemoryBuffer membuf( file->FileContent );
		std::istream stream( &membuf );

		/////////////////////////////////////
		auto skeletalMeshAsset = Ref<SkeletalMesh>::Create( rAsset );

		PhysicsShapeType shapeType = PhysicsShapeType::Unknown;
		AssetID physicsMaterial = 0, skeletonID = 0;

		RawSerialisation::ReadObject( skeletonID, stream );
		RawSerialisation::ReadObject( shapeType, stream );
		RawSerialisation::ReadObject( physicsMaterial, stream );

#if defined(SAT_DIST)
		skeletalMeshAsset->DistLoadSkeleton( skeletonID );
#endif
		skeletalMeshAsset->SetAttachedShape( shapeType );
		skeletalMeshAsset->SetPhysicsMaterial( physicsMaterial );

		skeletalMeshAsset->DeserialiseData( stream );

		// Set rAsset reference to point to our new SkeletalMesh.
		rAsset = skeletalMeshAsset;

		return true;
	}

	bool RawSkeletalMeshAssetSerialiser::DumpAndWriteToVFS( const Ref<Asset>& rAsset ) const
	{
		auto skMeshAsset = rAsset.As<SkeletalMesh>();

		std::filesystem::path out = Project::GetActiveProject()->GetTempDir();
		out /= std::to_string( rAsset->ID );
		out.replace_extension( ".vfs" );

		std::ofstream fout( out, std::ios::binary | std::ios::trunc );

		/////////////////////////////////////

		RawSerialisation::WriteObject( skMeshAsset->GetSkeletonAsset(), fout );
		RawSerialisation::WriteObject( skMeshAsset->GetAttachedShape(), fout );
		RawSerialisation::WriteObject( skMeshAsset->GetPhysicsMaterial(), fout );

		skMeshAsset->SerialiseData( fout );

		fout.close();

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// PHYSICS MATERIAL

	bool RawPhysicsMaterialAssetSerialiser::TryLoadData( Ref<Asset>& rAsset ) const
	{
		const std::string& rMountBase = Project::GetActiveConfig().Name;
		Ref<VFile> file = VirtualFS::Get().FindFile( rMountBase, rAsset->Path );

		if( !file )
			return false;

		/////////////////////////////////////
		PakFileMemoryBuffer membuf( file->FileContent );
		std::istream stream( &membuf );

		glm::vec3 StaticDynamicFrictionRestitution{};
		uint32_t assetFlags = 0;

		RawSerialisation::ReadObject( StaticDynamicFrictionRestitution.x, stream );
		RawSerialisation::ReadObject( StaticDynamicFrictionRestitution.y, stream );
		RawSerialisation::ReadObject( StaticDynamicFrictionRestitution.z, stream );

		RawSerialisation::ReadObject( assetFlags, stream );

		auto physMaterialAsset = Ref<PhysicsMaterialAsset>::Create( rAsset, 
			StaticDynamicFrictionRestitution.x,
			StaticDynamicFrictionRestitution.y,
			StaticDynamicFrictionRestitution.z );
		
		physMaterialAsset->SetFlag( (PhysicsMaterialFlags)assetFlags, true );

		// Set rAsset reference to point to our new PhysicsMaterial.
		rAsset = physMaterialAsset;

		return true;
	}

	bool RawPhysicsMaterialAssetSerialiser::DumpAndWriteToVFS( const Ref<Asset>& rAsset ) const
	{
		const auto physMaterialAsset = rAsset.As<PhysicsMaterialAsset>();

		std::filesystem::path out = Project::GetActiveProject()->GetTempDir();
		out /= std::to_string( rAsset->ID );
		out.replace_extension( ".vfs" );

		std::ofstream stream( out, std::ios::binary | std::ios::trunc );

		RawSerialisation::WriteObject( physMaterialAsset->GetStaticFriction(), stream );
		RawSerialisation::WriteObject( physMaterialAsset->GetDynamicFriction(), stream );
		RawSerialisation::WriteObject( physMaterialAsset->GetRestitution(), stream );

		RawSerialisation::WriteObject( physMaterialAsset->GetFlags(), stream );

		stream.close();

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// SOUND SPECIFICATION

	static void SerialiseSoundDecodedInformation( const SoundDecodedInformation& rObject, std::ofstream& rStream )
	{
		RawSerialisation::WriteObject( rObject.Format, rStream );
		RawSerialisation::WriteObject( rObject.Channels, rStream );
		RawSerialisation::WriteObject( rObject.SampleRate, rStream );
		RawSerialisation::WriteObject( rObject.PCMFrameCount, rStream );
		RawSerialisation::WriteObject( rObject.BytesPerFrame, rStream );
		RawSerialisation::WriteSaturnBuffer( rObject.PCMFrames, rStream );
	}

	static void DeserialiseSoundDecodedInformation( SoundDecodedInformation& rObject, std::istream& rStream ) 
	{
		RawSerialisation::ReadObject( rObject.Format, rStream );
		RawSerialisation::ReadObject( rObject.Channels, rStream );
		RawSerialisation::ReadObject( rObject.SampleRate, rStream );
		RawSerialisation::ReadObject( rObject.PCMFrameCount, rStream );
		RawSerialisation::ReadObject( rObject.BytesPerFrame, rStream );
		RawSerialisation::ReadSaturnBuffer( rObject.PCMFrames, rStream );
	}

	bool RawSoundSpecAssetSerialiser::DumpAndWriteToVFS( const Ref<Asset>& rAsset ) const
	{
		auto soundSpec = rAsset.As<SoundSpecification>();

		std::filesystem::path out = Project::GetActiveProject()->GetTempDir();
		out /= std::to_string( rAsset->ID );
		out.replace_extension( ".vfsn" );

		std::ofstream stream( out, std::ios::binary | std::ios::trunc );

		RawSerialisation::WriteString( std::filesystem::relative( soundSpec->SoundSourcePath, Project::GetActiveProject()->GetRootDir() ), stream );

		auto decodedInformation = AudioSystem::Get().DecodeSound( soundSpec );
		SerialiseSoundDecodedInformation( decodedInformation, stream );

		stream.close();

		return true;
	}

	bool RawSoundSpecAssetSerialiser::TryLoadData( Ref<Asset>& rAsset ) const
	{
#if defined( SAT_DIST )
		const std::string& rMountBase = Project::GetActiveConfig().Name;
		Ref<VFile> file = VirtualFS::Get().FindFile( rMountBase, rAsset->Path );

		if( !file )
			return false;

		/////////////////////////////////////

		PakFileMemoryBuffer membuf( file->FileContent );
		std::istream stream( &membuf );

		std::string sourcePath = RawSerialisation::ReadString( stream );

		auto soundSpec = Ref<SoundSpecification>::Create( rAsset );
		soundSpec->SoundSourcePath = sourcePath;

		DeserialiseSoundDecodedInformation( soundSpec->DecodedInformation, stream );

		// Set rAsset reference to point to our new SoundSpecification. 
		rAsset = soundSpec;

		return true;
#else
		SAT_CORE_ASSERT( false, "RawSoundSpecAssetSerialiser::TryLoadData must be called on Dist!" );
		return false;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// TEXTURE SOURCE

	bool RawTextureSourceAssetSerialiser::TryLoadData( Ref<Asset>& rAsset ) const
	{
		auto textureSourceAsset = Ref<TextureSourceAsset>::Create();
		textureSourceAsset->Path = rAsset->Path;
		textureSourceAsset->ID = rAsset->ID;
		textureSourceAsset->Name = rAsset->Name;
		textureSourceAsset->Flags = rAsset->Flags;
		textureSourceAsset->Type = rAsset->Type;

		textureSourceAsset->ReadFromVFS();

		rAsset = textureSourceAsset;

		return true;
	}

	bool RawTextureSourceAssetSerialiser::DumpAndWriteToVFS( const Ref<Asset>& rAsset ) const
	{
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	// BEHAVIOUR TREE MEMORY SPECIFICATION SERIALISER

	bool RawBehaviourTreeMemorySpecSerialiser::TryLoadData( Ref<Asset>& rAsset ) const
	{
		const std::string& rMountBase = Project::GetActiveConfig().Name;
		Ref<VFile> file = VirtualFS::Get().FindFile( rMountBase, rAsset->Path );

		if( !file )
			return false;

		/////////////////////////////////////

		PakFileMemoryBuffer membuf( file->FileContent );
		std::istream stream( &membuf );

		auto btMemSpecAsset = Ref<BehaviourTreeMemorySpecification>::Create( rAsset );

		size_t mapSize = 0llu;
		RawSerialisation::ReadObject( mapSize, stream );

		btMemSpecAsset->m_SpecificationData.reserve( mapSize );

		for( size_t i = 0; i < mapSize; ++i )
		{
			SPropertyType dataType = SPropertyType::Unknown;
			UUID variableID = 0;

			RawSerialisation::ReadObject( dataType, stream );
			RawSerialisation::ReadObject( variableID, stream );

			btMemSpecAsset->AddNew( "", dataType, variableID );
		}

		rAsset = btMemSpecAsset;

		return true;
	}

	bool RawBehaviourTreeMemorySpecSerialiser::DumpAndWriteToVFS( const Ref<Asset>& rAsset ) const
	{
		const auto behaviourTreeMemSpec = rAsset.As<BehaviourTreeMemorySpecification>();

		std::filesystem::path out = Project::GetActiveProject()->GetTempDir();
		out /= std::to_string( rAsset->ID );
		out.replace_extension( ".vfs" );

		std::ofstream stream( out, std::ios::binary | std::ios::trunc );

		const auto& rKeySpecs = behaviourTreeMemSpec->GetKeySpecs();

		RawSerialisation::WriteObject( rKeySpecs.size(), stream );
		
		for( const auto& rData : rKeySpecs )
		{
			RawSerialisation::WriteObject( ( const std::underlying_type_t<SPropertyType> )rData->DataType, stream );
			RawSerialisation::WriteObject( ( uint64_t )rData->VariableID, stream );
		}

		stream.close();

		return true;
	}

}
