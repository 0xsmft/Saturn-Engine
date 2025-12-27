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

#include "Saturn/Vulkan/AluraRenderer.h"

namespace Saturn {

	AluraCanvas::AluraCanvas( const std::string& rName, const glm::vec2& rSize, const glm::vec2& rPosition )
		: m_Name( rName ), m_Size( rSize ), m_Position( rPosition )
	{
		InitStyle();
	}

	void AluraCanvas::InitStyle()
	{
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
		// Font is null! Must have an active font
		SAT_CORE_ASSERT( m_ActiveFont );

		m_Layout = {};

		// Calculate mouse position relative to this canvas' position.
		m_MousePosition = Input::Get().MousePosition() - m_Position;

//		m_Renderer->SubmitRect( m_MousePosition, { m_MousePosition + glm::vec2{ 10.0f, 10.0f } }, { 1.0f, 0.0f, 0.0f, 1.0f } );
	}

	void AluraCanvas::Destory()
	{
		m_Renderer = nullptr;
	}

	void AluraCanvas::SetContext( Ref<AluraRenderer> context )
	{
		m_Renderer = context;
	}

	void AluraCanvas::PushFontAndSetActive( Ref<AluraFont> font )
	{
		m_Fonts.push_back( font );
		m_ActiveFont = font;
	}

	void AluraCanvas::PopFont()
	{
		if( m_Fonts.size() == 1 )
		{
			SAT_CORE_ERROR( "Fonts left after this one is popped: {0}", m_Fonts.size() - 1 );
			SAT_CORE_ASSERT( false, "No font can be selected after this font is popped!" );
		}
		else
		{
			m_Fonts.pop_back();
			m_ActiveFont = m_Fonts.back();

			// Font is null! Must have an active font
			SAT_CORE_ASSERT( m_ActiveFont );
		}
	}

	AluraElement* AluraCanvas::GetLastElement()
	{
		return m_Elements.size() ? &m_Elements.back() : nullptr;
	}

	AluraElement& AluraCanvas::AddRect( const glm::vec2& rSize, const glm::vec4& rColor )
	{
		// Handle SetNextItemPosition
		glm::vec2 posDependingLastCall = m_Layout.CursorPos;

		if( m_WantToSetItemPosition )
		{
			posDependingLastCall = m_PendingNextItemPosition;
			m_WantToSetItemPosition = false;
		}

		auto& rElement = m_Elements.emplace_back( "##noname", posDependingLastCall, rSize, rColor );
		m_Renderer->SubmitRect( { rElement.m_Position }, { rElement.m_Position + rElement.m_Size }, rElement.m_Color );

		AdvanceCursor( rSize );

		return rElement;
	}

	AluraElement& AluraCanvas::AddImage( const glm::vec2& rSize, Ref<Texture2D> image, const glm::vec4& rColor, const glm::vec2& rUV1, const glm::vec2& rUV2 )
	{
		// Handle SetNextItemPosition
		glm::vec2 posDependingLastCall = m_Layout.CursorPos;

		if( m_WantToSetItemPosition )
		{
			posDependingLastCall = m_PendingNextItemPosition;
			m_WantToSetItemPosition = false;
		}

		auto& rElement = m_Elements.emplace_back( "##noname", posDependingLastCall, rSize, rColor );
		AdvanceCursor( rSize );
		
		m_Renderer->SubmitRect( { rElement.m_Position }, { rElement.m_Position + rElement.m_Size }, image, rColor, rUV1, rUV2 );

		return rElement;
	}

