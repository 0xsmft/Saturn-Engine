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

#include "Saturn/Core/Base.h"

#define RC_CHECK( x ) _rcCheckResult(x)
#define DT_CHECK( x ) _dtCheckResult(x)

inline void _rcCheckResult( bool Result )
{
	if( !Result )
	{
		SAT_CORE_INFO( "[Recast/Detour Error] Function operation did not succeed!" );

		Saturn::Core::BreakDebug();
	}
}

inline void _dtCheckResult( unsigned int Result ) 
{
	if( Result & DT_SUCCESS )
		return;

	std::string errorCode;
	if( Result & DT_FAILURE )
		errorCode = "DT_FAILURE";
	
	if( Result & DT_IN_PROGRESS )
		errorCode = "DT_IN_PROGRESS";

	unsigned int detail = Result & DT_STATUS_DETAIL_MASK;
	if( detail & DT_WRONG_MAGIC )
		errorCode = "DT_WRONG_MAGIC";

	if( detail & DT_WRONG_VERSION )
		errorCode = "DT_WRONG_VERSION";

	if( detail & DT_OUT_OF_MEMORY )
		errorCode = "DT_OUT_OF_MEMORY";

	if( detail & DT_INVALID_PARAM )
		errorCode = "DT_INVALID_PARAM";

	if( detail & DT_BUFFER_TOO_SMALL )
		errorCode = "DT_BUFFER_TOO_SMALL";

	if( detail & DT_OUT_OF_NODES )
		errorCode = "DT_OUT_OF_NODES\n";

	if( detail & DT_PARTIAL_RESULT )
		errorCode = "DT_PARTIAL_RESULT";

	if( detail & DT_ALREADY_OCCUPIED )
		errorCode = "DT_ALREADY_OCCUPIED";

	SAT_CORE_INFO( "[Detour] Detour status check failed! STATUS/{0}", errorCode );

	Saturn::Core::BreakDebug();
}