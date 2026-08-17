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

#include "Saturn/Core/Input.h"

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
				m_TextDeletionDirection = AluraTextDeletionDirection::Backwards;
				EraseAtCursorOrSelection();
			} break;

			case RubyKey_Delete:
			{
				m_TextDeletionDirection = AluraTextDeletionDirection::Forwards;
				EraseAtCursorOrSelection();
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

			case RubyKey_LeftShift:
			case RubyKey_RightShift:
			{
				if( m_SelectionBegin == std::string::npos )
				{
					m_SelectionBegin = m_CursorIndex;
					m_SelectionEnd = 0;
					m_IsSelecting = true;
				}

				m_ShiftDown = true;
			} break;

			default:
				break;
		}

		if( Input::Get().KeyPressed( RubyKey_LeftCtrl ) || Input::Get().KeyPressed( RubyKey_RightCtrl ) )
		{
			switch( key )
			{
				case RubyKey_A:
				{
					m_IsSelecting = true;
					m_SelectionBegin = 0;
					m_SelectionEnd = m_Specification.pString->size();
				} break;

				default:
					break;
			}
		}
	}

	void AluraTextInputA::OnKeyReleased( RubyKey key )
	{
		switch( key )
		{
			case RubyKey_LeftShift:
			case RubyKey_RightShift:
			{
				m_ShiftDown = false;
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
			if( m_IsSelecting )
			{
				if( m_ShiftDown )
				{
					--m_SelectionEnd;
				}
				else
				{
					m_CursorIndex = m_SelectionBegin;
					ResetSelection();
					ResetCursorTime();
					
					return;
				}
			}
			
			--m_CursorIndex;
			ResetCursorTime();
		}
	}

	void AluraTextInputA::MoveRight()
	{
		if( m_CursorIndex < m_Specification.pString->size() )
		{
			if( m_IsSelecting )
			{
				if( m_ShiftDown )
				{
					++m_SelectionEnd;
				}
				else
				{
					m_CursorIndex = m_SelectionEnd;
					ResetSelection();
				}
			}

			++m_CursorIndex;
			ResetCursorTime();
		}
	}

	void AluraTextInputA::EraseAtCursorOrSelection()
	{
		if( m_Specification.pString->empty() || m_CursorIndex == std::string::npos )
			return;

		if( m_IsSelecting )
		{
			const auto deletionAmount = std::max( m_SelectionBegin, m_SelectionEnd );

			m_Specification.pString->erase( m_Specification.pString->size() - deletionAmount );

			m_CursorIndex = m_Specification.pString->size();

			ResetSelection();
		}
		else
		{
			switch( m_TextDeletionDirection )
			{
				case AluraTextDeletionDirection::Forwards:
				{
					if( m_CursorIndex + 1 <= m_Specification.pString->size() )
					{
						m_Specification.pString->erase( m_CursorIndex );
					}
				} break;
				
				case AluraTextDeletionDirection::Backwards: 
				{
					if( m_CursorIndex > 0 )
					{
						m_Specification.pString->erase( --m_CursorIndex );
					}
				} break;
		
				default:
					break;
			}
		}

		ResetCursorTime();
	}

	void AluraTextInputA::ResetSelection()
	{
		m_SelectionBegin = std::string::npos;
		m_SelectionEnd = std::string::npos;
		m_IsSelecting = false;
	}

}
