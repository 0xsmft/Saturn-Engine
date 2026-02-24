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
#include "SceneSerialiser.h"

#include "Saturn/Scene/Entity.h"
#include "Saturn/Scene/Components.h"
#include "Saturn/Vulkan/Mesh.h"

#include "Saturn/AI/Navigation/NavBoundsEntity.h"

#include "Saturn/Project/Project.h"

#include "YamlAux.h"
#include "EntitySerialisation.h"

#include <fstream>

#include <yaml-cpp/yaml.h>
#include "yaml-cpp/node/node.h"

namespace Saturn {

	SceneSerialiser::SceneSerialiser( Ref<Scene> scene )
		: m_Scene( scene )
	{
	}

	SceneSerialiser::~SceneSerialiser()
	{
		m_Scene = nullptr;
	}

	void SceneSerialiser::Serialise( const std::filesystem::path& rOverridePath, bool isAutoSave )
	{
		std::filesystem::path basePath = rOverridePath;
		if( rOverridePath.empty() )
			basePath = m_Scene->Path;

		const auto fullPath = Project::GetActiveProject()->FilepathAbs( basePath );

		YAML::Emitter out;
		
		out << YAML::BeginMap;

		out << YAML::Key << "Scene" << YAML::Value << "Untitled Scene";

#if !defined(SAT_DIST)
		const auto& rVisualisation = m_Scene->GetVisualisationOptions();
		out << YAML::Key << "Visualisation" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "ShowGrid" << rVisualisation.ShowGrid;
		out << YAML::Key << "ShowGridRT" << rVisualisation.ShowGridOnRuntime;
		out << YAML::Key << "PhysCollider" << ( std::underlying_type_t<PhysicsColliderVisualisationOptions> )rVisualisation.PhysColliderOptions;
		out << YAML::EndMap;
#endif

		out << YAML::Key << "Entities";

		out << YAML::BeginSeq;
		
		m_Scene->Each( [&]( SharedPtr<Entity> entity ) 
			{
				EntitySerialisation::SerialiseEntity( out, entity );
			} );

		out << YAML::EndSeq;
		out << YAML::EndMap;
		
		std::ofstream fout( fullPath );
		fout << out.c_str();

		// Don't mark auto saves as clean
		if( !isAutoSave )
			m_Scene->CleanDirty();
	}

	void SceneSerialiser::Deserialise( const Ref<Asset> asset )
	{
		const auto fullPath = Project::GetActiveProject()->FilepathAbs( asset->Path );

		std::ifstream stream( fullPath );
		std::stringstream ss;
		ss << stream.rdbuf();

		const YAML::Node data = YAML::Load( ss.str() );

		if( data.IsNull() )
			return;

		if( !data[ "Scene" ] )
			return;

		// Init scene asset fields
		m_Scene->Path = asset->Path;
		m_Scene->Name = asset->Name;
		m_Scene->ID = asset->ID;
		m_Scene->Type = asset->Type;
		m_Scene->Flags = asset->Flags;
		m_Scene->Version = asset->Version;

#if !defined(SAT_DIST)
		const auto visualisationNode = data[ "Visualisation" ];
		if( !visualisationNode.IsNull() )
		{
			auto& rVisualisation = m_Scene->GetVisualisationOptions();

			using U = std::underlying_type_t<PhysicsColliderVisualisationOptions>;

			rVisualisation.ShowGrid          = visualisationNode[ "ShowGrid" ].as<bool>( true );
			rVisualisation.ShowGridOnRuntime = visualisationNode[ "ShowGridRT" ].as<bool>( false );
			rVisualisation.PhysColliderOptions = ( PhysicsColliderVisualisationOptions )visualisationNode[ "PhysCollider" ].as<U>();
		}
#endif

		SAT_CORE_INFO( "Deserialising scene SCENE/0/{0}", asset->Path.stem().string() );
		
		const auto entities = data[ "Entities" ];
		if( entities.IsNull() )
		{
			SAT_CORE_ERROR( "Missing YAML Node \"Entities\"!" );
			stream.close();
		}

		for( const auto entityNode : entities )
		{
			EntitySerialisation::DeserialiseEntity( entityNode, m_Scene );
		}

		m_Scene->PostDeserialise();

		stream.close();
	}

}
