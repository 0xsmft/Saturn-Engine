/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2026 0xsmft https://0xsmft.dev                                       *
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
#include "BuiltinResourceManager.h"

#include "Saturn/Asset/Asset.h"
#include "Saturn/Vulkan/Texture.h"
#include "Saturn/Alura/AluraCanvas.h"

#include "Saturn/Project/Project.h"
#include "Saturn/Serialisation/Raw/RawSerialisation.h"

namespace Saturn {

	BuiltinResourceManager::BuiltinResourceManager()
	{
		SAT_CORE_ASSERT( !SingletonStorage::GetSingleton<BuiltinResourceManager>(), "A BuiltinResourceManager already exists!" );

		SingletonStorage::AddSingleton( this );

#if defined(SAT_DIST)
	 	SAT_CORE_VERIFY( ReadBundle(), "Failed to read build in resource manager." );
#endif
	}

	BuiltinResourceManager::~BuiltinResourceManager()
	{
		SingletonStorage::RemoveSingleton( this );
	}

	Ref<Asset> BuiltinResourceManager::GetResource( PackagedResourceType type )
	{
		return nullptr;
	}

	Ref<RefTarget> BuiltinResourceManager::CreateResourceFromType( PackagedResourceType type )
	{
#if defined(SAT_DIST)
		return CreateResourceFromType_Dist( type );
#else
		return CreateResourceFromType_Development( type );
#endif
	}

#if defined(SAT_DIST)
	Ref<RefTarget> BuiltinResourceManager::CreateResourceFromType_Dist( PackagedResourceType type )
	{
		const auto itr = m_Resources.find( type );
		if( itr != m_Resources.end() )
		{
			return itr->second;
		}

		SAT_CORE_VERIFY( false, "BuiltinResourceManager: Required built-in resource could not be found. Plese verify the game's integrity." );
		return nullptr;
	}
#else
	Ref<RefTarget> BuiltinResourceManager::CreateResourceFromType_Development( PackagedResourceType type )
	{
		switch( type )
		{
			case PackagedResourceType::DefaultAluraFont: 
			{
				return AluraCanvas::LoadEditorFont_ForEngineAB( Passkey<BuiltinResourceManager>() );
			}

			case PackagedResourceType::SMAA_AreaTexture: 
			{
				constexpr TextureLoadFlags kLoadFlags = TextureLoadFlags( TextureLoadFlags_LoadOnMainThread | TextureLoadFlags_NoMips );

				return Ref<Texture2D>::Create( "content/textures/SMAA_AreaTex.tga", AddressingMode::Repeat, kLoadFlags );
			}

			case PackagedResourceType::BRDFLutTexture:
				return Ref<Texture2D>::Create( "content/textures/BRDF_LUT.tga", AddressingMode::Repeat, TextureLoadFlags_LoadOnMainThread );

//			case PackagedResourceType::DefaultBloomDirtTexture:
//				break;

			case PackagedResourceType::WeatherMapTexture:
				return Ref<Texture2D>::Create( "content/textures/CloudsWeatherMap.tga", AddressingMode::Repeat, TextureLoadFlags_LoadOnMainThread );

			case PackagedResourceType::CurlTexture:
				return Ref<Texture2D>::Create( "content/textures/CloudsCurlNoise.tga", AddressingMode::Repeat, TextureLoadFlags_LoadOnMainThread );

			case PackagedResourceType::Unknown:
			default:
				break;
		}

		SAT_CORE_VERIFY( false );
		return nullptr;
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Packaging

	struct BuiltinAssetBundleFileHeader
	{
		// .RM + null
		const unsigned char Magic[ 4 ] = { 0x2E, 0x52, 0x4D, 0x00 };
		uint32_t EngineVersion = 0llu;
	};

	struct BuiltinAssetBundleTextureHeader
	{
		// TRM + null
		const unsigned char Magic[ 4 ] = { 0x54, 0x52, 0x4D, 0x00 };
	};

#if defined(SAT_DIST)
	static Ref<Texture2D> DeserialiseTexture( std::ifstream& rStream ) 
	{
		/*
		* 
		* 
		* BuiltinAssetBundleTextureHeader header{};
		RawSerialisation::WriteObject( header, rStream );

		RawSerialisation::WriteObject( texture2D->Width(), rStream );
		RawSerialisation::WriteObject( texture2D->Height(), rStream );
		RawSerialisation::WriteObject( rTextureSpec.Format, rStream );
		RawSerialisation::WriteObject( rTextureSpec.LoadFlags, rStream );
		RawSerialisation::WriteObject( rTextureSpec.AddressingMode, rStream );
		RawSerialisation::WriteObject( rTextureSpec.FilteringFlags, rStream );

		// Copy texture to CPU
		Buffer TemporaryBuffer = texture2D->X31CopyToBuffer();
		RawSerialisation::WriteSaturnBuffer( TemporaryBuffer, rStream );
		*/

		BuiltinAssetBundleTextureHeader header;
		RawSerialisation::ReadObject( header, rStream );

		if( std::memcmp( header.Magic, "TRM", 4 ) != 0 )
		{
			SAT_CORE_ERROR( "[BuiltinResourceManager]: Invalid file header for texture resource!" );
			return nullptr;
		}

		uint32_t w = 0u, h = 0u;
		ImageFormat format = ImageFormat::None;
		TextureLoadFlags loadFlags = TextureLoadFlags_None;
		AddressingMode addr = AddressingMode::Repeat;
		TextureFilteringFlags filteringFlags = TextureFilteringFlags::Linear;

		RawSerialisation::ReadObject( w, rStream );
		RawSerialisation::ReadObject( h, rStream );
		RawSerialisation::ReadObject( format, rStream );
		RawSerialisation::ReadObject( loadFlags, rStream );
		RawSerialisation::ReadObject( addr, rStream );
		RawSerialisation::ReadObject( filteringFlags, rStream );

		Buffer TemporaryBuffer;
		RawSerialisation::ReadSaturnBuffer( TemporaryBuffer, rStream );

		TextureSpecification spec{};
		spec.Width = w;
		spec.Height = h;
		spec.Format = format;
		spec.LoadFlags = loadFlags;
		spec.AddressingMode = addr;
		spec.FilteringFlags = filteringFlags;
		spec.CreateFromMemory = true;
		spec.pData = TemporaryBuffer.Data;

		auto texture = Ref<Texture2D>::Create( spec );
	
		TemporaryBuffer.Free();

		return texture;
	}

	bool BuiltinResourceManager::ReadBundle()
	{
		std::filesystem::path outPath = Project::GetActiveProject()->GetFullCachePath();
		outPath /= "AssetBundle-Builtin.srm";

		if( !std::filesystem::exists( outPath ) )
			return false;

		std::ifstream stream( outPath, std::ios::in | std::ios::binary );

		BuiltinAssetBundleFileHeader header;
		RawSerialisation::ReadObject( header, stream );

		if( std::memcmp( header.Magic, ".RM", 4 ) != 0 )
		{
			SAT_CORE_ERROR( "[BuiltinResourceManager]: Invalid file header!" );
			return false;
		}

		if( header.EngineVersion != SAT_CURRENT_VERSION )
		{
			SAT_CORE_ERROR( "[BuiltinResourceManager]: Invalid engine version!" );
			return false;
		}

		size_t resourceSize = 0llu;
		RawSerialisation::ReadObject( resourceSize, stream );

		m_Resources.reserve( resourceSize );

		for( size_t i = 0; i < resourceSize; ++i )
		{
			PackagedResourceType type = PackagedResourceType::Unknown;

			std::underlying_type_t<PackagedResourceType> ucType;
			RawSerialisation::ReadObject( ucType, stream );
			type = ( PackagedResourceType )ucType;

			switch( type )
			{
				case PackagedResourceType::BRDFLutTexture:
				case PackagedResourceType::CurlTexture:
				case PackagedResourceType::SMAA_AreaTexture:
//				case PackagedResourceType::DefaultBloomDirtTexture:
				case PackagedResourceType::WeatherMapTexture:
				{
					auto texture = DeserialiseTexture( stream );
					m_Resources[ type ] = texture;
				} break;

				case PackagedResourceType::DefaultAluraFont:
				{
					Ref<AluraFont> font = Ref<AluraFont>::Create();
					font->Deserialise_ForBRM( Passkey<BuiltinResourceManager>(), stream );

					m_Resources[ type ] = font;
				} break;

				default:
					break;
			}
		}

		stream.close();

		return true;
	}

#else
	//
	// Ugh, this is not great....
	// Ideally we'd want to use a texture source asset,
	// but, built in resources are not assets, they
	// are never touched by the AssetManager nor
	// can they be referenced by other assets.
	//
	static void SerialiseTexture( const Ref<Texture2D> texture2D, std::ofstream& rStream ) 
	{
		const auto& rTextureSpec = texture2D->GetTextureSpecification();

		BuiltinAssetBundleTextureHeader header{};
		RawSerialisation::WriteObject( header, rStream );

		RawSerialisation::WriteObject( texture2D->Width(), rStream );
		RawSerialisation::WriteObject( texture2D->Height(), rStream );
		RawSerialisation::WriteObject( texture2D->GetImageFormat(), rStream );
		RawSerialisation::WriteObject( rTextureSpec.LoadFlags, rStream );
		RawSerialisation::WriteObject( rTextureSpec.AddressingMode, rStream );
		RawSerialisation::WriteObject( rTextureSpec.FilteringFlags, rStream );

		// Copy texture to CPU
		Buffer TemporaryBuffer = texture2D->X31CopyToBuffer();
		RawSerialisation::WriteSaturnBuffer( TemporaryBuffer, rStream );

		TemporaryBuffer.Free();
	}

	void BuiltinResourceManager::LoadAllResources()
	{
		for( size_t i = 1; i < ( std::underlying_type_t<PackagedResourceType> )PackagedResourceType::MaxUsableIndex; ++i )
		{
			const PackagedResourceType type = ( PackagedResourceType ) i;

			const auto itr = m_Resources.find( type );
			if( itr == m_Resources.end() )
			{
				m_Resources[ type ] = CreateResourceFromType( type );
			}
		}
	}

	bool BuiltinResourceManager::PackageAll()
	{
		LoadAllResources();
		return PackageCurrent();
	}

	bool BuiltinResourceManager::PackageCurrent()
	{
		std::filesystem::path outPath = Project::GetActiveProject()->GetFullCachePath();
		outPath /= "AssetBundle-Builtin.srm";

		std::ofstream fout( outPath, std::ios::out | std::ios::binary );

		BuiltinAssetBundleFileHeader header;
		header.EngineVersion = SAT_CURRENT_VERSION;

		RawSerialisation::WriteObject( header, fout );

		RawSerialisation::WriteObject( m_Resources.size(), fout );

		for( const auto& [type, resource] : m_Resources )
		{
			RawSerialisation::WriteObject( ( std::underlying_type_t<PackagedResourceType> )type, fout );
			
			switch( type )
			{
				case PackagedResourceType::BRDFLutTexture:
				case PackagedResourceType::CurlTexture:
				case PackagedResourceType::SMAA_AreaTexture:
//				case PackagedResourceType::DefaultBloomDirtTexture:
				case PackagedResourceType::WeatherMapTexture:
				{
					const auto texture2D = resource.As<Texture2D>();
					if( texture2D )
					{
						SerialiseTexture( texture2D, fout );
					}
				} break;

				case PackagedResourceType::DefaultAluraFont: 
				{
					const Ref<AluraFont> font = resource.As<AluraFont>();
					if( font )
					{
						font->Serialise_ForBRM( Passkey<BuiltinResourceManager>(), fout );
					}
				} break;

				default:
					break;
			}
		}

		fout.close();

		return true;
	}
#endif

}
