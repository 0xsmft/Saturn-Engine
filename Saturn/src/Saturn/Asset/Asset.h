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

#include "Saturn/Core/App.h"

#include "Saturn/Core/UUID.h"
#include "Saturn/Core/Ref.h"

#include "Saturn/Core/Log.h"
#include "Saturn/Serialisation/RawSerialisation.h"

#include "Saturn/Project/Project.h"

#include <filesystem>

namespace Saturn {

	using AssetID = UUID;

	enum class AssetType
	{
		Texture,
		StaticMesh,
		SkeletalMesh,
		Material,
		MaterialInstance,
		Sound,
		GraphSound,
		Scene,
		Prefab,
		Script,
		PhysicsMaterial,
		Unknown,
		COUNT,
	};

	inline std::string_view AssetTypeToString( AssetType type )
	{
		switch( type )
		{
			case Saturn::AssetType::Texture:
				return "Texture";
			case Saturn::AssetType::StaticMesh:
				return "StaticMesh";
			case Saturn::AssetType::SkeletalMesh:
				return "SkeletalMesh";
			case Saturn::AssetType::Material:
				return "Material";
			case Saturn::AssetType::MaterialInstance:
				return "MaterialInstance";
			case Saturn::AssetType::Sound:
				return "Audio";
			case Saturn::AssetType::GraphSound:
				return "GraphSound";
			case Saturn::AssetType::Scene:
				return "Scene";
			case Saturn::AssetType::Prefab:
				return "Prefab";
			case Saturn::AssetType::Script:
				return "Script";
			case Saturn::AssetType::PhysicsMaterial:
				return "PhysicsMaterial";
			case Saturn::AssetType::Unknown:
				return "Unknown";
		}

		return "Unknown";
	}

	// I don't want to include the imgui header so we will just copy the macro.
	constexpr int R_SHIFT = 0;
	constexpr int G_SHIFT = 8;
	constexpr int B_SHIFT = 16;
	constexpr int A_SHIFT = 24;
	constexpr int A_MASK = 0xFF000000;

	template<typename Ty>
	consteval uint32_t COLOR_32( Ty R, Ty G, Ty B, Ty A ) { return ( ( ( uint32_t ) ( A ) << A_SHIFT ) | ( ( uint32_t ) ( B ) << B_SHIFT ) | ( ( uint32_t ) ( G ) << G_SHIFT ) | ( ( uint32_t ) ( R ) << R_SHIFT ) ); }

	inline constexpr uint32_t AssetTypeToColor( AssetType type )
	{
		switch( type )
		{
			case Saturn::AssetType::Texture:
				return COLOR_32( 160, 118, 249, 255 );
			case Saturn::AssetType::StaticMesh:
				return COLOR_32( 161, 103, 11, 255 );
			case Saturn::AssetType::SkeletalMesh:
				return COLOR_32( 161, 103, 11, 255 );
			case Saturn::AssetType::Material:
				return COLOR_32( 237, 5, 229, 255 );
			case Saturn::AssetType::MaterialInstance:
				return COLOR_32( 237, 5, 229, 255 );
			case Saturn::AssetType::Sound:
				return COLOR_32( 237, 202, 5, 255 );
			case Saturn::AssetType::GraphSound:
				return COLOR_32( 235, 122, 52, 255 );
			case Saturn::AssetType::Scene:
				return COLOR_32( 255, 0, 0, 255 );
			case Saturn::AssetType::Prefab:
				return COLOR_32( 255, 0, 255, 255 );
			case Saturn::AssetType::Script:
				return COLOR_32( 5, 183, 237, 255 );
			case Saturn::AssetType::PhysicsMaterial:
				return COLOR_32( 235, 0, 55, 255 );
			case Saturn::AssetType::Unknown:
				return COLOR_32( 255, 255, 255, 255 );
			default:
				break;
		}

		return 0;
	}

