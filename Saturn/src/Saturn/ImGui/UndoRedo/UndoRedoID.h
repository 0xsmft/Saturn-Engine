/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2026 0xsmft https://0xsmft.dev                                       *
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

#include "Saturn/Core/HashFNV1A.h"

namespace Saturn {
	
	// 64 bit ID, comes from a hash of a string.
	typedef uint64_t UndoRedoID;

	constexpr uint64_t kInvalidUndoRedoID = 0;
	
	namespace Auxiliary {
		
		template<typename... Args>
		static UndoRedoID BuildUndoRedoIDFormatted( std::format_string<Args...> fmt, Args&&... rrArgs ) 
		{
			std::string text;
			std::format_to( std::back_inserter( text ), fmt, std::forward<Args>( rrArgs )... );

			return FNV1A64( text.c_str() );
		}
		
		static UndoRedoID BuildUndoRedoID( const std::string& rPrefix, const std::string& rRestOfText )
		{
			std::string fullID = rPrefix + rRestOfText;
			return FNV1A64( fullID.c_str() );
		}

		static UndoRedoID BuildUndoRedoID( const std::string& rRestOfText )
		{
			std::string fullID = rRestOfText;
			return FNV1A64( fullID.c_str() );
		}
	}

}
