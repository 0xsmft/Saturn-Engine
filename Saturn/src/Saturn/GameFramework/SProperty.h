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

#pragma once

#include "Saturn/Core/Base.h"

#include <string>
#include <glm/glm.hpp>

namespace Saturn {

	class SClass;

	enum SPropertyFlags_
	{
		SPropertyFlags_None = BIT( 0 ),
		SPropertyFlags_ReadOnlyInEditor = BIT( 1 ), // NOTE: ReadOnlyInEditor is only available with the editor
		SPropertyFlags_Asset = BIT( 2 ),
		SPropertyFlags_Serialised = BIT( 3 )
	};

	typedef int SPropertyFlags;

	enum class SPropertyType
	{
		Char,
		Float,
		Int,
		Double,
		Uint8,
		Uint16,
		Uint32,
		Uint64,
		Int8,
		Int16,
		//Int32, /* same as int*/
		Int64,
		Vector2, /* glm::vec3 */
		Vector3, /* glm::vec2 */
		Vector4, /* glm::vec4 */
		String, /* std::string */
		AssetHandle,
		Class,
		Unknown
	};

	//////////////////////////////////////////////////////////////////////////
	// FUNCTION POINTERS

	template<typename Ty>
	using SetPropertyFn = void(__stdcall*)( SClass*, Ty );

	template<typename Ty>
	using GetPropertyFn = Ty( __stdcall* )( SClass* );

	template<SPropertyType Type>
	struct PropertyTypeTraits;

#define SAT_CREATE_PROPERTY_TYPE_TRAIT(PropertyType, CppType, IsRef, PerfConstRef) \
template<> struct PropertyTypeTraits<SPropertyType::PropertyType> \
	{  \
		using Value = CppType;  \
		using Reference = CppType&; \
		using ConstReference = const CppType&; \
		\
		using NeedsToBeReference = std::bool_constant<IsRef>; \
		using PerferConstReference = std::bool_constant<PerfConstRef>; \
		using Type = std::conditional_t<NeedsToBeReference::value, std::conditional_t<PerferConstReference::value, ConstReference, Reference>, std::conditional_t<PerferConstReference::value, ConstReference, Value>>; \
	}

	//								SType    Type         IsRef  PerfConstRef 
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Char,    char,        false, false );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Double,  double,      false, false );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Int,     int,         false, false );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Float,   float,       false, false );

	SAT_CREATE_PROPERTY_TYPE_TRAIT( Uint8,   uint8_t ,    false, false );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Uint16,  uint16_t,    false, false );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Uint32,  uint32_t,    false, false );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Uint64,  uint64_t,    false, false );

	SAT_CREATE_PROPERTY_TYPE_TRAIT( Int8,    int8_t,      false, false );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Int16,   int16_t,     false, false );
	//SAT_CREATE_PROPERTY_TYPE_TRAIT( Int32, int32_t,     false, false ); -- same as int
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Int64,   int64_t,     false, false );

	SAT_CREATE_PROPERTY_TYPE_TRAIT( Vector2, glm::vec2,   true, true );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Vector3, glm::vec3,   true, true );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Vector4, glm::vec4,   true, true );

	SAT_CREATE_PROPERTY_TYPE_TRAIT( String,  std::string, true, true ); 

	SAT_CREATE_PROPERTY_TYPE_TRAIT( Class,  SClass*,      false, false );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Unknown, void*,       false, true );

	// Where Ty is the cpp type i.e. float, int etc
	template<typename Ty>
	void ModifyPropertyInternal( SClass* pClass, const void* fnp, Ty value )
	{
		auto func = reinterpret_cast< SetPropertyFn<Ty> >( fnp );
		( func ) ( pClass, value );
	}

	// Where Ty is the cpp type i.e. float, int etc
	template<typename Ty>
	Ty ReadPropertyInternal( SClass* pClass, const void* fnp )
	{
		auto func = reinterpret_cast< GetPropertyFn<Ty> >( fnp );
		return ( func ) ( pClass );
	}

	//////////////////////////////////////////////////////////////////////////

	class SProperty
	{
	public:
		SProperty( const std::string& rName, const std::string& rNativeType, SPropertyType PropType )
			: m_Name( rName ), m_NativeType( rNativeType ), m_Type( PropType )
		{
		}

		SProperty() = default;
		~SProperty() = default;
	
	public:
		template<typename CppType>
		void SetProperty( SClass* pClass, CppType value )
		{
			// SPropertyFlags_ReadOnlyInEditor is only available with the editor
#if !defined(SAT_DIST)
			// SPropertyFlags_ReadOnlyInEditor will be defined in Dist builds however will never be set by the Header Tool so there is no need to check.
			if( IsFlagSet( SPropertyFlags_ReadOnlyInEditor ) ) 
			{
				// Unable to modify read only property!
				return;
			}
#endif

			// Convert cpp type to SPropertyType
			// TODO: Check if CppType is the same as our current type
			ModifyPropertyInternal<CppType>( pClass, pSetPropertyFunction, value );

#if !defined(SAT_DIST)
			m_Modified = true;
#endif
		}

		template<SPropertyType Ty>
		[[nodiscard]] typename PropertyTypeTraits<Ty>::Type Read( SClass* pClass ) const
		{
			return ReadPropertyInternal<typename PropertyTypeTraits<Ty>::Type>( pClass, pGetPropertyFunction );
		}

		void RtCopyFromOther( SClass* pSrcClass, SClass* pClass );

	public:
		void Serialise( SClass* pClass, std::ofstream& rStream );
		void Deserialise( SClass* pClass, std::ifstream& rStream );

	public:
		void SetFlag( SPropertyFlags flag, bool value );
		[[nodiscard]] bool IsFlagSet( SPropertyFlags flag ) const { return ( m_Flags & ( SPropertyFlags_ ) flag ) != 0; }
		SPropertyFlags GetFlags() { return m_Flags; }

		const std::string& GetName() const { return m_Name; }
		const std::string& GetNativeType() const { return m_NativeType; }

		SPropertyType GetType() const { return m_Type; }

#if defined(SAT_DIST)
		/*[[nodiscard]]*/ bool IsDirty() const { return false; }
#else
	[[nodiscard]] bool IsDirty() const { return m_Modified; }
#endif

		void SetType( SPropertyType type ) { m_Type = type; }
		void SetNativeType( const std::string& rNativeType ) { m_NativeType = rNativeType; }
		void SetName( const std::string& rName ) { m_Name = rName; }

	public:
		const void* pSetPropertyFunction = nullptr;
		const void* pGetPropertyFunction = nullptr;

	private:
		std::string m_Name;
		std::string m_NativeType;

		SPropertyType m_Type = SPropertyType::Unknown;
		SPropertyFlags m_Flags = SPropertyFlags_None;

#if !defined(SAT_DIST)
		bool m_Modified = false;
#endif
	};
}