	bool AluraCanvas::AddImageButton( const glm::vec2& rSize, Ref<Texture2D> image, const glm::vec4& rColor /*= glm::one<glm::vec4>()*/, const glm::vec2& rUV1 /*= { 0.0F, 1.0F }*/, const glm::vec2& rUV2 /*= { 1.0F, 0.0F } */ )
	{
		// Handle SetNextItemPosition
		glm::vec2 posDependingLastCall = m_Layout.CursorPos;

		if( m_WantToSetItemPosition )
		{
			posDependingLastCall = m_PendingNextItemPosition;
			m_WantToSetItemPosition = false;
		}

		const glm::vec2 frameThickness = glm::vec2{ 10.0f };
		auto& rElement = m_Elements.emplace_back( "##noname", posDependingLastCall - frameThickness, rSize + frameThickness, rColor );

		glm::vec4 frameColor = rColor;

		// Hit test on the frame
		if( IsItemHovered() )
		{
			frameColor = m_Style.Colors[ AluraColor_ButtonHovered ];
		}

		// Submit frame
		m_Renderer->SubmitRect( { rElement.m_Position - frameThickness }, { rElement.m_Position + rElement.m_Size + frameThickness }, frameColor );

		// Submit image
		m_Renderer->SubmitRect( { rElement.m_Position }, { rElement.m_Position + rElement.m_Size }, image, rElement.m_Color, rUV1, rUV2 );

		// Move on
		AdvanceCursor( rElement.m_Size );
		
		return IsItemClicked( RubyMouseButton_Left );
	}

	AluraElement& AluraCanvas::AddText( const std::string& rText, const glm::vec4& rColor )
	{
		m_Renderer->SubmitString( rText, m_ActiveFont, m_Style.CurrentFontSize, m_Layout.CursorPos, rColor );
		
		const auto textSize = m_ActiveFont->CalcTextSize( m_Style.CurrentFontSize, rText );
		auto& rElement = m_Elements.emplace_back( "##noname", m_Layout.CursorPos, textSize, rColor );

		AdvanceCursor( textSize );

		return rElement;
	}

	bool AluraCanvas::AddButton( const glm::vec2& rSize, const glm::vec4& rColor )
	{
		// Handle NextItemPosition
		glm::vec2 posDependingLastCall = m_Layout.CursorPos;

		if( m_WantToSetItemPosition )
		{
			posDependingLastCall = m_PendingNextItemPosition;
			m_WantToSetItemPosition = false;
		}

		auto& rElement = m_Elements.emplace_back( "##noname", posDependingLastCall, rSize, rColor );
		
		m_Renderer->SubmitRect( { rElement.m_Position }, { rElement.m_Position + rElement.m_Size }, rColor );
		
		AdvanceCursor( rElement.m_Size );

		// Hit tests
		if( IsItemHovered() )
		{
			rElement.m_Color = m_Style.Colors[ AluraColor_ButtonHovered ];
		}

		return IsItemClicked( RubyMouseButton_Left );
	}

	bool AluraCanvas::AddButton( const std::string& rText )
	{
		// Handle NextItemPosition
		glm::vec2 posDependingLastCall = m_Layout.CursorPos;

		if( m_WantToSetItemPosition )
		{
			posDependingLastCall = m_PendingNextItemPosition;
			m_WantToSetItemPosition = false;
		}

		const auto textSize = m_ActiveFont->CalcTextSize( m_Style.CurrentFontSize, rText );

		const glm::vec2 frameThickness = glm::vec2{ 10.0f };
		auto& rElement = m_Elements.emplace_back( "##noname", posDependingLastCall - frameThickness, textSize + frameThickness, m_Style.Colors[ AluraColor_Text ] );

		// Hit tests
		if( IsItemHovered() )
		{
			rElement.m_Color = m_Style.Colors[ AluraColor_ButtonHovered ];
		}

		// Button Rect
		m_Renderer->SubmitRect( { rElement.m_Position }, { rElement.m_Position + rElement.m_Size },  m_Style.Colors[ AluraColor_Button ] );

		// Submit Text centred inside the button.
		const glm::vec2 position = rElement.m_Position + ( rElement.m_Size - textSize ) * 0.5f;
		m_Renderer->SubmitString( rText, m_ActiveFont, m_Style.CurrentFontSize, position, m_Style.Colors[ AluraColor_Text ] );

		// Move on
		AdvanceCursor( rElement.m_Size );

		return IsItemClicked( RubyMouseButton_Left );
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

	void AluraCanvas::AlignNextItemCenterXY( const glm::vec2& rSize )
	{
		glm::vec2 position = { ( m_Size.x - rSize.x ) * 0.5f, ( m_Size.y - rSize.y ) * 0.5f };
		SetNextItemPosition( position );
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
