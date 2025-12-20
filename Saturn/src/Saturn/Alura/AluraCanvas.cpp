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

	void AluraCanvas::Draw( Timestep ts )
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

		m_Elements.clear();
	}

	void AluraCanvas::Destory()
	{
	}

	void AluraCanvas::SetContext( Ref<AluraRenderer> context )
	{
		m_Renderer = context;
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
