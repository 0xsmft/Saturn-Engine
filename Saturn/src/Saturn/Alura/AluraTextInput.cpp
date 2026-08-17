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
#include "AluraTextInput.h"

namespace Saturn {

	AluraTextInputA::AluraTextInputA()
	{
	}

	AluraTextInputA::~AluraTextInputA()
	{
	}

	void AluraTextInputA::Init( const AluraTextInputSpecification& rSpecification )
	{
		m_Specification = rSpecification;
		m_CursorIndex = 0llu;
		m_CursorBlinkingTime = m_Specification.CursorAnimDuration;

		SAT_CORE_ASSERT( m_Specification.pString, "No string." );
	}

	void AluraTextInputA::OnCharacter( uint32_t wc )
	{
		if( wc < 32 )
			return;

		if( !m_Specification.AcceptUnicode && wc > 0x7f )
			return;

		m_Specification.pString->insert( m_Specification.pString->size(), 1, wc );
		++m_CursorIndex;
		m_ModifiedSinceLastRender = true;
		
		ResetCursorTime();
	}

	void AluraTextInputA::OnKeyPressed( RubyKey key )
	{
		switch( key )
		{
			case RubyKey_Backspace:
			{
				EraseAtCursor();
			} break;

			case RubyKey_LeftArrow:
			{
				MoveLeft();
			} break;

			case RubyKey_RightArrow:
			{
				MoveRight();
			} break;

			case RubyKey_Enter:
			{
				m_EnterPressed = true;
			} break;

			default:
				break;
		}
	}

	void AluraTextInputA::DeleteAll()
	{
		m_Specification.pString->clear();
	}

	void AluraTextInputA::MoveLeft()
	{
		if( m_CursorIndex > 0 ) 
		{
			--m_CursorIndex;
			
			ResetCursorTime();
		}
	}

	void AluraTextInputA::MoveRight()
	{
		if( m_CursorIndex < m_Specification.pString->size() )
		{
			++m_CursorIndex;
			ResetCursorTime();
		}
	}

	void AluraTextInputA::EraseAtCursor()
	{
		if( m_Specification.pString->empty() || m_CursorIndex == std::string::npos )
			return;

		m_Specification.pString->erase( m_CursorIndex );
		--m_CursorIndex;

		ResetCursorTime();
	}

}
