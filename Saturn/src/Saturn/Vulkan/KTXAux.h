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

#include "Saturn/Core/Log.h"

#include <KTX/ktx.h>
#include <string>

#define VK_KTX_CHECK( x ) _VkKtxCheckResult( x )

inline std::string_view KtxErrorResultToStr( ktx_error_code_e Result )
{
	switch( Result )
	{
		case KTX_SUCCESS:
			return "KTX_SUCCESS";
		case KTX_FILE_DATA_ERROR:
			return "KTX_FILE_DATA_ERROR";
		case KTX_FILE_ISPIPE:
			return "KTX_FILE_ISPIPE";
		case KTX_FILE_OPEN_FAILED:
			return "KTX_FILE_OPEN_FAILED";
		case KTX_FILE_OVERFLOW:
			return "KTX_FILE_OVERFLOW";
		case KTX_FILE_READ_ERROR:
			return "KTX_FILE_READ_ERROR";
		case KTX_FILE_SEEK_ERROR:
			return "KTX_FILE_SEEK_ERROR";
		case KTX_FILE_UNEXPECTED_EOF:
			return "KTX_FILE_UNEXPECTED_EOF";
		case KTX_FILE_WRITE_ERROR:
			return "KTX_FILE_WRITE_ERROR";
		case KTX_GL_ERROR:
			return "KTX_GL_ERROR";
		case KTX_INVALID_OPERATION:
			return "KTX_INVALID_OPERATION";
		case KTX_INVALID_VALUE:
			return "KTX_INVALID_VALUE";
		case KTX_NOT_FOUND:
			return "KTX_NOT_FOUND";
		case KTX_OUT_OF_MEMORY:
			return "KTX_OUT_OF_MEMORY";
		case KTX_TRANSCODE_FAILED:
			return "KTX_TRANSCODE_FAILED";
		case KTX_UNKNOWN_FILE_FORMAT:
			return "KTX_UNKNOWN_FILE_FORMAT";
		case KTX_UNSUPPORTED_TEXTURE_TYPE:
			return "KTX_UNSUPPORTED_TEXTURE_TYPE";
		case KTX_UNSUPPORTED_FEATURE:
			return "KTX_UNSUPPORTED_FEATURE";
		case KTX_LIBRARY_NOT_LINKED:
			return "KTX_LIBRARY_NOT_LINKED";
		case KTX_DECOMPRESS_LENGTH_ERROR:
			return "KTX_DECOMPRESS_LENGTH_ERROR";
		case KTX_DECOMPRESS_CHECKSUM_ERROR:
			return "KTX_DECOMPRESS_CHECKSUM_ERROR";

		default: break;
	}

	return "KTX_ERROR_UNKNOWN";
}

inline void _VkKtxCheckResult( ktx_error_code_e Result )
{
	if( Result != KTX_SUCCESS )
	{
		auto ErrorStr = KtxErrorResultToStr( Result );

		SAT_CORE_INFO( "[Vulkan KTX Error] {0}", ErrorStr );

#if defined(SAT_DEBUG) || defined(SAT_RELEASE)

#if defined( _WIN32 )
		__debugbreak();
#else
		raise( SIGTRAP );
#endif // _MSC_VER

#else
		std::string errorMsg = std::format( "Vulkan KTX Result failed: {0}", ErrorStr );
		SAT_CORE_VERIFY( false, errorMsg );
#endif
	}
}