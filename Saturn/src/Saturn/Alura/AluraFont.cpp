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
#include "AluraFont.h"

#include "AluraMSDFData.h"

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

#include "Saturn/Project/Project.h"

#include <msdfgen/msdfgen.h>
#include <msdf-atlas-gen/msdf-atlas-gen.h>

namespace msdf = msdfgen;
namespace msdfag = msdf_atlas;

namespace Saturn {

	AluraFont::AluraFont( const std::filesystem::path& rFontPath, const Ref<Asset>& rBase )
		: Asset( rBase ), m_Filepath( rFontPath ), m_pMSDFData( new AluraMSDFData() )
	{
		CreateAtlas( true );
	}

	AluraFont::AluraFont( const Ref<Asset>& rBase )
		: Asset( rBase ), m_pMSDFData( new AluraMSDFData() )
	{
	}

	class FMsdfFont
	{
	public:
		FMsdfFont()
			: m_pFreetypeHandle( msdf::initializeFreetype() )
		{
		}

		bool Load( const std::filesystem::path& rPath ) 
		{
			if( m_pFreetypeHandle )
			{
				if( m_pFontHandle ) msdf::destroyFont( m_pFontHandle );

				if( ( m_pFontHandle = msdf::loadFont( m_pFreetypeHandle, rPath.string().c_str() ) ); m_pFontHandle ) return true;
			}

			return false;
		}

		~FMsdfFont() 
		{
			if( m_pFreetypeHandle )
			{
				if( m_pFontHandle )
					msdf::destroyFont( m_pFontHandle );

				msdf::deinitializeFreetype( m_pFreetypeHandle );
			}
		}

		operator msdf::FontHandle*() const 
		{
			return m_pFontHandle;
		}

		msdf::FontHandle* GetFontHandle() const { return m_pFontHandle; }

	private:
		msdf::FreetypeHandle* m_pFreetypeHandle;
		msdf::FontHandle* m_pFontHandle = nullptr;
	};

	void AluraFont::CreateAtlas( bool overrideCache /*= false*/ )
	{
		FMsdfFont font;
		if( !font.Load( m_Filepath.string().c_str() ) )
		{
			SAT_CORE_ERROR( "[Alura] Font was unabled to be loaded by Freetype or Msdf library was not initialised." );
			SAT_CORE_ASSERT( false );
			return;
		}

		msdfag::Charset charset;
		
		// Ascii
		for( uint32_t c = 0x0020; c < 0x00FF; c++ )
			charset.add( c );

		m_pMSDFData->FontGeometry = msdfag::FontGeometry( &m_pMSDFData->Glyphs );
		const int glyhsLoaded = m_pMSDFData->FontGeometry.loadCharset( font, 1.0, charset );

		if( m_pMSDFData->FontGeometry.getName() != nullptr )
		{
			m_Name = m_pMSDFData->FontGeometry.getName();
		}
		else
		{
			m_Name = m_Filepath.stem().string();
		}

		SAT_CORE_INFO( "[Alura] Loaded {0} glyhs out of {1}", glyhsLoaded, charset.size() );

		if( glyhsLoaded < charset.size() )
		{
			auto missingIndex = charset.size() - glyhsLoaded;

			SAT_CORE_WARN( "[Alura] Font is missing {0} glyhs", missingIndex );

			for( size_t i = glyhsLoaded; i < charset.size(); i++ )
			{
				SAT_CORE_WARN( " Missing: {0}", (char)i );
			}
		}

		msdfag::TightAtlasPacker packer;
		packer.setDimensionsConstraint( msdfag::TightAtlasPacker::DimensionsConstraint::MULTIPLE_OF_FOUR_SQUARE );
		packer.setPixelRange( 2.0 );
		packer.setMiterLimit( 1.0 );
		packer.setPadding( 0 );
		packer.setScale( 40.0 );

		const int remaining = packer.pack( m_pMSDFData->Glyphs.data(), (int)m_pMSDFData->Glyphs.size() );
		if( remaining > 0 )
		{
			SAT_CORE_ERROR( "Could not fit all glyhps into the texture atlas!" );
			SAT_CORE_ASSERT( false );
		}
		else if( remaining < 0 )
			SAT_CORE_ASSERT( false );

		int w, h;
		packer.getDimensions( w, h );

		unsigned long long glyphSeed = 0;
		for( msdfag::GlyphGeometry& rGlyph : m_pMSDFData->Glyphs )
		{
			glyphSeed *= 6364136223846793005ull;
			rGlyph.edgeColoring( msdf::edgeColoringInkTrap, 3.0, glyphSeed );
		}

		if( overrideCache /*|| shouldWriteToCache*/ )
		{
			const auto absolutePath = Project::GetActiveProject()->FilepathAbs( Path );
			Serialise( w, h, absolutePath );
		}
	}

