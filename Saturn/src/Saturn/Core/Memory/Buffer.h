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
#include <cstring>

namespace Saturn {

	class Buffer
	{
	public:
		Buffer() : Size( 0 ), Data( nullptr ) {}
		Buffer( uint32_t size, uint8_t* pData ) : Size( size ), Data( pData ) {}
		Buffer( uint32_t size, void* pData ) : Size( size ), Data( (uint8_t*)pData ) {}
		~Buffer() {}

		void Zero_Memory()
		{
			if( Data )
				memset( Data, 0, Size );
		}

		void Free() 
		{
			if( Data )
			{
				delete[] Data;
				Data = nullptr;
				Size = 0;
			}
		}

		template<typename Ty>
		Ty& Read( uint32_t Offset = 0 ) 
		{
			return *( Ty* ) ( Data + Offset );
		}

		template<typename Ty>
		Ty* As() 
		{
			return ( Ty* ) Data;
		}

		void Write( const void* pData, size_t size, uint32_t Offset )
		{
			SAT_CORE_ASSERT( Offset + size <= Size );

			memcpy( Data + Offset, pData, size );
		}

		// Clears the buffer and then reallocates it to the specified size.
		void Allocate( size_t size )
		{
			delete[] Data;
			Data = nullptr;

			if( size == 0 )
				return;

			Data = new uint8_t[ size ];
			Size = size;
		}

		static Buffer Copy( const void* pData, size_t size )
		{
			Buffer buffer;
			
			// Allocate the buffer
			buffer.Allocate( size );

			memcpy( buffer.Data, pData, size );

			return buffer;
		}

		operator bool() { return Data != nullptr; }
		uint8_t& operator [] ( uint32_t Offset ) { return Data[ Offset ]; }
		uint8_t operator [] ( uint32_t Offset ) const { return Data[ Offset ]; }
		
	public:
		size_t Size;
		uint8_t* Data;
	};
}