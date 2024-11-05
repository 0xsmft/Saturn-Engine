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

#include "SingletonStorage.h"
#include "Saturn/GameFramework/SClass.h"

#include <string>
#include <vector>

namespace Saturn {

	class ClassMetadataHandler : public RefTarget
	{
	public:
		static inline ClassMetadataHandler& Get() { return *SingletonStorage::GetOrCreateSingleton<ClassMetadataHandler>(); }
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
		
		std::vector<SProperty> GetAllProperties( const std::string& rMetadataName );

	public:
		SClassMetadata& GetSClassMetadata();

	private:
		std::unordered_map<std::string, SClassMetadata> m_MetadataTree;

		// Metadata name -> SProperties
		std::unordered_map<std::string, std::vector<SProperty>> m_Properties;
	};
}