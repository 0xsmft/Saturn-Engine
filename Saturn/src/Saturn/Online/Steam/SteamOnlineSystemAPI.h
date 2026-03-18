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

#include "Saturn/Online/OnlineAPI.h"
#include "SingletonStorage.h"

#include "SteamCurrentUser.h"
#include "SteamAvatarCache.h"

namespace Saturn {

	//
	// Represents the Steamworks API as a whole.
	// 
	// Allows access to the current steam user and the steam client application.
	//
	class SteamOnlineSystemAPI : public OnlineAPI
	{
	public:
		[[nodiscard]] static inline Ref<SteamOnlineSystemAPI> Get() { return SingletonStorage::GetSingleton<SteamOnlineSystemAPI>(); }
	public:
		SteamOnlineSystemAPI();
		virtual ~SteamOnlineSystemAPI();

	public:
		virtual bool Initialise() override;
		virtual void Tick() override;
		virtual void Terminate() override;

	public:
		//
		// Set the location of the steam overlay Notification Position
		//
		// NOTE: You must call this every time the game opens.
		//
		void SetOverlayLocation( ENotificationPosition position );

		// Number of seconds the steam application is active for.
		uint32_t GetNumSecondsSinceAppActive();

	public:
		SteamCurrentUser& GetCurrentUser() { return m_CurrentUser; }
		const SteamCurrentUser& GetCurrentUser() const { return m_CurrentUser; }

		SteamAvatarCache& GetAvatarCache() { return m_SteamAvatarCache; }
		const SteamAvatarCache& GetAvatarCache() const { return m_SteamAvatarCache; }

		uint32_t GetAppID() const { return m_AppID; }
		void SetAppID( uint32_t appID ) { m_AppID = appID; }

	private:
		uint32_t m_AppID = 0u;
		bool m_Initialised = false;
		SteamCurrentUser m_CurrentUser;
		SteamAvatarCache m_SteamAvatarCache;
	};

}

#endif
