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

#include "AluraStylingProfile.h"

#include "Saturn/Core/Input.h"
#include "Saturn/Core/App.h"
#include "Saturn/Core/Ruby/RubyWindow.h"

#include "Saturn/Vulkan/AluraRenderer.h"
#include "Saturn/Asset/AssetManager.h"

#include "SharedGlobals.h"

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// ALURA LAYOUT

	void AluraLayout::Reset()
	{
		CursorPos = CursorPosPrevLine = CurrLineSize = PrevLineSize = glm::zero<glm::vec2>();

		CurrLineTextBaseOffset = PrevLineTextBaseOffset = CurrentIndent = 0.0f;
		IsSameLine = false;
	}

	//////////////////////////////////////////////////////////////////////////
	// ALURA CANVAS

	AluraCanvas::AluraCanvas( const AluraCanvasSpecification& rSpecification )
		: m_CanvasSize( rSpecification.Size ), m_Position( rSpecification.Position )
	{
		SAT_CORE_ASSERT( !g_AluraCanvas, "Another canvas already exists!" );

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
		m_EditorFont->Deserialise_ForAluraCanvas( Passkey<AluraCanvas>(), "content/Templates/NotoSansMono-Regular.saf" );
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
		m_MousePosition = Input::Get().MousePositionRelativeToAppWindow() - m_Position;

		// Setup layout
		m_Hot = 0llu;
		m_Layout.CursorStartingPos = m_Style.WindowPadding;
		m_Layout.CurrentIndent     = m_Style.WindowPadding.x;
		m_Layout.CursorPos         = m_Layout.CursorStartingPos;
		m_Layout.CursorPosPrevLine = m_Layout.CursorStartingPos;

		// Push default clipping rect.
		m_Renderer->PushClipRect( m_CanvasSize );
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

	void AluraCanvas::OnSceneChange()
	{
		for( auto& rDrawer : m_Drawers )
		{
			rDrawer->OnDestroy();
		}

		m_Drawers.clear();

		// Rest persistent states.
		// Setting m_Hot here is me being a bit pedantic as AluraLayer
		// will lose it control over input after this function.
		m_Hot = m_Active = m_Focused = m_Selected = 0llu;
	}

	void AluraCanvas::EndFrame()
	{
		m_Renderer->PopClipRect();

		// If any keys are "released" from the previous frame, we
		// reset them.
		ResetInputStates();
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

		const AluraRect bb( posDependingLastCall, posDependingLastCall + rSize );
		ItemSize( rSize );
		if( !CanAddItem( bb ) )
			return;

		m_Renderer->SubmitRect( bb, rColor );
	}

		ItemSize( rSize );
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

		const AluraRect bb( posDependingLastCall, posDependingLastCall + rSize );
		ItemSize( bb.GetSize() );
		if( !CanAddItem( bb ) )
			return;

		m_Renderer->SubmitRect( bb, image, rColor, rUV1, rUV2 );
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

		const glm::vec2 padding = m_Style.WindowPadding;
		const AluraRect bb( posDependingLastCall, posDependingLastCall + rSize + padding );

		ItemSize( rSize );
		if( !CanAddItem( bb ) )
			return false;

		const UUID currentItemID = FNV1A64( "imgbtn" );

		bool hovered, held;
		bool pressed = ButtonBehaviour( bb, currentItemID, &hovered, &held );
		
		const glm::vec4 frameColor = hovered ? m_Style.Colors[ AluraColor_ButtonHovered ] : rColor;

		// Submit frame
		m_Renderer->SubmitRect( 
			bb,
			frameColor );

		// Submit image
		m_Renderer->SubmitRect( 
			posDependingLastCall, 
			{ posDependingLastCall + rSize }, 
			image, 
			rColor, 
			rUV1, 
			rUV2 );

		return pressed;
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

		const AluraRect bb( posDependingLastCall, posDependingLastCall + rSize );
		ItemSize( bb.GetSize() );
		if( !CanAddItem( bb ) )
			return;

		fraction = glm::clamp( fraction, 0.0f, 1.0f );
		
		glm::vec2 fillMax( std::lerp( bb.Min.x, bb.Max.x, fraction ), bb.Max.y );

		m_Renderer->SubmitRect( bb, m_Style.Colors[ AluraColor_FrameBackground ] );
		m_Renderer->SubmitRect( bb.Min, fillMax, m_Style.Colors[ AluraColor_ProgressColor ] );
		m_Renderer->SubmitRectFrame( bb.Min, bb.Max, 1.0f, m_Style.Colors[ AluraColor_Border ] );
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

		const auto textSize = m_ActiveFont->CalcTextSize( m_Style.CurrentFontSize, rText );
		AluraRect bb( posDependingLastCall, posDependingLastCall + textSize );
		
		ItemSize( bb.GetSize() );
		if( !CanAddItem( bb ) )
			return;

		m_Renderer->SubmitString( rText, m_ActiveFont, m_Style.CurrentFontSize, posDependingLastCall, rColor );
		
#if defined(SAT_ALURA_SHOW_TEXT_BB)
		m_Renderer->SubmitRect( bb, { 1.0f, 0.0f, 0.0f, 1.0f } );
#endif
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
		const AluraRect bb( posDependingLastCall, posDependingLastCall + size );

		ItemSize( size );
		if( !CanAddItem( bb ) )
			return false;

		uint64_t id = FNV1A64( "btnnoname" );

		bool hovered, held;
		bool pressed = ButtonBehaviour( bb, id, &hovered, &held );

		// Hit tests
		if( hovered )
		{
			color = m_Style.Colors[ AluraColor_ButtonHovered ];
		}

		m_Renderer->SubmitRect( bb, color );
		m_Renderer->SubmitRectFrame( bb, 1.0f, m_Style.Colors[ AluraColor_Border ] );

		return pressed;
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

		const glm::vec2 textSize = m_ActiveFont->CalcTextSize( m_Style.CurrentFontSize, rText );
		
		// The button rectangle needs to accommodate the inner spacing of the text in the X and Y
		const glm::vec2 rectSize = { textSize.x + m_Style.ItemInnerSpacing.x * 2.0f, textSize.y + m_Style.ItemInnerSpacing.y };

		const AluraRect bb( posDependingLastCall, posDependingLastCall + rectSize );

		// Move on
		ItemSize( bb.GetSize() );
		if( !CanAddItem( bb ) )
			return false;
			
		const UUID currentItemID = FNV1A64( rText.c_str() );

		bool hovered, held;
		bool pressed = ButtonBehaviour( bb, currentItemID, &hovered, &held );

		const glm::vec4 buttonColor = 
			hovered ? m_Style.Colors[ AluraColor_ButtonHovered ] : m_Style.Colors[ AluraColor_Button ];

		// Button Rect
		m_Renderer->SubmitRect( bb, buttonColor );
		m_Renderer->SubmitRectFrame( bb, 1.0f, m_Style.Colors[ AluraColor_Border ] );

		// Submit Text centred inside the button.
		// and bring it in by the padding on the X coord.
		const glm::vec2 textPos = { posDependingLastCall.x + m_Style.ItemInnerSpacing.x, posDependingLastCall.y };
		m_Renderer->SubmitString( rText, m_ActiveFont, m_Style.CurrentFontSize, textPos, m_Style.Colors[ AluraColor_Text ] );

		return pressed;
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

		m_Renderer->SubmitCircle( posDependingLastCall, radius, thinkness, rColor );
	}

	bool AluraCanvas::AddCheckbox( const std::string& rLabel, bool* pValue )
	{
		SAT_CORE_ASSERT( pValue );

		// Handle NextItemPosition
		glm::vec2 posDependingLastCall = m_Layout.CursorPos;

		if( m_WantToSetItemPosition )
		{
			posDependingLastCall = m_PendingNextItemPosition;
			m_WantToSetItemPosition = false;
		}

		const auto offsetToBeInLineWithText = m_ActiveFont->GetStartingYCoord();
		const auto offsetPosition = glm::vec2{ posDependingLastCall.x, posDependingLastCall.y + offsetToBeInLineWithText };

		const glm::vec2 textSize = m_ActiveFont->CalcTextSize( m_Style.CurrentFontSize, rLabel );

		const glm::vec2 squareSize = glm::vec2( textSize.y );

		const AluraRect bb( offsetPosition,
			offsetPosition + glm::vec2( squareSize.x + ( textSize.x > 0.0f ? m_Style.ItemInnerSpacing.x + textSize.x : 0.0f ), textSize.y ) );
		
		ItemSize( bb.GetSize() );
		if( !CanAddItem( bb ) )
			return false;

		const auto min = glm::vec2{ offsetPosition.x, offsetPosition.y };
		const AluraRect checkBoxBB( min, min + squareSize );

		bool hovered = false;
		const bool pressed = ButtonBehaviour( checkBoxBB, FNV1A64( rLabel.c_str() ), &hovered, nullptr );

		if( pressed )
		{
			*pValue ^= 1;
		}

		const glm::vec4 checkBoxColor = hovered ? m_Style.Colors[ AluraColor_ButtonHovered ] : m_Style.Colors[ AluraColor_Button ];

#if defined(SAT_ALURA_SHOW_TEXT_BB)
		m_Renderer->SubmitRect( bb, glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f } );
