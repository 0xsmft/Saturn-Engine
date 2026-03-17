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

#if defined(SAT_WITH_STEAM)

#include "SteamFriend.h"

#include <vector>

namespace Saturn {

	class SteamUser
	{
	public:
		SteamUser() = default;
		~SteamUser() = default;

		void Initialise();

	public:
		[[nodiscard]] bool IsLoggedOn() const;

		// Rebuilds the friends list.
		void RefreshFriends();

		// NOTE: Despite the name this function checks if a user
		// owns a DLC for the app.
		// 
		// @param appID -- the DLC app to check if the user owns it.
		bool UserHasLicenseForApp( uint32_t appID ) const;

		// Returns the account level
		// For example, my level is 41
		// this function would return 41.
		int GetUserLevel() const;

		// Returns if 2FA is enabled, useful when you only want certain
		// users to be able to access certain things.
		[[nodiscard]] bool IsTwoFactorEnabled() const;

		[[nodiscard]] bool IsPhoneVerified() const;

	public:
		//////////////////////////////////////////////////////////////////////////
		// STATS AND ACHIEVEMENTS

		// Store stats to Steam.
		// 
		// NOTE: May fail if nothing is sent to Steam
		//		 Rate limited by steam, send in order of minutes and not seconds.
		//		 If you've just set an achievement there is no need to call this! SetAchievement already does that for you.
		bool StoreStats();

		// Sets / update the value of a stat & uploads it to steam
		//
		// NOTE: This does NOT set an achievement please call SetAchievement!
		// 
		// @param rName -- name of the stat in App Admin on the Steamworks website.
		// @param data  -- value to be set, NOTE: This value will not increment you must do that yourself! Or call IncrementStat.
		//
		bool SetStat( const std::string& rName, int32_t data );
		
		// Sets / update the value of a stat & uploads it to steam
		//
		// NOTE: This does NOT set an achievement please call SetAchievement!
		// 
		// @param rName -- name of the stat in App Admin on the Steamworks website.
		// @param data  -- value to be set, NOTE: This value will not increment you must do that yourself! Or call IncrementStat.
		//
		bool SetStat( const std::string& rName, float data );

		// Update a stat that is an Average in Steamworks
		//
		// NOTE: Stat name MUST be an Average!
		//		 To get the new result you can call GetStat afterwards.
		// 
		// @param rName -- name of the stat in App Admin on the Steamworks website.
		// @param accumSinceLastCall -- the value accumulation before this call
		// @param durationSec -- duration since this was last called.
		//
		bool UpdateStatAvg( const std::string& rName, float accumSinceLastCall, double durationSec );

		// Increment the value of a stat by n. & uploads it to steam
		//
		// NOTE: This does NOT set an achievement please call SetAchievement!
		// 
		// @param rName -- name of the stat in App Admin on the Steamworks website.
		// @param incrementAmount -- amount to be increased by.
		//
		bool IncrementStat( const std::string& rName, int32_t incrementAmount );
		
		// Increment the value of a stat by n. & uploads it to steam
		//
		// NOTE: This does NOT set an achievement please call SetAchievement!
		// 
		// @param rName -- name of the stat in App Admin on the Steamworks website.
		// @param incrementAmount -- amount to be increased by.
		//
		bool IncrementStat( const std::string& rName, float incrementAmount );

		// Get stat from steam
		//
		// NOTE: This does NOT get an achievement, please call IsAchievementObtained! However, if an achievement is tied to a stat, then yes, use this function.
		// 
		// @param rName -- name of the stat in App Admin on the Steamworks website.
		// @param pValue -- the variable to return the stat value in.
		//
		bool GetStat( const std::string& rName, int32_t* pValue );

		// Get stat from steam
		//
		// NOTE: This does NOT get an achievement, please call IsAchievementObtained! However, if an achievement is tied to a stat, then yes, use this function.
		// 
		// @param rName -- name of the stat in App Admin on the Steamworks website.
		// @param pValue -- the variable to return the stat value in.
		//
		bool GetStat( const std::string& rName, float* pValue );

		//////////////////////////////////////////////////////////////////////////

		// Unlock achievement & sends to Steam to confirm (i.e. calls StoreStats)
		//
		// @param rName -- name of the achievement in App Admin on the Steamworks website.
		//
		bool UnlockAchievement( const std::string& rName );

		// Remove (relock) achievement from player & sends to Steam to confirm
		// 
		// @param rName -- name of the achievement in App Admin on the Steamworks website.
		//
		bool ClearAchievement( const std::string& rName );

		// Checks if the user has an achievement... pretty obvious
		// 
		// @param rName -- name of the achievement in App Admin on the Steamworks website.
		//
		bool IsAchievementObtained( const std::string& rName ) const;

		// Gets achievement percent
		//
		// @param rName -- name of the achievement in App Admin on the Steamworks website.
		//
		float GetAchievementPercent( const std::string& rName );

		// Bring up steam popup to show the progress.
		// See: https://partner.steamgames.com/doc/api/ISteamUserStats#IndicateAchievementProgress
		// 
		// @param rName -- name of the achievement in App Admin on the Steamworks website.
		// @param rName -- current progress.
		// @param rName -- max progress.
		//
		bool ShowAchievementProgress( const std::string& rName, int32_t current, int32_t max );

		// Shouldn't really need this because the Game would already have a list of Achievements
		// However, this can be used to find discrepancies between Steamworks and the Game itself.
		uint32_t GetNumberOfAchievements();

		// It is not recommended to call this every frame due to copying the vector of names.
		//
		// API Name may look like: ACH_OPEN_THE_GAME
		// 
		// @returns the API name for all achievement names.
		//
		std::vector<std::string> GetAllAchievementNames();

	public:
		Steam::CSteamID GetNativeID() const { return m_UserID; }
		OnlinePresence GetPresence() const { return m_UserPresence; }
		std::wstring GetUserName() const { return m_UserName; }

		std::vector<SteamFriend>& GetFriends() { return m_Friends; }
		const std::vector<SteamFriend>& GetFriends() const { return m_Friends; }

	private:
		Steam::CSteamID m_UserID;
		OnlinePresence m_UserPresence = OnlinePresence::Offline;
		std::wstring m_UserName;
		std::vector<SteamFriend> m_Friends;
	};

}

#endif
