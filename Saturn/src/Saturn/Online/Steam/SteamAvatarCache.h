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

#include "Saturn/Vulkan/Texture.h"

#include <steam/steam_api_common.h>
#include <steam/isteamfriends.h>

#include <unordered_map>
#include <queue>

namespace Saturn {

	struct SteamAvatarTemporaryGenerationData
	{
		Buffer ImageBuffer;
		uint32_t Width = 0u, Height = 0u;
		size_t Index = 0llu;
		uint64_t UserID = 0llu;
	};

	class SteamAvatarCache
	{
	public:
		SteamAvatarCache();
		~SteamAvatarCache();

		void Tick();

		//
		// Remove a texture from the cache.
		//
		// NOTE: The texture will be re-added if GetAvatarForUser() is called with the same ID
		// 
		// @param ID -- user ID to remove
		//
		void Invalidate( CSteamID ID );

		//
		// Get or differ an avatar.
		// 
		// If the avatar is loaded already (i.e. steam says it is), it will be created right now
		//
		// However, if it's not it will be differed until the steam callbacks are handled.
		//
		Ref<Texture2D> GetAvatarForUser( CSteamID ID );

		// Force clear everything.
		void ClearAll();

	private:
		STEAM_CALLBACK( SteamAvatarCache, OnAvatarImageLoaded, AvatarImageLoaded_t );

		void QueueAvatarImageCreation( size_t index, uint64_t ID );

	private:
		std::unordered_map<uint64_t, Ref<Texture2D>> m_UserIDToAvatar;
		std::queue<SteamAvatarTemporaryGenerationData> m_TemporaryGenerationData;
	};
}

#endif
