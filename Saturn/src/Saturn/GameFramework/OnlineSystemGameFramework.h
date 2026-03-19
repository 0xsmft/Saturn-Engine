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

#include "Saturn/Online/OnlineAPI.h"

#if defined(SAT_WITH_STEAM)
#include "Saturn/Online/Steam/SteamOnlineSystemAPI.h"
#include "Saturn/Vulkan/Texture.h"
#endif

namespace Saturn {

	/*
	+----------+-------------------+
	| PREFIX   | MEANING           |
	+----------+-------------------+
	| Os	   | Online System	   |
	| Oss	   | Online Sys. Steam |
	| Ose	   | Online Sys. EOS   |
	+---------------+--------------+
	*/
	
	// Get system type.
	extern OnlineSystemAPIType OsGetSystemType();

	// Get App ID.
	extern uint32_t OsGetAppID();

#if defined(SAT_WITH_STEAM)
	//
	// For documentation of this function view your subsystem API file
	// For Steam: /Saturn/Online/Steam/SteamOnlineSystemAPI.h
	//
	extern SteamCurrentUser& OsGetCurrentUser();
#endif

	//
	// For documentation of this function view your subsystem API file
	// Steam: /Saturn/Online/Steam/SteamCurrentUser.h
	//
	extern uint64_t OsGetCurrentUserID();

	//
	// For documentation of this function view your subsystem API file
	// Steam: /Saturn/Online/Steam/SteamAvatarCache.h
	//
	extern Ref<Texture2D> OsGetAvatarFromUser( uint64_t ID );

}