	AluraFont::~AluraFont()
	{
		delete m_pMSDFData;
	}
	
	struct AluraAtlasHeader
	{
		// Saturn Alura Font, could of been called Saturn Font Asset as well...
		static constexpr char Magic[4] = { '.', 'S', 'A', 'F' };
		uint32_t Width;
		uint32_t Height;
		uint32_t Type;
	};

	template<typename Ty, typename S, int N, msdfag::GeneratorFunction<S, N> Func>
	static Ref<Texture2D> CreateTextureAndCache( AluraMSDFData* pThis, int width, int height, const std::filesystem::path& rPath, const std::filesystem::path& rFontPath )
	{
		msdfag::GeneratorAttributes attributes;
		attributes.config.overlapSupport = true;
		attributes.scanlinePass = true;

		msdfag::ImmediateAtlasGenerator<S, N, Func, msdfag::BitmapAtlasStorage<Ty, N>> generator( width, height );
		generator.setThreadCount( 4 );
		generator.setAttributes( attributes );
		generator.generate( pThis->Glyphs.data(), pThis->Glyphs.size() );

		msdf::BitmapConstRef<Ty, N> bitmap = generator.atlasStorage();

		// Create texture atlas
		auto textureAtlas = Ref<Texture2D>::Create( ImageFormat::RGBA32F, bitmap.width, bitmap.height, bitmap.pixels );

		// Write cache
		std::ofstream fout( rPath, std::ios::binary | std::ios::trunc );

		RawSerialisation::WriteObject( ".SAF", fout );

		AluraAtlasHeader header{ .Width = ( uint32_t ) bitmap.width, .Height = ( uint32_t ) bitmap.height, .Type = 0 /*AluraFont_Unicode*/ };

		RawSerialisation::WriteObject( header, fout );

		// TODO: This path will be outwith the projects path so potential unicode characters maybe used!
		RawSerialisation::WriteString( rFontPath.string(), fout );

		fout.write( ( char* ) bitmap.pixels, bitmap.width * bitmap.height * N * sizeof( S ) );

		fout.close();

		return textureAtlas;
	}

	void AluraFont::Serialise( int width, int height, const std::filesystem::path& rPath )
	{
		m_TextureAtlas = CreateTextureAndCache<float, float, 4, msdfag::mtsdfGenerator>( m_pMSDFData, width, height, rPath, m_Filepath );
	}

	void AluraFont::LoadFromCache() 
	{
		const auto absolutePath = Project::GetActiveProject()->FilepathAbs( Path );
		std::ifstream stream( absolutePath, std::ios::binary | std::ios::in );
	
		char* pHeader = new char[ 5 ];
		stream.read( (char*)pHeader, sizeof( char ) * 5 );

		if( strcmp( pHeader, AluraAtlasHeader::Magic ) )
		{
			SAT_CORE_ERROR( "[Alura] Failed to load font, file header does not match!" );

			delete[] pHeader;
			return;
		}

		AluraAtlasHeader header;
		RawSerialisation::ReadObject( header, stream );

		std::string fontPath = RawSerialisation::ReadString( stream );
		m_Filepath = fontPath;

		void* pPixels = std::malloc( header.Width * header.Height * 4 * sizeof( float ) );
		stream.read( ( char* ) pPixels, header.Width * header.Height * 4 * sizeof( float ) );

		stream.close();

		delete[] pHeader;
		m_TextureAtlas = Ref<Texture2D>::Create( ImageFormat::RGBA32F, header.Width, header.Height, pPixels );
	
		std::free( pPixels );
	}

	void AluraFont::Deserialise()
	{
		LoadFromCache();
		CreateAtlas();
	}

	glm::vec2 AluraFont::CalcTextSize( float fontSize, const std::string& rText )
	{
		if( rText.empty() ) return { 0.0f, 0.0f };

		const double scale = fontSize / 1.0;

		glm::vec2 textSize{};
		double minY = DBL_MAX, maxY = -DBL_MAX, x = 0.0;

		for( char character : rText )
		{
			const auto glyph = m_pMSDFData->FontGeometry.getGlyph( character );
			if( !glyph ) continue;

			double pl, pb, pr, pt;
			glyph->getQuadPlaneBounds( pl, pb, pr, pt );

			minY = glm::min( minY, pb );
			maxY = glm::max( maxY, pt );

			x += glyph->getAdvance();
		}

		textSize.x = ( float ) ( x * scale );
		textSize.y = ( float ) ( ( maxY - minY ) * scale );
		return textSize;
	}

}
