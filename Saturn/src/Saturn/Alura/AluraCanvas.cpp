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
#include "AluraCanvas.h"

#include "AluraRect.h"

#include "AluraMSDFData.h"

#include "Saturn/Vulkan/AluraRenderer.h"

namespace Saturn {

	AluraCanvas::AluraCanvas( const std::string& rName, const glm::vec2& rSize, const glm::vec2& rPosition )
		: m_Name( rName ), m_Size( rSize ), m_Position( rPosition )
	{
		InitStyle();
	}

	void AluraCanvas::InitStyle()
	{
		m_Style.Colors.fill( glm::one<glm::vec4>() );

		m_Style.Colors[ AluraColor_Text          ] = glm::one<glm::vec4>();
		m_Style.Colors[ AluraColor_TextDisabled  ] = glm::vec4( 0.5f, 0.5f, 0.5f, 1.0f );
		m_Style.Colors[ AluraColor_Button        ] = glm::vec4( 0.26f, 0.59f, 0.98f, 0.40f );
		m_Style.Colors[ AluraColor_ButtonHovered ] = glm::vec4( 0.26f, 0.59f, 0.98f, 1.00f );
	}

	AluraCanvas::~AluraCanvas()
	{
		Destory();
	}

	void AluraCanvas::Begin()
	{
		m_Layout = {};

		// Calculate mouse position relative to this canvas' position.
		m_MousePosition = Input::Get().MousePosition() - m_Position;

//		m_Renderer->SubmitRect( m_MousePosition, { m_MousePosition + glm::vec2{ 10.0f, 10.0f } }, { 1.0f, 0.0f, 0.0f, 1.0f } );
	}

	void AluraCanvas::End( Timestep ts )
	{
		for( auto& rElement : m_Elements )
		{
			switch( rElement.m_RenderType )
			{
				default: break;
			
				case AluraRenderType::Quad:
				{
					m_Renderer->SubmitRect( { rElement.m_Position }, { rElement.m_Position + rElement.m_Size }, rElement.m_Color );
				} break;
			}
		}

		m_Renderer->SubmitRect( m_Layout.CursorPos, { m_Layout.CursorPos + glm::vec2{ 10.0f, 10.0f } }, { 1.0f, 0.0f, 0.0f, 1.0f } );

		m_Elements.clear();
	}

	void AluraCanvas::Destory()
	{
		m_Renderer = nullptr;
	}

	void AluraCanvas::SetContext( Ref<AluraRenderer> context )
	{
		m_Renderer = context;
	}

	AluraElement* AluraCanvas::GetLastElement()
	{
		return m_Elements.size() ? &m_Elements.back() : nullptr;
	}

	AluraElement& AluraCanvas::AddRect( const glm::vec2& rSize, const glm::vec4& rColor )
	{
		// Handle SetNextItemSize & SetNextItemPosition
		glm::vec2 sizeDependingLastCall = rSize;
		glm::vec2 posDependingLastCall = m_Layout.CursorPos;

		if( m_WantToSetItemSize ) 
		{
			sizeDependingLastCall = m_PendingNextItemSize;
			m_WantToSetItemSize = false;
		}

		if( m_WantToSetItemPosition )
		{
			posDependingLastCall = m_PendingNextItemPosition;
			m_WantToSetItemPosition = false;
		}

		m_Elements.emplace_back( "##noname", posDependingLastCall, sizeDependingLastCall, rColor );
		AdvanceCursor( rSize );

		return m_Elements.back();
	}

	AluraElement& AluraCanvas::AddText( const std::string& rText, Ref<AluraFont> font, const glm::vec4& rColor )
	{
		const auto& rFontGeo = font->GetMSDFData()->FontGeometry;
		const auto& rMetrics = rFontGeo.getMetrics();

		double x = 0.0;
		double fsScale = 64.0f / ( rMetrics.ascenderY - rMetrics.descenderY );
		double y = fsScale * rMetrics.ascenderY;

		const auto textSize = font->CalcTextSize( 64.0f, rText );
		AdvanceCursor( textSize );

		for( size_t i = 0; i < rText.size(); i++ )
		{
			const char character = rText[ i ];
			if( character == '\r' ) continue;

			if( character == '\n' )
			{
				x = 0;
				y -= fsScale * rMetrics.lineHeight;
				continue;
			}

			auto glyph = rFontGeo.getGlyph( character );
			if( character == ' ' )
			{
				double advance = glyph->getAdvance();
				x += fsScale * advance;
				continue;
			}
			// TOOD: Add a font setting or a style setting to determinate how many spaces a tab should be
			// Right now we'll do 4 spaces.
			else if( character == '\t' )
			{
				glyph = rFontGeo.getGlyph( ' ' );
				double advance = glyph->getAdvance() * 4 /* NUMBER_OF_SPACES_PER_TAB*/;
				x += fsScale * advance;
				continue;
			}

			if( !glyph ) glyph = rFontGeo.getGlyph( '?' );

			double atlasLeft, atlasBottom, atlasRight, atlasTop;
			glyph->getQuadAtlasBounds( atlasLeft, atlasBottom, atlasRight, atlasTop );

			// NOTE: Vulkan: We have to flip the atlasTop and atlasBottom because in the Editor the UI origin is the bottom-left
			// the reason why it's the bottom right is because when this image gets flipped in the viewport, the elements at the bottom-left
			// will be at the top-left, which is correct as the real origin is actually at the top-left.
			glm::vec2 texCoordMin( atlasLeft, atlasTop );
			glm::vec2 texCoordMax( atlasRight, atlasBottom );

			double planeLeft, planeBottom, planeRight, planeTop;
			glyph->getQuadPlaneBounds( planeLeft, planeBottom, planeRight, planeTop );

			// NOTE: Vulkan: Same as above.
			glm::vec2 quadMin( x + planeLeft * fsScale, y - planeTop * fsScale );
			glm::vec2 quadMax( x + planeRight * fsScale, y - planeBottom * fsScale );

			const float texelWidth = 1.0f / font->GetTexture()->Width();
			const float texelHeight = 1.0f / font->GetTexture()->Height();

			texCoordMin *= glm::vec2( texelWidth, texelHeight );
			texCoordMax *= glm::vec2( texelWidth, texelHeight );

			m_Renderer->SubmitText( quadMin, quadMax, texCoordMin, texCoordMax, rColor, font->GetTexture(), m_Layout.CursorPos );

			// Next character spacing
			if( i < rText.size() - 1 )
			{
				double advance = glyph->getAdvance();
				char next = rText[ i + 1 ];
				rFontGeo.getAdvance( advance, character, next );

				x += fsScale * advance + 0.0f;
			}
		}

		auto& rElement = m_Elements.emplace_back( "##noname", m_Layout.CursorPos, textSize, rColor );
		rElement.m_RenderType = AluraRenderType::Text;
		return rElement;
	}

