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

#if defined(SAT_WITH_STEAM)
#include "SteamOnlineSystemAPI.h"

#include "Saturn/Project/Project.h"

#include <steam/steam_api.h>

#include <filesystem>

namespace Saturn {

	SteamOnlineSystemAPI::SteamOnlineSystemAPI()
	{
		SingletonStorage::AddSingleton( this );
	}

	SteamOnlineSystemAPI::~SteamOnlineSystemAPI()
	{
		SAT_CORE_VERIFY( !m_Initialised, "SteamOnlineSystemAPI::Terminate not called before this SteamOnlineSystemAPI destroys." )

		SingletonStorage::RemoveSingleton( this );
	}

	static void CreateSteamAppIDFileIfNeeded() 
	{
		// NOTE: Relative to the Editor/Game working dir
		if( !std::filesystem::exists( "steam_appid.txt" ) )
		{
			std::ofstream fout( "steam_appid.txt", std::ios::trunc );
			fout << Project::GetActiveProject()->GetOnlineAppID() << std::endl;
			fout.close();
		}
	}

	static void DeleteSteamAPIFileIfNeeded() 
	{
		if( std::filesystem::exists( "steam_appid.txt" ) )
			std::filesystem::remove( "steam_appid.txt" );
	}

	bool SteamOnlineSystemAPI::Initialise()
	{
		CreateSteamAppIDFileIfNeeded();

		bool result = false;

		SteamErrMsg errorMessage{0};
		switch( SteamAPI_InitEx( &errorMessage ) )
		{
			case k_ESteamAPIInitResult_OK: 
			{
				SAT_CORE_INFO( "[SteamOnlineSystemAPI]: Initialised SteamAPI" );
				result = true;

				m_CurrentUser.Initialise();
			} break;

			case k_ESteamAPIInitResult_FailedGeneric: 
			{
				SAT_CORE_ERROR( "[SteamOnlineSystemAPI]: Failed to initialise! SteamAPI_InitEx retured 0x01 (FailedGeneric)!" );
				SAT_CORE_ERROR( "[SteamOnlineSystemAPI]: Steam Error Message: {0}", errorMessage );
			} break;

			case k_ESteamAPIInitResult_NoSteamClient:
			{
				SAT_CORE_ERROR( "[SteamOnlineSystemAPI]: Failed to initialise! SteamAPI_InitEx retured 0x02, is steam running?" );
				SAT_CORE_ERROR( "[SteamOnlineSystemAPI]: Steam Error Message: {0}", errorMessage );
			} break;

			case k_ESteamAPIInitResult_VersionMismatch: 
			{
				SAT_CORE_ERROR( "[SteamOnlineSystemAPI]: Failed to initialise! SteamAPI_InitEx retured 0x03, version mismatch." );
				SAT_CORE_ERROR( "[SteamOnlineSystemAPI]: Steam Error Message: {0}", errorMessage );
			} break;

			default: break;
		}

		return result;
	}

	void SteamOnlineSystemAPI::Tick()
	{
		if( m_Initialised )
		{
			SteamAPI_RunCallbacks();
		}
	}

	void SteamOnlineSystemAPI::Terminate()
	{
		SteamAPI_Shutdown();
		DeleteSteamAPIFileIfNeeded();
		m_Initialised = false;
	}

	void SteamOnlineSystemAPI::SetOverlayLocation( ENotificationPosition position )
	{
		SteamUtils()->SetOverlayNotificationPosition( position );
	}

	uint32_t SteamOnlineSystemAPI::GetNumSecondsSinceAppActive()
	{
		return SteamUtils()->GetSecondsSinceAppActive();
	}

}

#endif
