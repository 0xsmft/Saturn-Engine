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

#include "Saturn/Core/Base.h"

#include <string>
#include <glm/glm.hpp>

namespace Saturn {

	class SObject;
	class SClass;
	class Entity;

	// If you modify this enum you must ditto to XSP auto completion enum
	enum SPropertyFlags_
	{
		SPropertyFlags_None,

		// NOTE: ReadOnlyInEditor is only available with the editor
		SPropertyFlags_ReadOnlyInEditor = BIT( 0 ),

		// Help out the build tool, if a Saturn type is in the namespace.
		SPropertyFlags_UsingSaturnNamespace = BIT( 1 ), 
	};

	typedef int SPropertyFlags;

	//
	// Valid data types for an SProperty
	//
	enum class SPropertyType
	{
		Double,
		Float,
		Uint8,
		Uint16,
		Uint32,
		Uint64,
		Int8,
		Int16,
		Int32,
		Int64,
		Vector2, /* glm::vec3 */
		Vector3, /* glm::vec2 */
		Vector4, /* glm::vec4 */
		String, /* std::string */
		Asset,
		EntityType, // SharedPtr<Entity>
		Class,
		Unknown
	};

	// NOTE: The build tool has its own function of this 
	// however it is the full enum name (so, SPropertyType::Char instead of Char)
	inline std::string SPropertyTypeToStringInNamespace( SPropertyType type )
	{
		switch( type )
		{
			case SPropertyType::Float: return "Float";
			case SPropertyType::Double: return "Double";
			case SPropertyType::Uint8: return "Uint8";
			case SPropertyType::Uint16: return "Uint16";
			case SPropertyType::Uint32: return "Uint32";
			case SPropertyType::Uint64: return "Uint64";
			case SPropertyType::Int8: return "Int8";
			case SPropertyType::Int16: return "Int16";
			case SPropertyType::Int32: return "Int";
			case SPropertyType::Int64: return "Int64";
			case SPropertyType::Vector2: return "Vector2";
			case SPropertyType::Vector3: return "Vector3";
			case SPropertyType::Vector4: return "Vector4";
			case SPropertyType::String: return "String";
			case SPropertyType::Asset: return "Asset";
			case SPropertyType::EntityType: return "Entity";
			case SPropertyType::Class: return "Class";
			case SPropertyType::Unknown: return "Unknown";

			default: break;
		}

		return "<unhandled>";
	}

	//////////////////////////////////////////////////////////////////////////
	// FUNCTION POINTERS

	template<typename Ty>
	using SetPropertyFn = void( SAT_MSVC_STDCALL* )( SObject*, Ty );

	template<typename Ty>
	using GetPropertyFn = Ty( SAT_MSVC_STDCALL* )( const SObject* );

	template<SPropertyType Type>
	struct PropertyTypeTraits;

#define SAT_CREATE_PROPERTY_TYPE_TRAIT(PropertyType, CppType, IsRef, PrefConstRef) \
template<> struct PropertyTypeTraits<SPropertyType::PropertyType> \
	{  \
		using Value = CppType;  \
		using Reference = CppType&; \
		using Const = const CppType; \
		using ConstReference = const CppType&; \
		\
		using NeedsToBeReference = std::bool_constant<IsRef>; \
		using PreferConstReference = std::bool_constant<PrefConstRef>; \
		using Type = std::conditional_t<NeedsToBeReference::value, std::conditional_t<PreferConstReference::value, ConstReference, Reference>, std::conditional_t<PreferConstReference::value, ConstReference, Value>>; \
	}

