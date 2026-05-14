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
#include "OnlineSystemGameFramework.h"

#include "Saturn/Project/Project.h"

namespace Saturn {

	OnlineSystemAPIType OsGetSystemType()
	{
		return Project::GetActiveProject()->GetOnlineAPIType();
	}

	uint32_t OsGetAppID()
	{
		return Project::GetActiveProject()->GetOnlineAppID();
	}

#if defined(SAT_WITH_STEAM)
	SteamCurrentUser& OsGetCurrentUser()
	{
		return SteamOnlineSystemAPI::Get()->GetCurrentUser();
	}

	uint64_t OsGetCurrentUserID()
	{
		return SteamOnlineSystemAPI::Get()->GetCurrentUser().GetNativeID().ConvertToUint64();
	}

	Ref<Texture2D> OsGetAvatarFromUser( uint64_t ID )
	{
		return SteamOnlineSystemAPI::Get()->GetAvatarCache().GetAvatarForUser( ID );
	}
#endif

}
