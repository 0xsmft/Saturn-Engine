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
#include "TextureFlags.h"

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

namespace Saturn {

	class Texture;
	class Texture2D;

	class TextureSourceAsset : public Asset
	{
	public:
		TextureSourceAsset();
		TextureSourceAsset( const Ref<Asset>& rBase );
		TextureSourceAsset( const Ref<Asset>& rBase, std::filesystem::path AbsolutePath, bool Flip = false );

		~TextureSourceAsset();


		void WriteToVFS();
		void ReadFromVFS();

	public:
		uint32_t Width()  const { return m_Width; }
		uint32_t Height() const { return m_Height; }
		uint32_t Channels() const { return m_Channels; }
		bool IsHdr() const { return m_HDR; }
		TextureFlags GetFlags() const { return m_Flags; }
		void SetFlags( TextureFlags flags ) { m_Flags = flags; }

		Buffer TextureData() const { return m_TextureBuffer; }

		Ref<Texture2D> GetTexture() const { return m_Texture; }

		const std::filesystem::path& GetTextureAbsolutePath() const { return m_AbsolutePath; }

	public:
		//////////////////////////////////////////////////////////////////////////
		// Raw binary serialisation.

		void SerialiseData( std::ofstream& rStream )
		{ 
			RawSerialisation::WriteString( m_AbsolutePath.string(), rStream );

			RawSerialisation::WriteObject( m_Width, rStream );
			RawSerialisation::WriteObject( m_Height, rStream );
			RawSerialisation::WriteObject( m_Channels, rStream );
			RawSerialisation::WriteObject( m_Flipped, rStream );
			RawSerialisation::WriteObject( m_HDR, rStream );

			// Buffer
			RawSerialisation::WriteSaturnBuffer( m_TextureBuffer, rStream );
		}

		void DeserialiseData( std::ifstream& rStream )
		{
			m_AbsolutePath = RawSerialisation::ReadString( rStream );

			RawSerialisation::ReadObject( m_Width, rStream );
			RawSerialisation::ReadObject( m_Height, rStream );
			RawSerialisation::ReadObject( m_Channels, rStream );
			RawSerialisation::ReadObject( m_Flipped, rStream );
			RawSerialisation::ReadObject( m_HDR, rStream );

			// Buffer
			// Don't read the buffer just yet.
			//RawSerialisation::ReadSaturnBuffer( m_TextureBuffer, rStream );
		}

	private:
		void Load();
		void LoadRawTexture();

	private:
		std::filesystem::path m_AbsolutePath;

		uint32_t m_Width = 0;
		uint32_t m_Height = 0;
		uint32_t m_Channels = 0;
		TextureFlags m_Flags = TextureFlags::FlipVertically;

		bool m_Flipped = false;
		bool m_HDR = false;
		bool m_FullyLoaded = false;

		Buffer m_TextureBuffer;

		Ref<Texture2D> m_Texture;

	private:
		friend class TextureSourceAssetSerialiser;
	};
}