	//								SType    Type         IsRef  PrefConstRef 
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Double,  double,      false, false );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Float,   float,       false, false );

	SAT_CREATE_PROPERTY_TYPE_TRAIT( Uint8,   uint8_t ,    false, false );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Uint16,  uint16_t,    false, false );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Uint32,  uint32_t,    false, false );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Uint64,  uint64_t,    false, false );

	SAT_CREATE_PROPERTY_TYPE_TRAIT( Int8,    int8_t,      false, false );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Int16,   int16_t,     false, false );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Int32,   int32_t,     false, false );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Int64,   int64_t,     false, false );

	SAT_CREATE_PROPERTY_TYPE_TRAIT( Vector2, glm::vec2,   true, true );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Vector3, glm::vec3,   true, true );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Vector4, glm::vec4,   true, true );

	SAT_CREATE_PROPERTY_TYPE_TRAIT( String,  std::string, true, true ); 

	SAT_CREATE_PROPERTY_TYPE_TRAIT( Asset,  uint64_t,	  false, false );

	SAT_CREATE_PROPERTY_TYPE_TRAIT( EntityType,  SharedPtr<Entity>, true, false );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Class,   SClass*,     false, false );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Unknown, void*,       false, true  );

	// Where Ty is the cpp type i.e. float, int etc
	template<typename Ty>
	void ModifyPropertyInternal( SObject* pObject, const void* const fnp, Ty value )
	{
		auto func = reinterpret_cast< SetPropertyFn<Ty> >( fnp );
		( func ) ( pObject, value );
	}

	// Where Ty is the cpp type i.e. float, int etc
	template<typename Ty>
	Ty ReadPropertyInternal( const SObject* pObject, GetPropertyFn<Ty> fnp )
	{
		return fnp( pObject );
	}

	//////////////////////////////////////////////////////////////////////////

	//
	// An SProperty wraps a reflected C++ type.
	// 
	// Any value that is preceeded with the SPROPERTY() macro
	// will be picked up by the HeaderTool and the value will
	// become reflected.
	// 
	// A value that's like:
	// 
	// SPROPERTY()
	// float m_spHealth = 0.0f;
	// 
	// SProperty's are set via the SObject's internal class.
	// 
	// For example: If we have a class such as Player. SProperty
	// will use PlayerInt to get/set the property.
	//
	class SProperty
	{
		using SPropertyGetFunc = void( * )( const SObject*, void* );
		using SPropertySetFunc = void( * )( SObject*, const void* );
	public:
		// INTERNAL, FOR USE BY HEADER TOOL ONLY!
		SProperty( const std::string& rName, const std::string& rNativeType, SPropertyType PropType )
			: m_Name( rName ), m_NativeType( rNativeType ), m_Type( PropType )
		{
		}

		SProperty( const std::string& rName, SPropertyType propType, const void* pGetFnp, void* pSetFnp )
			: m_Name( rName ), m_Type( propType ), m_pGetPropertyFunction( pGetFnp ), m_pSetPropertyFunction( pSetFnp )
		{
		}

		SProperty() = default;
		virtual ~SProperty() = default;
	
	public:
		template<typename CppType>
		void SetProperty( SObject* pObject, CppType value )
		{
			// SPropertyFlags_ReadOnlyInEditor is only available with the editor.
#if !defined(SAT_DIST)
			// SPropertyFlags_ReadOnlyInEditor will be defined in Dist.
			if( IsFlagSet( SPropertyFlags_ReadOnlyInEditor ) ) 
			{
				// Unable to modify read only property!
				return;
			}
#endif

			OnPropertyModified();

			// Convert cpp type to SPropertyType
			// TODO: Check if CppType is the same as our current type
			ModifyPropertyInternal<CppType>( pObject, m_pSetPropertyFunction, value );
		}

		template<SPropertyType Ty>
		[[nodiscard]] typename PropertyTypeTraits<Ty>::Type Read( SObject* pObject ) const
		{
			return ReadPropertyInternal<typename PropertyTypeTraits<Ty>::Type>( pObject, ( GetPropertyFn<typename PropertyTypeTraits<Ty>::Type> )m_pGetPropertyFunction );
		}

		template<SPropertyType Ty>
		[[nodiscard]] typename PropertyTypeTraits<Ty>::Type Read( const SObject* pObject ) const
		{
			return ReadPropertyInternal<typename PropertyTypeTraits<Ty>::Type>( pObject, ( GetPropertyFn<typename PropertyTypeTraits<Ty>::Type> )m_pGetPropertyFunction );
		}

		template<typename Ty>
		[[nodiscard]] Ty GetCopy( SObject* pObject ) const 
		{
			return ReadPropertyInternal<Ty>( pObject, ( GetPropertyFn<Ty> )m_pGetPropertyFunction );
		}

		template<typename Ty>
		[[nodiscard]] const Ty GetCopy( const SObject* pObject ) const
		{
			return ReadPropertyInternal<Ty>( pObject, ( GetPropertyFn<Ty> )m_pGetPropertyFunction );
		}

		void RtCopyFromOther( const SObject* pSrcObject, SObject* pDstObject ) const;

	public:
		void Serialise( const SObject* pObject, std::ofstream& rStream ) const;
		void Deserialise( SObject* pObject, std::istream& rStream );

	public:
		// NOTE: This function HAS to be defined inline as the HeaderTool will call this function
		inline void SetFlag( SPropertyFlags flag, bool value ) 
		{
			if( value )
				m_Flags |= flag;
			else
				m_Flags &= ~flag;
		}

		[[nodiscard]] bool IsFlagSet( SPropertyFlags flag ) const { return ( m_Flags & ( SPropertyFlags_ ) flag ) != 0; }
		SPropertyFlags GetFlags() const { return m_Flags; }

		const std::string& GetName() const { return m_Name; }
		const std::string& GetNativeType() const { return m_NativeType; }

		SPropertyType GetType() const { return m_Type; }

		inline void SetType( SPropertyType type ) { m_Type = type; }
		inline void SetNativeType( const std::string& rNativeType ) { m_NativeType = rNativeType; }
		inline void SetName( const std::string& rName ) { m_Name = rName; }

	protected:
		virtual void OnPropertyModified() {}

		template<typename CppType>
		void SetPropertyInternal( SObject* pObject, CppType value ) const
		{
			// TODO: Check if CppType is the same as our current type
			ModifyPropertyInternal<CppType>( pObject, m_pSetPropertyFunction, value );
		}

	protected:
		std::string m_Name;
		// HEADER TOOL ONLY!, stores the native C++ type, i.e. float, bool, int, double, as a string
		std::string m_NativeType;

		SPropertyType m_Type = SPropertyType::Unknown;
		SPropertyFlags m_Flags = SPropertyFlags_None;

	private:
		const void* m_pGetPropertyFunction = nullptr;
		const void* m_pSetPropertyFunction = nullptr;
	};

	// For backwards compatibility with older code.
	using SPropertyEditor = SProperty;

}