#endif

		m_Renderer->SubmitRect( checkBoxBB, checkBoxColor );
		m_Renderer->SubmitRectFrame( checkBoxBB, 1.0f, m_Style.Colors[ AluraColor_Border ] );

		if( *pValue )
		{
			const float pad = glm::max( 1.0f, glm::trunc( squareSize.x / 6.0f ) );
			m_Renderer->SubmitCheckMark( checkBoxBB.Min + pad, glm::one<glm::vec4>(), squareSize.x - pad * 2.0f );
		}

		const glm::vec2 textPos = { posDependingLastCall.x + squareSize.x + m_Style.ItemInnerSpacing.x, posDependingLastCall.y };
		m_Renderer->SubmitString( rLabel, m_ActiveFont, m_Style.CurrentFontSize, textPos, m_Style.Colors[ AluraColor_Text ] );

		return false;
	}

	bool AluraCanvas::AddCheckboxRight( const std::string& rLabel, bool* pValue )
	{
		SAT_CORE_ASSERT( pValue );

		// Handle NextItemPosition
		glm::vec2 posDependingLastCall = m_Layout.CursorPos;

		if( m_WantToSetItemPosition )
		{
			posDependingLastCall = m_PendingNextItemPosition;
			m_WantToSetItemPosition = false;
		}

		const auto offsetToBeInLineWithText = m_ActiveFont->GetStartingYCoord();
		const auto offsetPosition = glm::vec2{ posDependingLastCall.x, posDependingLastCall.y + offsetToBeInLineWithText };

		const glm::vec2 textSize = m_ActiveFont->CalcTextSize( m_Style.CurrentFontSize, rLabel );

		const glm::vec2 squareSize = glm::vec2( textSize.y );

		const AluraRect bb( offsetPosition,
			offsetPosition + glm::vec2( squareSize.x + ( textSize.x > 0.0f ? m_Style.ItemInnerSpacing.x + textSize.x : 0.0f ), textSize.y ) );

		ItemSize( bb.GetSize() );
		if( !CanAddItem( bb ) )
			return false;

		const auto min = glm::vec2{ offsetPosition.x + textSize.x + m_Style.ItemInnerSpacing.x, offsetPosition.y };
		const AluraRect checkBoxBB( min, min + squareSize );

		bool hovered = false;
		const bool pressed = ButtonBehaviour( checkBoxBB, FNV1A64( rLabel.c_str() ), &hovered, nullptr );

		if( pressed )
		{
			*pValue ^= 1;
		}

		const glm::vec4 checkBoxColor = hovered ? m_Style.Colors[ AluraColor_ButtonHovered ] : m_Style.Colors[ AluraColor_Button ];

#if defined(SAT_ALURA_SHOW_TEXT_BB)
		m_Renderer->SubmitRect( bb, glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f } );
