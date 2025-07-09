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

#include <yaml-cpp/yaml.h>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <filesystem>

#include "Saturn/Scene/Entity.h"
#include "Saturn/Scene/Components.h"

namespace YAML {

	template<>
	struct convert<glm::vec2>
	{
		static Node encode( const glm::vec2& rhs )
		{
			Node node;

			node.push_back( rhs.x );
			node.push_back( rhs.y );

			return node;
		}

		static bool decode( const Node& node, glm::vec2& rhs )
		{
			if( !node.IsSequence() || node.size() != 2 )
				return false;

			rhs.x = node[ 0 ].as<float>();
			rhs.y = node[ 1 ].as<float>();

			return true;
		}
	};

	template<>
	struct convert<glm::vec3>
	{
		static Node encode( const glm::vec3& rhs )
		{
			Node node;

			node.push_back( rhs.x );
			node.push_back( rhs.y );
			node.push_back( rhs.z );

			return node;
		}

		static bool decode( const Node& node, glm::vec3& rhs )
		{
			if( !node.IsSequence() || node.size() != 3 )
				return false;

			rhs.x = node[ 0 ].as<float>();
			rhs.y = node[ 1 ].as<float>();
			rhs.z = node[ 2 ].as<float>();

			return true;
		}
	};

	template<>
	struct convert<glm::vec4>
	{
		static Node encode( const glm::vec4& rhs )
		{
			Node node;

			node.push_back( rhs.x );
			node.push_back( rhs.y );
			node.push_back( rhs.z );
			node.push_back( rhs.w );

			return node;
		}

		static bool decode( const Node& node, glm::vec4& rhs )
		{
			if( !node.IsSequence() || node.size() != 4 )
				return false;

			rhs.x = node[ 0 ].as<float>();
			rhs.y = node[ 1 ].as<float>();
			rhs.z = node[ 2 ].as<float>();
			rhs.w = node[ 3 ].as<float>();

			return true;
		}
	};

	template<>
	struct convert<glm::quat>
	{
		static Node encode( const glm::quat& rhs )
		{
			Node node;

			node.push_back( rhs.x );
			node.push_back( rhs.y );
			node.push_back( rhs.z );
			node.push_back( rhs.w );

			return node;
		}

		static bool decode( const Node& node, glm::quat& rhs )
		{
			if( !node.IsSequence() || node.size() != 4 )
				return false;

			rhs.x = node[ 0 ].as<float>();
			rhs.y = node[ 1 ].as<float>();
			rhs.z = node[ 2 ].as<float>();
			rhs.w = node[ 3 ].as<float>();

			return true;
		}
	};

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

	inline Emitter& operator<<( Emitter& out, const glm::vec3& vec )
	{
		out << Flow;
		out << BeginSeq << vec.x << vec.y << vec.z << EndSeq;
		return out;
	}

	inline Emitter& operator<<( Emitter& out, const glm::quat& vec )
	{
		out << Flow;
		out << BeginSeq << vec.x << vec.y << vec.z << vec.w << EndSeq;
		return out;
	}


	inline Emitter& operator<<( Emitter& out, const glm::vec2& vec )
	{
		out << Flow;
		out << BeginSeq << vec.x << vec.y << EndSeq;
		return out;
	}

	inline Emitter& operator<<( Emitter& out, const glm::vec4& vec )
	{
		out << Flow;
		out << BeginSeq << vec.x << vec.y << vec.z << vec.w << EndSeq;
		return out;
	}
}

namespace Saturn::Auxiliary {

	extern void SerialiseEntity( YAML::Emitter& rEmitter, const Ref<Entity> entity );

	extern void DeserialiseEntities( YAML::Node& rNode, Ref<Scene> scene );
}
