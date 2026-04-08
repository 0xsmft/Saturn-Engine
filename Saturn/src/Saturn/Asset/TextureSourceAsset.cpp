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
#include "TextureSourceAsset.h"

#include "Saturn/Vulkan/Texture.h"

#include "Saturn/Core/VirtualFS.h"
#include "Saturn/Core/MemoryStream.h"

#include "Saturn/Project/Project.h"

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

#if !defined(SAT_DIST)
#include <stb_image.h>
#endif

namespace Saturn {

	TextureSourceAsset::TextureSourceAsset()
	{
	}

	TextureSourceAsset::TextureSourceAsset( const Ref<Asset>& rBase, std::filesystem::path AbsolutePath, TextureLoadFlags flags )
		: Asset( rBase ), 
#if !defined(SAT_DIST)
		m_AbsolutePath( std::move( AbsolutePath ) ), 
#endif
		m_LoadFlags( flags )
	{
		LoadRawTexture();
	}

	TextureSourceAsset::TextureSourceAsset( const Ref<Asset>& rBase )
		: Asset( rBase )
	{
	}

	TextureSourceAsset::~TextureSourceAsset()
	{
	}

	void TextureSourceAsset::Load()
	{
#if !defined(SAT_DIST)
		SAT_CORE_ASSERT( std::filesystem::exists( m_AbsolutePath ), "Path does not exist!" );

		int Width, Height, Channels;
		bool hdr = false;
		stbi_uc* pTextureData;

		stbi_set_flip_vertically_on_load( IsFlagSet( TextureLoadFlags_FlipVertically ) );

		hdr = stbi_is_hdr( m_AbsolutePath.string().c_str() );
		SAT_CORE_ASSERT( m_HDR == hdr, "Image hdr types don't match!" );

		if( hdr )
		{
			SAT_CORE_INFO( "Loading HDR texture {0}", m_AbsolutePath.string() );
			pTextureData = ( uint8_t* ) stbi_loadf( m_AbsolutePath.string().c_str(), &Width, &Height, &Channels, 4 );
		}
		else
		{
			SAT_CORE_INFO( "Loading texture {0}", m_AbsolutePath.string() );

			pTextureData = stbi_load( m_AbsolutePath.string().c_str(), &Width, &Height, &Channels, 4 );
		}

		SAT_CORE_ASSERT( m_Width == Width, "Image width does not match!" );
		SAT_CORE_ASSERT( m_Height == Height, "Image height does not match!" );
		SAT_CORE_ASSERT( m_Channels == Channels, "Image channels does not match!" );

		// NOTE: We should use the proper channel count
		//		 and stop assuming that all textures have
		//		 an alpha channel.
		const uint32_t ImageSize = m_Width * m_Height * 4;

		Buffer textureBuffer = Buffer::Copy( pTextureData, static_cast< size_t >( ImageSize ) );
		stbi_image_free( pTextureData );

		m_Texture = Ref<Texture2D>::Create( ImageFormat::RGBA8, m_Width, m_Height, textureBuffer.Data );
		m_Texture->SetSourceID( ID );

		textureBuffer.Free();
#endif
	}

	void TextureSourceAsset::LoadRawTexture()
	{
#if !defined(SAT_DIST)
		SAT_CORE_ASSERT( std::filesystem::exists( m_AbsolutePath ), "Path does not exist!" );

		int Width, Height, Channels;

		stbi_uc* pTextureData;

		stbi_set_flip_vertically_on_load( IsFlagSet( TextureLoadFlags_FlipVertically ) );

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

		// NOTE: We should use the proper channel count
		//		 and stop assuming that all textures have
		//		 an alpha channel.
		const uint32_t ImageSize = m_Width * m_Height * 4;
		Buffer textureBuffer = Buffer::Copy( pTextureData, static_cast<size_t>( ImageSize ) );
		stbi_image_free( pTextureData );

		m_Texture = Ref<Texture2D>::Create( ImageFormat::RGBA8, m_Width, m_Height, textureBuffer.Data );
		m_Texture->SetSourceID( ID );

		textureBuffer.Free();
#endif
	}

	void TextureSourceAsset::WriteToVFS()
	{
		std::filesystem::path out = Project::GetActiveProject()->GetTempDir();
		out /= std::to_string( ID );
		out.replace_extension( ".vfs" );

		std::ofstream stream( out, std::ios::binary | std::ios::trunc );

		RawSerialisation::WriteObject( m_Width, stream );
		RawSerialisation::WriteObject( m_Height, stream );
		RawSerialisation::WriteObject( m_Channels, stream );
		RawSerialisation::WriteObject( m_HDR, stream );

		// Buffer
		Buffer TemporaryBuffer = m_Texture->X31CopyToBuffer();
		RawSerialisation::WriteSaturnBuffer( TemporaryBuffer, stream );

		TemporaryBuffer.Free();
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

		RawSerialisation::ReadObject( m_Width, stream );
		RawSerialisation::ReadObject( m_Height, stream );
		RawSerialisation::ReadObject( m_Channels, stream );
		RawSerialisation::ReadObject( m_HDR, stream );

		// Buffer
		Buffer TemporaryBuffer;
		RawSerialisation::ReadSaturnBuffer( TemporaryBuffer, stream );

		m_Texture = Ref<Texture2D>::Create( ImageFormat::RGBA8, m_Width, m_Height, TemporaryBuffer.Data, false );
		m_Texture->SetSourceID( ID );

		TemporaryBuffer.Free();
#endif
	}

	void TextureSourceAsset::OnDelete()
	{
		if( std::filesystem::exists( m_AbsolutePath ) )
			std::filesystem::remove( m_AbsolutePath );
	}

	void TextureSourceAsset::OnReimport( const std::filesystem::path& rPath )
	{
		m_AbsolutePath = rPath;
		LoadRawTexture();
	}

}
