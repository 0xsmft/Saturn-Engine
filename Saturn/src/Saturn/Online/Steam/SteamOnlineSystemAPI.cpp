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

namespace Steam {
	#include <steam/steam_api.h>
}

#include <filesystem>

namespace Saturn {

	SteamOnlineSystemAPI::SteamOnlineSystemAPI()
	{

	}

	SteamOnlineSystemAPI::~SteamOnlineSystemAPI()
	{

	}

	static void CreateSteamAppIDFileIfNeeded() 
	{
		// NOTE: Relative to the Editor working dir
		// OR
		// The game working dir
		if( !std::filesystem::exists( "steam_appid.txt" ) )
		{
			std::ofstream fout( "steam_appid.txt", std::ios::trunc );
			fout << "480" << std::endl;
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

		Steam::SteamErrMsg errorMessage{0};
		switch( Steam::SteamAPI_InitEx( &errorMessage ) )
		{
			case Steam::k_ESteamAPIInitResult_OK: 
			{
				SAT_CORE_INFO( "[SteamOnlineSystemAPI]: Initialised SteamAPI" );
				result = true;

				m_CurrentUser.Initialise();
			} break;

			case Steam::k_ESteamAPIInitResult_FailedGeneric: 
			{
				SAT_CORE_ERROR( "[SteamOnlineSystemAPI]: Failed to initialise! SteamAPI_InitEx retured 0x01 (FailedGeneric)!" );
				SAT_CORE_ERROR( "[SteamOnlineSystemAPI]: Steam Error Message: {0}", errorMessage );
			} break;

			case Steam::k_ESteamAPIInitResult_NoSteamClient:
			{
				SAT_CORE_ERROR( "[SteamOnlineSystemAPI]: Failed to initialise! SteamAPI_InitEx retured 0x02, is steam running?" );
				SAT_CORE_ERROR( "[SteamOnlineSystemAPI]: Steam Error Message: {0}", errorMessage );
			} break;

			case Steam::k_ESteamAPIInitResult_VersionMismatch: 
			{
				SAT_CORE_ERROR( "[SteamOnlineSystemAPI]: Failed to initialise! SteamAPI_InitEx retured 0x03, version mismatch." );
				SAT_CORE_ERROR( "[SteamOnlineSystemAPI]: Steam Error Message: {0}", errorMessage );
			} break;

			default: break;
		}

		return result;
	}

	void SteamOnlineSystemAPI::Terminate()
	{
		Steam::SteamAPI_Shutdown();
		DeleteSteamAPIFileIfNeeded();
	}

}

#endif
