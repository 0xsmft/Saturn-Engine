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

#include "sppch.h"
#include "ClassMetadataHandler.h"

namespace Saturn {

	ClassMetadataHandler::ClassMetadataHandler()
	{	
#if !defined(SAT_DIST)
		SingletonStorage::AddSingleton( this );
#endif
	}

	ClassMetadataHandler::~ClassMetadataHandler()
	{
		m_Classes.clear();
	}

	void ClassMetadataHandler::BeginHotReload()
	{
		ClearExternalData();
	}

	void ClassMetadataHandler::AcknowledgeHotReload()
	{
	}

	Saturn::SObject* ClassMetadataHandler::CreateClassObject( const std::string& rScriptName )
	{
		const auto Itr = m_Classes.find( rScriptName );
		if( Itr != m_Classes.end() )
		{
			SObject* pObject = Itr->second->CreateDefaultObject();
			pObject->m_pClass = Itr->second;

			return pObject;
		}
		else
		{
			const std::string message = std::format( "Class/{0} does not exist in any module! Unable to continue!", rScriptName );
			SAT_CORE_VERIFY( false, message );
		}

		return nullptr;
	}

	Saturn::SObject* ClassMetadataHandler::CreateClassObject( SClass* pClass )
	{
		return CreateClassObject( pClass->GetName() );
	}

	void ClassMetadataHandler::AddMetadata( const SClassExtendedMetadata& rData )
	{
	}

	SClass* ClassMetadataHandler::GetSObjectMetadata()
	{
		return SObject::StaticClass();
	}

	std::vector<SProperty>& ClassMetadataHandler::GetAllProperties( const std::string& rMetadataName )
	{
		/*
		const auto Itr = m_MetadataTree.find( rMetadataName );

		if( Itr != m_MetadataTree.end() )
		{
			auto& properties = m_Properties[ rMetadataName ];
			return properties;
		}

		*/
		static std::vector<SProperty> s_EmptyMap;
		return s_EmptyMap;
	}

	void ClassMetadataHandler::ClearExternalData()
	{
		/*
		std::erase_if( m_MetadataTree, []( const auto& kv )
		{
			return kv.second.ExternalData;
		} );
		*/
	}

	void ClassMetadataHandler::RegisterClass( SClass* pClass )
	{
		const auto Itr = m_Classes.find( pClass->GetName() );
		if( Itr == m_Classes.end() )
		{
			m_Classes[ pClass->GetName() ] = pClass;
		}
	}

}
