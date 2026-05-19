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

#include "NodeEditorVariableLocator.h"

#include "Saturn/GameFramework/SClass.h"
#include "Saturn/Serialisation/Raw/RawSerialisationBase.h"
#include "Saturn/Core/UUID.h"

#include <glm/glm.hpp>

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

	//
	// NodeEditorVariable
	// 
	// Represents a user defined "variable" in the NodeEditor.
	// 
	// For example, the user in an AnimGraph may define "IsInAir" as a variable.
	//
	class NodeEditorVariable : public RefTarget
	{
	public:
		NodeEditorVariable() = default;
		NodeEditorVariable( const NodeEditorVariable* pOther ) 
			: m_Name( pOther->m_Name ),
			m_VariableID( pOther->m_VariableID ),
			m_DataType( pOther->m_DataType )
		{
		}

		NodeEditorVariable( NodeEditorVariableDataType variableType ) 
			: m_DataType( variableType )
		{
		}

		template<typename TCppType>
		typename const TCppType Get() const 
		{
			SAT_CORE_ASSERT( CheckTypeSafety() );

			return *( TCppType* ) m_Data.Get();
		}

		template<typename TCppType>
		typename TCppType* GetPtr()
		{
			SAT_CORE_ASSERT( CheckTypeSafety() );

			return ( TCppType* ) m_Data.Get();
		}

		template<typename TCppType>
		void Set( TCppType* pType ) 
		{
			SAT_CORE_ASSERT( CheckTypeSafety() );

			m_Data.Set( pType );
		}

	public:
		static void Serialise( const Ref<NodeEditorVariable>& rObject, std::ofstream& rStream );
		static void Deserialise( Ref<NodeEditorVariable>& rObject, FDependentIStream& rStream );

	public:
		NodeEditorVariableDataType GetType() const { return m_DataType; }
		Saturn::UUID GetUUID() const { return m_VariableID; }
		const std::string& GetName() const { return m_Name; }

	private:
		bool CheckTypeSafety() const { return true; }

	private:
		std::string m_Name;
		NodeEditorVariableLocator m_Data;
		Saturn::UUID m_VariableID;
		NodeEditorVariableDataType m_DataType = NodeEditorVariableDataType::Unknown;

	private:
		friend class NodeEditor;
	};

}
