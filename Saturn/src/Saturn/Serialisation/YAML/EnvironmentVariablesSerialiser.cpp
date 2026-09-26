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
#include "EnvironmentVariablesSerialiser.h"

#include "Saturn/Core/App.h"
#include "Saturn/Core/EnvironmentVariablesStorage.h"

#include <yaml-cpp/yaml.h>

namespace Saturn {
    
	void EnvironmentVariablesSerialiser::Serialise()
    {
		const auto path = Application::Get()->GetAppDataFolder() / "EnvironmentVariables.yaml";

		YAML::Emitter out;

		out << YAML::BeginMap;
		out << YAML::Key << "EnvironmentVariables";

		out << YAML::BeginSeq;
		
		for( const auto& [rKey, rPath] : EnvironmentVariablesStorage::Get().m_Variables )
		{
			out << YAML::Key << rKey << YAML::Value << rPath.string();
		}

		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout( path );
		fout << out.c_str();
    }

    void EnvironmentVariablesSerialiser::Deserialise()
    {
		const auto path = Application::Get()->GetAppDataFolder() / "EnvironmentVariables.yaml";

		std::ifstream stream( path );
		std::stringstream ss;
		ss << stream.rdbuf();

		YAML::Node data = YAML::Load( ss.str() );
		if( data.IsNull() )
			return;

		auto& rStorage = EnvironmentVariablesStorage::Get();

		const auto envVars = data[ "EnvironmentVariables" ];
		for( const auto key : envVars )
		{
			//rStorage.m_Variables.emplace( kStr, vStr );
		}
    }

}
