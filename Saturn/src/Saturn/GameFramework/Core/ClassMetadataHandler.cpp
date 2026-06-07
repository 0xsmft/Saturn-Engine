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
#include "ClassMetadataHandler.h"

#include "Saturn/Core/Memory/SObjectAllocator.h"

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// CLASS METADATA HANDLER

	ClassMetadataHandler::ClassMetadataHandler()
	{
	}
	
	ClassMetadataHandler::~ClassMetadataHandler()
	{
	}

	void ClassMetadataHandler::DestroyAndFreeAllSClasses()
	{
		for( auto& [hash, pClass] : m_Classes )
		{
			FSObjectAllocator::DeallocateSObject<SClass>( pClass );
		}

		m_Classes.clear();
	}

	void ClassMetadataHandler::BeginHotReload()
	{
	}

	void ClassMetadataHandler::AcknowledgeHotReload()
	{
	}

	SClassHotReloadChanges ClassMetadataHandler::DistinguishBetweenSClass( const SClass* const pA, const SClass* const pB )
	{
		SClassHotReloadChanges changes = SCHotReload_NoChange;
		if( pA->GetSize() != pB->GetSize() )
		{
			changes = SCHotReload_ClassSizeChange;
		}

		if( pA->GetAlignment() != pB->GetAlignment() )
		{
			changes |= SCHotReload_AlignmentChange;
		}

		if( pA->GetParentClass() != pB->GetParentClass() )
		{
			changes |= SCHotReload_ParentClassChange;
		}

		if( pA->GetPropertyCount() != pB->GetPropertyCount() )
		{
			changes |= SCHotReload_SPropertyCountChange;
		}

		return changes;
	}

	void ClassMetadataHandler::HandleSClassChanges( const SClassHotReloadChanges changes, SClass* pExisiting, SClass* pHotReloaded )
	{
		// If we have no changes then its simple and we safe to transfer this class to new the hot-reloaded one.
		if( ( changes & SCHotReload_NoChange ) != 0 )
		{
			SAT_CORE_INFO( "[HotReload]: HandleSClassChanges - No Change, Class Name: {0}", pExisiting->GetName() );

			// Deallocate old.
			FSObjectAllocator::DeallocateSObject<SClass>( pExisiting );

			pHotReloaded->SetFlag( SC_Initialised );

			m_Classes[ pHotReloaded->GetHash() ] = pHotReloaded;
		}
	}

	std::vector<SClass*> ClassMetadataHandler::FindClassInLinkedList( SClass* pParentClass )
	{
		std::vector<SClass*> result;
		return result;
	}

	void ClassMetadataHandler::CreateLinkedClassList()
	{
		m_LinkedListClasses.pClassPtr = SObject::StaticClass();

		std::unordered_map<const SClass*, std::vector<const SClass*>> childMap;

		// Build parent to child map
		// does not account for grandparents etc.
		for( const auto& [hash, pClass] : m_Classes )
		{
			if( const SClass* pParent = pClass->GetParentClass() )
			{
				childMap[ pParent ].push_back( pClass );
			}
		}

		// Now we build the tree.
		BuildLinkedListRecursive( m_LinkedListClasses, childMap );
	}

	const SClassLinkedListNode* ClassMetadataHandler::FindNodeRecursive( const SClassLinkedListNode* pNode, const SClass* pClass ) const
	{
		if( !pNode )
			return nullptr;

		if( pNode->pClassPtr == pClass )
			return pNode;

		for( const auto& child : pNode->Children )
		{
			if( const SClassLinkedListNode* pFound =
				FindNodeRecursive( child.get(), pClass ) )
			{
				return pFound;
			}
		}

		return nullptr;
	}

	const SClassLinkedListNode* ClassMetadataHandler::FindNode( const SClass* pClass ) const
	{
		return FindNodeRecursive( &m_LinkedListClasses, pClass );
	}

	void ClassMetadataHandler::BuildLinkedListRecursive( SClassLinkedListNode& node, const std::unordered_map<const SClass*, std::vector<const SClass*>>& childMap )
	{
		const auto it = childMap.find( node.pClassPtr );
		if( it == childMap.end() )
			return;

		for( const SClass* childClass : it->second )
		{
			auto childNode =
				std::make_shared<SClassLinkedListNode>( childClass, &node );

			BuildLinkedListRecursive( *childNode, childMap );

			node.Children.push_back( std::move( childNode ) );
		}
	}

	Saturn::SObject* ClassMetadataHandler::CreateClassObject( const std::string& rScriptName, SObject* pParentObject )
	{
		SObject* pObject = CreateClassObject( FNV1A64( rScriptName.c_str() ), pParentObject );

		if( !pObject )
		{
			const std::string message = std::format( "Class/{0} does not exist in any module! Unable to continue!", rScriptName );
			SAT_CORE_VERIFY( false, message );
		}

		return pObject;
	}

	Saturn::SObject* ClassMetadataHandler::CreateClassObject( SClass* pClass, SObject* pParentObject )
	{
		SObject* pObject = pClass->CreateDefaultObject();
		pObject->m_pClass = pClass;
		pObject->m_pParentObject = pParentObject;

		return pObject;
	}

	Saturn::SObject* ClassMetadataHandler::CreateClassObject( uint64_t classHash, SObject* pParentObject )
	{
		const auto Itr = m_Classes.find( classHash );
		if( Itr != m_Classes.end() )
		{
			SObject* pObject = Itr->second->CreateDefaultObject();
			pObject->m_pClass = Itr->second;
			pObject->m_pParentObject = pParentObject;

			return pObject;
		}

		return nullptr;
	}

	SObject* ClassMetadataHandler::CreateClassObject( const SClass* pClass, SObject* pParentObject /*= nullptr */ )
	{
		SObject* pObject = pClass->CreateDefaultObject();
		pObject->m_pClass = const_cast< SClass* >( pClass );
		pObject->m_pParentObject = pParentObject;

		return pObject;
	}

	void ClassMetadataHandler::RegisterSClass( SClass* pClass )
	{
		m_Classes.emplace( pClass->GetHash(), pClass );
	}

	SClass* ClassMetadataHandler::RFastCheckClass( uint64_t classHash )
	{
		const auto Itr = m_Classes.find( classHash );
		
		return Itr == m_Classes.end() ? nullptr : Itr->second;
	}

}
