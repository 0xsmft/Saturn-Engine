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

#include "SingletonStorage.h"

#include <thread>
#include <iostream>
#include <mutex>

namespace Saturn {

	static std::unordered_map<std::type_index, SingletonHolder> s_Singletons;

	std::unordered_map<std::type_index, Saturn::SingletonHolder>& SingletonStorage::GetSingletonMap()
	{
		return s_Singletons;
	}

#if !defined(SATURN_SS_STATIC)
	static std::mutex s_Mutex;
#endif

	void SS_API Internal::FindSharedClassInstance( const std::type_index& rIndex, void* ( *pStaticClass )( ), void*& pOutInstace )
	{
		SingletonHolder type = {};

		if( s_Singletons.find( rIndex ) == s_Singletons.end() )
		{
			type.pObject = ( *pStaticClass )( );
			s_Singletons.insert( { rIndex, type } );
		}
		else
		{
			type = s_Singletons.at( rIndex );
		}

		if( type.pObject == nullptr )
		{
			// Create the static instance.
			type.pObject = ( *pStaticClass )( );
		}

		pOutInstace = type.pObject;
	}
}