#endif

		const glm::vec2 textPos = { posDependingLastCall.x, posDependingLastCall.y };
		m_Renderer->SubmitString( rLabel, m_ActiveFont, m_Style.CurrentFontSize, textPos, m_Style.Colors[ AluraColor_Text ] );

		m_Renderer->SubmitRect( checkBoxBB, checkBoxColor );
		m_Renderer->SubmitRectFrame( checkBoxBB, 1.0f, m_Style.Colors[ AluraColor_Border ] );

		if( *pValue )
		{
			const float pad = glm::max( 1.0f, glm::trunc( squareSize.x / 6.0f ) );
			m_Renderer->SubmitCheckMark( checkBoxBB.Min + pad, glm::one<glm::vec4>(), squareSize.x - pad * 2.0f );
		}

		return false;
	}

	bool AluraCanvas::AddPopup( const std::string& rLabel )
	{
		// Handle NextItemPosition
		// NB: Popups open from where the mouse is.
		glm::vec2 posDependingLastCall = m_MousePosition;

		if( m_WantToSetItemPosition )
		{
			posDependingLastCall = m_PendingNextItemPosition;
			m_WantToSetItemPosition = false;
		}

		const std::string popupName = rLabel + "AlrPopup";
		const auto itemID = FNV1A64( rLabel.c_str() );
		
		auto& rPopupData = GetOrCreatePopup( itemID );
		if( rPopupData.JustCreated )
		{
			rPopupData.PopupName = popupName;
			rPopupData.OpeningPosition = m_MousePosition;
		}

		SetNextItemPosition( posDependingLastCall );
		bool visible = BeginRegion( rLabel, rPopupData.AlreadyMeasured ? rPopupData.MeasuredSize : glm::vec2{ 1.0f, 1.0f } );
		if( visible )
		{
			rPopupData.pRegionData = m_ActiveRegions.top();
			m_OpenPopups.push( &rPopupData );
		}

		return visible;
	}

	void AluraCanvas::CloseCurrentPopup()
	{
		SAT_CORE_ASSERT( m_OpenPopups.size(), "Alura: CloseCurrentPopup needs to be called inside of an active popup, call AddPopup before calling this. (m_OpenPopups is empty)" );

		auto* pPopup = m_OpenPopups.top();
		pPopup->Closed = true;
	}

	void AluraCanvas::EndPopup()
	{
		SAT_CORE_ASSERT( m_OpenPopups.size(), "Alura: EndPopup needs to be called inside of an active popup, call AddPopup before calling this. (m_OpenPopups is empty)" );

		// End inner region.
		EndRegion();

		auto* pPopup = m_OpenPopups.top();

		if( pPopup->NeedsMeasured && !pPopup->AlreadyMeasured )
		{
			pPopup->MeasuredSize = pPopup->pRegionData->PerFrame.ContentSize;
			pPopup->NeedsMeasured = false;
			pPopup->AlreadyMeasured = true;
		}

		if( pPopup->Closed )
		{
			const auto targetID = pPopup->ID;
			std::erase_if( m_Popups,
				[ targetID ]( const auto& rCandidate ) -> bool
			{
				return rCandidate.ID == targetID;
			} );

			pPopup = nullptr;
		}
	
		m_OpenPopups.pop();
	}

	void AluraCanvas::AddSeparator()
	{
		SAT_CORE_ASSERT( m_ActiveRegions.size(), "Alura: AddSeparator needs to be called inside of an active region, call BeginRegion before calling this. (m_ActiveRegions is empty)" );

		const auto* pRegion = m_ActiveRegions.top();

		const glm::vec2 size( GetContentRegionAvail().x, 1.0f );
		const AluraRect bb( m_Layout.CursorPos, m_Layout.CursorPos + size );
		
		ItemSize( bb.GetSize() );
		if( !CanAddItem( bb ) )
			return;

		m_Renderer->SubmitRect( bb, m_Style.Colors[ AluraColor_Separator ] );
	}

	bool AluraCanvas::BeginRegion( const std::string& rID, const glm::vec2& rBounds )
	{
		// Handle NextItemPosition
		glm::vec2 posDependingLastCall = m_Layout.CursorPos;

		if( m_WantToSetItemPosition )
		{
			posDependingLastCall = m_PendingNextItemPosition;
			m_WantToSetItemPosition = false;
		}

		const AluraRect bb( posDependingLastCall, posDependingLastCall + rBounds );
		if( !CanAddItem( bb ) )
			return false;

		const uint64_t itemID = FNV1A64( rID.c_str() );

		m_Renderer->SubmitRect( bb, m_Style.Colors[ AluraColor_RegionBackground ] );
		m_Renderer->SubmitRectFrame( bb, 1.0f, m_Style.Colors[ AluraColor_Border ] );

		AluraRegionData& rData = GetOrCreateRegion( itemID );
		if( rData.JustCreated )
		{
			rData.ParentID = m_ActiveRegions.empty() ? 0llu : m_ActiveRegions.top()->ID;
		}
	
		// Set up per-frame data.
		rData.Size = rBounds;
		rData.Rect = bb;
		rData.InnerRect = bb;

		// Round up cursor position to nearest pixel.
		rData.PerFrame.StartingPosition = glm::ceil( posDependingLastCall );

		// Initial working rect is the full size of the rect, because nothing has been drawn. 
		rData.PerFrame.WorkingRect = bb;
		rData.PerFrame.ClippingRect = bb;
		rData.PerFrame.ContentSize = glm::zero<glm::vec2>();

		if( m_ActiveRegions.size() )
		{
			const auto* pParent = m_ActiveRegions.top();

			// Take the smallest clipping rect to make sure that a sub-region doesn't
			// extend over the parent.
			rData.PerFrame.ClippingRect.Min = glm::max(
				rData.PerFrame.ClippingRect.Min,
				pParent->PerFrame.ClippingRect.Min
			);

			rData.PerFrame.ClippingRect.Max = glm::min(
				rData.PerFrame.ClippingRect.Max,
				pParent->PerFrame.ClippingRect.Max
			);
		}

		m_Renderer->PushClipRect( rData.PerFrame.ClippingRect );

#if defined( SAT_ALURA_SHOW_TEXT_BB )
		m_Renderer->SubmitRectFrame( rData.PerFrame.ClippingRect, 1.0f, glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f } );
