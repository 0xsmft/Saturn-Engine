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
#include "EngineSettingsSerialiser.h"

#include "Saturn/Core/App.h"

#include <fstream>
#include <yaml-cpp/yaml.h>
#include <filesystem>

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

	// Engine Settings are designed to be per user and have nothing to do with the project
	// So, we will always prefer to use the OS preferred separator
	void EngineSettingsSerialiser::Serialise()
	{
		auto AppDataPath = Application::Get()->GetAppDataFolder();
		auto& rSettings = EngineSettings::Get();

		YAML::Emitter out;

		out << YAML::BeginMap;

		out << YAML::Key << "Startup Project" << YAML::Value << rSettings.StartupProject;

		out << YAML::Key << "Recent Projects";
		
		out << YAML::BeginSeq;
		
		for( auto& rPath : rSettings.m_RecentProjects )
		{
			out << YAML::Key << YAML::Value << rPath;
		}

		out << YAML::EndSeq;
		out << YAML::EndMap;
		
		auto userSettingsPath = AppDataPath / "EngineSettings.yaml";

		std::ofstream file( userSettingsPath );
		file << out.c_str();
	}

	void EngineSettingsSerialiser::Deserialise()
	{
		const auto AppDataPath = Application::Get()->GetAppDataFolder();
		const auto userSettingsPath = AppDataPath / "EngineSettings.yaml";

		std::ifstream FileIn( userSettingsPath );
		std::stringstream ss;
		ss << FileIn.rdbuf();

		YAML::Node data = YAML::Load( ss.str() );

		if( data.IsNull() )
			return;

		auto& rSettings = EngineSettings::Get();
		
		const auto startup = data[ "Startup Project" ];
		if( !startup.IsNull() )
		{
			const std::filesystem::path startupPath = startup.as<std::filesystem::path>();
			if( !startupPath.empty() )
			{
				rSettings.StartupProjectName = startupPath.stem().string();
				rSettings.StartupProject = startupPath;
			}
		}

		const auto recentProjects = data[ "Recent Projects" ];
		for( const auto project : recentProjects )
		{
			const auto path = project.as<std::filesystem::path>();
			
			if( std::filesystem::exists( path ) )
				rSettings.m_RecentProjects.push_back( path );
		}
	}

}
