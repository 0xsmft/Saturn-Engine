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
#include "BehaviourTreeMemory.h"

#include "Saturn/Asset/AssetManager.h"

namespace Saturn {

	BehaviourTreeMemory::BehaviourTreeMemory()
	{
	}

	BehaviourTreeMemory::~BehaviourTreeMemory()
	{
		m_Data.clear();
	}

	bool BehaviourTreeMemory::ContainsVariable( const std::string& rName ) const
	{
		return m_Data.contains( rName );
	}

	void BehaviourTreeMemory::InitialiseVariables( AssetID id )
	{
		m_Specification = AssetManager::Get().GetAssetAs<BehaviourTreeMemorySpecification>( id );

		for( const auto& rVariable : m_Specification->GetVariableSpecs() )
		{
			m_Data[ rVariable->Name ] = Ref<BehaviourTreeMemoryVariable>::Create( rVariable->VariableID, rVariable->DataType );

			m_Data[ rVariable->Name ]->Init();
		}
	}

	void BehaviourTreeMemoryVariable::Init()
	{
		switch( m_DataType )
		{
			case Saturn::SPropertyType::Char:
			//	Set<char>( 0 );
				break;
			
			case Saturn::SPropertyType::Float:
				Set<float>( 0.0f );
				break;
			
			case Saturn::SPropertyType::Int:
				Set<int>( 0 );
				break;
			
			case Saturn::SPropertyType::Double:
				Set<double>( 0.0 );
				break;
			
			case Saturn::SPropertyType::Uint8:
				Set<uint8_t>( uint8_t( 0 ) );
				break;
			
			case Saturn::SPropertyType::Uint16:
				Set<uint16_t>( uint16_t( 0 ) );
				break;
			
			case Saturn::SPropertyType::Uint32:
				Set<uint32_t>( uint32_t( 0 ) );
				break;
			
			case Saturn::SPropertyType::Uint64:
				Set<uint64_t>( 0llu );
				break;
			
			case Saturn::SPropertyType::Int8:
				Set<uint8_t>( uint8_t( 0 ) );
				break;
			
			case Saturn::SPropertyType::Int16:
				Set<int16_t>( int64_t( 0 ) );
				break;
			
			case Saturn::SPropertyType::Int64:
				Set<int64_t>( 0ll );
				break;
			
			case Saturn::SPropertyType::Vector2:
				Set<glm::vec2>( glm::vec2( 0.0f ) );
				break;
			
			case Saturn::SPropertyType::Vector3:
				Set<glm::vec3>( glm::vec3( 0.0f ) );
				break;
			
			case Saturn::SPropertyType::Vector4:
				Set<glm::vec4>( glm::vec4( 0.0f ) );
				break;
			
			case Saturn::SPropertyType::String:
			case Saturn::SPropertyType::Asset:
			case Saturn::SPropertyType::Entity:
			case Saturn::SPropertyType::Class:
			case Saturn::SPropertyType::Unknown:
			default: break;
		}
	}

}
