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
#include "EnvironmentVariables.h"

#include "EnvironmentVariablesStorage.h"

namespace Saturn::Auxiliary {

	std::optional<std::filesystem::path> GetEnvironmentVariable( const std::string& rKey )
	{
		return EnvironmentVariablesStorage::Get().GetVariable( rKey );
	}

	void SetEnvironmentVariable( const std::string& rKey, const std::filesystem::path& rValue )
	{
		EnvironmentVariablesStorage::Get().SetVariable( rKey, rValue );
	}

	std::optional<std::filesystem::path> System_GetEnvironmentVariable( const std::string& rKey )
	{
#if defined( SAT_PLATFORM_WINDOWS )
		HKEY key;
		LPCSTR keyPath = "Environment";
		DWORD keyWasCreated;
		LSTATUS result = ::RegCreateKeyExA( HKEY_CURRENT_USER, keyPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, &keyWasCreated );

		if( result == ERROR_SUCCESS )
		{
			DWORD type;
			char* pBuffer = new char[ 512 ];
			DWORD bufferSize = 512;

			result = ::RegGetValueA( key, NULL, rKey.c_str(), RRF_RT_ANY, &type, ( PBYTE ) pBuffer, &bufferSize );

			::RegCloseKey( key );

			if( result == ERROR_SUCCESS )
			{
				std::string res( pBuffer );
				delete[] pBuffer;

				return std::filesystem::path( res );
			}
			else
			{
				DWORD errorCode = ::GetLastError();
				LPTSTR error = NULL;

				::FormatMessage(
					FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					result,
					MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
					( LPTSTR ) &error,
					0,
					NULL
				);

				SAT_CORE_ASSERT( false, "HResult failed." );

				::LocalFree( error );
				error = NULL;
			}
		}
#else
		const char* pValue = getenv( rKey.c_str() );
		if( pValue )
			return std::filesystem::path( pValue );
#endif
		return std::nullopt;
	}

	void System_SetEnvironmentVariable( const std::string& rKey, const std::string& rValue )
	{
#if defined( SAT_PLATFORM_WINDOWS )
		HKEY key;
		DWORD keyWasCreated;
		LONG result = ::RegCreateKeyExA( HKEY_CURRENT_USER, "Environment", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, &keyWasCreated );

		if( result == ERROR_SUCCESS )
		{
			result = ::RegSetValueExA( key, rKey.c_str(), 0, REG_SZ, ( PBYTE ) rValue.c_str(), ( DWORD ) rValue.size() + 1 );
			::RegCloseKey( key );

			if( result == ERROR_SUCCESS )
			{
				// Alert other processes that an environment variable has changed...
				::SendMessageTimeoutA( HWND_BROADCAST, WM_SETTINGCHANGE, 0, ( LPARAM ) "Environment", SMTO_BLOCK, 100, NULL );
				return;
			}
			else
			{
				DWORD errorCode = ::GetLastError();
				LPTSTR error = NULL;

				::FormatMessage(
					FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					result,
					MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
					( LPTSTR ) &error,
					0,
					NULL
				);

				SAT_CORE_ASSERT( false, "HResult failed." );

				::LocalFree( error );
				error = NULL;
			}
		}
		else
		{
			DWORD errorCode = ::GetLastError();
			LPTSTR error = NULL;

			::FormatMessage(
				FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				result,
				MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
				( LPTSTR ) &error,
				0,
				NULL
			);

			SAT_CORE_ASSERT( false, "HResult failed." );

			::LocalFree( error );
			error = NULL;
		}
#else
		if( !setenv( rKey.c_str(), rValue.c_str(), 1 ) ) 
		{
			SAT_CORE_ERROR( "Unable to set environment variable!" );
		}
#endif
	}

}
