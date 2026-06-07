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

#include "SingletonStorage.h"

#include "Saturn/GameFramework/SClass.h"
#include "Saturn/GameFramework/SProperty.h"

#include <string>

namespace Saturn {

	struct SClassLinkedListNode
	{
		SClassLinkedListNode() = default;

		SClassLinkedListNode( SClass* inClass, SClassLinkedListNode* inParentNode )
			: pClassPtr( inClass ), pParentNode( inParentNode )
		{
		}

		SClassLinkedListNode( const SClass* inClass, const SClassLinkedListNode* inParentNode )
			: pClassPtr( inClass ), pParentNode( inParentNode )
		{
		}

		const SClass* pClassPtr = nullptr;
		const SClassLinkedListNode* pParentNode = nullptr;

		std::vector<std::shared_ptr<SClassLinkedListNode>> Children;
	};

	// Changes that happen when a hot reload happens
	enum SCHotReloadFlags : uint8_t
	{
		// No change to the SClass structure itself
		// but obviously this could mean that an SClass is internally modified but thats fine because we don't care about that.
		SCHotReload_NoChange = 0,

		// The following flags means that the SClass was structurally modified...
		SCHotReload_ClassSizeChange       = BIT( 0 ),
		SCHotReload_AlignmentChange       = BIT( 1 ),
		SCHotReload_ParentClassChange     = BIT( 2 ),
		SCHotReload_SPropertyCountChange  = BIT( 3 ),
	};

	// enum SCHotReloadFlags
	typedef	uint8_t SClassHotReloadChanges;

	class ClassMetadataHandler
	{
	public:
		SAT_SINGLETON_LAZY( ClassMetadataHandler )

	public:
		ClassMetadataHandler();
		~ClassMetadataHandler();

		void DestroyAndFreeAllSClasses();

		template<typename Fn>
		void EachClassNode2( const SClass* pParentClass, Fn Function )
		{
			const SClassLinkedListNode* pNode = FindNode( pParentClass );

			if( !pNode )
				return;

			for( const auto& child : pNode->Children )
			{
				Function( *child );
			}
		}

		template<typename Fn>
		void EveryClass( Fn Function )
		{
			for( const auto& [classHash, pClass] : m_Classes )
			{
				Function( pClass );
			}
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

		template<typename Ty>
		inline std::vector<SClass*> GetClassesWithParentClassOf() const
		{
			std::vector<SClass*> map;
			for( const auto& [hash, pClass] : m_Classes )
			{
				if( pClass->GetParentClass() == Ty::StaticClass() )
				{
					map.push_back( pClass );
				}
			}

			return map;
		}

		inline std::vector<SClass*> GetClassesWithParentClassOf( uint64_t classHash ) const
		{
			std::vector<SClass*> map;
			for( const auto& [hash, pClass] : m_Classes )
			{
				if( !pClass->GetParentClass() )
					continue;

				if( pClass->GetParentClass()->GetHash() == classHash )
				{
					map.push_back( pClass );
				}
			}

			return map;
		}

		void CreateLinkedClassList();
		const SClassLinkedListNode* FindNode( const SClass* pClass ) const;

	public:
		[[nodiscard]] SObject* CreateClassObject( const std::string& rScriptName, SObject* pParentObject = nullptr );
		[[nodiscard]] SObject* CreateClassObject( uint64_t classHash, SObject* pParentObject = nullptr );
		[[nodiscard]] SObject* CreateClassObject( SClass* pClass, SObject* pParentObject = nullptr );
		[[nodiscard]] SObject* CreateClassObject( const SClass* pClass, SObject* pParentObject = nullptr );

		template<typename Ty, typename... VaArgs>
		[[nodiscard]] Ty* CreateClassObject( SClass* pClass, SObject* pOuter, VaArgs&&... args ) 
		{
			static_assert( std::is_base_of<SObject, Ty>::value, "Ty must be a child of SObject class!" );

			Ty* pObject = new Ty( std::forward<VaArgs>( args )... );
			pObject->m_pClass = pClass;
			pObject->m_pParentObject = pOuter;

			return pObject;
		}

		void RegisterSClass( SClass* pClass );
		SClass* RFastCheckClass( uint64_t classHash );

		[[nodiscard]] size_t GetNumberOfClasses() const { return m_Classes.size(); }

	public:
		// Hot reload
		void BeginHotReload();
		void AcknowledgeHotReload();

		SClassHotReloadChanges DistinguishBetweenSClass( const SClass* const pA, const SClass* const pB );

		void HandleSClassChanges( const SClassHotReloadChanges changes, SClass* pExisiting, SClass* pHotReloaded );

	private:
		void BuildLinkedListRecursive( SClassLinkedListNode& node, const std::unordered_map<const SClass*, std::vector<const SClass*>>& childMap );
		const SClassLinkedListNode* FindNodeRecursive( const SClassLinkedListNode* pNode, const SClass* pClass ) const;

		std::vector<SClass*> FindClassInLinkedList( SClass* pParentClass );

	private:		
		// All of the classes that have reflection data tied to them.
		//                 HASH    -> CLASS*
		std::unordered_map<uint64_t, SClass*> m_Classes;

#if !defined(SAT_DIST)
		// Linked list of SClasses (purely for informative reasons).
		// Fist node is the SObject node which all SObjects are based from.
		SClassLinkedListNode m_LinkedListClasses;
#endif
	};

	//////////////////////////////////////////////////////////////////////////
	// Create an SObject with it's SClass
	// TODO: This should be moved into the SObject file and this function should maybe defined inline,
	//		 The goal is to not have to include ClassMetadataHandler
	template<typename TObject, typename... VaArgs>
	[[nodiscard]] inline TObject* NewObject( SObject* pOuter, VaArgs&&... rrArgs ) 
	{
		return ClassMetadataHandler::Get().CreateClassObject<TObject>( TObject::StaticClass(), pOuter, std::forward<VaArgs>( rrArgs )... );
	}

}
