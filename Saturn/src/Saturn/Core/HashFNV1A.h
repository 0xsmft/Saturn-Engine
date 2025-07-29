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

#include <stdint.h>

namespace Saturn {

	static constexpr uint64_t FNV1A64( const char* pStr )
	{
		constexpr uint64_t offset = 14695981039346656037ull;
		constexpr uint64_t prime = 1099511628211ull;

		uint64_t hash = offset;
		while( *pStr )
		{
			hash ^= static_cast< uint64_t >( *pStr++ );
			hash *= prime;
		}

		return hash;
	}
	
	static constexpr uint32_t FNV1A32( const char* pStr ) 
	{
		constexpr uint32_t offset = 2166136261;
		constexpr uint32_t prime = 16777619;

		uint32_t hash = offset;
		while( *pStr )
		{
			hash ^= static_cast< uint32_t >( *pStr++ );
			hash *= prime;
		}

		return hash;
	}
}
