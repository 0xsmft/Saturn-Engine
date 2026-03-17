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
#include "SteamUser.h"

#include "Saturn/Core/StringAuxiliary.h"

namespace Steam {
	#include <steam/steam_api.h>
	#include <steam/isteamuser.h>
	#include <steam/isteamfriends.h>
	#include <steam/isteamuserstats.h>

	static Saturn::OnlinePresence SteamPersonaStateToSaturn( EPersonaState state )
	{
		using namespace Saturn;

		switch( state )
		{
			case Steam::k_EPersonaStateOffline:
				return OnlinePresence::Offline;

			case Steam::k_EPersonaStateOnline:
				return OnlinePresence::Online;
			
			case Steam::k_EPersonaStateBusy:
			case Steam::k_EPersonaStateAway:
				return OnlinePresence::Away;
			
			case Steam::k_EPersonaStateSnooze:
			case Steam::k_EPersonaStateLookingToTrade:
			case Steam::k_EPersonaStateLookingToPlay:
				return OnlinePresence::Online;

			case Steam::k_EPersonaStateInvisible:
				return OnlinePresence::AppearingOffline;
			
			case Steam::k_EPersonaStateMax:
			default:
				break;
		}

		return OnlinePresence::Offline;
	}
}

namespace Saturn {

	void SteamUser::Initialise()
	{
		m_UserID = Steam::SteamUser()->GetSteamID();
		m_UserPresence = Steam::SteamPersonaStateToSaturn( Steam::SteamFriends()->GetPersonaState() );
		m_UserName = Auxiliary::ConvertString( Steam::SteamFriends()->GetPersonaName() );

		SAT_CORE_INFO( "[SteamUser] User ID: {0}, Presence: {1}", m_UserID.ConvertToUint64(), ( uint32_t ) m_UserPresence );

		RefreshFriends();
	}

	bool SteamUser::IsLoggedOn() const
	{
		return Steam::SteamUser()->BLoggedOn();
	}

	void SteamUser::RefreshFriends()
	{
		if( !IsLoggedOn() )
			return;

		m_Friends.resize( Steam::SteamFriends()->GetFriendCount( Steam::k_EFriendFlagImmediate ) );

		int32_t i = 0;
		for( auto& rFriend : m_Friends )
		{
			rFriend.UserID = Steam::SteamFriends()->GetFriendByIndex( i, Steam::k_EFriendFlagImmediate );
			rFriend.Name = Auxiliary::ConvertString( Steam::SteamFriends()->GetFriendPersonaName( rFriend.UserID ) );

			const auto presence = Steam::SteamFriends()->GetFriendPersonaState( rFriend.UserID );
			rFriend.Presence = Steam::SteamPersonaStateToSaturn( presence );

			++i;
		}
	}

	bool SteamUser::UserHasLicenseForApp( uint32_t appID ) const
	{
		return Steam::SteamUser()->UserHasLicenseForApp( m_UserID, appID );
	}

	int SteamUser::GetUserLevel() const
	{
		return Steam::SteamUser()->GetPlayerSteamLevel();
	}

	bool SteamUser::IsTwoFactorEnabled() const
	{
		return Steam::SteamUser()->BIsTwoFactorEnabled();
	}

	bool SteamUser::IsPhoneVerified() const
	{
		return Steam::SteamUser()->BIsPhoneVerified();
	}

	bool SteamUser::StoreStats()
	{
		return Steam::SteamUserStats()->StoreStats();
	}

	bool SteamUser::SetStat( const std::string& rName, int32_t data )
	{
		bool val = Steam::SteamUserStats()->SetStat( rName.data(), data );
		StoreStats();

		return val;
	}

	bool SteamUser::SetStat( const std::string& rName, float data )
	{
		bool val = Steam::SteamUserStats()->SetStat( rName.data(), data );
		StoreStats();
	
		return val;
	}

	bool SteamUser::UpdateStatAvg( const std::string& rName, float accumSinceLastCall, double durationSec )
	{
		bool val = Steam::SteamUserStats()->UpdateAvgRateStat( rName.data(), accumSinceLastCall, durationSec );
		StoreStats();

		return val;
	}

	bool SteamUser::IncrementStat( const std::string& rName, int32_t incrementAmount )
	{
		int32_t amount = 0;
		if( Steam::SteamUserStats()->GetStat( rName.data(), &amount ) ) 
		{
			return SetStat( rName, amount + incrementAmount );
		}

		return false;
	}

	bool SteamUser::IncrementStat( const std::string& rName, float incrementAmount )
	{
		float amount = 0.0f;
		if( Steam::SteamUserStats()->GetStat( rName.data(), &amount ) )
		{
			return SetStat( rName, amount + incrementAmount );
		}

		return false;
	}

	bool SteamUser::GetStat( const std::string& rName, int32_t* pValue )
	{
		return Steam::SteamUserStats()->GetStat( rName.data(), pValue );
	}

	bool SteamUser::GetStat( const std::string& rName, float* pValue )
	{
		return Steam::SteamUserStats()->GetStat( rName.data(), pValue );
	}

	bool SteamUser::UnlockAchievement( const std::string& rName )
	{
		bool val = Steam::SteamUserStats()->SetAchievement( rName.data() );
		StoreStats();

		return val;
	}

	bool SteamUser::ClearAchievement( const std::string& rName )
	{
		bool val = Steam::SteamUserStats()->ClearAchievement( rName.data() );
		StoreStats();

		return val;
	}

	bool SteamUser::IsAchievementObtained( const std::string& rName ) const
	{
		bool unlocked = false;
		if( !Steam::SteamUserStats()->GetUserAchievement( m_UserID, rName.data(), &unlocked ) )
			return false;

		return unlocked;
	}

	float SteamUser::GetAchievementPercent( const std::string& rName )
	{
		float amount = -1.0f;
		if( Steam::SteamUserStats()->RequestGlobalAchievementPercentages() )
			Steam::SteamUserStats()->GetAchievementAchievedPercent( rName.data(), &amount );

		return amount;
	}

	bool SteamUser::ShowAchievementProgress( const std::string& rName, int32_t current, int32_t max )
	{
		return Steam::SteamUserStats()->IndicateAchievementProgress( rName.data(), current, max );
	}

	uint32_t SteamUser::GetNumberOfAchievements()
	{
		return Steam::SteamUserStats()->GetNumAchievements();
	}

	std::vector<std::string> SteamUser::GetAllAchievementNames()
	{
		const auto numAch = GetNumberOfAchievements();

		std::vector<std::string> names;
		names.reserve( numAch );

		for( auto i = 0; i < numAch; ++i )
		{
			names[ i ] = Steam::SteamUserStats()->GetAchievementName( i );
		}

		return names;
	}

}

#endif