	bool AluraCanvas::AddButton( const glm::vec2& rSize, const glm::vec4& rColor )
	{
		// Handle SetNextItemSize & SetNextItemPosition
		glm::vec2 sizeDependingLastCall = rSize;
		glm::vec2 posDependingLastCall = m_Layout.CursorPos;

		if( m_WantToSetItemSize )
		{
			sizeDependingLastCall = m_PendingNextItemSize;
			m_WantToSetItemSize = false;
		}

		if( m_WantToSetItemPosition )
		{
			posDependingLastCall = m_PendingNextItemPosition;
			m_WantToSetItemPosition = false;
		}

		auto& rElement = m_Elements.emplace_back( "##noname", posDependingLastCall, sizeDependingLastCall, rColor );
		AdvanceCursor( rSize );

		// Hit tests
		if( IsItemHovered() )
		{
			rElement.m_Color = m_Style.Colors[ AluraColor_ButtonHovered ];
		}

		return IsItemClicked( RubyMouseButton_Left );
	}

	void AluraCanvas::SetNextItemSize( const glm::vec2& rSize )
	{
		m_WantToSetItemSize = true;
		m_PendingNextItemSize = rSize;
	}

	void AluraCanvas::SetNextItemPosition( const glm::vec2& rPosition )
	{
		m_WantToSetItemPosition = true;
		m_PendingNextItemPosition = rPosition;
	}

	void AluraCanvas::Indent( float width )
	{
		m_Layout.CurrentIndent += ( width == 0.0f ) ? m_Style.IndentSpacing : width;
		m_Layout.CursorPos.x = m_Layout.CurrentIndent;
	}

	void AluraCanvas::Unindent( float width )
	{
		m_Layout.CurrentIndent -= ( width == 0.0f ) ? m_Style.IndentSpacing : width;
		m_Layout.CursorPos.x = m_Layout.CurrentIndent;
	}

	bool AluraCanvas::IsItemHovered()
	{
		if( !m_Elements.size() )
			return false;

		AluraElement& rElement = m_Elements.back();
		return IsMouseHoveringRect( rElement.m_Position, { rElement.m_Position + rElement.m_Size } );
	}

	bool AluraCanvas::IsItemClicked( RubyMouseButton mouseBtn )
	{
		return Input::Get().MouseButtonPressed( mouseBtn ) && IsItemHovered();
	}

	void AluraCanvas::AlignItemCenterXY()
	{
		auto* pElement = GetLastElement();
		if( pElement )
		{
			pElement->m_Position.x = ( m_Size.x - pElement->m_Size.x ) * 0.5f;
			pElement->m_Position.y = ( m_Size.y - pElement->m_Size.y ) * 0.5f;
		}
	}

	void AluraCanvas::AdvanceCursor( const glm::vec2& rSize )
	{
		const float offsetInlineWithBaselineY = glm::max( 0.0f, m_Layout.CurrLineTextBaseOffset );
		const float lineY = m_Layout.IsSameLine ? m_Layout.CursorPosPrevLine.y : m_Layout.CursorPos.y;
		const float lineHeight = glm::max( m_Layout.CurrLineSize.y, m_Layout.CursorPos.y - lineY + rSize.y + offsetInlineWithBaselineY );

		m_Layout.CursorPosPrevLine.x = m_Layout.CursorPos.x + rSize.x;
		m_Layout.CursorPosPrevLine.y = lineY;
		m_Layout.CursorPos.x = glm::trunc( m_Layout.CurrentIndent );
		m_Layout.CursorPos.y = glm::trunc( lineY + lineHeight + m_Style.ItemSpacing.y );
		m_Layout.IsSameLine = false;
	}
	
	bool AluraCanvas::IsMouseHoveringRect( const glm::vec2& rMin, const glm::vec2& rMax ) const
	{
		AluraRect rect( rMin, rMax );
		return rect.Contains( m_MousePosition );
	}

}