	inline AssetType AssetTypeFromString( const std::string& str )
	{
		if( str == "Texture" )
			return AssetType::Texture;
		else if( str == "StaticMesh" )
			return AssetType::StaticMesh;
		else if( str == "SkeletalMesh" )
			return AssetType::SkeletalMesh;
		else if( str == "Material" )
			return AssetType::Material;
		else if( str == "MaterialInstance" )
			return AssetType::MaterialInstance;
		else if( str == "Audio" )
			return AssetType::Sound;
		else if( str == "GraphSound" )
			return AssetType::GraphSound;
		else if( str == "Scene" )
			return AssetType::Scene;
		else if( str == "Prefab" )
			return AssetType::Prefab;
		else if( str == "Script" )
			return AssetType::Script;
		else if( str == "PhysicsMaterial" )
			return AssetType::PhysicsMaterial;
		else
			return AssetType::Unknown;
	}

	inline AssetType ExtensionToAssetType( const std::string& str )
	{
		if( str == ".png" || str == ".tga" || str == ".jpg" || str == ".jpeg" || str == ".hdr" || str == ".ico" )
			return AssetType::Texture;
		else if( str == ".snd" )
			return AssetType::Sound;
		else if( str == ".gsnd" )
			return AssetType::GraphSound;
		else if( str == ".scene" )
			return AssetType::Scene;
		else if( str == ".smaterial" )
			return AssetType::Material;
		else if( str == ".cpp" || str == ".h" )
			return AssetType::Script;
		else if( str == ".prefab" )
			return AssetType::Prefab;
		else if( str == ".stmesh" )
			return AssetType::StaticMesh;
		else if( str == ".sphymaterial" )
			return AssetType::PhysicsMaterial;
		else
			return AssetType::Unknown;
	}

	class Asset : public RefTarget
	{
	public:
		AssetID ID = 0;
		AssetType Type = AssetType::Unknown;
		uint32_t Flags = 0;
		uint32_t Version = SAT_CURRENT_VERSION;

		// The relative path to this Asset
		std::filesystem::path Path;

		std::string Name;

	public:
		Asset() = default;
		
		Asset( const Ref<Asset>& rOther ) 
			: ID( rOther->ID ), Type( rOther->Type ), Flags( rOther->Flags ), Version( rOther->Version ), Path( rOther->Path ), Name( rOther->Name )
		{
		}

		virtual ~Asset() = default;
	
		// Path must be an absolute path.
		// If you want to set a relative path just modify the 'Path' variable directly and update the name accordingly.
		void SetAbsolutePath( const std::filesystem::path& rPath )
		{
			std::filesystem::path rootDir = Project::GetActiveProject()->GetRootDir();

			Path = std::filesystem::relative( rPath, rootDir );
			Name = Path.stem().string();
		}

	public:
		//////////////////////////////////////////////////////////////////////////
		// #WARNING This should not be confused with AssetSerialisers. This is for raw binary serialisation! (see: AssetBundle)

		void SerialiseData( std::ofstream& rStream ) const
		{
			// TODO: Support writing for a filesystem path.
			RawSerialisation::WriteString( Name, rStream );
			RawSerialisation::WriteString( Path, rStream );

			RawSerialisation::WriteObject( ID, rStream );
			RawSerialisation::WriteObject( Type, rStream );
			RawSerialisation::WriteObject( Flags, rStream );
		}

		void DeserialiseData( std::ifstream& rStream )
		{
			// TODO: Support reading for a filesystem path.
			Name = RawSerialisation::ReadString( rStream );
			Path = RawSerialisation::ReadString( rStream );

			RawSerialisation::ReadObject( ID, rStream );
			RawSerialisation::ReadObject( Type, rStream );
			RawSerialisation::ReadObject( Flags, rStream );
		}

	private:
		friend class AssetManagerSerialiser;
		friend class AssetRegistry;
	};

	class AssetReference
	{
	public:
		AssetID ID = 0;
		AssetType ExpectedType = AssetType::Unknown;

		Ref<Asset> LoadedAsset = nullptr;

		Ref<Asset> operator->() { return LoadedAsset; }
		const Ref<Asset> operator->() const { return LoadedAsset; }
	};
}