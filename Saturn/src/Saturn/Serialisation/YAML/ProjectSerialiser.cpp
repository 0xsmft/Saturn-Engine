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
#include "ProjectSerialiser.h"

#include "YamlAux.h"
#include "Saturn/Audio/AudioSystem.h"

#if defined(SAT_WITH_STEAM)
#include "Saturn/Online/Steam/SteamOnlineSystemAPI.h"
#endif

#include <fstream>
#include <yaml-cpp/yaml.h>

namespace Saturn {

	ProjectSerialiser::ProjectSerialiser( const Ref< Project >& rProject )
		: m_Project( rProject )
	{
	}

	ProjectSerialiser::~ProjectSerialiser()
	{
	}

	void ProjectSerialiser::Serialise()
	{
		const Ref<Project> rProject = Project::GetActiveProject();

		YAML::Emitter out;

		out << YAML::BeginMap;

		out << YAML::Key << "Project" << YAML::Value;

		out << YAML::BeginMap;

		{
			out << YAML::Key << "Name" << YAML::Value << rProject->GetConfig().Name;
			out << YAML::Key << "StartupScene" << YAML::Value << rProject->GetConfig().StartupSceneID;
			out << YAML::Key << "DefaultMaterialAsset" << YAML::Value << rProject->GetDefaultMaterialAsset();
			out << YAML::Key << "DefaultPhyMaterialAsset" << YAML::Value << rProject->GetDefaultPhysicsMaterialAsset();
			out << YAML::Key << "DefaultFontAsset" << YAML::Value << rProject->GetDefaultFontAsset();
			out << YAML::Key << "DefaultPhysRegAsset" << YAML::Value << rProject->GetDefaultPhysRegAsset();
			out << YAML::Key << "ABCompressThreshold" << YAML::Value << rProject->GetCompressionThresholdForAssetBundle();
#if !defined(SAT_DIST)
			out << YAML::Key << "IsAutoSavesEnabled" << YAML::Value << rProject->IsAutoSavesEnabled();
			out << YAML::Key << "AutoSaveInterval" << YAML::Value << rProject->GetAutoSaveInterval();
			out << YAML::Key << "DeveloperVersion" << YAML::Value << rProject->GetDeveloperVersion();
#endif

			out << YAML::Key << "Online" << YAML::Value;
			out << YAML::BeginMap;
			out << YAML::Key << "API" << YAML::Value << ( std::underlying_type_t<OnlineSystemAPIType> )rProject->GetOnlineAPIType();
			out << YAML::Key << "Settings" << YAML::Value;
			out << YAML::BeginMap;

			switch( rProject->GetOnlineAPIType() )
			{
				case OnlineSystemAPIType::Null:
				default:
					out << YAML::Key << "AppID" << 0u;
					break;

#if defined(SAT_WITH_STEAM)
				case OnlineSystemAPIType::Steam:
				{
					out << YAML::Key << "AppID" << YAML::Value << rProject->GetOnlineAppID();
				} break;
#endif
			}

			out << YAML::EndMap;
			out << YAML::EndMap;

			out << YAML::Key << "ActionBindings";
			out << YAML::BeginSeq;

			for( const auto& rBinding : rProject->GetActionBindings() )
			{
				out << YAML::BeginMap;

				out << YAML::Key << "ActionBinding" << YAML::Value << rBinding.Name;

#if !defined(SAT_DIST)
				out << YAML::Key << "ActionName" << YAML::Value << rBinding.ActionName;
				out << YAML::Key << "ID" << YAML::Value << ( uint64_t ) rBinding.ID;
#endif
				out << YAML::Key << "Key" << YAML::Value << ( int ) rBinding.Key;
				out << YAML::Key << "MouseButton" << YAML::Value << ( int ) rBinding.MouseButton;
				out << YAML::Key << "Type" << YAML::Value << ( int ) rBinding.Type;

				out << YAML::EndMap;
			}

			out << YAML::EndSeq;

			out << YAML::Key << "SoundGroups";
			out << YAML::BeginSeq;

			for( const auto& rSoundGroup : rProject->GetSoundGroups() )
			{
				out << YAML::BeginMap;

				out << YAML::Key << "SoundGroup" << YAML::Value << rSoundGroup->GetName();

				out << YAML::Key << "Volume" << YAML::Value << rSoundGroup->GetVolume();
				out << YAML::Key << "Pitch" << YAML::Value << rSoundGroup->GetPitch();

				out << YAML::EndMap;
			}

			out << YAML::EndSeq;
		}

		out << YAML::EndMap;

		out << YAML::EndMap;

		std::ofstream file( rProject->GetConfig().Path );
		file << out.c_str();
	}

