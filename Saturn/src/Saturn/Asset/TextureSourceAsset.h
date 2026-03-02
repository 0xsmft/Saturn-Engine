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

#include "Asset.h"
#include "TextureLoadFlags.h"

namespace Saturn {

	class Texture;
	class Texture2D;

	// A texture source asset is an asset that holds a texture,
	// I recommended that you do NOT hold a reference to the texture that this asset holds,
	// instead you should hold a reference to this asset it self.
	//
	// This is because the texture that is held by this asset can change entirely or it's flags could change causing
	// the texture to be reloaded from disk (if load flags changed) or recreated internally (if filtering flags changed).
	//
	// If you must hold a reference to the texture it self then do so, but you've be warned...
	// It is perfectly fine to hold a reference to the texture it self on Dist because the texture is immutable.
	//
	class TextureSourceAsset : public Asset
	{
	public:
		TextureSourceAsset();
		TextureSourceAsset( const Ref<Asset>& rBase );
		TextureSourceAsset( const Ref<Asset>& rBase, std::filesystem::path AbsolutePath, TextureLoadFlags flags = TextureLoadFlags_FlipVertically );

		~TextureSourceAsset();

		void WriteToVFS();
		void ReadFromVFS();

	public:
		uint32_t Width()  const { return m_Width; }
		uint32_t Height() const { return m_Height; }
		uint32_t Channels() const { return m_Channels; }
		bool IsHdr() const { return m_HDR; }
		Ref<Texture2D> GetTexture() const { return m_Texture; }
		const std::filesystem::path& GetTextureAbsolutePath() const { return m_AbsolutePath; }

		TextureLoadFlags GetFlags() const { return ( TextureLoadFlags ) m_LoadFlags; }
		bool IsFlagSet( TextureLoadFlags flag ) const { return ( m_LoadFlags & flag ) != 0; }
		void SetFlag( TextureLoadFlags flag, bool val ) 
		{
			if( val )
				m_LoadFlags |= flag;
			else
				m_LoadFlags &= ~flag;
		}

		TextureFilteringFlags GetFilteringFlags() const { return m_SamplerFliteringFlags; }
		void SetFilteringFlags( TextureFilteringFlags flags ) { m_SamplerFliteringFlags = flags; }

	private:
		void Load();
		void LoadRawTexture();

	private:
#if !defined(SAT_DIST)
		// Path to the real .png/.jpg file
		// The path is not needed on Dist as we don't load from the raw file
		// we load from the image data that is already part of this asset in the AssetBundle.
		std::filesystem::path m_AbsolutePath;
#endif

		uint32_t m_Width = 0;
		uint32_t m_Height = 0;
		uint32_t m_Channels = 0;
		std::underlying_type_t<TextureLoadFlags> m_LoadFlags = TextureLoadFlags_FlipVertically;
		TextureFilteringFlags m_SamplerFliteringFlags = TextureFilteringFlags::Linear;
		bool m_HDR = false;

		Ref<Texture2D> m_Texture;

	private:
		friend class TextureSourceAssetSerialiser;
		friend class TextureViewer;
		friend class Renderer;
	};
}
