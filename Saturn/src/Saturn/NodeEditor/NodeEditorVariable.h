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

#include "NodeEditorVariableDataType.h"
#include "NodeEditorVariableLocator.h"

#include "Saturn/Core/UUID.h"

#include "Saturn/Serialisation/Raw/RawSerialisationBase.h"

namespace Saturn {

	//
	// NodeEditorVariable
	// 
	// Represents a user defined "variable" in the NodeEditor.
	// 
	// For example, the user in an AnimGraph may define "IsInAir" as a variable.
	// 
	// A NodeEditorVariable does not hold the value in place. It's a wrapper around a pointer to another variable.
	// 
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

	public:
		template<typename TCppType>
		typename const TCppType Get() const 
		{
			SAT_CORE_ASSERT( CheckTypeSafety<TCppType>() );

			return *( TCppType* ) m_Data.Get();
		}

		template<typename TCppType>
		typename TCppType* GetPtr()
		{
			SAT_CORE_ASSERT( CheckTypeSafety<TCppType>() );

			return ( TCppType* ) m_Data.Get();
		}

		template<typename TCppType>
		void Set( TCppType* pType ) 
		{
			SAT_CORE_ASSERT( CheckTypeSafety<TCppType>() );

			m_Data.Set( pType );
		}

		template<typename TCppType>
		void Set( const TCppType& rValue )
		{
			SAT_CORE_ASSERT( CheckTypeSafety<TCppType>() );

			m_Data.SetValue( rValue );
		}

	public:
		static void Serialise( const Ref<NodeEditorVariable>& rObject, std::ofstream& rStream );
		static void Deserialise( Ref<NodeEditorVariable>& rObject, FDependentIStream& rStream );

	public:
		NodeEditorVariableDataType GetType() const { return m_DataType; }
		Saturn::UUID GetUUID() const { return m_VariableID; }
		const std::string& GetName() const { return m_Name; }

	private:
		// Returns true if we are safe to get/set the type.
		template<typename TCppType>
		bool CheckTypeSafety() const 
		{
			return NodeEditorDataTypeTraits<TCppType>::GetDataType() == m_DataType;
		}

	private:
		std::string m_Name;
		NodeEditorVariableLocator m_Data;
		Saturn::UUID m_VariableID;
		NodeEditorVariableDataType m_DataType = NodeEditorVariableDataType::Unknown;

	private:
		friend class NodeEditor;
	};

}
