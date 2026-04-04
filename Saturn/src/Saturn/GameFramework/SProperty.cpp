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
#include "SProperty.h"

#include "SObject.h"

#include "Saturn/Asset/Asset.h"
#include "Saturn/Scene/Entity.h"
#include "Saturn/Serialisation/Raw/RawSerialisation.h"

namespace Saturn {

	void SProperty::RtCopyFromOther( const SObject* pSrcObject, SObject* pDstObject ) const
	{
		/*
#define SAT_HANDLE_TYPE( PropertyType ) \
{\
typename PropertyTypeTraits<Saturn::SPropertyType::PropertyType>::Type value = Read<Saturn::SPropertyType::PropertyType>( pSrcObject ); \
SetPropertyInternal<typename PropertyTypeTraits<Saturn::SPropertyType::PropertyType>::Type>( pDstObject, value ); \
} break

		switch( m_Type )
		{
			case Saturn::SPropertyType::Float:
				SAT_HANDLE_TYPE( Float );

			case Saturn::SPropertyType::Int32:
				SAT_HANDLE_TYPE( Int32 );

			case Saturn::SPropertyType::Double:
				SAT_HANDLE_TYPE( Double );

			case Saturn::SPropertyType::Uint8:
				SAT_HANDLE_TYPE( Uint8 );

			case Saturn::SPropertyType::Uint16:
				SAT_HANDLE_TYPE( Uint16 );

			case Saturn::SPropertyType::Uint32:
				SAT_HANDLE_TYPE( Uint32 );

			case Saturn::SPropertyType::Uint64:
				SAT_HANDLE_TYPE( Uint64 );

			case Saturn::SPropertyType::Int8:
				SAT_HANDLE_TYPE( Int8 );

			case Saturn::SPropertyType::Int16:
				SAT_HANDLE_TYPE( Int16 );

			case Saturn::SPropertyType::Int64:
				SAT_HANDLE_TYPE( Int64 );

			case Saturn::SPropertyType::Vector2:
			{
				const glm::vec2& rValue = Read<Saturn::SPropertyType::Vector2>( pSrcObject );
				SetPropertyInternal<const glm::vec2&>( pDstObject, rValue );
			} break;

			case Saturn::SPropertyType::Vector3:
			{
				const glm::vec3& rValue = Read<Saturn::SPropertyType::Vector3>( pSrcObject );
				SetPropertyInternal<const glm::vec3&>( pDstObject, rValue );
			} break;

			case Saturn::SPropertyType::Vector4:
			{
				const glm::vec4& rValue = Read<Saturn::SPropertyType::Vector4>( pSrcObject );
				SetPropertyInternal<const glm::vec4&>( pDstObject, rValue );
			} break;

			case Saturn::SPropertyType::Asset:
			{
				const uint64_t value = Read<Saturn::SPropertyType::Asset>( pSrcObject );
				SetPropertyInternal<AssetID>( pDstObject, value );
			} break;

			case Saturn::SPropertyType::EntityType:
			case Saturn::SPropertyType::String:
			case Saturn::SPropertyType::Class:
			case Saturn::SPropertyType::Unknown:
			default:
				break;
		}

		*/
	}