	void ProjectSerialiser::Deserialise( const std::filesystem::path& rFilePath )
	{
		std::ifstream FileIn( rFilePath );

		std::stringstream ss;
		ss << FileIn.rdbuf();

		YAML::Node data = YAML::Load( ss.str() );

		if( data.IsNull() )
			return;

		const auto project = data[ "Project" ];

		ProjectConfig newConfig{};

		newConfig.Name = project[ "Name" ].as<std::string>();
		newConfig.StartupSceneID = project[ "StartupScene" ].as<uint64_t>( 0 );
		newConfig.Path = rFilePath;

		// Create project
		Ref<Project> newProject = Ref<Project>::Create( newConfig );

		newProject->SetDefaultMaterialAsset( project[ "DefaultMaterialAsset" ].as<uint64_t>( 0 ) );
		newProject->SetDefaultPhysicsMaterialAsset( project[ "DefaultPhyMaterialAsset" ].as<uint64_t>( 0 ) );
		newProject->SetDefaultFontAsset( project[ "DefaultFontAsset" ].as<uint64_t>( 0 ) );
		newProject->SetDefaultPhysRegAsset( project[ "DefaultPhysRegAsset" ].as<uint64_t>( 0 ) );
		newProject->SetCompressionThresholdForAssetBundle( project[ "ABCompressThreshold" ].as<uint64_t>( 0 ) );

#if !defined(SAT_DIST)
		newProject->EnableAutoSaves( project[ "IsAutoSavesEnabled" ].as<bool>( false ) );
		newProject->SetAutoSaveInterval( project[ "AutoSavesInterval" ].as<float>( 300.0f ) );
		newProject->SetDeveloperVersion( project[ "DeveloperVersion" ].as<std::string>( std::string() ) );
#endif

		const auto online = project[ "Online" ];
		if( online )
		{
			OnlineSystemAPIType savedType = ( OnlineSystemAPIType ) online[ "API" ].as<std::underlying_type<OnlineSystemAPIType>::type>( 0 );
			newProject->SetOnlineSystemAPI( savedType );

			const auto apiSettings = online[ "Settings" ];
			if( apiSettings )
			{
				switch( savedType )
				{
#if defined(SAT_WITH_STEAM)
					case OnlineSystemAPIType::Steam:
					{
						const auto appID = apiSettings[ "AppID" ].as<uint32_t>( 0u );
						newProject->SetOnlineAppID( appID );
					} break;
#endif

#if defined(SAT_WITH_EPIC)
					case OnlineSystemAPIType::Epic:
						break;
#endif

					case OnlineSystemAPIType::Null:
					default:
						break;
				}

			}
		}

		const auto actionBindings = project[ "ActionBindings" ];
		if( actionBindings )
		{
			for( const auto& binding : actionBindings )
			{
				ActionBindingData ab;

#if !defined(SAT_DIST)
				ab.ActionName = binding[ "ActionName" ].as<std::string>();
				ab.ID = ( UUID ) binding[ "ID" ].as<uint64_t>();
#endif
				ab.Name = binding[ "ActionBinding" ].as<std::string>();
				ab.Key = ( RubyKey ) binding[ "Key" ].as<int>( 0 );
				ab.MouseButton = ( RubyMouseButton ) binding[ "Key" ].as<int>( 6 );
				ab.Type = ( ActionBindingType ) binding[ "Type" ].as<int>( 0 );

#if !defined(SAT_DIST)
				SAT_CORE_INFO( "Adding new action binding with ID: BINDING/ACTION/{0} ({1})", ab.ID, ab.Name );
#endif

				newProject->AddActionBinding( ab );
			}
		}

#if SAT_FEATURE_SOUND_GROUPS
		AudioSystem::Get().WaitForInit();

		const auto soundGroups = project[ "SoundGroups" ];
		if( soundGroups )
		{
			for( const auto& grp : soundGroups )
			{
				Ref<SoundGroup> sndGrp = Ref<SoundGroup>::Create();
				sndGrp->Init( false );

				sndGrp->SetName( grp[ "SoundGroup" ].as<std::string>() );
				sndGrp->SetVolume( grp[ "Volume" ].as<float>( 1.0f ) );
				sndGrp->SetPitch( grp[ "Pitch" ].as<float>( 1.0f ) );

				newProject->AddSoundGroup( sndGrp );
			}
		}
#endif

		Project::SetActiveProject( newProject );

#if SAT_FEATURE_SOUND_GROUPS
		AudioSystem::Get().StartSoundGroups();
#endif
	}

}
