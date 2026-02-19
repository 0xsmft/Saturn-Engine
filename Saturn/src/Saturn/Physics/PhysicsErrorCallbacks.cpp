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

#include "sppch.h"
#include "PhysicsErrorCallbacks.h"

namespace Saturn {

	void PhysicsAssertCallback::operator()( const char* pError, const char* pFile, int Line, bool& rTarget )
	{
		SAT_CORE_ERROR( "PhsyX error : {0}, File : {1}, Line : {2}", pError, pFile, Line );
		SAT_CORE_ASSERT( false );
	}

	void PhysicsErrorCallback::reportError( physx::PxErrorCode::Enum Code, const char* pMessage, const char* pFile, int Line )
	{
		switch( Code )
		{
			case physx::PxErrorCode::eNO_ERROR:
				SAT_CORE_INFO( "PhysX: {0} {1} {2}", pMessage, pFile, Line );
				break;

			case physx::PxErrorCode::eDEBUG_INFO:
				SAT_CORE_INFO( "PhysX: {0} {1} {2}", pMessage, pFile, Line );
				break;

			case physx::PxErrorCode::eDEBUG_WARNING:
				SAT_CORE_WARN( "PhysX: {0} {1} {2}", pMessage, pFile, Line );
				break;

			case physx::PxErrorCode::eINVALID_PARAMETER: 
			{
				SAT_CORE_WARN( "PhysX: {0} {1} {2}", pMessage, pFile, Line );
				SAT_CORE_WARN( "PhysX: Invalid paramater!" );
			} break;

			case physx::PxErrorCode::eINVALID_OPERATION: 
			{
				SAT_CORE_WARN( "PhysX: {0} {1} {2}", pMessage, pFile, Line );
				SAT_CORE_WARN( "PhysX: Invalid operation!" );
			}	break;

			case physx::PxErrorCode::eOUT_OF_MEMORY:
				SAT_CORE_ASSERT( false, "PhysX: Out of memory" );
				break;

			case physx::PxErrorCode::eINTERNAL_ERROR:
				SAT_CORE_ERROR( "PhysX: Interal Error! " );
				break;

			case physx::PxErrorCode::eABORT:
				SAT_CORE_ASSERT( false, "PhysX requested an abort!" );
				break;

			case physx::PxErrorCode::ePERF_WARNING:
				break;
			case physx::PxErrorCode::eMASK_ALL:
				break;
		}
	}

}
