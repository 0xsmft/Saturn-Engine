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

#include <xhash>

namespace Saturn {

	// UUID (universally unique identifier)
	class UUID
	{
	public:
		UUID();
		UUID( uint64_t uuid );
		UUID( const UUID& other );

		operator uint64_t() { return m_UUID; }
		operator const uint64_t() const { return m_UUID; }

	private:
		inline void SetID( uint64_t id ) { m_UUID = id; }

	private:
		uint64_t m_UUID;

	private:
		friend class RawSerialisation;
	};

}

namespace std {

	template <>
	struct hash<Saturn::UUID>
	{
		std::size_t operator()( const Saturn::UUID& uuid ) const
		{
			return hash<uint64_t>()( ( uint64_t )uuid );
		}
	};

	template<>
	struct formatter<Saturn::UUID>
	{
		constexpr auto parse( format_parse_context& ctx )
		{
			return ctx.begin();
		}

		template<typename FormatContext>
		auto format( Saturn::UUID result, FormatContext& ctx ) const
		{
			return std::format_to( ctx.out(), "{}", static_cast< uint64_t >( result ) );
		}
	};

}
