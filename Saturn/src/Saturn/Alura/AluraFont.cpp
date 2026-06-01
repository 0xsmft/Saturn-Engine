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
#include "AluraFont.h"

#if !defined(SAT_DIST)
#include "AluraMSDFGenerationData.h"
#endif

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

#include "Saturn/Project/Project.h"

#if !defined(SAT_DIST)
#include <msdfgen/msdfgen.h>
#include <msdf-atlas-gen/msdf-atlas-gen.h>

namespace msdf = msdfgen;
namespace msdfag = msdf_atlas;
#endif

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// ALURA FONT DATA
	
	void AluraFontData::AddCodepointToGlyph( uint32_t codepoint )
	{
		m_CodepointToGlyph[ codepoint ] = m_AluraGlyphs.size() - 1;
	}

	void AluraFontData::SetKerning( const std::map<std::pair<int, int>, double>& rMap )
	{
		m_Kerning = rMap;
	}

	AluraSerialisedGlyph* AluraFontData::GetGlyph( uint32_t codepoint )
	{
		if( auto itr = m_CodepointToGlyph.find( codepoint ); itr != m_CodepointToGlyph.end() )
		{
			return &m_AluraGlyphs[ itr->second ];
		}

		return nullptr;
	}

	bool AluraFontData::GetAdvance( double& adv, uint32_t a, uint32_t b )
	{
		AluraSerialisedGlyph* pGlyph1, * pGlyph2;
		if( ( pGlyph1 = GetGlyph( a ) ) == nullptr || ( pGlyph2 = GetGlyph( b ) ) == nullptr )
			return false;

		adv = pGlyph1->Advance;

		if( auto itr = m_Kerning.find( std::make_pair( a, b ) ); itr != m_Kerning.end() )
		{
			adv += pGlyph2->Advance;
		}

		return true;
	}

	void AluraFontData::ClearData()
	{
		m_AluraGlyphs.clear();
		m_CodepointToGlyph.clear();
		m_Kerning.clear();
	}

	//////////////////////////////////////////////////////////////////////////

	AluraFont::AluraFont( const std::filesystem::path& rFontPath, const Ref<Asset>& rBase )
		: Asset( rBase )
#if !defined(SAT_DIST)
		, m_FontFilepath( rFontPath )
#endif
	{
#if !defined(SAT_DIST)
		CreateOrLoadAtlas( true );
#endif
	}

	AluraFont::AluraFont( const Ref<Asset>& rBase )
		: Asset( rBase )
	{
	}

	AluraFont::~AluraFont()
	{
		m_TextureAtlas = nullptr;
	}
	
#if !defined(SAT_DIST)
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

		operator msdf::FontHandle* ( ) const
		{
			return m_pFontHandle;
		}

		msdf::FontHandle* GetFontHandle() const { return m_pFontHandle; }

	private:
		msdf::FreetypeHandle* m_pFreetypeHandle;
		msdf::FontHandle* m_pFontHandle = nullptr;
	};

	void AluraFont::OnReimport( const std::filesystem::path& rPath )
	{
		m_FontFilepath = rPath;
		CreateOrLoadAtlas( true );
	}
