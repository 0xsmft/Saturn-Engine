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

#include "SingletonStorage.h"
#include "Saturn/GameFramework/SClass.h"
#include "Saturn/GameFramework/SProperty.h"

#include <string>

namespace Saturn {

#define SAT_ClassMetadataHandler_EachTreeNode_Deprecated [[deprecated( "Saturn::ClassMetadataHandler::EachTreeNode is deprecated and will be removed. Consider using \"ClassMetadataHandler::EachClassNode\" instead." )]]

	class ClassMetadataHandler
	{
	public:
		SAT_SINGLETON_LAZY( ClassMetadataHandler )

	public:
		ClassMetadataHandler();
		~ClassMetadataHandler();

		void DestroyAndFreeAllSClasses();

		template<typename Fn>
		SAT_ClassMetadataHandler_EachTreeNode_Deprecated void EachTreeNode( Fn Function )
		{
			EachClassNode( Function );
		}

		template<typename Fn>
		void EachClassNode( Fn Function )
		{
			for( auto&& [name, pClass] : m_Classes )
				Function( pClass );
		}

		template<typename Ty>
		inline std::vector<SClass*> GetAllClassesBasedFrom() const
		{
			std::vector<SClass*> map;
			for( const auto& [hash, pClass] : m_Classes )
			{
				if( pClass->IsChildOf( Ty::StaticClass() ) )
				{
					map.push_back( pClass );
				}
			}

			return map;
		}

	public:
		[[nodiscard]] SObject* CreateClassObject( const std::string& rScriptName );
		[[nodiscard]] SObject* CreateClassObject( uint64_t classHash );
		[[nodiscard]] SObject* CreateClassObject( SClass* pClass );

		template<typename Ty, typename... VaArgs>
		[[nodiscard]] Ty* CreateClassObject( SClass* pClass, VaArgs&&... args ) 
		{
			static_assert( std::is_base_of<SObject, Ty>::value, "Ty must be a child of SObject class!" );

			Ty* pObject = new Ty( std::forward<VaArgs>( args )... );
			pObject->m_pClass = pClass;

			return pObject;
		}

		void RegisterSClass( SClass* pClass, const std::string& rModuleName );
		SClass* RFastCheckClass( uint64_t classHash );

		[[nodiscard]] size_t GetNumberOfClasses() const { return m_Classes.size(); }

	public:
		[[deprecated( "Saturn::ClassMetadataHandler::GetSObjectMetadata is deprecated and will be removed. Consider using \"SObject::StaticClass\" instead." )]]
		SClass* GetSObjectMetadata();

	public:
		// Hot reload
		void BeginHotReload();
		void AcknowledgeHotReload();

	private:		
		// All of the classes that have reflection data tied to them.
		//                 HASH    -> CLASS*
		// TODO: Convert to a linked listed
		std::unordered_map<uint64_t, SClass*> m_Classes;
	};

	//////////////////////////////////////////////////////////////////////////
	// Create an SObject with it's SClass
	// TODO: This should be moved into the SObject file and this function should maybe defined inline,
	//		 The goal is to not have to include ClassMetadataHandler
	template<typename TObject, typename... VaArgs>
	[[nodiscard]] inline TObject* NewObject( VaArgs&&... rrArgs ) 
	{
		return ClassMetadataHandler::Get().CreateClassObject<TObject>( TObject::StaticClass(), std::forward<VaArgs>( rrArgs )... );
	}

}
