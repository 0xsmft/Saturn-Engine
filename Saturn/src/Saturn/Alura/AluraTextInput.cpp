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
	
		if( ( m_Specification.Flags & AluraTextInputFlags_CharsToLower ) && 
			( m_Specification.Flags & AluraTextInputFlags_CharsToUpper ) )
		{
			SAT_CORE_ASSERT( false, "[AluraTextInputA]: Invalid flag combination, cannot have AluraTextInputFlags_CharsToLower and AluraTextInputFlags_CharsToUpper set at the same time!" );
		}

		SAT_CORE_ASSERT( m_Specification.pString, "No string." );
	}

	void AluraTextInputA::Reset()
	{
		m_Specification = {};
		m_CursorIndex = m_SelectionBegin = m_SelectionEnd = m_SelectionAnchor = std::string::npos;
		m_EnterPressed = m_ModifiedSinceLastRender = m_IsSelecting = m_ShiftDown = m_CursorFollow = false;
		m_TextDeletionDirection = AluraTextDeletionDirection::Backwards;
		m_CursorBlinkingTime = 0.0f;
		m_ScrollX = 0.0f;
	}

	bool AluraTextInputA::FilterCharacter( uint32_t wc )
	{
		// Skip non-printable characters.
		if( wc < 32 )
			return false;

		// If outside of ASCII range and we aren't unicode, we reject.
		if( !m_Specification.AcceptUnicode && wc > 0x7f )
			return false;

		bool shouldInsert = true;

		// Allow only 0 to 9 and "."
		if( ( m_Specification.Flags & AluraTextInputFlags_NumbersOnly ) )
		{
			if( ( wc >= '0' && wc <= '9' ) || wc == '.' )
				shouldInsert = true;
			else
				shouldInsert = false;
		}

		if( ( m_Specification.Flags & AluraTextInputFlags_NoSpacesOrTabs ) )
		{
			if( wc == ' ' || wc == '\t' )
				shouldInsert = false;
		}

		return shouldInsert;
	}

	void AluraTextInputA::OnCharacter( uint32_t wc )
	{
		if( !FilterCharacter( wc ) )
			return;

		if( ( m_Specification.Flags & AluraTextInputFlags_CharsToLower ) )
		{
			wc = ( uint32_t ) std::tolower( wc );
		}
		if( ( m_Specification.Flags & AluraTextInputFlags_CharsToUpper ) )
		{
			wc = ( uint32_t ) std::toupper( wc );
		}

		m_Specification.pString->insert( m_Specification.pString->size(), 1, wc );
		++m_CursorIndex;
		m_ModifiedSinceLastRender = true;
		m_CursorFollow = true;

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

			case RubyKey_Home:
			{
				ResetSelection();
				ResetCursorTime();

				m_CursorIndex = 0llu;
			} break;

			case RubyKey_End: 
			{
				ResetSelection();
				ResetCursorTime();

				m_CursorIndex = m_Specification.pString->size();
			} break;

			case RubyKey_LeftShift:
			case RubyKey_RightShift:
			{
				if( m_SelectionBegin == std::string::npos )
				{
					m_SelectionAnchor = m_CursorIndex;
					m_SelectionBegin = m_CursorIndex;
					m_SelectionEnd = m_SelectionBegin;
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
					m_SelectionAnchor = 0llu;
					m_SelectionBegin = 0llu;
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
			m_CursorFollow = true;

			// Handle selection...
			if( m_IsSelecting )
			{
				// Continue selecting...
				if( m_ShiftDown )
				{
					--m_CursorIndex;

					m_SelectionBegin = std::min( m_SelectionAnchor, m_CursorIndex );
					m_SelectionEnd = std::max( m_SelectionAnchor, m_CursorIndex );
					
					ResetCursorTime();
					return;
				}
				else
				{
					m_CursorIndex = m_SelectionBegin;
					ResetSelection();
					ResetCursorTime();
					
					return;
				}
			}
			
			// No selection so we just move back.
			--m_CursorIndex;
			ResetCursorTime();
		}
		// We are at the start but we attempt to move left
		// we now end the selection.
		else if( m_IsSelecting && !m_ShiftDown )
		{
			ResetSelection();
			ResetCursorTime();
		}
	}

	void AluraTextInputA::MoveRight()
	{
		if( m_CursorIndex < m_Specification.pString->size() )
		{
			m_CursorFollow = true;

			if( m_IsSelecting )
			{
				if( m_ShiftDown )
				{
					++m_CursorIndex;
					ResetCursorTime();

					m_SelectionBegin = std::min( m_SelectionAnchor, m_CursorIndex );
					m_SelectionEnd = std::max( m_SelectionAnchor, m_CursorIndex );

					return;
				}
				else
				{
					m_CursorIndex = m_SelectionEnd;
					ResetSelection();
					ResetCursorTime();
					
					return;
				}
			}

			++m_CursorIndex;
			ResetCursorTime();
		}
		// We are at the end but we attempt to move right
		// we now end the selection.
		else if( m_IsSelecting && !m_ShiftDown )
		{
			ResetSelection();
			ResetCursorTime();
		}
	}

	void AluraTextInputA::EraseAtCursorOrSelection()
	{
		if( m_Specification.pString->empty() || m_CursorIndex == std::string::npos )
			return;

		if( m_IsSelecting )
		{
			const auto deletionAmount = m_SelectionEnd - m_SelectionBegin;

			m_Specification.pString->erase( m_SelectionBegin, deletionAmount );
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
						m_Specification.pString->erase( m_CursorIndex, 1llu );
					}
				} break;
				
				case AluraTextDeletionDirection::Backwards: 
				{
					if( m_CursorIndex > 0 )
					{
						m_Specification.pString->erase( --m_CursorIndex, 1llu );
					}
				} break;
		
				default:
					break;
			}

			m_CursorFollow = true;
		}

		ResetCursorTime();
	}

	void AluraTextInputA::ResetSelection()
	{
		m_SelectionBegin = std::string::npos;
		m_SelectionEnd = std::string::npos;
		m_SelectionAnchor = std::string::npos;
		m_IsSelecting = false;
	}

}
