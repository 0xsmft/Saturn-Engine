/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2025 BEAST                                                           *
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
#include "TextureSourceAsset.h"

#include "Saturn/Vulkan/Texture.h"

#include "Saturn/Core/VirtualFS.h"
#include "Saturn/Core/MemoryStream.h"

#include "Saturn/Project/Project.h"

#include <stb_image.h>

namespace Saturn {

	TextureSourceAsset::TextureSourceAsset()
	{
	}

	TextureSourceAsset::TextureSourceAsset( std::filesystem::path AbsolutePath, bool Flip )
		: m_AbsolutePath( std::move( AbsolutePath ) ), m_Flipped( Flip )
	{
		LoadRawTexture();
	}

	TextureSourceAsset::~TextureSourceAsset()
	{
		m_TextureBuffer.Free();
	}

	void TextureSourceAsset::LoadRawTexture()
	{
#if !defined(SAT_DIST)
		SAT_CORE_ASSERT( std::filesystem::exists( m_AbsolutePath ), "Path does not exist!" );

		int Width, Height, Channels;

		stbi_uc* pTextureData;

		m_HDR = stbi_is_hdr( m_AbsolutePath.string().c_str() );

		if( m_HDR )
		{
			SAT_CORE_INFO( "Loading HDR texture {0}", m_AbsolutePath.string() );
			pTextureData = ( uint8_t* ) stbi_loadf( m_AbsolutePath.string().c_str(), &Width, &Height, &Channels, 4 );
		}
		else
		{
			SAT_CORE_INFO( "Loading texture {0}", m_AbsolutePath.string() );

			pTextureData = stbi_load( m_AbsolutePath.string().c_str(), &Width, &Height, &Channels, 4 );
		}

		m_Width = Width;
		m_Height = Height;
		m_Channels = Channels;

		const uint32_t ImageSize = m_Width * m_Height * 4;
		m_TextureBuffer = Buffer::Copy( pTextureData, static_cast<size_t>( ImageSize ) );

		stbi_image_free( pTextureData );
#endif
	}

	void TextureSourceAsset::WriteToVFS()
	{
		std::filesystem::path out = Project::GetActiveProject()->GetTempDir();
		out /= std::to_string( ID );
		out.replace_extension( ".vfs" );

		std::ofstream stream( out, std::ios::binary | std::ios::trunc );

//		RawSerialisation::WriteString( m_AbsolutePath.string(), stream );

		RawSerialisation::WriteObject( m_Width, stream );
		RawSerialisation::WriteObject( m_Height, stream );
		RawSerialisation::WriteObject( m_Channels, stream );
		RawSerialisation::WriteObject( m_Flipped, stream );
		RawSerialisation::WriteObject( m_HDR, stream );

		// Buffer
		RawSerialisation::WriteSaturnBuffer( m_TextureBuffer, stream );
	}

	void TextureSourceAsset::ReadFromVFS()
	{
#if defined(SAT_DIST)
		const std::string& rMountBase = Project::GetActiveConfig().Name;
		Ref<VFile> file = VirtualFS::Get().FindFile( rMountBase, Path );

		if( !file )
			return;

		PakFileMemoryBuffer membuf( file->FileContent );
		std::istream stream( &membuf );

		/////////////////////////////////////

//		m_AbsolutePath = RawSerialisation::ReadString( stream );
		RawSerialisation::ReadObject( m_Width, stream );
		RawSerialisation::ReadObject( m_Height, stream );
		RawSerialisation::ReadObject( m_Channels, stream );
		RawSerialisation::ReadObject( m_Flipped, stream );
		RawSerialisation::ReadObject( m_HDR, stream );

		// Buffer
		RawSerialisation::ReadSaturnBuffer( m_TextureBuffer, stream );
#endif
	}
}