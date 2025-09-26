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

#include "Saturn/GameFramework/SClass.h"
#include "Saturn/Serialisation/Raw/RawSerialisation.h"
#include "Saturn/Core/UUID.h"

#include <glm/glm.hpp>
#include <variant>

namespace Saturn {

	enum class NodeEditorVariableDataType
	{
		Float, Int, Double, Bool, Vec2, Vec3, Vec4, Class, String, Unknown
	};

	inline std::string NodeEditorVariableDataTypeToString( NodeEditorVariableDataType type )
	{
		switch( type )
		{
			case NodeEditorVariableDataType::Float:
				return "Float";
			case NodeEditorVariableDataType::Int:
				return "Int32";
			case NodeEditorVariableDataType::Double:
				return "Double";
			case NodeEditorVariableDataType::Bool:
				return "Bool";
			case NodeEditorVariableDataType::Vec2:
				return "Vector2";
			case NodeEditorVariableDataType::Vec3:
				return "Vector3";
			case NodeEditorVariableDataType::Vec4:
				return "Vector4";
			case NodeEditorVariableDataType::Class:
				return "Class";
			case NodeEditorVariableDataType::String:
				return "String";
		
			case NodeEditorVariableDataType::Unknown:
			default:
				return "Unknown Data Type";
		}
	}

	using NodeEditorVariableTypes = std::variant<
		std::monostate, // null state!
		float,
		int,
		double, 
		bool, 
		glm::vec2,
		glm::vec3,
		glm::vec4,
		SClass*,
		std::string>;

	class NodeEditorVariable : public RefTarget
	{
	public:
		NodeEditorVariable() = default;

		NodeEditorVariable( NodeEditorVariableDataType variableType ) 
			: m_DataType( variableType )
		{
		}

		template<typename TCppType>
		typename const TCppType Get() const 
		{
			if( HoldsProperType() )
				return std::get<TCppType>( m_Value );

			return TCppType{};
		}

		template<typename TCppType>
		void Set( TCppType value );

		[[nodiscard]] inline bool HoldsProperType() const 
		{
			return !std::holds_alternative<std::monostate>( m_Value );
		}

	public:
		static void Serialise( const Ref<NodeEditorVariable>& rObject, std::ofstream& rStream );
		static void Deserialise( Ref<NodeEditorVariable>& rObject, FDependentIStream& rStream );

	public:
		NodeEditorVariableDataType GetType() const { return m_DataType; }
		Saturn::UUID GetUUID() const { return m_VariableID; }
		const std::string& GetName() const { return m_Name; }

	private:
		NodeEditorVariableDataType m_DataType = NodeEditorVariableDataType::Unknown;
		Saturn::UUID m_VariableID;
		NodeEditorVariableTypes m_Value;
		std::string m_Name;

	private:
		friend class NodeEditor;
	};

#define SAT_NODE_EDITOR_VAR_CREATE_SET_FN( TCppType )					\
template<>																\
inline void NodeEditorVariable::Set<TCppType>( TCppType val )			\
{																		\
m_Value = val;															\
}	

	SAT_NODE_EDITOR_VAR_CREATE_SET_FN( float );
	SAT_NODE_EDITOR_VAR_CREATE_SET_FN( int );
	SAT_NODE_EDITOR_VAR_CREATE_SET_FN( double );
	SAT_NODE_EDITOR_VAR_CREATE_SET_FN( bool );
	SAT_NODE_EDITOR_VAR_CREATE_SET_FN( glm::vec2 );
	SAT_NODE_EDITOR_VAR_CREATE_SET_FN( glm::vec3 );
	SAT_NODE_EDITOR_VAR_CREATE_SET_FN( glm::vec4 );
}
