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

#include <typeindex>
#include <unordered_map>

#include "SSBase.h"

// Not the best way. 
// "needs to have dll-interface to be used by clients of class 'Saturn::SingletonStorage'"
#if defined(_MSC_VER)
#pragma warning(disable:4251)
#endif

namespace Saturn {

	struct SingletonHolder
	{
		void* pObject = nullptr;
	};

	namespace Internal {

		void SS_API FindSharedClassInstance( const std::type_index& rIndex, void* (*pStaticClass)(), void*& pOutInstace );

		template<typename Ty>
		class InternalSingleton
		{
		public:
			static Ty* Instance()
			{
				static void* pClass = nullptr;

				if( !pClass ) 
					FindSharedClassInstance( typeid( Ty ), &GetStaticClass, pClass );

				return reinterpret_cast<Ty*>( pClass );
			}

		private:
			// Possible undefined behavior.
			static void* GetStaticClass()
			{
				static Ty _; return static_cast<void*>(&_);
			}
		};
	}

	class SS_API SingletonStorage
	{
	public:
		template<typename Ty>
		static Ty* GetOrCreateSingleton()
		{
			return Internal::InternalSingleton<Ty>::Instance();
		}

		template<typename Ty>
		static Ty* GetSingleton()
		{
			std::type_index info = typeid( Ty );
			auto& map = GetSingletonMap();

			return reinterpret_cast<Ty*>( map[ info ].pObject );
		}

		template<typename Ty>
		static void AddSingleton( Ty* type )
		{
			std::type_index info = typeid( Ty );
			auto& map = GetSingletonMap();

			auto Itr = map.find( info );

			if( Itr == map.end() )
			{
				map[ info ] = { .pObject = type };
			}
		}

		template<typename Ty>
		static void RemoveSingleton( Ty* type )
		{
			std::type_index info = typeid( Ty );
			auto& map = GetSingletonMap();

			map.erase( info );
		}

	private:
		static std::unordered_map<std::type_index, SingletonHolder>& GetSingletonMap();
	};
}
