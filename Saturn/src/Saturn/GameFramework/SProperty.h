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

#include <string>
#include <glm/glm.hpp>

namespace Saturn {

	class SClass;

	enum class SPropertyFlags
	{
		None,
		EditInEditor,
		ReadOnlyInEditor,
		Asset,
		Serialised
	};

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

#define SAT_CREATE_PROPERTY_TYPE_TRAIT(PropertyType, CppType) \
template<> struct PropertyTypeTraits<SPropertyType::PropertyType> { using Type = CppType; }

	SAT_CREATE_PROPERTY_TYPE_TRAIT( Char,   char );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Double, double );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Int,    int );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Float,  float );

	SAT_CREATE_PROPERTY_TYPE_TRAIT( Uint8,  uint8_t );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Uint16, uint16_t );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Uint32, uint32_t );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Uint64, uint64_t );

	SAT_CREATE_PROPERTY_TYPE_TRAIT( Int8,  int8_t );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Int16, int16_t );
	//SAT_CREATE_PROPERTY_TYPE_TRAIT( Int32, int32_t ); -- same as int
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Int64, int64_t );

	SAT_CREATE_PROPERTY_TYPE_TRAIT( Vector2, glm::vec2 );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Vector3, glm::vec3 );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Vector4, glm::vec4 );

	SAT_CREATE_PROPERTY_TYPE_TRAIT( String, std::string );

	SAT_CREATE_PROPERTY_TYPE_TRAIT( Class, SClass* );
	SAT_CREATE_PROPERTY_TYPE_TRAIT( Unknown, void* );

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
			: Name( rName ), NativeType( rNativeType ), Type( PropType )
		{
		}

		SProperty() = default;
		~SProperty() = default;
	
		template<typename CppType>
		void SetProperty( SClass* pClass, CppType value )
		{
			// Convert cpp type to SPropertyType
			// TODO: Check if CppType is the same as our current type

			ModifyPropertyInternal<CppType>( pClass, pSetPropertyFunction, value );
		}

		template<SPropertyType Ty>
		[[nodiscard]] typename PropertyTypeTraits<Ty>::Type Read( SClass* pClass )
		{
			return ReadPropertyInternal<typename PropertyTypeTraits<Ty>::Type>( pClass, pGetPropertyFunction );
		}

		void Serialise( SClass* pClass ) {}
		void Deserialise( SClass* pClass ) {}

	public:
		std::string Name;
		std::string NativeType;

		SPropertyType Type = SPropertyType::Unknown;
		SPropertyFlags Flags = SPropertyFlags::None;

		const void* pSetPropertyFunction = nullptr;
		const void* pGetPropertyFunction = nullptr;
	};

	/*
#define SAT_CREATE_FNP_FOR_TYPE( typename, x ) \
	typedef void( __stdcall* SetPropertyFn_##typename )( SClass*, x ); \
	typedef x( __stdcall* GetPropertyFn_##typename )( SClass* ); \

#define SAT_CALL_SET_FUNCTION_FOR_TYPE( typename, x, pClass, value ) \
	SetPropertyFn_##typename fnp = ( SetPropertyFn_##typename )x.pSetPropertyFunction; \
	( fnp ) ( pClass, value ) \

#define SAT_CALL_GET_FUNCTION_FOR_TYPE( typename, x, pClass ) \
	GetPropertyFn_##typename fnp = ( GetPropertyFn_##typename )x.pGetPropertyFunction; \
	return ( fnp ) ( pClass ) \


	//////////////////////////////////////////////////////////////////////////
	// Function types
	SAT_CREATE_FNP_FOR_TYPE( Char,    char );
	SAT_CREATE_FNP_FOR_TYPE( Float,   float );
	SAT_CREATE_FNP_FOR_TYPE( Int,     int );
	SAT_CREATE_FNP_FOR_TYPE( Double,  double );
	SAT_CREATE_FNP_FOR_TYPE( Uint8,   unsigned char );
	SAT_CREATE_FNP_FOR_TYPE( Uint16,  unsigned short );
	SAT_CREATE_FNP_FOR_TYPE( Uint32,  unsigned int );
	SAT_CREATE_FNP_FOR_TYPE( Uint64,  unsigned long long );
	SAT_CREATE_FNP_FOR_TYPE( Int8,    signed char );
	SAT_CREATE_FNP_FOR_TYPE( Int16,   short );
	SAT_CREATE_FNP_FOR_TYPE( Int32,   int );
	SAT_CREATE_FNP_FOR_TYPE( Int64,   long long );
	SAT_CREATE_FNP_FOR_TYPE( vec2,    glm::vec2 );
	SAT_CREATE_FNP_FOR_TYPE( vec3,    glm::vec3 );
	SAT_CREATE_FNP_FOR_TYPE( vec4,    glm::vec4 );
	SAT_CREATE_FNP_FOR_TYPE( String,  std::string );
	SAT_CREATE_FNP_FOR_TYPE( Unknown, void* );

	template<typename Ty>
	static void SetSProperty( const SProperty& rProperty, SClass* pClass, Ty value )
	{
		switch( rProperty.Type )
		{
			case SPropertyType::Char:    { SAT_CALL_SET_FUNCTION_FOR_TYPE( Char, rProperty, pClass, value );    } break;
			case SPropertyType::Float:   { SAT_CALL_SET_FUNCTION_FOR_TYPE( Float, rProperty, pClass, value );   } break;
			case SPropertyType::Int:     { SAT_CALL_SET_FUNCTION_FOR_TYPE( Int, rProperty, pClass, value );     } break;
			case SPropertyType::Double:  { SAT_CALL_SET_FUNCTION_FOR_TYPE( Double, rProperty, pClass, value );  } break;
			case SPropertyType::Uint8:   { SAT_CALL_SET_FUNCTION_FOR_TYPE( Uint8, rProperty, pClass, value );   } break;
			case SPropertyType::Uint16:  { SAT_CALL_SET_FUNCTION_FOR_TYPE( Uint16, rProperty, pClass, value );  } break;
			case SPropertyType::Uint32:  { SAT_CALL_SET_FUNCTION_FOR_TYPE( Uint32, rProperty, pClass, value );  } break;
			case SPropertyType::Uint64:  { SAT_CALL_SET_FUNCTION_FOR_TYPE( Uint64, rProperty, pClass, value );  } break;
			case SPropertyType::Int8:    { SAT_CALL_SET_FUNCTION_FOR_TYPE( Int8, rProperty, pClass, value );    } break;
			case SPropertyType::Int16:   { SAT_CALL_SET_FUNCTION_FOR_TYPE( Int16, rProperty, pClass, value );   } break;
			case SPropertyType::Int32:   { SAT_CALL_SET_FUNCTION_FOR_TYPE( Int32, rProperty, pClass, value );   } break;
			case SPropertyType::Int64:   { SAT_CALL_SET_FUNCTION_FOR_TYPE( Int16, rProperty, pClass, value );   } break;
			//case SPropertyType::Vector2: { SAT_CALL_SET_FUNCTION_FOR_TYPE( vec2, rProperty, pClass, value );    } break;
			//case SPropertyType::Vector3: { SAT_CALL_SET_FUNCTION_FOR_TYPE( vec3, rProperty, pClass, value );    } break;
			//case SPropertyType::Vector4: { SAT_CALL_SET_FUNCTION_FOR_TYPE( vec4, rProperty, pClass, value );    } break;
			//case SPropertyType::String:  { SAT_CALL_SET_FUNCTION_FOR_TYPE( String, rProperty, pClass, value );  } break;
			//case SPropertyType::Unknown: { SAT_CALL_SET_FUNCTION_FOR_TYPE( Unknown, rProperty, pClass, value ); } break;
		}
	}

	template<typename Ty>
	static Ty GetSProperty( const SProperty& rProperty, SClass* pClass )
	{
		switch( rProperty.Type )
		{
			case SPropertyType::Char:    { SAT_CALL_GET_FUNCTION_FOR_TYPE( Char, rProperty, pClass );    } break;
			case SPropertyType::Float:   { SAT_CALL_GET_FUNCTION_FOR_TYPE( Float, rProperty, pClass );   } break;
			case SPropertyType::Int:     { SAT_CALL_GET_FUNCTION_FOR_TYPE( Int, rProperty, pClass );     } break;
			case SPropertyType::Double:  { SAT_CALL_GET_FUNCTION_FOR_TYPE( Double, rProperty, pClass );  } break;
			case SPropertyType::Uint8:   { SAT_CALL_GET_FUNCTION_FOR_TYPE( Uint8, rProperty, pClass );   } break;
			case SPropertyType::Uint16:  { SAT_CALL_GET_FUNCTION_FOR_TYPE( Uint16, rProperty, pClass );  } break;
			case SPropertyType::Uint32:  { SAT_CALL_GET_FUNCTION_FOR_TYPE( Uint32, rProperty, pClass );  } break;
			case SPropertyType::Uint64:  { SAT_CALL_GET_FUNCTION_FOR_TYPE( Uint64, rProperty, pClass );  } break;
			case SPropertyType::Int8:    { SAT_CALL_GET_FUNCTION_FOR_TYPE( Int8, rProperty, pClass );    } break;
			case SPropertyType::Int16:   { SAT_CALL_GET_FUNCTION_FOR_TYPE( Int16, rProperty, pClass );   } break;
			case SPropertyType::Int32:   { SAT_CALL_GET_FUNCTION_FOR_TYPE( Int32, rProperty, pClass );   } break;
			case SPropertyType::Int64:   { SAT_CALL_GET_FUNCTION_FOR_TYPE( Int16, rProperty, pClass );   } break;
			//case SPropertyType::Vector2: { SAT_CALL_GET_FUNCTION_FOR_TYPE( vec2, rProperty, pClass );    } break;
			//case SPropertyType::Vector4: { SAT_CALL_GET_FUNCTION_FOR_TYPE( vec4, rProperty, pClass );    } break;
			//case SPropertyType::Vector3: { SAT_CALL_GET_FUNCTION_FOR_TYPE( vec3, rProperty, pClass );    } break;
			//case SPropertyType::String:  { SAT_CALL_GET_FUNCTION_FOR_TYPE( String, rProperty, pClass );  } break;
			//case SPropertyType::Unknown: { SAT_CALL_GET_FUNCTION_FOR_TYPE( Unknown, rProperty, pClass ); } break;
		}
	}
	*/
}