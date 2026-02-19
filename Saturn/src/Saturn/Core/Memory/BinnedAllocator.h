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

#pragma once

#include <array>
#include <vector>

namespace Saturn {

	class FBinnedAllocator
	{
	public:
		// 4KIB max page
		static constexpr size_t PAGE_SIZE = 4096;
		static constexpr size_t NUM_BINS  = 10;

		static constexpr std::array<size_t, NUM_BINS> BIN_SIZES = { 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 };
	public:
		explicit FBinnedAllocator() 
		{
			m_FreeLists.fill( nullptr );
		}

		~FBinnedAllocator() 
		{
			for( void* pPage : m_AllocatedPages )
			{
				std::free( pPage );
			}
		}

		void* Allocate( size_t size ) 
		{
			const auto binIndex = GetBinIndex( size );
			if( !m_FreeLists[ binIndex ] )
			{
				// ... allocate new page
				AllocateNewPage( binIndex );
			}

			// Pop from free list
			void* pBlock = m_FreeLists[ binIndex ];
			m_FreeLists[ binIndex ] = *reinterpret_cast< void** >( pBlock );

			return pBlock;
		}

		void Free( void* pPtr, size_t size ) 
		{
			const size_t binIndex = GetBinIndex( size );

			// Move ptr back into the free list
			*reinterpret_cast< void** >( pPtr ) = m_FreeLists[ binIndex ];
			m_FreeLists[ binIndex ] = pPtr;
		}

	private:
		size_t GetBinIndex( size_t blockSize ) 
		{
			for( size_t i = 0; i < NUM_BINS; i++ )
			{
				if( blockSize <= BIN_SIZES[ i ] )
					return i;
			}

			return -1;
		}

		void AllocateNewPage( size_t binIndex ) 
		{
			const size_t blockSize = BIN_SIZES[ binIndex ];
			const size_t blocksPerPage = PAGE_SIZE / blockSize;

			void* pPage = std::malloc( PAGE_SIZE );
			if( !pPage ) throw std::bad_alloc();

			m_AllocatedPages.push_back( pPage );

			uint8_t* pCurrent = static_cast< uint8_t* >( pPage );
			for( size_t i = 0; i < blocksPerPage; i++ )
			{
				void* pNext = ( i + 1 < blocksPerPage ) ? ( pCurrent + blockSize ) : nullptr;

				*reinterpret_cast< void** >( pCurrent ) = pNext;
				pCurrent += blockSize;
			}

			m_FreeLists[ binIndex ] = pPage;
		}

	private:
		std::array<void*, NUM_BINS> m_FreeLists;
		std::vector<void*> m_AllocatedPages;
	};
	
}
