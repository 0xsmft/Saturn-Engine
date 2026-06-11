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

#include <glm/glm.hpp>
#include <variant>

namespace Saturn {

	enum class NodeEditorVariableDataType : uint8_t
	{
		Float, Int, ID, Bool, Vec2, Vec3, Vec4, Class, String, Unknown
	};

	inline std::string NodeEditorVariableDataTypeToString( NodeEditorVariableDataType type )
	{
		switch( type )
		{
			case NodeEditorVariableDataType::Float:
				return "Float";
			case NodeEditorVariableDataType::Int:
				return "Int32";
			case NodeEditorVariableDataType::ID:
				return "ID";
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

	// A type safe union of the raw C++ types that a node editor variable can hold.
	// TODO: Class, string
	using NodeEditorVarTypeSafeUnion = std::variant<
		std::monostate,
		float,
		int,
		uint64_t,
		bool,
		glm::vec2,
		glm::vec3,
		glm::vec4>;
	
	//////////////////////////////////////////////////////////////////////////
	// Type traits mapping

	template<typename Ty>
	struct NodeEditorDataTypeTraits;

#define SAT_DEFINE_NODE_EDITOR_VARIABLE_TYPE_TRAITS( EnumType, CppType )	\
template<> struct NodeEditorDataTypeTraits<CppType>							\
{																			\
	using Value = CppType;													\
	static constexpr NodeEditorVariableDataType GetDataType() { return EnumType; } \
}

	// NB: If you get an error here about use of undefined type 'Saturn::NodeEditorDataTypeTraits<TCppType>'
	//	   with TCppType == <MyType>
	//     then you need to add the type to the type traits table below....

	SAT_DEFINE_NODE_EDITOR_VARIABLE_TYPE_TRAITS( NodeEditorVariableDataType::Float, float );
	SAT_DEFINE_NODE_EDITOR_VARIABLE_TYPE_TRAITS( NodeEditorVariableDataType::Int,   int );
	SAT_DEFINE_NODE_EDITOR_VARIABLE_TYPE_TRAITS( NodeEditorVariableDataType::ID,    uint64_t );
	SAT_DEFINE_NODE_EDITOR_VARIABLE_TYPE_TRAITS( NodeEditorVariableDataType::Bool,  bool );
	SAT_DEFINE_NODE_EDITOR_VARIABLE_TYPE_TRAITS( NodeEditorVariableDataType::Vec2,	glm::vec2 );
	SAT_DEFINE_NODE_EDITOR_VARIABLE_TYPE_TRAITS( NodeEditorVariableDataType::Vec3,  glm::vec3 );
	SAT_DEFINE_NODE_EDITOR_VARIABLE_TYPE_TRAITS( NodeEditorVariableDataType::Vec4,  glm::vec4 );

}