#endif

		rData.PerFrame.CanScroll = false;

		// Shrink the initial working rect to not include the padding.
		rData.PerFrame.WorkingRect.ShrinkX( m_Style.WindowPadding.x );
		rData.PerFrame.WorkingRect.ShrinkY( m_Style.WindowPadding.y );

		// LAYOUT
		m_Layout.CursorPos = posDependingLastCall;
		m_Layout.CursorPos += m_Style.WindowPadding;
		m_Layout.CursorPos.y -= rData.Scroll.y;

		// Set active.
		m_ActiveRegions.push( &rData );

		// Mouse testing.
		if( bb.Contains( m_MousePosition ) )
		{
			m_Hot = m_ID;
		}

		return true;
	}

	void AluraCanvas::EndRegion()
	{
		SAT_CORE_ASSERT( m_ActiveRegions.size(), "Alura: Forgot to call BeginRegion or called EndRegion too many times. (m_ActiveRegions is empty!)" );

		auto* pRegion = m_ActiveRegions.top();
		ClampRegionScroll( *pRegion );

		// Go back to the start so when we do ItemSize,
		// it will just "work".
		m_Layout.CursorPos = pRegion->PerFrame.StartingPosition;
		m_Layout.CurrentIndent -= m_Style.ItemInnerSpacing.x;

		m_Renderer->PopClipRect();

		// Push this item.
		ItemSize( pRegion->Size );

		// Remove from active.
		m_ActiveRegions.pop();
	}

	void AluraCanvas::AddDummy( const glm::vec2& rSize )
	{
		// Handle NextItemPosition
		glm::vec2 posDependingLastCall = m_Layout.CursorPos;

		if( m_WantToSetItemPosition )
		{
			posDependingLastCall = m_PendingNextItemPosition;
			m_WantToSetItemPosition = false;
		}

		const AluraRect bb( posDependingLastCall, posDependingLastCall + rSize );
		ItemSize( bb.GetSize() );
		if( !CanAddItem( bb ) )
			return;
	}

	void AluraCanvas::DrawDemo()
	{
		PushFontAndSetActive( m_EditorFont );
		PushFontSize( 32.0f );

		if( BeginRegion( "##testing", { 250.0f, 250.0f } ) )
		{
			AddSeparator();

			PushStyle( AluraColor_FrameBackground, { 1.0f, 1.0f, 1.0f, 1.0f } );
			if( BeginRegion( "##testing1", { 250.0f / 2.0f, 250.0f / 2.0f } ) )
			{
				bool clicked = AddButton( "1" );
				clicked = AddButton( "2" );
				clicked = AddButton( "3" );
				clicked = AddButton( "4" );
				clicked = AddButton( "5" );
				clicked = AddButton( "6" );
				clicked = AddButton( "7" );
				clicked = AddButton( "8" );
				clicked = AddButton( "9" );

				EndRegion();
			}
			PopStyle();
			EndRegion();
		}

		AddText( "This is text at size 32px" );
		PopFontSize();

		PushFontSize( 16.0f );
		AddText( "This is text at size 16px" );
		PopFontSize();

		PushFontSize( 12.0f );
		AddText( "This is text at size 12px" );
		PopFontSize();

		constexpr float myFloat = 21.1234567f;
		constexpr uint64_t myUInt = 21lu;
		constexpr int64_t mySInt = -21;

		PushFontSize( 32.0f );
		TextFormatted( "This is formatted floating point text, value: {}", myFloat );
		TextFormatted( "This is formatted floating point text w. 2 dps, value: {:.2f}", myFloat );
		TextFormatted( "This is formatted uint text, value: {}", myUInt );
		TextFormatted( "This is formatted sint text, value: {}", mySInt );

		AddText( "Long Text:\nUhm im testing this shit okay...\npenis" );

		bool clicked = AddButton( "Yep" );

		AddImage( { 24.0f, 24.0f }, Renderer::Get()->GetPinkTexture() );

		static bool test = false;
		clicked = AddCheckbox( "Testing checkbox", &test );
		clicked = AddCheckboxRight( "Testing checkbox RHS", &test );

		TextFormatted( "Tests: {}", test );
		TextFormatted( "Hot: {}", m_Hot );
		TextFormatted( "Active: {}", m_Active );
		TextFormatted( "Focused: {}", m_Focused );
		TextFormatted( "Selected: {}", m_Selected );

		PopFontSize();
		PopFont();
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
		const glm::vec2 position = { ( m_CanvasSize.x - rSize.x ) * 0.5f, ( m_CanvasSize.y - rSize.y ) * 0.5f };
		SetNextItemPosition( position );
	}
	
	void AluraCanvas::AlignNextItemCenterX( const glm::vec2& rSize )
	{
		const glm::vec2 position = { ( m_CanvasSize.x - rSize.x ) * 0.5f, m_Layout.CursorPos.y };
		SetNextItemPosition( position );
	}

	void AluraCanvas::NudgeNextItemPosition( const glm::vec2& rOffset, bool addItemSpacing )
	{
		m_WantToSetItemPosition = true;
		m_PendingNextItemPosition += rOffset;

		if( addItemSpacing )
			m_PendingNextItemPosition += glm::vec2{ 0.0f, m_Style.ItemSpacing.y };
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
		SAT_CORE_ASSERT( m_ColorStack.size(), "Alura: Forgot to call PushStyle or PopStyle called too many times (m_ColorStack is empty)!" );

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

	float AluraCanvas::GetFrameHeight() const
	{
		return m_Style.CurrentFontSize + m_Style.WindowPadding.y * 2.0f;
	}

	float AluraCanvas::GetRegionMaxScroll( const AluraRegionData& rData ) const
	{
		const float visibleHeight = rData.Rect.GetHeight();
		const float contentHeight = rData.PerFrame.ContentSize.y + m_Style.ItemInnerSpacing.y * 2.0f;

		return glm::max( 0.0f, contentHeight - visibleHeight );
	}

	glm::vec2 AluraCanvas::GetContentRegionAvail()
	{
		SAT_CORE_ASSERT( m_ActiveRegions.size(), "Alura: GetContentRegionAvail needs to be called inside of an active region, call BeginRegion before calling this. (m_ActiveRegions is empty)" );

		const auto* pRegion = m_ActiveRegions.top();

		return pRegion->PerFrame.WorkingRect.GetSize();
	}

	glm::vec2 AluraCanvas::CalcTextSize( const std::string& rText )
	{
		SAT_CORE_ASSERT( m_ActiveFont );
		return m_ActiveFont->CalcTextSize( m_Style.CurrentFontSize, rText );
	}

#if !defined(SAT_DIST)
	void AluraCanvas::EdClearCanvas()
	{
		m_Layout.Reset();
		m_Renderer->EdClearCommands();
	}
#endif

	void AluraCanvas::UpdateMouseInputState( const RubyMouseButton btn, const AluraInputState state )
	{
		m_MouseInputStates[ btn ] = state;
	}

	void AluraCanvas::UpdateKeyInputState( const RubyKey btn, const AluraInputState state )
	{
		m_KeyInputStates[ btn ] = state;
	}

	void AluraCanvas::UpdateMouseScroll( const glm::vec2& rScrollOffset )
	{
		// Must use m_Regions here, as no matter what there are
		// not active regions at this point,
		// polling events happens way before Alura even begins a frame.
		for( auto itr = m_Regions.rbegin(); itr != m_Regions.rend(); ++itr )
		{
			auto& rRegion = *itr;

			if( !rRegion.Rect.Contains( m_MousePosition ) )
				continue;

			if( !rRegion.PerFrame.CanScroll )
				continue;

			if( rScrollOffset.x != 0.0f )
			{
			}

			if( rScrollOffset.y != 0.0f )
			{
				const float scrollStep = glm::max( 1.0f, m_Style.CurrentFontSize );
				rRegion.Scroll.y -= rScrollOffset.y * scrollStep;
				ClampRegionScroll( rRegion );
			}

			break;
		}
	}

	void AluraCanvas::ItemSize( const glm::vec2& rSize, float textBaselineY )
	{
		//
		// TODO: Y layout only!
		// Meaning we move down a "line" every time we advance the cursor.
		//
		// Adding support for X layout would be very simple, we'd just need to check if out current layout type is horizontal and if so move along the X coord instead of the Y
		//
		const float offsetInlineWithBaselineY = ( textBaselineY >= 0.0f ) ? glm::max( 0.0f, m_Layout.CurrLineTextBaseOffset - textBaselineY ) : 0.0f;
		const float lineY = m_Layout.IsSameLine ? m_Layout.CursorPosPrevLine.y : m_Layout.CursorPos.y;
		const float lineHeight = glm::max( m_Layout.CurrLineSize.y, m_Layout.CursorPos.y - lineY + rSize.y + offsetInlineWithBaselineY );

#if defined(SAT_ALURA_SHOW_TEXT_BB)
		m_Renderer->SubmitRect( m_Layout.CursorPos, { m_Layout.CursorPos + 10.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } );
#endif

		m_Layout.CursorPosPrevLine.x = m_Layout.CursorPos.x + rSize.x;
		m_Layout.CursorPosPrevLine.y = lineY;
		m_Layout.CursorPos.x = glm::trunc( m_Layout.CurrentIndent );
		m_Layout.CursorPos.y = glm::trunc( lineY + lineHeight + m_Style.ItemSpacing.y );
		m_Layout.IsSameLine = false;

#if defined(SAT_ALURA_SHOW_TEXT_BB)
		m_Renderer->SubmitRect( m_Layout.CursorPos, { m_Layout.CursorPos + 10.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } );
#endif

		// TODO: What if rSize is bigger than the object size?
		// TODO: Do not remove working rect from self
		//		 "self" meaning the region itself because the region will
		//		 submit itself to the layout system.
		if( !m_ActiveRegions.empty() )
		{
			auto* pRegion = m_ActiveRegions.top();

			m_Layout.CursorPos.x += glm::trunc( pRegion->PerFrame.StartingPosition.x );

			const float consumedX = rSize.x + ( m_Style.ItemSpacing.x * 2.0f );
			const float consumedY = rSize.y + m_Style.ItemSpacing.y;

			pRegion->PerFrame.ContentSize.x = glm::max( pRegion->PerFrame.ContentSize.x, consumedX );
			pRegion->PerFrame.ContentSize.y += consumedY;
			
			// Reduce working rect.
			pRegion->PerFrame.WorkingRect.Min.y += consumedY;

			// Element was too big...
			if( pRegion->PerFrame.WorkingRect.Min.y > pRegion->PerFrame.WorkingRect.Max.y )
			{
				pRegion->PerFrame.WorkingRect.Min.y = pRegion->PerFrame.WorkingRect.Max.y;
			}
		}
	}
	
	bool AluraCanvas::CanAddItem( const AluraRect& rBoundingBox )
	{
		if( !m_ActiveRegions.empty() )
		{
			auto* pRegion = m_ActiveRegions.top();
			const auto workingRectSize = pRegion->PerFrame.WorkingRect.GetSize();
			const auto itemSize = rBoundingBox.GetSize();

			if( !rBoundingBox.Overlaps( pRegion->PerFrame.ClippingRect ) )
			{
				pRegion->PerFrame.CanScroll = true;
				return false;
			}

			/*
			if( itemSize.x > workingRectSize.x || itemSize.y > workingRectSize.y )
			{
				pRegion->PerFrame.CanScroll = true;
				return false;
			}
			*/
		}

		// TODO: Support for CanAddItem when drawing on the directly on the viewport.

		return true;
	}

	bool AluraCanvas::IsMouseHoveringRect( const glm::vec2& rMin, const glm::vec2& rMax ) const
	{
		AluraRect rect( rMin, rMax );
		return rect.Contains( m_MousePosition );
	}

	bool AluraCanvas::IsMouseHoveringRect( const AluraRect& rRect ) const
	{
		return rRect.Contains( m_MousePosition );
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

	bool AluraCanvas::ButtonBehaviour( const AluraRect& rRect, uint64_t id, bool* pOutHovered, bool* pOutHeld )
	{
		bool clicked = false, hovered = false, held = false;
		hovered = IsMouseHoveringRect( rRect );
	
		if( hovered )
		{
			m_Hot = id;
			clicked = MouseButtonPressed( RubyMouseButton_Left );
		}

		if( clicked )
		{
			m_Active = id;
		}
		
		if( m_Active == id && MouseButtonReleased( RubyMouseButton_Left ) )
		{
			m_Active = 0llu;
		}

		if( pOutHovered ) *pOutHovered = hovered;
		if( pOutHeld )	  *pOutHeld = held;

		return clicked;
	}

	void AluraCanvas::ResetInputStates()
	{
		for( size_t i = 0llu; i < m_MouseInputStates.size(); ++i )
		{
			// Any mouse buttons that were pressed are not released.
			// Ruby doesn't keep track of mouse buttons being held yet...
			if( m_MouseInputStates[ i ] == AluraInputState::Pressed )
			{
				m_MouseInputStates[ i ] = AluraInputState::Released;
			}

			// Any keys that were released are now cleared to no-state.
			if( m_MouseInputStates[ i ] == AluraInputState::Released )
			{
				m_MouseInputStates[ i ] = AluraInputState::NoState;
			}
		}

		for( size_t i = 0llu; i < m_KeyInputStates.size(); ++i )
		{
			// Any keys that were pressed at the start of the frame will now be
			// set to release, the AluraLayer will also send this event,
			// if the key is still physically down AluraLayer will send the Held event.
			if( m_KeyInputStates[ i ] == AluraInputState::Pressed )
			{
				m_KeyInputStates[ i ] = AluraInputState::Released;
			}

			// Any keys that were released are now cleared to no-state.
			if( m_KeyInputStates[ i ] == AluraInputState::Released )
			{
				m_KeyInputStates[ i ] = AluraInputState::NoState;
			}
		}
	}

	void AluraCanvas::ClampRegionScroll( AluraRegionData& rData )
	{
		const float maxScroll = GetRegionMaxScroll( rData );

		rData.Scroll.y = glm::clamp( rData.Scroll.y, 0.0f, maxScroll );

		rData.PerFrame.CanScroll = maxScroll > 0.0f;
	}

	bool AluraCanvas::MouseButtonPressed( RubyMouseButton btn )
	{
		return m_MouseInputStates[ btn ] == AluraInputState::Pressed;
	}

	bool AluraCanvas::MouseButtonReleased( RubyMouseButton btn )
	{
		return m_MouseInputStates[ btn ] == AluraInputState::Released;
	}

	bool AluraCanvas::KeyPressed( RubyMouseButton btn )
	{
		return m_KeyInputStates[ btn ] == AluraInputState::Pressed;
	}

	bool AluraCanvas::KeyReleased( RubyMouseButton btn )
	{
		return m_KeyInputStates[ btn ] == AluraInputState::Released;
	}

	bool AluraCanvas::KeyHeld( RubyMouseButton btn )
	{
		return m_KeyInputStates[ btn ] == AluraInputState::Held;
	}

	AluraRegionData& AluraCanvas::GetOrCreateRegion( uint64_t itemID )
	{
		const auto itr = std::find_if( m_Regions.begin(), m_Regions.end(), 
			[ itemID ](const auto& rCandidate)
		{
			return rCandidate.ID == itemID;
		} );

		if( itr != m_Regions.end() )
		{
			AluraRegionData& rRegion = *itr;
			rRegion.JustCreated = false;
			return *itr;
		}
		else
		{
			AluraRegionData& rNewRegion = m_Regions.emplace_back();
			rNewRegion.ID = itemID;
			rNewRegion.JustCreated = true;
			return rNewRegion;
		}
	}

	AluraPopupData& AluraCanvas::GetOrCreatePopup( uint64_t itemID )
	{
		const auto itr = std::find_if( m_Popups.begin(), m_Popups.end(),
			[ itemID ]( const auto& rCandidate )
		{
			return rCandidate.ID == itemID;
		} );

		if( itr != m_Popups.end() )
		{
			AluraPopupData& rPopup = *itr;
			rPopup.JustCreated = false;
			return *itr;
		}
		else
		{
			AluraPopupData& rNewPopup = m_Popups.emplace_back();
			rNewPopup.ID = itemID;
			rNewPopup.JustCreated = true;
			return rNewPopup;
		}
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
