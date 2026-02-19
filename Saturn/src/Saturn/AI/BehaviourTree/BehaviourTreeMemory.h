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

#include "BehaviourTreeMemorySpecification.h"

#include <variant>

namespace Saturn {

	using BehaviourTreeMemoryKeyTypes = std::variant<
		std::monostate,
		double,
		int,
		float,
		uint8_t, uint16_t, uint32_t, uint64_t,
		int8_t, int16_t, int64_t,
		PropertyTypeTraits<SPropertyType::Vector2>::Value,
		PropertyTypeTraits<SPropertyType::Vector3>::Value, 
		PropertyTypeTraits<SPropertyType::Vector4>::Value,
		std::string>;

	class BehaviourTreeMemoryKey : public RefTarget
	{
	public:
		BehaviourTreeMemoryKey( UUID variableID, SPropertyType dataType )
			: m_VariableID( variableID ), m_DataType( dataType )
		{
		}

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
		SPropertyType GetType() const { return m_DataType; }

	private:
		UUID m_VariableID;
		SPropertyType m_DataType;

		BehaviourTreeMemoryKeyTypes m_Value;
	};

#define SAT_BEHAVIOUR_TREE_MEM_VAR_CREATE_SET_FN( CppType )				\
template<>																\
inline void BehaviourTreeMemoryKey::Set<CppType>( CppType val )			\
{																		\
m_Value = val;															\
}																		\

	SAT_BEHAVIOUR_TREE_MEM_VAR_CREATE_SET_FN( int );
	SAT_BEHAVIOUR_TREE_MEM_VAR_CREATE_SET_FN( float );
	SAT_BEHAVIOUR_TREE_MEM_VAR_CREATE_SET_FN( double );

	SAT_BEHAVIOUR_TREE_MEM_VAR_CREATE_SET_FN( uint8_t );
	SAT_BEHAVIOUR_TREE_MEM_VAR_CREATE_SET_FN( uint16_t );
	SAT_BEHAVIOUR_TREE_MEM_VAR_CREATE_SET_FN( uint32_t );
	SAT_BEHAVIOUR_TREE_MEM_VAR_CREATE_SET_FN( uint64_t );

	SAT_BEHAVIOUR_TREE_MEM_VAR_CREATE_SET_FN( int8_t );
	SAT_BEHAVIOUR_TREE_MEM_VAR_CREATE_SET_FN( int16_t );
//	SAT_BEHAVIOUR_TREE_MEM_VAR_CREATE_SET_FN( int32_t );
	SAT_BEHAVIOUR_TREE_MEM_VAR_CREATE_SET_FN( int64_t );

	SAT_BEHAVIOUR_TREE_MEM_VAR_CREATE_SET_FN( glm::vec2 );
	SAT_BEHAVIOUR_TREE_MEM_VAR_CREATE_SET_FN( glm::vec3 );
	SAT_BEHAVIOUR_TREE_MEM_VAR_CREATE_SET_FN( glm::vec4 );

	class BehaviourTreeMemory : public RefTarget
	{
	public:
		BehaviourTreeMemory();
		~BehaviourTreeMemory();

		void InitialiseVariables( AssetID id );
		[[nodiscard]] bool ContainsVariable( const std::string& rName ) const;
	
	public:
		// Try get the BehaviourTreeMemoryKey
		// Not the same as GetKeyValue<>() as that returns the value stored in the Key
		// This function returns the key itself.
		inline Ref<BehaviourTreeMemoryKey> GetKey( UUID id ) const
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
		// NOTE: The data stored in a BehaviourTreeMemoryKey isn't optional, there always is a value even if its std::monostate
		//       However, the BehaviourTreeMemoryKey itself is optional because it may not exist and it communicate this clearly std::optional is used.
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

		template<SPropertyType Type>
		inline void Set( typename PropertyTypeTraits<Type>::Value newValue, UUID varID ) 
		{
			auto itr = std::find_if( m_Data.begin(), m_Data.end(), [ varID ]( const auto& rItem )
			{
				return rItem.second->GetID() == varID;
			} );

			if( itr != m_Data.end() )
			{
				itr->second->Set<typename PropertyTypeTraits<Type>::Value>( newValue );
			}
		}

	private:
		//					NAME      ->	VARIABLE
		std::unordered_map<std::string, Ref<BehaviourTreeMemoryKey>> m_Data;
	};
	
}