#endif

	void AluraFont::CreateOrLoadAtlas( bool overrideCache /*= false*/ )
	{
#if !defined(SAT_DIST)
		m_AluraFontData.ClearData();

		FMsdfFont font;
		if( !font.Load( m_FontFilepath.string().c_str() ) )
		{
			SAT_CORE_ERROR( "[Alura] Font was unabled to be loaded by Freetype or Msdf library was not initialised." );
			SAT_CORE_ASSERT( false );
			return;
		}

		msdfag::Charset charset;

		AluraMSDFGenerationData MSDFData;

		// Ascii
		for( uint32_t c = 0x0020; c < 0x00FF; c++ )
			charset.add( c );

		MSDFData.FontGeometry = msdfag::FontGeometry( &MSDFData.Glyphs );
		const int glyphsLoaded = MSDFData.FontGeometry.loadCharset( font, 1.0, charset );

		if( MSDFData.FontGeometry.getName() != nullptr )
		{
			m_Name = MSDFData.FontGeometry.getName();
		}
		else
		{
			m_Name = m_FontFilepath.stem().string();
		}

		SAT_CORE_INFO( "[Alura] Loading new font {0}.", m_Name );
		SAT_CORE_INFO( "[Alura] Loaded {0} glyhs out of {1}", glyphsLoaded, charset.size() );

		if( glyphsLoaded < charset.size() )
		{
			const auto missingCount = charset.size() - glyphsLoaded;
			SAT_CORE_WARN( "[Alura] Font is missing {0} glyhs", missingCount );
		}

		// Pack glyphs
		msdfag::TightAtlasPacker packer;
		packer.setDimensionsConstraint( msdfag::TightAtlasPacker::DimensionsConstraint::MULTIPLE_OF_FOUR_SQUARE );
		packer.setPixelRange( 2.0 );
		packer.setMiterLimit( 1.0 );
		packer.setPadding( 0 );
		packer.setScale( 40.0 );

		const int remaining = packer.pack( MSDFData.Glyphs.data(), ( int ) MSDFData.Glyphs.size() );
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
		for( msdfag::GlyphGeometry& rGlyph : MSDFData.Glyphs )
		{
			glyphSeed *= 6364136223846793005ull;
			rGlyph.edgeColoring( msdf::edgeColoringInkTrap, 3.0, glyphSeed );
		}

		// Convert to Alura
		{
			const auto& rMsdfMetrics = MSDFData.FontGeometry.getMetrics();

			AluraFontMetrics& rMetrics = m_AluraFontData.GetMetrics();
			rMetrics.EmSize = rMsdfMetrics.emSize;
			rMetrics.AscenderY = rMsdfMetrics.ascenderY;
			rMetrics.DescenderY = rMsdfMetrics.descenderY;
			rMetrics.LineHeight = rMsdfMetrics.lineHeight;
			rMetrics.UnderlineY = rMsdfMetrics.underlineY;
			rMetrics.UnderlineThickness = rMsdfMetrics.underlineThickness;

			m_AluraFontData.GetGlyphs().reserve( MSDFData.Glyphs.size() );

			for( const auto& rGlyphGeometry : MSDFData.Glyphs )
			{
				auto codepoint = rGlyphGeometry.getCodepoint();
				auto adv = rGlyphGeometry.getAdvance();

				double pl, pb, pr, pt;
				rGlyphGeometry.getQuadPlaneBounds( pl, pb, pr, pt );

				double al, ab, ar, at;
				rGlyphGeometry.getQuadAtlasBounds( al, ab, ar, at );

				m_AluraFontData.GetGlyphs().emplace_back( codepoint, ( float ) adv,
					( float ) pl, ( float ) pb, ( float ) pr, ( float ) pt, // Plane bounds
					( float ) al, ( float ) ab, ( float ) ar, ( float ) at ); // Atlas bounds

				m_AluraFontData.AddCodepointToGlyph( codepoint );
			}

			// We want to avoid moving the rKerningMap just in case Msdf uses it.
			const auto& rKerningMap = MSDFData.FontGeometry.getKerning();
			m_AluraFontData.SetKerning( rKerningMap );
		}

		if( overrideCache /*|| shouldWriteToCache*/ )
		{
			// Create texture
			CreateAtlasTexture( MSDFData, w, h );

			// Serialise Font data
			const auto absolutePath = Project::GetActiveProject()->FilepathAbs( Path );
			Serialise( absolutePath );
		}
#endif
	}

