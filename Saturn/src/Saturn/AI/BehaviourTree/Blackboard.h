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

#include "BlackboardSpecificationAsset.h"

#include <variant>

namespace Saturn {

	// 
	// BlackboardVariable
	// 
	// A BlackboardVariable holds the value in place on the stack and owns it.
	// 
	// Different to a NodeEditorVariable because a node does not hold the value in place.
	//
	class BlackboardVariable : public RefTarget
	{
	public:
		BlackboardVariable( UUID variableID, NodeEditorVariableDataType dataType )
			: m_VariableID( variableID ), m_DataType( dataType )
		{
		}

		virtual ~BlackboardVariable() = default;

		template<typename CppType>
		typename const CppType Get() const
		{
			if( HoldsAnyValue() )
				return std::get< CppType >( m_Value );

			return CppType{};
		}

		template<typename CppType>
		void Set( CppType value );

		[[nodiscard]] inline bool HoldsAnyValue() const
		{
			return !std::holds_alternative<std::monostate>( m_Value );
		}

		inline void ClearValue()
		{
			m_Value = std::monostate{};
		}

	public:
		UUID GetID() const { return m_VariableID; }
		NodeEditorVariableDataType GetType() const { return m_DataType; }

	private:
		UUID m_VariableID;
		NodeEditorVariableDataType m_DataType = NodeEditorVariableDataType::Unknown;

		// The value itself.
		NodeEditorVarTypeSafeUnion m_Value;
	};

#define SAT_BEHAVIOUR_TREE_MEM_VAR_CREATE_SET_FN( CppType )				\
template<>																\
inline void BlackboardVariable::Set<CppType>( CppType val )				\
{																		\
	m_Value = val;														\
}																		\

	SAT_BEHAVIOUR_TREE_MEM_VAR_CREATE_SET_FN( int );
	SAT_BEHAVIOUR_TREE_MEM_VAR_CREATE_SET_FN( float );
	SAT_BEHAVIOUR_TREE_MEM_VAR_CREATE_SET_FN( uint8_t );
	SAT_BEHAVIOUR_TREE_MEM_VAR_CREATE_SET_FN( uint64_t );
	SAT_BEHAVIOUR_TREE_MEM_VAR_CREATE_SET_FN( glm::vec2 );
	SAT_BEHAVIOUR_TREE_MEM_VAR_CREATE_SET_FN( glm::vec3 );

	//
	// Blackboard
	// 
	// Represents the Blackboard at runtime.
	// 
	// A Blackboard is based from the specification asset.
	//
	class Blackboard : public RefTarget
	{
	public:
		Blackboard();
		virtual ~Blackboard();

		void InitialiseVariables( AssetID id );
		[[nodiscard]] bool ContainsVariable( const std::string& rName ) const;
	
	public:
		// Try get the BlackboardVariable
		// Not the same as GetKeyValue<>() as that returns the value stored in the Key
		// This function returns the key itself.
		inline Ref<BlackboardVariable> GetKey( UUID id ) const
		{
			const auto itr = std::find_if( m_Data.begin(), m_Data.end(), [ id ]( const auto& rItem )
			{
				return rItem.second->GetID() == id;
			} );

			return itr == m_Data.end() ? nullptr : itr->second;
		}

		// Attempts to get the blackboard key value associated with the ID.
		// If the key does not exist it will return std::nullopt.
		//		
		// NOTE: The data stored in a BlackboardVariable isn't optional, there always is a value even if its std::monostate
		//       However, the BlackboardVariable itself is optional because it may not exist in our map so it communicate this clearly std::optional is used.
		template<typename CppType>
		inline std::optional<CppType> GetKeyValue( UUID id ) const
		{
			const auto itr = std::find_if( m_Data.begin(), m_Data.end(), [ id ]( const auto& rItem )
			{
				return rItem.second->GetID() == id;
			} );

			if( itr != m_Data.end() )
			{
				return itr->second->Get<CppType>();
			}

			return std::nullopt;
		}

		template<typename TCppType>
		inline void Set( const TCppType& newValue, UUID varID )
		{
			auto itr = std::find_if( m_Data.begin(), m_Data.end(), [ varID ]( const auto& rItem )
			{
				return rItem.second->GetID() == varID;
			} );

			if( itr != m_Data.end() )
			{
				itr->second->Set<TCppType>( newValue );
			}
		}

	private:
		//					NAME      ->	VARIABLE
		std::unordered_map<std::string, Ref<BlackboardVariable>> m_Data;
	};
	
}
