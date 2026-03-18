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
#include "SteamAvatarCache.h"

#include "SteamOnlineSystemAPI.h"

#include "Saturn/Vulkan/Renderer.h"

#include <steam/isteamutils.h>

namespace Saturn {
	
	SteamAvatarCache::SteamAvatarCache()
	{
	}

	SteamAvatarCache::~SteamAvatarCache()
	{
		ClearAll();
	}

	void SteamAvatarCache::Invalidate( CSteamID ID )
	{
		const auto itr = m_UserIDToAvatar.find( ID.ConvertToUint64() );
		if( itr != m_UserIDToAvatar.end() )
		{
			m_UserIDToAvatar.erase( itr );
		}
	}

	void SteamAvatarCache::OnAvatarImageLoaded( AvatarImageLoaded_t* pData )
	{
		const auto itr = m_UserIDToAvatar.find( pData->m_steamID.ConvertToUint64() );
		if( itr != m_UserIDToAvatar.end() )
		{
			uint32_t imageWidth = 0, imageHeight = 0;
			SteamUtils()->GetImageSize( pData->m_iImage, &imageWidth, &imageHeight );
			if( imageWidth > 0 && imageHeight > 0 )
			{
				Buffer TemporaryBuffer;
				TemporaryBuffer.Allocate( static_cast< size_t >( imageWidth * imageHeight * 4 ) );
				TemporaryBuffer.Zero_Memory();

				// Valve... why is the size of the texture a signed number??
				SteamUtils()->GetImageRGBA( pData->m_iImage, TemporaryBuffer.Data, ( int ) TemporaryBuffer.Size );

				itr->second = Ref<Texture2D>::Create( ImageFormat::RGBA8, imageWidth, imageHeight, TemporaryBuffer.Data );

				TemporaryBuffer.Free();
			}
		}
	}

	Ref<Texture2D> SteamAvatarCache::GetAvatarForUser( CSteamID ID )
	{
		const auto IDull = ID.ConvertToUint64();
		const auto Itr = m_UserIDToAvatar.find( IDull );
		if( Itr == m_UserIDToAvatar.end() )
		{
			const auto index = SteamFriends()->GetMediumFriendAvatar( ID );

			// -1 == has not been downloaded yet.
			// Meaning that we have to wait until our call back does this...
			if( index == -1 )
			{
				//... while we wait for that we can add it to our queue so the callback knows where to load the texture in
				// and use the pink texture for a fallback image.
				m_UserIDToAvatar.emplace( IDull, Renderer::Get()->GetPinkTexture() );
			}
			else
			{
				// Get the texture now.
				uint32_t imageWidth = 0, imageHeight = 0;
				SteamUtils()->GetImageSize( index, &imageWidth, &imageHeight );
				if( imageWidth > 0 && imageHeight > 0 )
				{
					Buffer TemporaryBuffer;
					TemporaryBuffer.Allocate( static_cast< size_t >( imageWidth * imageHeight * 4 ) );
					TemporaryBuffer.Zero_Memory();

					// Valve... why is the size of the texture a signed number??
					SteamUtils()->GetImageRGBA( index, TemporaryBuffer.Data, ( int ) TemporaryBuffer.Size );

					m_UserIDToAvatar[ IDull ] = Ref<Texture2D>::Create( ImageFormat::RGBA8, imageWidth, imageHeight, TemporaryBuffer.Data );

					TemporaryBuffer.Free();
				}
			}
		}

		return m_UserIDToAvatar[ IDull ];
	}

	void SteamAvatarCache::ClearAll()
	{
		m_UserIDToAvatar.clear();
	}

}

#endif
