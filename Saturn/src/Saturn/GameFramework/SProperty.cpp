/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2024 BEAST                                                           *
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

#include "SClass.h"
#include "Saturn/Scene/Entity.h"
#include "Saturn/Serialisation/RawSerialisation.h"

namespace Saturn {

	void SProperty::RtCopyFromOther( SClass* pSrcClass, SClass* pClass )
	{
#define SAT_HANDLE_TYPE(PropertyType, src, pClass) \
{\
typename PropertyTypeTraits<Saturn::SPropertyType::PropertyType>::Type value = Read<Saturn::SPropertyType::PropertyType>( src ); \
SetProperty( pClass, value ); \
} break

		switch( m_Type )
		{
			case Saturn::SPropertyType::Char:
				SAT_HANDLE_TYPE( Char, pSrcClass, pClass );

			case Saturn::SPropertyType::Float:
				SAT_HANDLE_TYPE( Float, pSrcClass, pClass );

			case Saturn::SPropertyType::Int:
				SAT_HANDLE_TYPE( Int, pSrcClass, pClass );

			case Saturn::SPropertyType::Double:
				SAT_HANDLE_TYPE( Double, pSrcClass, pClass );

			case Saturn::SPropertyType::Uint8:
				SAT_HANDLE_TYPE( Uint8, pSrcClass, pClass );

			case Saturn::SPropertyType::Uint16:
				SAT_HANDLE_TYPE( Uint16, pSrcClass, pClass );

			case Saturn::SPropertyType::Uint32:
				SAT_HANDLE_TYPE( Uint32, pSrcClass, pClass );

			case Saturn::SPropertyType::AssetHandle:
			case Saturn::SPropertyType::Uint64:
				SAT_HANDLE_TYPE( Uint64, pSrcClass, pClass );

			case Saturn::SPropertyType::Int8:
				SAT_HANDLE_TYPE( Int8, pSrcClass, pClass );

			case Saturn::SPropertyType::Int16:
				SAT_HANDLE_TYPE( Int16, pSrcClass, pClass );

			case Saturn::SPropertyType::Int64:
				SAT_HANDLE_TYPE( Int64, pSrcClass, pClass );

			case Saturn::SPropertyType::Vector2:
				SAT_HANDLE_TYPE( Vector2, pSrcClass, pClass );

			case Saturn::SPropertyType::Vector3:
				SAT_HANDLE_TYPE( Vector3, pSrcClass, pClass );

			case Saturn::SPropertyType::Vector4:
				SAT_HANDLE_TYPE( Vector4, pSrcClass, pClass );

			case Saturn::SPropertyType::Entity:
			case Saturn::SPropertyType::String:
			case Saturn::SPropertyType::Class:
			case Saturn::SPropertyType::Unknown:
			default:
				break;
		}
	}

	void SProperty::Serialise( SClass* pClass, std::ofstream& rStream )
	{
#define SAT_SERIALISE_PROPERTY( PropertyType, pClass, rStream ) \
{ \
typename PropertyTypeTraits<Saturn::SPropertyType::PropertyType>::Type value = Read<Saturn::SPropertyType::PropertyType>( pClass ); \
RawSerialisation::WriteObject( value, rStream ); \
} break

		// Referring to YamlAux -- SProperty
		RawSerialisation::WriteString( m_Name, rStream );
		RawSerialisation::WriteObject( (int)m_Type, rStream );

		switch( m_Type )
		{
			case Saturn::SPropertyType::Char:
				SAT_SERIALISE_PROPERTY( Char, pClass, rStream );

			case Saturn::SPropertyType::Float:
				SAT_SERIALISE_PROPERTY( Float, pClass, rStream );

			case Saturn::SPropertyType::Int:
				SAT_SERIALISE_PROPERTY( Int, pClass, rStream );

			case Saturn::SPropertyType::Double:
				SAT_SERIALISE_PROPERTY( Double, pClass, rStream );

			case Saturn::SPropertyType::Uint8:
				SAT_SERIALISE_PROPERTY( Uint8, pClass, rStream );

			case Saturn::SPropertyType::Uint16:
				SAT_SERIALISE_PROPERTY( Uint16, pClass, rStream );

			case Saturn::SPropertyType::Uint32:
				SAT_SERIALISE_PROPERTY( Uint32, pClass, rStream );

			case Saturn::SPropertyType::AssetHandle:
			case Saturn::SPropertyType::Uint64:
				SAT_SERIALISE_PROPERTY( Uint64, pClass, rStream );

			case Saturn::SPropertyType::Int8:
				SAT_SERIALISE_PROPERTY( Int8, pClass, rStream );

			case Saturn::SPropertyType::Int16:
				SAT_SERIALISE_PROPERTY( Int16, pClass, rStream );

			case Saturn::SPropertyType::Int64:
				SAT_SERIALISE_PROPERTY( Int64, pClass, rStream );

			case Saturn::SPropertyType::Vector2:
				SAT_SERIALISE_PROPERTY( Vector2, pClass, rStream );

			case Saturn::SPropertyType::Vector3:
				SAT_SERIALISE_PROPERTY( Vector3, pClass, rStream );

			case Saturn::SPropertyType::Vector4:
				SAT_SERIALISE_PROPERTY( Vector4, pClass, rStream );

			case Saturn::SPropertyType::String:
			{
				const std::string& rValue = Read<Saturn::SPropertyType::String>( pClass );

				RawSerialisation::WriteString( rValue, rStream );
			} break;
			
			case Saturn::SPropertyType::Entity: 
			{
				Ref<Entity>& rEntity = Read<Saturn::SPropertyType::Entity>( pClass );

				RawSerialisation::WriteObject( rEntity->GetUUID(), rStream );
			} break;

			case Saturn::SPropertyType::Class:
			case Saturn::SPropertyType::Unknown:
			default:
				break;
		}
	}