	void SProperty::Serialise( const SObject* pObject, std::ofstream& rStream ) const
	{
		/*
#define SAT_SERIALISE_PROPERTY( PropertyType ) \
{ \
typename PropertyTypeTraits<Saturn::SPropertyType::PropertyType>::Type value = Read<Saturn::SPropertyType::PropertyType>( pObject ); \
RawSerialisation::WriteObject( value, rStream ); \
} break

		// Referring to YamlAux -- SProperty (YamlAux.cpp)
		RawSerialisation::WriteObject( ( int ) m_Type, rStream );

		switch( m_Type )
		{
			case Saturn::SPropertyType::Float:
				SAT_SERIALISE_PROPERTY( Float );

			case Saturn::SPropertyType::Int32:
				SAT_SERIALISE_PROPERTY( Int32 );

			case Saturn::SPropertyType::Double:
				SAT_SERIALISE_PROPERTY( Double );

			case Saturn::SPropertyType::Uint8:
				SAT_SERIALISE_PROPERTY( Uint8 );

			case Saturn::SPropertyType::Uint16:
				SAT_SERIALISE_PROPERTY( Uint16 );

			case Saturn::SPropertyType::Uint32:
				SAT_SERIALISE_PROPERTY( Uint32 );

			case Saturn::SPropertyType::Uint64:
				SAT_SERIALISE_PROPERTY( Uint64 );

			case Saturn::SPropertyType::Int8:
				SAT_SERIALISE_PROPERTY( Int8 );

			case Saturn::SPropertyType::Int16:
				SAT_SERIALISE_PROPERTY( Int16 );

			case Saturn::SPropertyType::Int64:
				SAT_SERIALISE_PROPERTY( Int64 );

			case Saturn::SPropertyType::Vector2: 
			{
				const glm::vec2& value = Read<Saturn::SPropertyType::Vector2>( pObject );
				RawSerialisation::WriteVec2( value, rStream );
			} break;

			case Saturn::SPropertyType::Vector3:
			{
				const glm::vec3& value = Read<Saturn::SPropertyType::Vector3>( pObject );
				RawSerialisation::WriteVec3( value, rStream );
			} break;

			case Saturn::SPropertyType::Vector4:
			{
				const glm::vec4& value = Read<Saturn::SPropertyType::Vector4>( pObject );
				RawSerialisation::WriteVec4( value, rStream );
			} break;

			case Saturn::SPropertyType::String:
			{
				const std::string& rValue = Read<Saturn::SPropertyType::String>( pObject );

				RawSerialisation::WriteString( rValue, rStream );
			} break;

			case Saturn::SPropertyType::EntityType:
			{
				//SharedPtr<Entity>& rEntity = Read<Saturn::SPropertyType::Entity>( pObject );

				//if( rEntity )
				//{
				//	RawSerialisation::WriteObject( rEntity->GetUUID(), rStream );
				//}
			} break;

			case Saturn::SPropertyType::Asset:
			{
				uint64_t id = Read<Saturn::SPropertyType::Asset>( pObject );
				RawSerialisation::WriteObject( id, rStream );
			} break;

			case Saturn::SPropertyType::Class:
			case Saturn::SPropertyType::Unknown:
			default:
				break;
		}
		*/
	}

	void SProperty::Deserialise( SObject* pObject, std::istream& rStream )
	{
		/*
#define SAT_DESERIALISE_PROPERTY( PropertyType ) \
{ \
typename PropertyTypeTraits<Saturn::SPropertyType::PropertyType>::Type value{}; \
RawSerialisation::ReadObject( value, rStream ); \
\
SetProperty( pObject, value );\
} break

		// Referring to YamlAux -- SProperty (YamlAux.cpp)
		int type = 0;
		RawSerialisation::ReadObject( type, rStream );

		SAT_CORE_VERIFY( m_Type == (SPropertyType)type, "SPROPERTY MISMATCH! Property loaded from the ScriptComponent at the same index does not match with the module property data type." );

		switch( m_Type )
		{
			case Saturn::SPropertyType::Float:
				SAT_DESERIALISE_PROPERTY( Float );

			case Saturn::SPropertyType::Int32:
				SAT_DESERIALISE_PROPERTY( Int32 );

			case Saturn::SPropertyType::Double:
				SAT_DESERIALISE_PROPERTY( Double );

			case Saturn::SPropertyType::Uint8:
				SAT_DESERIALISE_PROPERTY( Uint8 );

			case Saturn::SPropertyType::Uint16:
				SAT_DESERIALISE_PROPERTY( Uint16 );

			case Saturn::SPropertyType::Uint32:
				SAT_DESERIALISE_PROPERTY( Uint32 );

			case Saturn::SPropertyType::Uint64:
				SAT_DESERIALISE_PROPERTY( Uint64 );

			case Saturn::SPropertyType::Int8:
				SAT_DESERIALISE_PROPERTY( Int8 );

			case Saturn::SPropertyType::Int16:
				SAT_DESERIALISE_PROPERTY( Int16 );

			case Saturn::SPropertyType::Int64:
				SAT_DESERIALISE_PROPERTY( Int64 );

			case Saturn::SPropertyType::Vector2: 
			{
				glm::vec2 rValue{};
				RawSerialisation::ReadVec2( rValue, rStream );
				
				SetProperty<const glm::vec2&>( pObject, rValue );
			} break;

			case Saturn::SPropertyType::Vector3:
			{
				glm::vec3 rValue{};
				RawSerialisation::ReadVec3( rValue, rStream );

				SetProperty<const glm::vec3&>( pObject, rValue );
			} break;

			case Saturn::SPropertyType::Vector4:
			{
				glm::vec4 rValue{};
				RawSerialisation::ReadVec4( rValue, rStream );

				SetProperty<const glm::vec4&>( pObject, rValue );
			} break;

			case Saturn::SPropertyType::String:
			{
				const std::string& rValue = RawSerialisation::ReadString( rStream );
				SetProperty( pObject, rValue );
			} break;

			case Saturn::SPropertyType::Asset:
			{
				uint64_t id = 0;
				RawSerialisation::ReadObject( id, rStream );

				SetProperty( pObject, id );
			} break;

			case Saturn::SPropertyType::EntityType:
			case Saturn::SPropertyType::Class:
			case Saturn::SPropertyType::Unknown:
			default:
				break;
		}
		*/
	}

}
