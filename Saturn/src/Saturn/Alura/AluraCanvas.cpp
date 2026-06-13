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
#include "AluraCanvas.h"

#include "AluraRect.h"
#include "AluraStylingProfile.h"

#include "Saturn/Vulkan/AluraRenderer.h"
#include "Saturn/Asset/AssetManager.h"

namespace Saturn {

	void AluraLayout::Reset()
	{
		CursorPos = CursorPosPrevLine = CurrLineSize = PrevLineSize = glm::zero<glm::vec2>();

		CurrLineTextBaseOffset = PrevLineTextBaseOffset = CurrentIndent = 0.0f;
		IsSameLine = false;
	}

	//////////////////////////////////////////////////////////////////////////

	AluraCanvas::AluraCanvas( const AluraCanvasSpecification& rSpecification )
		: m_Size( rSpecification.Size ), m_Position( rSpecification.Position )
	{
		m_ActiveFont = AssetManager::Get()->GetAssetAs<AluraFont>( rSpecification.MasterFontAssetID );

		// Reserve some space for fonts so assume, regular, bold, italics...
		m_Fonts.reserve( 3 );
		m_Fonts.push_back( m_ActiveFont );

		Ref<AluraStylingProfile> stylingProf = AssetManager::Get()->GetAssetAs<AluraStylingProfile>( rSpecification.StylingProfile );
		if( stylingProf )
		{
			m_Style = stylingProf->GetStyle();
		}

		// Load editor font
		m_EditorFont = Ref<AluraFont>::Create();

		// NB: Remember this is an editor filepath!
		m_EditorFont->Deserialise_ForAluraCanvas( "content/Templates/trebuc.saf" );
	}

	AluraCanvas::~AluraCanvas()
	{
		Destroy();
	}

	void AluraCanvas::NewFrame()
	{
		// Font is null! Must have an active font.
//		SAT_CORE_ASSERT( m_ActiveFont );

		// Forgot to call PopStyle()
		SAT_CORE_ASSERT( m_ColorStack.size() == 0 );

		// Forgot to call PopFontSize()
		SAT_CORE_ASSERT( m_PushedFontSize == 0.0f );

		m_Layout.Reset();

		// Calculate mouse position relative to this canvas' position.
		m_MousePosition = Input::Get().MousePosition() - m_Position;
	
		if( m_FirstFrameEver ) 
		{
			m_Layout.CursorStartingPos = m_Style.WindowPadding;
			m_Layout.CurrentIndent = m_Style.WindowPadding.x;

			m_Layout.CursorPos = m_Layout.CursorStartingPos;
			m_Layout.CursorPosPrevLine = m_Layout.CursorStartingPos;

			m_FirstFrameEver = false;
		}
	}

	void AluraCanvas::DrawAllDrawers( Timestep ts )
	{
		for( auto& rDrawer : m_Drawers )
		{
			rDrawer->OnDraw( ts );
		}
	}

	void AluraCanvas::HandleDrawerEvents( Event& rEvent )
	{
		// TODO: reverse
		for( auto& rDrawer : m_Drawers )
		{
			rDrawer->OnEvent( rEvent );
		}
	}

	void AluraCanvas::Destroy()
	{
		for( auto& rDrawer : m_Drawers )
		{
			rDrawer->OnDestroy();
		}

		m_Drawers.clear();

		m_Renderer = nullptr;
		m_ActiveFont = nullptr;
		m_EditorFont = nullptr;
		m_Fonts.clear();
	}

	void AluraCanvas::EndFrame()
	{
		m_FirstFrameEver = true;
	}

