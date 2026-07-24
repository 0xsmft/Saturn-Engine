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

#include "Saturn/Core/UUID.h"
#include "Saturn/Core/Ref.h"

namespace Saturn {

	using AssetID = UUID;

	enum class AssetType : uint8_t
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
		Skeleton, /* was Script in 0.2.3 below */
		PhysicsMaterial,
		BehaviourTree,
		BehaviourTreeMemory,
		SkeletalAnimation,
		AnimationController,
		Font, // Alura
		StyleProfile, // Alura
		PhysSurfaceRegistry,
		// ^^^ ADD NEW ASSET TYPES HERE ^^^
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
			case Saturn::AssetType::Skeleton:
				return "Skeleton";
			case Saturn::AssetType::PhysicsMaterial:
				return "PhysicsMaterial";
			case Saturn::AssetType::BehaviourTree:
				return "BehaviourTree";
			case Saturn::AssetType::BehaviourTreeMemory:
				return "BehaviourTreeMemory";
			case Saturn::AssetType::SkeletalAnimation:
				return "SkeletalAnimation";
			case Saturn::AssetType::AnimationController:
				return "AnimationController";
			case Saturn::AssetType::Font:
				return "Font";
			case Saturn::AssetType::StyleProfile:
				return "StyleProfile";
			case Saturn::AssetType::PhysSurfaceRegistry:
				return "PhysSurfaceRegistry";

			default:
			case Saturn::AssetType::Unknown:
				return "Unknown";
		}
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
			case Saturn::AssetType::Skeleton:
				return COLOR_32( 5, 183, 237, 255 );
			case Saturn::AssetType::PhysicsMaterial:
				return COLOR_32( 235, 0, 55, 255 );
			case Saturn::AssetType::BehaviourTree:
				return COLOR_32( 50, 168, 82, 255 );
			case Saturn::AssetType::BehaviourTreeMemory:
				return COLOR_32( 168, 50, 82, 255 );
			case Saturn::AssetType::SkeletalAnimation:
				return COLOR_32( 210, 227, 30, 255 );
			case Saturn::AssetType::AnimationController:
				return COLOR_32( 112, 11, 156, 255 );
			case Saturn::AssetType::Font:
				return COLOR_32( 92, 100, 112, 255 );
			case Saturn::AssetType::StyleProfile:
				return COLOR_32( 22, 74, 12, 255 );
			case Saturn::AssetType::PhysSurfaceRegistry:
				return COLOR_32( 84, 33, 58, 255 );

			default:
			case Saturn::AssetType::Unknown:
				return COLOR_32( 255, 255, 255, 255 );
		}
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
		else if( str == "Skeleton" )
			return AssetType::Skeleton;
		else if( str == "PhysicsMaterial" )
			return AssetType::PhysicsMaterial;
		else if( str == "BehaviourTree" )
			return AssetType::BehaviourTree;
		else if( str == "BehaviourTreeMemory" )
			return AssetType::BehaviourTreeMemory;
		else if( str == "SkeletalAnimation" )
			return AssetType::SkeletalAnimation;
		else if( str == "AnimationController" )
			return AssetType::AnimationController;
		else if( str == "Font" )
			return AssetType::Font;
		else if( str == "StyleProfile" )
			return AssetType::StyleProfile;
		else if( str == "PhysSurfaceRegistry" )
			return AssetType::PhysSurfaceRegistry;
		else
			return AssetType::Unknown;
	}

	inline AssetType ExtensionToAssetType( const std::string& str )
	{
		if( str == ".stx" )
			return AssetType::Texture;
		else if( str == ".snd" )
			return AssetType::Sound;
		else if( str == ".gsnd" )
			return AssetType::GraphSound;
		else if( str == ".scene" )
			return AssetType::Scene;
		else if( str == ".smaterial" )
			return AssetType::Material;
		else if( str == ".skel" )
			return AssetType::Skeleton;
		else if( str == ".prefab" )
			return AssetType::Prefab;
		else if( str == ".stmesh" )
			return AssetType::StaticMesh;
		else if( str == ".skmesh" )
			return AssetType::SkeletalMesh;
		else if( str == ".sphymaterial" )
			return AssetType::PhysicsMaterial;
		else if( str == ".sbt" )
			return AssetType::BehaviourTree;
		else if( str == ".sbtm" )
			return AssetType::BehaviourTreeMemory;
		else if( str == ".skanim" )
			return AssetType::SkeletalAnimation;
		else if( str == ".sac" )
			return AssetType::AnimationController;
		else if( str == ".saf" )
			return AssetType::Font;
		else if( str == ".ssp" )
			return AssetType::StyleProfile;
		else if( str == ".spsr" )
			return AssetType::PhysSurfaceRegistry;
		else
			return AssetType::Unknown;
	}

	struct AssetTypeTraits
	{
		uint8_t CanBeReimported : 1 = false;
		uint8_t HasLoadSettings : 1 = false;
	};

	enum class AssetVersion : uint8_t
	{
		BeforeVersionWasAdded,

		// Version added and Asset class size changed
		VersionTypeChangedAndClassSize,

		// add new version above ^^^ and not below vvvv 
		Lowest = BeforeVersionWasAdded,
		Latest = VersionTypeChangedAndClassSize
	};

	class Asset : public RefTarget
	{
	public:
		// The relative path to this Asset.
		// Relative to the project root path, e.g. Assets/Textures/Floor.png
		std::filesystem::path Path;
		std::string Name;

		AssetID ID = 0;
		AssetType Type = AssetType::Unknown;
		AssetVersion Version = AssetVersion::Latest;

	public:
		Asset() = default;
		
		Asset( const Ref<Asset>& rOther ) 
			: ID( rOther->ID ), Type( rOther->Type ), Version( rOther->Version ), Path( rOther->Path ), Name( rOther->Name )
		{
		}

		virtual ~Asset() = default;
	
		// Path must be an absolute path.
		// If you want to set a relative path just modify the 'Path' variable directly and update the name accordingly.
		// NB: This will update the Name as well!
		void SetAbsolutePath( const std::filesystem::path& rPath );

#if !defined(SAT_DIST)
		// Called when this asset is about to be deleted,
		// use this if this asset needs to clean up before its deleted,
		// for example, any type of mesh needs to also delete its source file or for sounds they need to do the same.
		virtual void OnDelete() {}

		// Called when an Asset Dependency needs to be replaced with a new ID.
		virtual void OnAssetDependencyReplace( AssetID oldID, AssetID newID ) {}
#endif

	public:
		//////////////////////////////////////////////////////////////////////////
		// #WARNING This should not be confused with AssetSerialisers. This is for raw binary serialisation! (see: AssetBundle)

		void SerialiseData( std::ofstream& rStream ) const;
		void DeserialiseData( std::ifstream& rStream );

	private:
		friend class AssetManagerSerialiser;
		friend class AssetRegistry;
	};

	class AssetReference
	{
	public:
		AssetID ID = 0;
		Ref<Asset> LoadedAsset = nullptr;

		AssetType ExpectedType = AssetType::Unknown;

		Ref<Asset> operator->() { return LoadedAsset; }
		const Ref<Asset> operator->() const { return LoadedAsset; }
	};
}
