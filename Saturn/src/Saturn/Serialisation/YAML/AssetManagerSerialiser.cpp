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
#include "AssetManagerSerialiser.h"

#include "Saturn/Project/Project.h"

#include "Saturn/Core/VirtualFS.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

namespace YAML {

	template <>
	struct convert<std::filesystem::path>
	{
		static Node encode( std::filesystem::path rhs )
		{
			return Node( rhs.string() );
		}

		static bool decode( const Node& node, std::filesystem::path& rhs )
		{
			rhs = node.as<std::string>();

			return true;
		}
	};

	inline Emitter& operator<<( Emitter& emitter, const std::filesystem::path& v )
	{
		return emitter.Write( v.string() );
	}
}

namespace Saturn {

	void AssetManagerSerialiser::Serialise()
	{
		YAML::Emitter out;

		auto& rAssetMap = AssetManager::Get().GetAssetRegistry()->GetAssetMap();

		out << YAML::BeginMap;

		out << YAML::Key << "Assets";

		out << YAML::BeginSeq;

		for( const auto& [id, asset] : rAssetMap )
		{
			out << YAML::BeginMap;

			out << YAML::Key << "Asset" << YAML::Value << id;

			// On Windows serialise as a Linux path for Linux support 
#if defined(SAT_PLATFORM_WINDOWS)
			std::wstring path = asset->Path.wstring();

			std::replace( path.begin(), path.end(), L'\\', L'/' );

			out << YAML::Key << "Path" << YAML::Value << path;
#else
			out << YAML::Key << "Path" << YAML::Value << asset->GetPath();
#endif

			out << YAML::Key << "Type" << YAML::Value << AssetTypeToString( asset->Type );

			out << YAML::Key << "Version" << YAML::Value << asset->Version;

			out << YAML::EndMap;
		}

		out << YAML::EndSeq;

		out << YAML::Key << "Asset Dependencies";

		out << YAML::BeginSeq;

		for( const auto& [id, dependencies] : AssetManager::Get().GetPureAssetDependencies() )
		{
			out << YAML::BeginMap;

			out << YAML::Key << "AssetID" << YAML::Value << id;

			out << YAML::Key << "Dependencies";
			
			out << YAML::BeginSeq;
			
			for( const auto& rDependencyID : dependencies )
			{
				out << YAML::BeginMap;
				out << YAML::Key << "Dependency" << YAML::Value << rDependencyID;
				out << YAML::EndMap;
			}

			out << YAML::EndSeq;

			out << YAML::EndMap;
		}

		out << YAML::EndSeq;

		out << YAML::EndMap;

		std::ofstream stream( AssetManager::Get().GetAssetRegistry()->GetPath() );
		stream << out.c_str();
	}

	void AssetManagerSerialiser::Deserialise()
	{
		std::ifstream FileIn( AssetManager::Get().GetAssetRegistry()->GetPath() );
		std::stringstream ss;
		ss << FileIn.rdbuf();

		YAML::Node data = YAML::Load( ss.str() );

		auto assets = data[ "Assets" ];

		if( assets.IsNull() )
			return;

		Ref<AssetRegistry>& rAssetRegistry = AssetManager::Get().GetAssetRegistry();

		bool differingAssetVersions = false;
		for( auto asset : assets )
		{
			UUID assetID = asset[ "Asset" ].as< uint64_t >();

			auto path = asset[ "Path" ].as< std::filesystem::path >();
			auto type = asset[ "Type" ].as< std::string >();

			// Fallback to newest version if no version is present.
			auto version = asset[ "Version" ].as< uint32_t >( SAT_CURRENT_VERSION );

			rAssetRegistry->AddAsset( assetID );

			Ref<Asset> DeserialisedAsset = rAssetRegistry->FindAsset( assetID );

#if defined(SAT_PLATFORM_WINDOWS)
			std::wstring windowsPath = path.wstring();

			std::replace( windowsPath.begin(), windowsPath.end(), L'/', L'\\' );

			DeserialisedAsset->Path = windowsPath;
#else
			DeserialisedAsset->Path = path;
#endif

			DeserialisedAsset->Name = path.stem().string();
			DeserialisedAsset->Type = AssetTypeFromString( type );
			DeserialisedAsset->Version = version;

			if( version != SAT_CURRENT_VERSION )
			{
				std::string versionString = SAT_CURRENT_VERSION_STRING;

				std::string assetVersionString;
				SAT_DECODE_VER_STRING( version, assetVersionString );

				SAT_CORE_WARN( "Asset \"{0}\" was created in a different version (asset version was {1}) Saturn version is {2}", DeserialisedAsset->Name, assetVersionString, versionString );

				differingAssetVersions |= true;
			}
		}

		if( differingAssetVersions )
		{
			SAT_CORE_WARN( "In order to fix differing versions go to \"Project->Upgrade Assets\" in the editor title bar." );
		}

		auto assetDependencies = data[ "Asset Dependencies" ];
		if( !assetDependencies.IsNull() )
		{
			for( auto assetDep : assetDependencies )
			{
				UUID depID = assetDep[ "AssetID" ].as< uint64_t >();

				auto deps = assetDep[ "Dependencies" ];

				for( auto assetDep : deps )
				{
					UUID dependsOn = assetDep[ "Dependency" ].as< uint64_t >();

					AssetManager::Get().RegisterAssetDependency( depID, dependsOn );
				}
			}
		}
	}

}
