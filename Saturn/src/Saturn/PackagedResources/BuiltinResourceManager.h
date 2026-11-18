/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2026 0xsmft https://0xsmft.dev                                       *
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

#include "PackagedResourceType.h"

namespace Saturn {

	class BuiltinResourceManager : public RefTarget
	{
	public:
		static inline BuiltinResourceManager* Get() { return SingletonStorage::GetSingleton<BuiltinResourceManager>(); }

	public:
		BuiltinResourceManager();
		virtual ~BuiltinResourceManager();

		Ref<class Asset> GetResource( PackagedResourceType type );
		
		template<typename Ty>
		Ref<Ty> GetOrLoadResource( PackagedResourceType type ) 
		{
			static_assert( std::is_base_of<RefTarget, Ty>::value, "Ty must be an intrusive reference counted base object i.e. based from RefTarget." );

			const auto itr = m_Resources.find( type );
			if( itr != m_Resources.end() )
			{
				return itr->second.As<Ty>();
			}

			m_Resources[ type ] = CreateResourceFromType( type );

			return m_Resources[ type ].As<Ty>();
		}

#if !defined(SAT_DIST)
		bool Package();
#endif

	private:
		Ref<RefTarget> CreateResourceFromType( PackagedResourceType type );
		
#if defined(SAT_DIST)
		Ref<RefTarget> CreateResourceFromType_Dist( PackagedResourceType type );
		bool ReadBundle();
#else
		Ref<RefTarget> CreateResourceFromType_Development( PackagedResourceType type );
#endif

	private:
		std::unordered_map<PackagedResourceType, Ref<class RefTarget>> m_Resources;

#if defined(SAT_DIST)
		bool m_LoadedBundle = false;
#endif
	};
	
}
