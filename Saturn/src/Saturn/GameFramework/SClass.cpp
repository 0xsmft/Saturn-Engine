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
#include "SClass.h"
#include "SProperty.h"

#include "Core/ClassMetadataHandler.h"

#include "Saturn/Core/Memory/SObjectAllocator.h"

#define SAT_FORCE_VERBOSE_SCLASS_REG

#if defined(SAT_DEBUG) || defined(SAT_FORCE_VERBOSE_SCLASS_REG)
#define SAT_VERBOSE_SCLASS_REG
#endif

namespace Saturn {

	void SClass::RConstructClass( SClass*& outClass, const SClassSpecification& rSpec )
	{
		// Skip if null or already initialised.
		if( outClass != nullptr && ( outClass->GetFlags() & SC_Initialised ) == 0 )
		{
			return;
		}

		// Now, we need to check if the class already exists.
		// This may happen because in the Game module it will register the same set of classes
		// as the engine it self, and we want the engine to own it's classes.
		// 
		// It may also happen if this is a hot-reload because of course the class would of already
		// been loaded.
		//
		SClass* pFoundClass = ClassMetadataHandler::Get().RFastCheckClass( rSpec.Hash );
		if( pFoundClass )
		{
			// If we are a hold-reload, we need to handle this differently.
			if( ClassMetadataHandler::Get().IsHotReload() )
			{
				// Allocate the new class...
				SClass* pNewClass = FSObjectAllocator::AllocateSObject<SClass>( rSpec );

				const auto changes = ClassMetadataHandler::Get().DistinguishBetweenSClass( pFoundClass, pNewClass );
				ClassMetadataHandler::Get().HandleSClassChanges( changes, pFoundClass, pNewClass );

#if defined(SAT_VERBOSE_SCLASS_REG)
				SAT_CORE_INFO( "[SC] Hot-Reloading class with the same name {0}", rSpec.Name );
#endif

				outClass = pNewClass;
				return;
			}

#if defined(SAT_VERBOSE_SCLASS_REG)
			SAT_CORE_WARN( "[SC] An exisiting class with the same name ({0}) already exists!", rSpec.Name );
#endif
			outClass = pFoundClass;
			return;
		}

#if defined(SAT_VERBOSE_SCLASS_REG)
		SAT_CORE_INFO( "[SC] Registering SClass SC/{0}", rSpec.Name );
#endif

		// Allocate the object....
		SClass* pNewClass = FSObjectAllocator::AllocateSObject<SClass>( rSpec );
		outClass = pNewClass;

		outClass->SetFlag( SC_Initialised );

		ClassMetadataHandler::Get().RegisterSClass( outClass );
	}

	void SClass::RConstructClassHotReloaded( SClass*& outClass, const SClassSpecification& rSpec )
	{
		if( outClass != nullptr && ( outClass->GetFlags() & SC_Initialised ) == 0 )
		{
			return;
		}
		
		// Allocate the object.
		SClass* pNewClass = FSObjectAllocator::AllocateSObject<SClass>( rSpec );

		// Try to find the existing class...
		SClass* pExistingClass = ClassMetadataHandler::Get().RFastCheckClass( rSpec.Hash );
		if( pExistingClass )
		{
			const auto changes = ClassMetadataHandler::Get().DistinguishBetweenSClass( pExistingClass, pNewClass );
			ClassMetadataHandler::Get().HandleSClassChanges( changes, pExistingClass, pNewClass );
		}
		else
		{
			outClass->SetFlag( SC_Initialised );
			ClassMetadataHandler::Get().RegisterSClass( outClass );
		}
	
		outClass = pNewClass;
	}

	SProperty& SClass::GetProperty( const std::string& rPropertyName ) const
	{
		for( int i = 0; i < m_PropertyCount; ++i )
		{
			const SProperty* pProp = m_Properties[ i ];
			
			if( pProp->GetName() == rPropertyName )
			{
				return *( SProperty* ) pProp;
			}
		}

#if !defined(SAT_DIST)
		static SPropertyEditor s_Empty;
#else
		static SProperty s_Empty;
#endif
		return s_Empty;
	}

	bool SClass::IsChildOf( const SClass* pBase ) const
	{
		if( !pBase ) return false;

		const SClass* pClass = this;
		while( pClass )
		{
			if( pClass == pBase )
			{
				return true;
			}

			pClass = pClass->GetParentClass();
		}

		return false;
	}

	typedef SClass* ( __stdcall* RClassFunc )();

	/* TODO: Pre-alloc */
	// WARNING: Module local..., game module will have it's own copy of this, which is not great
	static std::vector<RClassFunc> s_RClassQueue;

	void SClass::ProcessNewlyLoadedSClasses()
	{
		for( const auto& rFunc : s_RClassQueue )
		{
			( rFunc ) ( );
		}

		s_RClassQueue.clear();
	}

	//////////////////////////////////////////////////////////////////////////
	// RClass Static link

	void RClassCompiledIn( SClass* ( *pStaticLinkFunction )( ) )
	{
//		s_RClassQueue.push_back( pStaticLinkFunction );

		( pStaticLinkFunction ) ( );
	}

}
