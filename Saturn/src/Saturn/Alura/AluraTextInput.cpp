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

#include "Saturn/Core/App.h"
#include "Saturn/Core/Ruby/RubyWindow.h"

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
		m_OffsetFromStart = 0llu;
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
		m_OffsetFromStart = m_SelectionBegin = m_SelectionEnd = m_SelectionAnchor = std::string::npos;
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

	void AluraTextInputA::TryInsertCharacterAtInsertionPoint( uint32_t wc, size_t insertionPoint )
	{
		if( m_Specification.pString->size() + 1 > m_Specification.MaxCharacters )
			return;

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

		// Insert the fucking character right here.
		m_Specification.pString->insert( insertionPoint, 1, wc );

		++m_OffsetFromStart;
		m_CursorFollow = true;

		m_ModifiedSinceLastRender = true;
		ResetCursorTime();
	}

	void AluraTextInputA::OnCharacter( uint32_t wc )
	{
		TryInsertCharacterAtInsertionPoint( wc, m_Specification.pString->size() );
	}

	std::string AluraTextInputA::GetTextBetweenSelection()
	{
		return m_Specification.pString->substr( m_SelectionBegin, m_SelectionBegin - m_SelectionEnd );
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

				m_OffsetFromStart = 0llu;
			} break;

			case RubyKey_End: 
			{
				ResetSelection();
				ResetCursorTime();

				m_OffsetFromStart = m_Specification.pString->size();
			} break;

			case RubyKey_LeftShift:
			case RubyKey_RightShift:
			{
				if( m_SelectionBegin == std::string::npos )
				{
					m_SelectionAnchor = m_OffsetFromStart;
					m_SelectionBegin = m_OffsetFromStart;
					m_SelectionEnd = m_SelectionBegin;
					m_IsSelecting = true;
				}

				m_ShiftDown = true;
			} break;

			case RubyKey_LeftCtrl:
			case RubyKey_RightCtrl:
			{
				m_ControlDown = true;
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

				case RubyKey_C:
				{
					if( m_IsSelecting )
					{
						const auto str = GetTextBetweenSelection();
						Application::Get()->GetWindow()->SetClipboardText( str );
					}
				} break;

				case RubyKey_V:
				{
					const auto clipboardText = Application::Get()->GetWindow()->GetClipboardText();

					// Insert string at cursor position.
					CopyPaste_InsertBulk( clipboardText, m_OffsetFromStart );
				} break;

				case RubyKey_X:
				{
					if( m_IsSelecting )
					{
						const auto str = GetTextBetweenSelection();
						Application::Get()->GetWindow()->SetClipboardText( str );
					
						// Erase
						EraseAtCursorOrSelection();
					}
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

			case RubyKey_LeftCtrl:
			case RubyKey_RightCtrl:
			{
				m_ControlDown = false;
			} break;

			case RubyKey_Enter:
			{
				m_EnterPressed = false;
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
		if( m_OffsetFromStart > 0 ) 
		{
			m_CursorFollow = true;

			// Move to the next word before the space.
			if( m_ControlDown )
			{
				--m_OffsetFromStart;
				ResetCursorTime();

				const auto spacePos = m_Specification.pString->find_last_of( ' ', m_OffsetFromStart );
				if( spacePos == std::string::npos )
				{
					m_OffsetFromStart = 0llu;
				}
				else
				{
					m_OffsetFromStart = spacePos + 1;
				}

				// Make sure we don't go beyond the string range if we are selecting,
				// so we'll just set the selection like so.
				if( m_OffsetFromStart == 0 && m_IsSelecting )
				{
					m_SelectionBegin = std::min( m_SelectionAnchor, m_OffsetFromStart );
					m_SelectionEnd = std::max( m_SelectionAnchor, m_OffsetFromStart );
					return;
				}
			}

			// Handle selection...
			if( m_IsSelecting )
			{
				// Continue selecting...
				if( m_ShiftDown )
				{
					--m_OffsetFromStart;

					m_SelectionBegin = std::min( m_SelectionAnchor, m_OffsetFromStart );
					m_SelectionEnd = std::max( m_SelectionAnchor, m_OffsetFromStart );
					
					ResetCursorTime();
					return;
				}
				else
				{
					m_OffsetFromStart = m_SelectionBegin;
					ResetSelection();
					ResetCursorTime();
					
					return;
				}
			}
			
			// No selection so we just move back.
			if( m_OffsetFromStart > 0llu )
			{
				--m_OffsetFromStart;
				ResetCursorTime();
			}
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
		if( m_OffsetFromStart < m_Specification.pString->size() )
		{
			m_CursorFollow = true;

			// Move to the next word after the space.
			if( m_ControlDown )
			{
				auto spacePos = m_Specification.pString->find_first_of( ' ', m_OffsetFromStart );

				if( spacePos == std::string::npos )
				{
					m_OffsetFromStart = m_Specification.pString->empty() ? 0 : m_Specification.pString->size() - 1;
				}
				else
				{
					// Now we found the first space but there may be like 10 spaces between the words
					// so find_first_not_of of space characters.
					spacePos = m_Specification.pString->find_first_not_of( ' ', spacePos );

					if( spacePos == std::string::npos )
						m_OffsetFromStart = m_Specification.pString->empty() ? 0 : m_Specification.pString->size() - 1;
					else
						m_OffsetFromStart = spacePos - 1;
				}
			}

			if( m_IsSelecting )
			{
				if( m_ShiftDown )
				{
					++m_OffsetFromStart;
					ResetCursorTime();

					m_SelectionBegin = std::min( m_SelectionAnchor, m_OffsetFromStart );
					m_SelectionEnd = std::max( m_SelectionAnchor, m_OffsetFromStart );

					return;
				}
				else
				{
					m_OffsetFromStart = m_SelectionEnd;
					ResetSelection();
					ResetCursorTime();
					
					return;
				}
			}

			++m_OffsetFromStart;
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

	bool AluraTextInputA::GetReturnValue()
	{
		if( ( m_Specification.Flags & AluraTextInputFlags_EnterReturnsTrue ) )
		{
			// Release enter, because it may take a few frames for the user to release
			// the enter button and we don't want to report enter held as modified.
			return std::exchange( m_EnterPressed, false );
		}
		else
		{
			return std::exchange( m_ModifiedSinceLastRender, false );
		}
	}

	void AluraTextInputA::EraseAtCursorOrSelection()
	{
		if( m_Specification.pString->empty() || m_OffsetFromStart == std::string::npos )
			return;

		if( m_IsSelecting )
		{
			const auto deletionAmount = m_SelectionEnd - m_SelectionBegin;

			m_Specification.pString->erase( m_SelectionBegin, deletionAmount );
			m_OffsetFromStart = m_Specification.pString->size();

			ResetSelection();
		}
		else
		{
			switch( m_TextDeletionDirection )
			{
				case AluraTextDeletionDirection::Forwards:
				{
					if( m_OffsetFromStart + 1 <= m_Specification.pString->size() )
					{
						m_Specification.pString->erase( m_OffsetFromStart, 1llu );
					}
				} break;
				
				case AluraTextDeletionDirection::Backwards: 
				{
					if( m_OffsetFromStart > 0 )
					{
						m_Specification.pString->erase( --m_OffsetFromStart, 1llu );
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

	void AluraTextInputA::SelectWord()
	{
		if( m_Specification.pString->empty() )
			return;

		const std::string& rString = *m_Specification.pString;

		// If we are on a space, we do nothing.
		if( m_OffsetFromStart < rString.size() && rString[ m_OffsetFromStart ] == ' ' )
			return;

		size_t begin = m_OffsetFromStart;

		while( begin > 0 && rString[ begin - 1 ] != ' ' )
		{
			--begin;
		}
		
		size_t end = m_OffsetFromStart;

		while( end < rString.size() && rString[ end ] != ' ' )
		{
			++end;
		}

		if( begin == end )
			return;

		m_IsSelecting = true;
		m_OffsetFromStart = end;
		m_SelectionBegin = begin;
		m_SelectionAnchor = begin;
		m_SelectionEnd = end;
	}

	void AluraTextInputA::MoveCursorTo( size_t numberOfCharacters )
	{
		if( numberOfCharacters > m_Specification.pString->size() )
			return;

		m_OffsetFromStart = numberOfCharacters;

		ResetSelection();
		ResetCursorTime();
	}

	void AluraTextInputA::CopyPaste_InsertBulk( const std::string& rText, size_t insertionPoint )
	{
		std::string filteredText = rText;

		std::erase_if( filteredText, 
			[this]( auto c ) 
		{
			return !FilterCharacter( ( uint32_t ) c );
		} );

		if( filteredText.empty() )
			return;

		// Now we can bulk insert.
		m_Specification.pString->insert( m_Specification.pString->begin() + insertionPoint, filteredText.cbegin(), filteredText.cend() );

		m_OffsetFromStart += filteredText.size();

		m_ModifiedSinceLastRender = true;
		m_CursorFollow = true;
		ResetCursorTime();
	}

}