#if !defined(SAT_DIST)
	template<typename Ty, typename S, int N, msdfag::GeneratorFunction<S, N> Func>
	static Ref<Texture2D> CreateTextureAndCache( AluraMSDFGenerationData& rThis, int width, int height )
	{
		msdfag::GeneratorAttributes attributes;
		attributes.config.overlapSupport = true;
		attributes.scanlinePass = true;

		msdfag::ImmediateAtlasGenerator<S, N, Func, msdfag::BitmapAtlasStorage<Ty, N>> generator( width, height );
		generator.setThreadCount( std::thread::hardware_concurrency() / 2 );
		generator.setAttributes( attributes );
		generator.generate( rThis.Glyphs.data(), ( int ) rThis.Glyphs.size() );

		msdf::BitmapConstRef<Ty, N> bitmap = generator.atlasStorage();

		// Create texture atlas.
		auto textureAtlas = Ref<Texture2D>::Create( ImageFormat::RGBA32F, bitmap.width, bitmap.height, bitmap.pixels );
		return textureAtlas;
	}

	void AluraFont::CreateAtlasTexture( AluraMSDFGenerationData& rGenerationData, int width, int height )
	{
		m_TextureAtlas = CreateTextureAndCache<float, float, 4, msdfag::mtsdfGenerator>( rGenerationData, width, height );
	}
