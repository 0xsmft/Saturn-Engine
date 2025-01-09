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
#include "StorageBufferSet.h"

namespace Saturn {

	StorageBufferSet::StorageBufferSet( uint32_t size, uint32_t binding )
	{
	}

	StorageBufferSet::~StorageBufferSet()
	{
		for( auto& [set, binding] : m_Buffers )
		{
			for( auto& [ frame, frameMap ] : binding )
			{
				for( auto& [ index, buffer ] : frameMap )
				{
					buffer = nullptr;
				}
			}
		}

		m_Buffers.clear();
	}

	void StorageBufferSet::Create( uint32_t set, uint32_t binding )
	{
		Ref<StorageBuffer> sb = Ref<StorageBuffer>::Create( set, binding );

		Set( sb, set, binding );
	}

	void StorageBufferSet::Set( Ref<StorageBuffer>& rBuffer, uint32_t set, uint32_t binding )
	{
		for( int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
		{
			m_Buffers[ set ][ binding ][ i ] = rBuffer;
		}
	}

	void StorageBufferSet::Resize( uint32_t set, uint32_t binding, uint32_t frame, size_t newSize )
	{
		m_Buffers.at( set ).at( binding ).at( frame )->Resize( ( uint32_t ) newSize );
	}

	void StorageBufferSet::Resize( uint32_t set, uint32_t binding, size_t newSize )
	{
		for( int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
		{
			m_Buffers.at( set ).at( binding ).at( i )->Resize( ( uint32_t ) newSize );
		}
	}

	Ref<StorageBuffer> StorageBufferSet::Get( uint32_t set, uint32_t binding, uint32_t frame )
	{
		return m_Buffers.at( set ).at( binding ).at( frame );
	}
}