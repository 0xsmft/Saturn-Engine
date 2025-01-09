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

#include "sppch.h"
#include "EnvironmentVariables.h"

namespace Saturn {

	namespace Auxiliary {
		
		bool HasEnvironmentVariable( const std::string& rKey )
		{
			HKEY hKey;
			LONG lResult = RegOpenKeyExA( HKEY_CURRENT_USER, "Environment", 0, KEY_READ, &hKey );

			if( lResult == ERROR_SUCCESS )
			{
				DWORD dwType = REG_SZ;
				DWORD dwSize = 0;
				lResult = RegQueryValueExA( hKey, rKey.c_str(), NULL, &dwType, NULL, &dwSize );
				RegCloseKey( hKey );
				return lResult == ERROR_SUCCESS;
			}

			return lResult == ERROR_SUCCESS;
		}

		std::string GetEnvironmentVariable( const std::string& rKey )
		{
			HKEY hKey;
			LPCSTR keyPath = "Environment";
			DWORD dwKeyWasCreated;
			LSTATUS lResult = RegCreateKeyExA( HKEY_CURRENT_USER, keyPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwKeyWasCreated );

			if( lResult == ERROR_SUCCESS )
			{
				DWORD dwType;
				char* pBuffer = new char[ 512 ];
				DWORD dwSize = 512;
				
				lResult = RegGetValueA( hKey, NULL, rKey.c_str(), RRF_RT_ANY, &dwType, ( PBYTE ) pBuffer, &dwSize );

				RegCloseKey( hKey );

				if( lResult == ERROR_SUCCESS )
				{
					std::string res( pBuffer );

					delete[] pBuffer;

					return res;
				}
				else
				{
					DWORD ErrorCode = GetLastError();
					LPTSTR Error = NULL;

					FormatMessage(
						FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,
						lResult,
						MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
						( LPTSTR ) &Error,
						0,
						NULL
					);

					SAT_CORE_ASSERT( false, "HResult failed." );

					LocalFree( Error );
					Error = NULL;
				}
			}

			return "";
		}

		std::wstring GetEnvironmentVariableWs( const std::wstring& rKey )
		{
			HKEY hKey;
			LPCSTR keyPath = "Environment";
			DWORD dwKeyWasCreated;
			LSTATUS lResult = RegCreateKeyExA( HKEY_CURRENT_USER, keyPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwKeyWasCreated );

			if( lResult == ERROR_SUCCESS )
			{
				DWORD dwType;
				wchar_t* pBuffer = new wchar_t[ 512 ];
				DWORD dwSize = 512;

				lResult = RegGetValueW( hKey, NULL, rKey.c_str(), RRF_RT_ANY, &dwType, ( PBYTE ) pBuffer, &dwSize );

				RegCloseKey( hKey );

				if( lResult == ERROR_SUCCESS )
				{
					std::wstring res( pBuffer );

					delete[] pBuffer;

					return res;
				}
				else
				{
					DWORD ErrorCode = GetLastError();
					LPTSTR Error = NULL;

					FormatMessage(
						FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,
						lResult,
						MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
						( LPTSTR ) &Error,
						0,
						NULL
					);

					SAT_CORE_ASSERT( false, "HResult failed." );

					LocalFree( Error );
					Error = NULL;
				}
			}

			return L"";
		}

		void SetEnvironmentVariable( const std::string& rKey, const std::string& rValue )
		{
			HKEY hKey;
			DWORD dwKeyWasCreated;
			LONG lResult = RegCreateKeyExA( HKEY_CURRENT_USER, "Environment", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwKeyWasCreated );

			if( lResult == ERROR_SUCCESS )
			{
				lResult = RegSetValueExA( hKey, rKey.c_str(), 0, REG_SZ, ( PBYTE ) rValue.c_str(), (DWORD) rValue.size() + 1 );
				RegCloseKey( hKey );

				if( lResult == ERROR_SUCCESS )
				{
					SendMessageTimeoutA( HWND_BROADCAST, WM_SETTINGCHANGE, 0, ( LPARAM ) "Environment", SMTO_BLOCK, 100, NULL );

					return;
				}
				else
				{
					DWORD ErrorCode = GetLastError();
					LPTSTR Error = NULL;

					FormatMessage(
						FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,
						lResult,
						MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
						( LPTSTR ) &Error,
						0,
						NULL
					);

					SAT_CORE_ASSERT( false, "HResult failed." );

					LocalFree( Error );
					Error = NULL;
				}
			}
			else
			{
				DWORD ErrorCode = GetLastError();
				LPTSTR Error = NULL;

				FormatMessage(
					FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					lResult,
					MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
					( LPTSTR ) &Error,
					0,
					NULL
				);

				SAT_CORE_ASSERT( false, "HResult failed." );

				LocalFree( Error );
				Error = NULL;
			}
		}
	}
}