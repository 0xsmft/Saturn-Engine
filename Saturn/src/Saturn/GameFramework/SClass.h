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

#pragma once

#include "SObject.h"
#include "Core/GameScript.h"

#include "Saturn/Core/Timestep.h"

#include <filesystem>

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// SClass Metadata
	// Describes the metadata of a class in the game framework.
	struct SClassExtendedMetadata 
	{
#if !defined(SAT_DIST)
		std::filesystem::path HeaderPath;
#endif
	};

	enum SClassFlags
	{
		SC_None = 0,
		SC_Initialised = 1 << 0,
		SC_Abstract   = 1 << 1,
		SC_Spawnable  = 1 << 2,
		SC_VisibleInEditor = 1 << 3,
		SC_NoExtendedMetadata = 1 << 4,
		SC_External = 1 << 5,
	};

	class SClass;
	class SProperty;

	struct SClassSpecification
	{
		std::string Name;
		std::underlying_type_t<SClassFlags> Flags = SClassFlags::SC_None;
		int Properties = 0;
		size_t Size = 0;
		size_t Alignment = 0;

		SClass* pParentClass = nullptr;

		SObject* ( *pClassConstructor )( ) = nullptr;
		SClass* ( *pStaticLinkFunction )() = nullptr;

		const SProperty* const* SProperties = nullptr;
	};

	static inline void RClassCompiledIn( SClass* ( *pStaticLinkFunction )( ) )
	{
		( pStaticLinkFunction ) ( );
	}

	struct SClassRegistrar
	{
		SClassRegistrar( SClass* ( *pStaticLinkFunction )( ) )
		{
			RClassCompiledIn( pStaticLinkFunction );
		}
	};

	class SClass : public SObject
	{
		SAT_DECLARE_CLASS_NO_INTER( SClass, SObject )
	public:
		SClass() {}
		SClass( const SClassSpecification& rSpec ) 
			: SObject(), m_Name( rSpec.Name ), m_Flags( (SClassFlags)rSpec.Flags ), m_PropertyCount( rSpec.Properties ), m_Size( rSpec.Size ), m_Alignment( rSpec.Alignment ), m_pClassConstructor( rSpec.pClassConstructor ), m_pStaticLinkFunction( rSpec.pStaticLinkFunction ), m_pParentClass( rSpec.pParentClass ), m_Properties( rSpec.SProperties )
		{
		}

		virtual ~SClass() = default;

		inline SObject* CreateDefaultObject() 
		{
			return ( m_pClassConstructor )( );
		}

	public:
		inline SClassFlags GetFlags() const { return m_Flags; }
		inline const std::string& GetName() const { return m_Name; }

		inline size_t GetSize() const { return m_Size; }
		inline size_t GetAlignment() const { return m_Alignment; }

		inline void SetFlag( SClassFlags flag )
		{
			m_Flags = static_cast<SClassFlags>( m_Flags | flag );
		}

		inline const SClass* GetParentClass() const { return m_pParentClass; }
		inline int GetPropertyCount() const { return m_PropertyCount; }

		// Pointer to the first element
		inline const SProperty* const* GetProperties() const { return m_Properties; }

	public:
		static void RConstructClass( SClass** ppClass, const SClassSpecification& rSpec );

		SProperty& GetProperty( const std::string& rPropertyName ) const;

	private:
		std::string m_Name;
		SClassFlags m_Flags = SClassFlags::SC_None;
		int m_PropertyCount = 0;
		size_t m_Size = 0;
		size_t m_Alignment = 0;

		SObject* ( *m_pClassConstructor )( ) = nullptr;
		SClass* ( *m_pStaticLinkFunction )( ) = nullptr;

		SClass* m_pParentClass = nullptr;
		const SProperty* const* m_Properties = nullptr;
	};

	template<class RClass>
	[[nodiscard]] SObject* RInternalConstructor() 
	{
		return ( SObject* ) RClass::X31_DefConstructor();
	}
}
