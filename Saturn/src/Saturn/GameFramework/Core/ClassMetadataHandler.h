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
#include "FunctionTypedefs.h"

#include <string>
#include <vector>

// In distribution builds the class metadata handler is a lazy loaded singleton because it will be initialised when the application starts up and calls the ReflRegisterPropertiesFor_XXX functions. Lifetime is tied to the lifetime of the application.
// However,
// in development builds we want to tie the lifetime of the class to the lifetime of the editor layer. So ClassMetadataHandler is a owned by the editor layer.
// X31 is used to as a unique identifier.
#if defined(SAT_DIST)
#define SAT_CMH_SINGLETON_X31( x ) SAT_SINGLETON_LAZY( x )
#else
//#define SAT_CMH_SINGLETON_X31( x ) static inline x& Get() { return *SingletonStorage::GetSingleton<x>(); }
#define SAT_CMH_SINGLETON_X31( x ) SAT_SINGLETON_LAZY( x )
#endif

namespace Saturn {

	class ClassMetadataHandler : public RefTarget
	{
	public:
		SAT_CMH_SINGLETON_X31( ClassMetadataHandler )

	public:
		ClassMetadataHandler();
		~ClassMetadataHandler();

		template<typename Fn>
		void EachTreeNode( Fn Function )
		{
			for( auto&& [name, data] : m_MetadataTree )
				Function( data );
		}

		void AddMetadata( const SClassMetadata& rData );
		bool IsEngineMetadata( const SClassMetadata& rData ) { return !rData.ExternalData; }

		void RegisterProperty( const std::string& rMetadataName, const SProperty& rProperty );
		
		std::vector<SProperty>& GetAllProperties( const std::string& rMetadataName );
		SProperty& GetProperty( const std::string& rMetadataName, const std::string& rPropertyName );

		void ClearExternalData();

	public:
		SClassMetadata& GetSClassMetadata();

	public:
		// Hot reload
		void BeginHotReload();
		void AcknowledgeHotReload();

	public:
		// Engine internal
		void InitialiseEngineClass( const std::string& rName, SClassFlags flags, CreateSClassFn function );
		[[nodiscard]] Entity* SpawnEngineClass( const std::string& rScriptName );

	private:
		std::unordered_map<std::string, SClassMetadata> m_MetadataTree;
		
		// Metadata name -> SProperties
		std::unordered_map<std::string, std::vector<SProperty>> m_Properties;

		// Engine spawnable classes
		// TEMP
		std::unordered_map<std::string, CreateSClassFn > m_SpawnableEngineClasses;
	};
}