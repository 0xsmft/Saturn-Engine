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
#include <fstream>

namespace Saturn {

	class PakFileMemoryBuffer : public std::streambuf
	{
	public:
		PakFileMemoryBuffer( char* begin, char* end )
		{
			Set( begin, end );
		}

		PakFileMemoryBuffer( std::vector<char>& rStream )
		{
			Set( rStream.data(), rStream.data() + rStream.size() );
		}

		virtual ~PakFileMemoryBuffer() noexcept {}

	private:
		void Set( char* pFirst, char* pEnd )
		{
			setg( pFirst, pFirst, pEnd );
		}

	protected:
		pos_type __CLR_OR_THIS_CALL seekpos( pos_type position, std::ios_base::openmode = std::ios_base::in | std::ios_base::out ) override
		{
			char* pNewPos = eback() + position;

			// Check if the new position is within the buffer
			if( pNewPos < egptr() && pNewPos >= eback() )
			{
				setg( eback(), pNewPos, egptr() );
				return position;
			}
			else
			{
				return pos_type( off_type( -1 ) );
			}
		}

		pos_type __CLR_OR_THIS_CALL seekoff( off_type offset, std::ios_base::seekdir seekdir, std::ios_base::openmode = std::ios_base::in | std::ios_base::out ) override
		{
			char* pNewPos = nullptr;

			switch( seekdir )
			{
				case std::ios::beg:
					pNewPos = eback() + offset;
					break;

				case std::ios::cur:
					pNewPos = gptr() + offset;
					break;

				case std::ios::end:
					pNewPos = egptr() + offset;
					break;
			}

			// Check if the new position is within the buffer
			if( pNewPos < egptr() && pNewPos >= eback() )
			{
				setg( eback(), pNewPos, egptr() );

				return static_cast< pos_type >( gptr() - eback() );
			}
			else
			{
				return pos_type( off_type( -1 ) );
			}
		}
	};

}