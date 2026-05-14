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
#include "SteamCurrentUser.h"

#include "SteamOnlineSystemAPI.h"

#include "Saturn/Core/StringAuxiliary.h"

#include <steam/steam_api.h>
#include <steam/isteamuser.h>
#include <steam/isteamfriends.h>
#include <steam/isteamuserstats.h>

static Saturn::OnlinePresence SteamPersonaStateToSaturn( EPersonaState state )
{
	using namespace Saturn;

	switch( state )
	{
		case k_EPersonaStateOffline:
			return OnlinePresence::Offline;

		case k_EPersonaStateOnline:
			return OnlinePresence::Online;

		case k_EPersonaStateBusy:
		case k_EPersonaStateAway:
			return OnlinePresence::Away;

		case k_EPersonaStateSnooze:
		case k_EPersonaStateLookingToTrade:
		case k_EPersonaStateLookingToPlay:
			return OnlinePresence::Online;

		case k_EPersonaStateInvisible:
			return OnlinePresence::AppearingOffline;

		case k_EPersonaStateMax:
		default:
			break;
	}

	return OnlinePresence::Offline;
}

namespace Saturn {

	void SteamCurrentUser::Initialise()
	{
		m_UserID = SteamUser()->GetSteamID();
		m_UserPresence = SteamPersonaStateToSaturn( SteamFriends()->GetPersonaState() );
		m_UserName = Auxiliary::ConvertString( SteamFriends()->GetPersonaName() );

		SAT_CORE_INFO( "[SteamUser] User ID: {0}, Presence: {1}", m_UserID.ConvertToUint64(), ( uint32_t ) m_UserPresence );

		RefreshFriends();
	}

	bool SteamCurrentUser::IsLoggedOn() const
	{
		return SteamUser()->BLoggedOn();
	}

	void SteamCurrentUser::RefreshFriends()
	{
		if( !IsLoggedOn() )
			return;

		m_Friends.resize( SteamFriends()->GetFriendCount( k_EFriendFlagImmediate ) );

		int32_t i = 0;
		for( auto& rFriend : m_Friends )
		{
			rFriend.UserID = SteamFriends()->GetFriendByIndex( i, k_EFriendFlagImmediate );
			rFriend.Name = Auxiliary::ConvertString( SteamFriends()->GetFriendPersonaName( rFriend.UserID ) );

			const auto presence = SteamFriends()->GetFriendPersonaState( rFriend.UserID );
			rFriend.Presence = SteamPersonaStateToSaturn( presence );

			++i;
		}
	}

	bool SteamCurrentUser::UserHasLicenseForApp( uint32_t appID ) const
	{
		return SteamUser()->UserHasLicenseForApp( m_UserID, appID );
	}

	int SteamCurrentUser::GetUserLevel() const
	{
		return SteamUser()->GetPlayerSteamLevel();
	}

	bool SteamCurrentUser::IsTwoFactorEnabled() const
	{
		return SteamUser()->BIsTwoFactorEnabled();
	}

	bool SteamCurrentUser::IsPhoneVerified() const
	{
		return SteamUser()->BIsPhoneVerified();
	}

	Ref<Texture2D> SteamCurrentUser::GetAvatar() const
	{
		return SteamOnlineSystemAPI::Get()->GetAvatarCache().GetAvatarForUser( m_UserID );
	}

	bool SteamCurrentUser::IsFriendsWith( CSteamID UserID )
	{
		return SteamFriends()->HasFriend( UserID, k_EFriendFlagImmediate );
	}

	uint32_t SteamCurrentUser::GetFriendCount()
	{
		// Valve why is friend count a signed int?? cant have negative friends...
		return ( uint32_t ) SteamFriends()->GetFriendCount( k_EFriendFlagImmediate );
	}

	uint32_t SteamCurrentUser::GetBlockedUsersCount()
	{
		// Valve why is this a signed int??
		return ( uint32_t ) SteamFriends()->GetFriendCount( k_EFriendFlagBlocked );
	}

	void SteamCurrentUser::AddRecentlyPlayedWith( CSteamID UserID )
	{
		SteamFriends()->SetPlayedWith( UserID );
	}

	bool SteamCurrentUser::StoreStats()
	{
		return SteamUserStats()->StoreStats();
	}

	bool SteamCurrentUser::SetStat( const std::string& rName, int32_t data )
	{
		bool val = SteamUserStats()->SetStat( rName.data(), data );
		StoreStats();

		return val;
	}

	bool SteamCurrentUser::SetStat( const std::string& rName, float data )
	{
		bool val = SteamUserStats()->SetStat( rName.data(), data );
		StoreStats();
	
		return val;
	}

	bool SteamCurrentUser::UpdateStatAvg( const std::string& rName, float accumSinceLastCall, double durationSec )
	{
		bool val = SteamUserStats()->UpdateAvgRateStat( rName.data(), accumSinceLastCall, durationSec );
		StoreStats();

		return val;
	}

	bool SteamCurrentUser::IncrementStat( const std::string& rName, int32_t incrementAmount )
	{
		int32_t amount = 0;
		if( SteamUserStats()->GetStat( rName.data(), &amount ) ) 
		{
			return SetStat( rName, amount + incrementAmount );
		}

		return false;
	}

	bool SteamCurrentUser::IncrementStat( const std::string& rName, float incrementAmount )
	{
		float amount = 0.0f;
		if( SteamUserStats()->GetStat( rName.data(), &amount ) )
		{
			return SetStat( rName, amount + incrementAmount );
		}

		return false;
	}

	bool SteamCurrentUser::GetStat( const std::string& rName, int32_t* pValue )
	{
		return SteamUserStats()->GetStat( rName.data(), pValue );
	}

	bool SteamCurrentUser::GetStat( const std::string& rName, float* pValue )
	{
		return SteamUserStats()->GetStat( rName.data(), pValue );
	}

	bool SteamCurrentUser::UnlockAchievement( const std::string& rName )
	{
		bool val = SteamUserStats()->SetAchievement( rName.data() );
		StoreStats();

		return val;
	}

	bool SteamCurrentUser::ClearAchievement( const std::string& rName )
	{
		bool val = SteamUserStats()->ClearAchievement( rName.data() );
		StoreStats();

		return val;
	}

	bool SteamCurrentUser::IsAchievementObtained( const std::string& rName ) const
	{
		bool unlocked = false;
		if( !SteamUserStats()->GetUserAchievement( m_UserID, rName.data(), &unlocked ) )
			return false;

		return unlocked;
	}

	bool SteamCurrentUser::ShowAchievementProgress( const std::string& rName, int32_t current, int32_t max )
	{
		return SteamUserStats()->IndicateAchievementProgress( rName.data(), current, max );
	}

	uint32_t SteamCurrentUser::GetNumberOfAchievements()
	{
		return SteamUserStats()->GetNumAchievements();
	}

	std::vector<std::string> SteamCurrentUser::GetAllAchievementNames()
	{
		const uint32_t numAch = GetNumberOfAchievements();

		std::vector<std::string> names;
		names.reserve( numAch );

		for( uint32_t i = 0u; i < numAch; ++i )
		{
			names[ i ] = SteamUserStats()->GetAchievementName( i );
		}

		return names;
	}

}

#endif