#endif

	struct AluraSerialiedFontHeader
	{
		// .SAF
		const unsigned char Magic[ 4 ] = { 0x2E, 0x53, 0x41, 0x46 };
		AluraFontAssetVersion Version = AluraFontAssetVersion::Lowest;
	};

	void AluraFont::Serialise( const std::filesystem::path& rPath ) const
	{
		std::ofstream fout( rPath, std::ios::binary | std::ios::trunc );

		// Header
		AluraSerialiedFontHeader header{};
		RawSerialisation::WriteObject( header.Magic, fout );
		RawSerialisation::WriteObject( ( uint8_t ) m_Version, fout );

#if !defined(SAT_DIST)
		// Name of the font file
		RawSerialisation::WriteString( m_Name, fout );
		
		// Write path
		RawSerialisation::WriteString( m_FontFilepath, fout );
#endif

		// Metrics
		RawSerialisation::WriteObject( m_AluraFontData.GetMetrics(), fout );

		// Glyph to codepoint map
		RawSerialisation::WriteMap( m_AluraFontData.GetCodepointToGlyph(), fout );

		// Kerning
		size_t mapSize = m_AluraFontData.GetKerning().size();
		RawSerialisation::WriteObject( mapSize, fout );

		for( const auto& [kv, kerning] : m_AluraFontData.GetKerning() )
		{
			RawSerialisation::WriteObject( kv.first, fout );
			RawSerialisation::WriteObject( kv.second, fout );
			RawSerialisation::WriteObject( kerning, fout );
		}

		// Write glyph geo.
		// Write the map manually
		mapSize = m_AluraFontData.GetGlyphs().size();
		RawSerialisation::WriteObject( mapSize, fout );

		for( size_t i = 0; i < mapSize; ++i )
		{
			const AluraSerialisedGlyph& asg = m_AluraFontData.GetGlyphs()[ i ];
			RawSerialisation::WriteObject( asg.Codepoint, fout );
			RawSerialisation::WriteObject( asg.Advance, fout );

			RawSerialisation::WriteObject( asg.PlaneLeft, fout );
			RawSerialisation::WriteObject( asg.PlaneBottom, fout );
			RawSerialisation::WriteObject( asg.PlaneRight, fout );
			RawSerialisation::WriteObject( asg.PlaneTop, fout );

			RawSerialisation::WriteObject( asg.AtlasLeft, fout );
			RawSerialisation::WriteObject( asg.AtlasBottom, fout );
			RawSerialisation::WriteObject( asg.AtlasRight, fout );
			RawSerialisation::WriteObject( asg.AtlasTop, fout );
		}

		// Now write the texture atlas image
		RawSerialisation::WriteObject( m_TextureAtlas->Width(), fout );
		RawSerialisation::WriteObject( m_TextureAtlas->Height(), fout );

		Buffer TemporaryBuffer = m_TextureAtlas->X31CopyToBuffer();
		RawSerialisation::WriteSaturnBuffer( TemporaryBuffer, fout );
		TemporaryBuffer.Free();

		fout.close();
	}

	void AluraFont::Deserialise( FDependentIStream& rStream )
	{
		AluraSerialiedFontHeader header{};
		
		char magic[ 4 ]{ 0 };
		RawSerialisation::ReadObject( magic, rStream );

		if( std::memcmp( header.Magic, "SAF.", 4 ) != 0 )
		{
			SAT_CORE_ERROR( "[AluraFont]: File magic does not match!" );
			return;
		}

		RawSerialisation::ReadObject( header.Version, rStream );

		// Name of the font file
		m_Name = RawSerialisation::ReadString( rStream );

		m_FontFilepath = RawSerialisation::ReadString( rStream );

		// Metrics
		RawSerialisation::ReadObject( m_AluraFontData.GetMetrics(), rStream );

		// Glyph to codepoint map
		RawSerialisation::ReadMap( m_AluraFontData.GetCodepointToGlyph(), rStream );

		// Kerning
		size_t mapSize = 0llu;
		RawSerialisation::ReadObject( mapSize, rStream );

		for( size_t i = 0; i < mapSize; i++ )
		{
			uint32_t a, b;
			double kerning;

			RawSerialisation::ReadObject( a, rStream );
			RawSerialisation::ReadObject( b, rStream );
			RawSerialisation::ReadObject( kerning, rStream );

			m_AluraFontData.GetKerning().emplace( std::make_pair( a, b ), kerning );
		}

		// Read the map manually
		RawSerialisation::ReadObject( mapSize, rStream );

		m_AluraFontData.GetGlyphs().resize( mapSize );

		for( auto& rGlyph : m_AluraFontData.GetGlyphs() )
		{
			RawSerialisation::ReadObject( rGlyph.Codepoint, rStream );
			RawSerialisation::ReadObject( rGlyph.Advance, rStream );

			RawSerialisation::ReadObject( rGlyph.PlaneLeft, rStream );
			RawSerialisation::ReadObject( rGlyph.PlaneBottom, rStream );
			RawSerialisation::ReadObject( rGlyph.PlaneRight, rStream );
			RawSerialisation::ReadObject( rGlyph.PlaneTop, rStream );

			RawSerialisation::ReadObject( rGlyph.AtlasLeft, rStream );
			RawSerialisation::ReadObject( rGlyph.AtlasBottom, rStream );
			RawSerialisation::ReadObject( rGlyph.AtlasRight, rStream );
			RawSerialisation::ReadObject( rGlyph.AtlasTop, rStream );
		}

		// Now write the texture atlas image
		uint32_t width = 0u, height = 0u;

		RawSerialisation::ReadObject( width, rStream );
		RawSerialisation::ReadObject( height, rStream );

		Buffer imageData;
		RawSerialisation::ReadSaturnBuffer( imageData, rStream );
		m_TextureAtlas = Ref<Texture2D>::Create( ImageFormat::RGBA32F, width, height, imageData.Data );
		imageData.Free();
	}

	glm::vec2 AluraFont::CalcTextSize( float fontSize, const std::string& rText )
	{
		if( rText.empty() ) return { 0.0f, 0.0f };

		const float scale = fontSize / 1.0F;

		float minX = FLT_MAX, maxX = -FLT_MAX;
		float minY = FLT_MAX, maxY = -FLT_MAX;

		float cursorX = 0.0f;
		for( char character : rText )
		{
			const auto glyph = m_AluraFontData.GetGlyph( character );
			if( !glyph ) continue;

			float pl, pb, pr, pt;
			glyph->GetQuadPlaneBounds( pl, pb, pr, pt );

			// Horizontal bounds.
			minX = glm::min( minX, cursorX + pl );
			maxX = glm::max( maxX, cursorX + pr );

			// Vertical bounds.
			minY = glm::min( minY, pb );
			maxY = glm::max( maxY, pt );

			cursorX += glyph->GetAdvance();
		}

		return { ( maxX - minX ) * scale, ( maxY - minY ) * scale };
	}

}