	void SProperty::Deserialise( SClass* pClass, std::ifstream& rStream )
	{
#define SAT_DESERIALISE_PROPERTY( PropertyType, pClass, rStream ) \
{ \
typename PropertyTypeTraits<Saturn::SPropertyType::PropertyType>::Type value{}; \
RawSerialisation::ReadObject( value, rStream ); \
\
SetProperty( pClass, value );\
} break

		// Referring to YamlAux -- SProperty
		const auto& rName = RawSerialisation::ReadString( rStream );
		
		int type = 0;
		RawSerialisation::ReadObject( type, rStream );

		switch( m_Type )
		{
			case Saturn::SPropertyType::Char:
				SAT_DESERIALISE_PROPERTY( Char, pClass, rStream );

			case Saturn::SPropertyType::Float:
				SAT_DESERIALISE_PROPERTY( Float, pClass, rStream );

			case Saturn::SPropertyType::Int:
				SAT_DESERIALISE_PROPERTY( Int, pClass, rStream );

			case Saturn::SPropertyType::Double:
				SAT_DESERIALISE_PROPERTY( Double, pClass, rStream );

			case Saturn::SPropertyType::Uint8:
				SAT_DESERIALISE_PROPERTY( Uint8, pClass, rStream );

			case Saturn::SPropertyType::Uint16:
				SAT_DESERIALISE_PROPERTY( Uint16, pClass, rStream );

			case Saturn::SPropertyType::Uint32:
				SAT_DESERIALISE_PROPERTY( Uint32, pClass, rStream );

			case Saturn::SPropertyType::AssetHandle:
			case Saturn::SPropertyType::Uint64:
				SAT_DESERIALISE_PROPERTY( Uint64, pClass, rStream );

			case Saturn::SPropertyType::Int8:
				SAT_DESERIALISE_PROPERTY( Int8, pClass, rStream );

			case Saturn::SPropertyType::Int16:
				SAT_DESERIALISE_PROPERTY( Int16, pClass, rStream );

			case Saturn::SPropertyType::Int64:
				SAT_DESERIALISE_PROPERTY( Int64, pClass, rStream );

			//case Saturn::SPropertyType::Vector2:
			//	SAT_DESERIALISE_PROPERTY( Vector2, pClass, rStream );

			//case Saturn::SPropertyType::Vector3:
			//	SAT_DESERIALISE_PROPERTY( Vector3, pClass, rStream );

			//case Saturn::SPropertyType::Vector4:
			//	SAT_DESERIALISE_PROPERTY( Vector4, pClass, rStream );

			case Saturn::SPropertyType::String:
			{
				const std::string& rValue = RawSerialisation::ReadString( rStream );
				SetProperty( pClass, rValue );
			} break;

			case Saturn::SPropertyType::Entity:
			case Saturn::SPropertyType::Class:
			case Saturn::SPropertyType::Unknown:
			default:
				break;
		}
	}

	void SProperty::SetFlag( SPropertyFlags flag, bool value )
	{
		if( value )
			m_Flags |= flag;
		else
			m_Flags &= ~flag;
	}

}