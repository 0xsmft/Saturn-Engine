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

#include <functional>
#include <algorithm>
#include <vector>
#include <type_traits>

// This Event class is based from Geno IDE's class, however slightly modified:
// Courtesy of https://github.com/Geno-IDE/Geno/blob/master/include/Common/Event.h

namespace Saturn {
	
	template<typename Instigator, typename FalseFunction>
	class Event 
	{
	};

	template<typename Instigator, typename... Params>
	class Event<Instigator, void(Params...)>
	{
	public:
		struct Receiver
		{
			std::function<void( Instigator&, Params... )> Function;
			uint64_t ID;
		};

	public:
		template<typename F>
		void operator+=( F&& rrFunctor )
		{
			static_assert( std::is_invocable_r_v<void, std::decay_t<F>, Instigator&, Params...>, "Event Receiver must be invocable as void(Instigator&, Params...), you may be missing an argument!" );

			Receiver r;
			r.Function = std::forward<F>( rrFunctor );
			r.ID = m_Counter++;

			m_Receivers.emplace_back( std::move( r ) );
		}

		void operator()( Instigator& rSender, Params... params )
		{
			std::vector<Receiver> copy( m_Receivers.begin(), m_Receivers.end() );
			for( auto& rReceiver : copy )
			{
				std::invoke( rReceiver.Function, rSender, params... );
			}

			while( !copy.empty() )
			{
				typename std::vector<Receiver>::iterator itr = std::find_if( m_Receivers.begin(), m_Receivers.end(),
					[ &copy ]( const Receiver& rCandidate )
				{
					return rCandidate.ID == copy.back().ID;
				} );

				m_Receivers.erase( itr );
				copy.pop_back();
			}
		}

	private:
		std::vector<Receiver> m_Receivers;
		uint64_t m_Counter = 0;
	};

}