	void AluraCanvas::AddDrawer( Ref<AluraDrawer> drawer )
	{
		m_Drawers.push_back( drawer );
		drawer->OnInit();
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

	void AluraCanvas::AddRect( const glm::vec2& rSize, const glm::vec4& rColor )
	{
		// Handle SetNextItemPosition
		glm::vec2 posDependingLastCall = m_Layout.CursorPos;

		if( m_WantToSetItemPosition )
		{
			posDependingLastCall = m_PendingNextItemPosition;
			m_WantToSetItemPosition = false;
		}

		m_Renderer->SubmitRect( posDependingLastCall, { posDependingLastCall + rSize }, rColor );

		AdvanceCursor( rSize );
	}

	void AluraCanvas::AddImage( const glm::vec2& rSize, Ref<Texture2D> image, const glm::vec4& rColor, const glm::vec2& rUV1, const glm::vec2& rUV2 )
	{
		// Handle SetNextItemPosition
		glm::vec2 posDependingLastCall = m_Layout.CursorPos;

		if( m_WantToSetItemPosition )
		{
			posDependingLastCall = m_PendingNextItemPosition;
			m_WantToSetItemPosition = false;
		}

		AdvanceCursor( rSize );
		
		m_Renderer->SubmitRect( posDependingLastCall, { posDependingLastCall + rSize }, image, rColor, rUV1, rUV2 );
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
		glm::vec4 frameColor = rColor;
		
		// Hit test on the frame
		const bool hovered = IsMouseHoveringRect( posDependingLastCall, { posDependingLastCall + rSize } );
		if( hovered )
		{
			frameColor = m_Style.Colors[ AluraColor_ButtonHovered ];
		}

		// Submit frame
		m_Renderer->SubmitRect( { posDependingLastCall - frameThickness }, { posDependingLastCall + rSize + frameThickness }, frameColor );

		// Submit image
		m_Renderer->SubmitRect( posDependingLastCall, { posDependingLastCall + rSize }, image, rColor, rUV1, rUV2 );

		// Move on
		AdvanceCursor( rSize );
		
		return Input::Get().MouseButtonPressed( RubyMouseButton_Left ) && hovered;
	}

	void AluraCanvas::AddProgressBar( float fraction, const glm::vec2& rSize )
	{
		// Handle SetNextItemPosition
		glm::vec2 posDependingLastCall = m_Layout.CursorPos;

		if( m_WantToSetItemPosition )
		{
			posDependingLastCall = m_PendingNextItemPosition;
			m_WantToSetItemPosition = false;
		}

		AluraRect boundingBox( posDependingLastCall, posDependingLastCall + rSize );

		fraction = glm::clamp( fraction, 0.0f, 1.0f );
		
		glm::vec2 fillMax( std::lerp( boundingBox.Min.x, boundingBox.Max.x, fraction ), boundingBox.Max.y );

		m_Renderer->SubmitRect( boundingBox.Min, boundingBox.Max, m_Style.Colors[ AluraColor_FrameBackground ] );
		m_Renderer->SubmitRect( boundingBox.Min, fillMax, m_Style.Colors[ AluraColor_ProgressColor ] );
		m_Renderer->SubmitRectFrame( boundingBox.Min, boundingBox.Max, 1.0f, m_Style.Colors[ AluraColor_FrameBorder ] );
		
		// Move on.
		AdvanceCursor( boundingBox.GetSize() );
	}

	void AluraCanvas::AddText( const std::string& rText, const glm::vec4& rColor )
	{
		// Handle SetNextItemPosition
		glm::vec2 posDependingLastCall = m_Layout.CursorPos;

		if( m_WantToSetItemPosition )
		{
			posDependingLastCall = m_PendingNextItemPosition;
			m_WantToSetItemPosition = false;
		}

		m_Renderer->SubmitString( rText, m_ActiveFont, m_Style.CurrentFontSize, posDependingLastCall, rColor );
		
		const auto textSize = m_ActiveFont->CalcTextSize( m_Style.CurrentFontSize, rText );

		AdvanceCursor( textSize );
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

		glm::vec4 color = rColor;

		// Adjust size for padding
		glm::vec2 size = rSize;
		size += m_Style.WindowPadding * 2.0f;

		AdvanceCursor( size );

		// Hit tests
		const bool hovered = IsMouseHoveringRect( posDependingLastCall, { posDependingLastCall + size } );
		if( hovered )
		{
			color = m_Style.Colors[ AluraColor_ButtonHovered ];
		}

		m_Renderer->SubmitRect( posDependingLastCall, { posDependingLastCall + size }, color );

		return Input::Get().MouseButtonPressed( RubyMouseButton_Left ) && hovered;
	}

	bool AluraCanvas::AddButton( const std::string& rText, const glm::vec2& rSize /*= glm::zero<glm::vec2>()*/ )
	{
		// Handle NextItemPosition
		glm::vec2 posDependingLastCall = m_Layout.CursorPos;

		if( m_WantToSetItemPosition )
		{
			posDependingLastCall = m_PendingNextItemPosition;
			m_WantToSetItemPosition = false;
		}

		//////////////////////////////////////////////////////////////////////////

		const auto textSize = m_ActiveFont->CalcTextSize( m_Style.CurrentFontSize, rText );
		glm::vec2 size = CalcItemSize( rSize, textSize.x + m_Style.WindowPadding.x * 2.0f, textSize.y + m_Style.WindowPadding.y * 2.0f );

		// Hit tests
		glm::vec4 buttonColor = m_Style.Colors[ AluraColor_Button ];
		const bool hovered = IsMouseHoveringRect( posDependingLastCall, { posDependingLastCall + size } );
		if( hovered )
		{
			buttonColor = m_Style.Colors[ AluraColor_ButtonHovered ];
		}

		// Button Rect
		m_Renderer->SubmitRect( posDependingLastCall, { posDependingLastCall + size }, buttonColor );

		// Submit Text centred inside the button.
		const glm::vec2 position = posDependingLastCall;
		m_Renderer->SubmitString( rText, m_ActiveFont, m_Style.CurrentFontSize, position, m_Style.Colors[ AluraColor_Text ] );

		// Move on
		AdvanceCursor( size );

		return Input::Get().MouseButtonPressed(  RubyMouseButton_Left ) && hovered;
	}

	void AluraCanvas::AddCircle( float radius, float thinkness, bool filled /*= false*/, const glm::vec4& rColor /*= glm::one<glm::vec4>() */ )
	{
		// Handle NextItemPosition
		glm::vec2 posDependingLastCall = m_Layout.CursorPos;

		if( m_WantToSetItemPosition )
		{
			posDependingLastCall = m_PendingNextItemPosition;
			m_WantToSetItemPosition = false;
		}

		m_Renderer->SubmitCircle( posDependingLastCall + radius * 2.0f, radius, thinkness, rColor );
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

	void AluraCanvas::AlignNextItemCenterXY( const glm::vec2& rSize )
	{
		glm::vec2 position = { ( m_Size.x - rSize.x ) * 0.5f, ( m_Size.y - rSize.y ) * 0.5f };
		SetNextItemPosition( position );
	}
	
	void AluraCanvas::SameLine( float offset /*= 0.0f*/, float spacing /*= -1.0f */ )
	{
		if( offset != 0.0f )
		{
			if( spacing < 0.0f )
				spacing = 0.0f;

			// TODO
		}
		else
		{
			if( spacing < 0.0f )
				spacing = m_Style.ItemSpacing.x;

			m_Layout.CursorPos.x = m_Layout.CursorPosPrevLine.x + spacing;
			m_Layout.CursorPos.y = m_Layout.CursorPosPrevLine.y;
		}

		m_Layout.CurrLineSize = m_Layout.PrevLineSize;
		m_Layout.CurrLineTextBaseOffset = m_Layout.PrevLineTextBaseOffset;
		m_Layout.IsSameLine = true;
	}

	void AluraCanvas::PushStyle( std::underlying_type_t<AluraColor> index, const glm::vec4& rNewValue )
	{
		m_ColorStack.emplace( m_Style.Colors[ index ], index );
		m_Style.Colors[ index ] = rNewValue;
	}

	void AluraCanvas::PopStyle()
	{
		const auto& rBackupData = m_ColorStack.top();
		m_Style.Colors[ rBackupData.Index ] = rBackupData.OldValue;
		m_ColorStack.pop();
	}

	void AluraCanvas::PushFontSize( float newSize )
	{
		m_PushedFontSize = m_Style.CurrentFontSize;
		m_Style.CurrentFontSize = newSize;
	}

	void AluraCanvas::PopFontSize()
	{
		m_Style.CurrentFontSize = m_PushedFontSize;
		m_PushedFontSize = 0.0f;
	}

	glm::vec2 AluraCanvas::CalcTextSize( const std::string& rText )
	{
		SAT_CORE_ASSERT( m_ActiveFont );
		return m_ActiveFont->CalcTextSize( m_Style.CurrentFontSize, rText );
	}

#if !defined(SAT_DIST)
	void AluraCanvas::EdClearCanvas()
	{
		m_Renderer->EdClearCommands();
	}
#endif

	void AluraCanvas::AdvanceCursor( const glm::vec2& rSize )
	{
		//
		// TODO: Y layout only!
		// Meaning we move down a "line" every time we advance the cursor.
		//
		// Adding support for X layout would be very simple, we'd just need to check if out current layout type is horizontal and if so move along the X coord instead of the Y
		//
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

	// @see imgui.cpp - CalcItemSize
	glm::vec2 AluraCanvas::CalcItemSize( glm::vec2 size, float w, float h )
	{
		if( size.x == 0.0f )
			size.x = w;
		else if( size.x < 0.0f )
			size.x = glm::max( 4.0f, m_Layout.CursorPos.x + size.x );

		if( size.y == 0.0f )
			size.y = h;
		else if( size.y < 0.0f )
			size.y = glm::max( 4.0f, m_Layout.CursorPos.y + size.y );

		return size;
	}
}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

constexpr std::underlying_type_t<Saturn::SClassFlags> RStaticClassFlagsAluraDrawer = ( Saturn::SClassFlags ) Saturn::SC_VisibleInEditor | Saturn::SC_NoExtendedMetadata | Saturn::SC_Abstract;

static Saturn::SClass* RStaticLnkAluraDrawer()
{
	static Saturn::SClass* pClass = nullptr;
	if( !pClass )
	{
		const Saturn::SClassSpecification spec
		{
			"AluraDrawer",
			RStaticClassFlagsAluraDrawer,
			0,
			sizeof( Saturn::AluraDrawer ), alignof( Saturn::AluraDrawer ),
			Saturn::FNV1A64( "AluraDrawer" ),
			Saturn::AluraDrawer::Super::StaticClass(), nullptr, RStaticLnkAluraDrawer, nullptr, {}
		};

		Saturn::SClass::RConstructClass( pClass, spec );
	}

	return pClass;
}

Saturn::SClass* Saturn::AluraDrawer::GetStaticClassInternal()
{
	return RStaticLnkAluraDrawer();
}

static Saturn::SClassRegistrar RCRBehaviourTreeNodeBase( RStaticLnkAluraDrawer );
