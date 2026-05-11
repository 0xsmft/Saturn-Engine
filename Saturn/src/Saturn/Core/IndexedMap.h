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

#include <vector>

namespace Saturn {

	//
	// Array of key-value pairs
	//
	template<typename Key, typename Value>
	class IndexedMap 
	{
	public:
		using PairType = std::pair<Key, Value>;
		using Container = std::vector<PairType, std::allocator<PairType>>;

		[[nodiscard]] Value Find( const Key& rKey ) const
		{
			for( auto& rPair : m_Container )
				if( rPair.first == rKey )
					return rPair.second;

			return nullptr;
		}

		[[nodiscard]] Value& operator[](const Key& rKey)
		{
			for( auto& rPair : m_Container )
				if( rPair.first == rKey )
					return rPair.second;

			auto& rPairKV = m_Container.emplace_back( rKey, Value{} );
			return rPairKV.second;
		}

		[[nodiscard]] auto begin() noexcept       { return m_Container.begin(); }
		[[nodiscard]] auto end()   noexcept       { return m_Container.end(); }
		[[nodiscard]] auto begin() const noexcept { return m_Container.begin(); }
		[[nodiscard]] auto end()   const noexcept { return m_Container.end(); }

	private:
		Container m_Container;
	};	
	
}
