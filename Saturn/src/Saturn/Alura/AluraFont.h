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

#include "Saturn/Asset/Asset.h"
#include "Saturn/Vulkan/Texture.h"

#include <filesystem>

namespace Saturn {

	class AluraSerialisedGlyph
	{
	public:
		uint32_t Codepoint = 0u;

		float Advance = 0.0f;

		float PlaneLeft = 0.0f;
		float PlaneBottom = 0.0f;
		float PlaneRight = 0.0f;
		float PlaneTop = 0.0f;

		float AtlasLeft = 0.0f;
		float AtlasBottom = 0.0f;
		float AtlasRight = 0.0f;
		float AtlasTop = 0.0f;

	public:
		AluraSerialisedGlyph() = default;
		
		AluraSerialisedGlyph( 
			uint32_t codepoint, 
			float adv, 
			float pl, float pb, float pr, float pt, 
			float al, float ab, float ar, float at 
		) 
			: Codepoint( codepoint ), 
			Advance( adv ),
			PlaneLeft( pl ), PlaneBottom( pb ), PlaneRight( pr ), PlaneTop( pt ),
			AtlasLeft( al ), AtlasBottom( ab ), AtlasRight( ar ), AtlasTop( at )
		{
		}

		~AluraSerialisedGlyph() = default;

		[[nodiscard]] inline float GetAdvance() const { return Advance; }

		void GetQuadPlaneBounds( float& rLeft, float& rBottom, float& rRight, float& rTop ) const
		{
			rLeft   = PlaneLeft;
			rBottom = PlaneBottom;
			rRight  = PlaneRight;
			rTop    = PlaneTop;
		}
		
		void GetQuadAtlasBounds( float& rLeft, float& rBottom, float& rRight, float& rTop ) const
		{
			rLeft = AtlasLeft;
			rBottom = AtlasBottom;
			rRight = AtlasRight;
			rTop = AtlasTop;
		}
	};

	struct AluraFontMetrics
	{
		// The size of one EM.
		double EmSize;

		// The vertical position of the ascender and descender relative to the baseline.
		double AscenderY, DescenderY;

		// The vertical difference between consecutive baselines.
		double LineHeight;

		// The vertical position and thickness of the underline.
		double UnderlineY, UnderlineThickness;
	};

	static_assert( std::is_trivially_copyable_v<AluraFontMetrics>, "AluraFontMetrics must be a POD type!" );

	class AluraFontData
	{
	public:
		AluraFontData() = default;
		~AluraFontData() = default;

		void AddCodepointToGlyph( uint32_t codepoint );
		void SetKerning( const std::map<std::pair<int, int>, double>& rMap );

		AluraSerialisedGlyph* GetGlyph( uint32_t codepoint );
		bool GetAdvance( double& adv, uint32_t a, uint32_t b );

		void ClearData();

	public:
		std::vector<AluraSerialisedGlyph>& GetGlyphs() { return m_AluraGlyphs; }
		const std::vector<AluraSerialisedGlyph>& GetGlyphs() const { return m_AluraGlyphs; }

		std::map<uint32_t, size_t>& GetCodepointToGlyph() { return m_CodepointToGlyph; }
		const std::map<uint32_t, size_t>& GetCodepointToGlyph() const { return m_CodepointToGlyph; }
		
		std::map<std::pair<int, int>, double>& GetKerning() { return m_Kerning; }
		const std::map<std::pair<int, int>, double>& GetKerning() const { return m_Kerning; }

		AluraFontMetrics& GetMetrics() { return m_Metrics; }
		const AluraFontMetrics& GetMetrics() const { return m_Metrics; }

	private:
		AluraFontMetrics m_Metrics{};
		std::vector<AluraSerialisedGlyph> m_AluraGlyphs;
		//      CODEPOINT -> INDEX
		std::map<uint32_t, size_t> m_CodepointToGlyph;

		//      {A, B} -> KERNING AMOUNT
		std::map<std::pair<int, int>, double> m_Kerning;
	};

	struct AluraMSDFGenerationData;

	class AluraFont : public Asset
	{
	public:
		AluraFont( const std::filesystem::path& rFontPath, const Ref<Asset>& rBase );
		AluraFont( const Ref<Asset>& rBase );
		virtual ~AluraFont();

		void Serialise( const std::filesystem::path& rPath ) const;
		void Deserialise( FDependentIStream& rStream );

		Ref<Texture2D> GetTexture() const { return m_TextureAtlas; }

		AluraFontData& GetFontData() { return m_AluraFontData; }
		const AluraFontData& GetFontData() const { return m_AluraFontData; }

		glm::vec2 CalcTextSize( float fontSize, const std::string& rText );

		std::filesystem::path GetFontFilepath() const { return m_FontFilepath; }
		std::string GetFontName() const { return m_Name; }

#if !defined(SAT_DIST)
		void OnReimport( const std::filesystem::path& rPath );
#endif

	private:
		void CreateOrLoadAtlas( bool overrideCache = false );
#if !defined(SAT_DIST)
		void CreateAtlasTexture( AluraMSDFGenerationData& rGenerationData, int width, int height );
#endif

	private:
		std::string m_Name;
		// The path to the font source i.e. MyFont.ttf
		std::filesystem::path m_FontFilepath;
		AluraFontData m_AluraFontData{};
		Ref<Texture2D> m_TextureAtlas;
		
	private:
		friend class AluraFontAssetViewer;
	};	
	
}
