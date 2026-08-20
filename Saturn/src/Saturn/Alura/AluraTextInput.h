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

#include "Saturn/Core/Ruby/RubyEventType.h"

namespace Saturn {

	enum AluraTextInputFlags_
	{
		AluraTextInputFlags_NoFlags = 0u,

		// Convert all characters to upper case
		// upon insertion.
		// Mutually exclusive with 
		// AluraTextInputFlags_CharsToLower.
		AluraTextInputFlags_CharsToUpper = BIT( 0 ),
		
		// Convert all characters to lower case
		// upon insertion.
		// Mutually exclusive with 
		// AluraTextInputFlags_CharsToUpper.
		AluraTextInputFlags_CharsToLower = BIT( 1 ),

		// Allow 0 to 9 and "."
		AluraTextInputFlags_NumbersOnly = BIT( 2 ),
		
		// Do not allow spaces and/or tabs.
		AluraTextInputFlags_NoSpacesOrTabs = BIT( 3 ),

		// Return true on enter.
		AluraTextInputFlags_EnterReturnsTrue = BIT( 4 ),
	};

	// enum AluraTextInputFlags_
	typedef uint8_t AluraTextInputFlags;

	struct AluraTextInputSpecification
	{
		std::string* pString = nullptr;
		
		uint64_t ItemID = 0llu;
		size_t MaxCharacters = 1024llu;
		float CursorAnimDuration = 0.5f;
		AluraTextInputFlags Flags = AluraTextInputFlags_NoFlags;

		bool AcceptUnicode = true;
	};

	enum class AluraTextDeletionDirection : uint8_t
	{
		// Delete key, cursor position remains in the same
		// place.
		Forwards,

		// Backspace key, cursor moves back.
		Backwards
	};

	class AluraTextInputA
	{
	public:
		AluraTextInputA();
		~AluraTextInputA();

		void Init( const AluraTextInputSpecification& rSpecification );
		void Reset();

		void OnCharacter( uint32_t wc );
		void OnKeyPressed( RubyKey key );
		void OnKeyReleased( RubyKey key );

		void DeleteAll();
		void MoveLeft();
		void MoveRight();

		void IncrementCursorTime( Timestep ts ) 
		{ 
			m_CursorBlinkingTime += ts;

			// Show if we are over.
			if( m_CursorBlinkingTime >= m_Specification.CursorAnimDuration ) 
			{
				m_CursorBlinkingTime = -m_Specification.CursorAnimDuration;
			}
		}

		void ResetCursorTime() { m_CursorBlinkingTime = -m_Specification.CursorAnimDuration; }

		bool IsModifiedAndAcknowledgeModification() { bool modified = m_ModifiedSinceLastRender; m_ModifiedSinceLastRender = false; return modified; }
		bool CursorIsVisible() const { return m_CursorBlinkingTime <= 0.0f; }
		void SetScrollX( float x ) { m_ScrollX = x; }
		void SetCursorShouldFollow( bool follow ) { m_CursorFollow = follow; }
		
		void ResetSelection();

	public:
		uint64_t GetItemID() const { return m_Specification.ItemID; }
		size_t GetCursorIndex() const { return m_CursorIndex; }
		float GetBlinkingTime() const { return m_CursorBlinkingTime; }
		float GetScrollX() const { return m_ScrollX; }

		bool CursorShouldFollow() const { return m_CursorFollow; }
		bool IsSelecting() const { return m_IsSelecting; }
		size_t GetSelectionMin() const { if( m_IsSelecting ) return std::min( m_SelectionBegin, m_SelectionEnd ); return 0llu; }
		size_t GetSelectionMax() const { if( m_IsSelecting ) return std::max( m_SelectionBegin, m_SelectionEnd ); return 0llu; }

		size_t GetSelectionStart() const { return m_SelectionBegin; }
		size_t GetSelectionEnd() const { return m_SelectionEnd; }

		const AluraTextInputSpecification& GetSpecification() const { return m_Specification; }

	private:
		void EraseAtCursorOrSelection();
		void TryInsertCharacterAtInsertionPoint( uint32_t wc, size_t insertionPoint );
		void CopyPaste_InsertBulk( const std::string& rText, size_t insertionPoint );

		[[nodiscard]] bool FilterCharacter( uint32_t wc );
		std::string GetTextBetweenSelection();

	private:
		AluraTextInputSpecification m_Specification{};

		size_t m_CursorIndex = std::string::npos;
		size_t m_SelectionBegin = std::string::npos;
		size_t m_SelectionEnd = std::string::npos;
		size_t m_SelectionAnchor = std::string::npos;

		bool m_EnterPressed = false;
		bool m_ModifiedSinceLastRender = false;
		bool m_IsSelecting = false;
		bool m_ShiftDown = false;
		bool m_ControlDown = false;
		bool m_CursorFollow = false;

		AluraTextDeletionDirection m_TextDeletionDirection = AluraTextDeletionDirection::Backwards;

		//
		// Cursor blinking time...
		// 
		// When this value is <= 0.0f the cursor is visible
		// when it's > 0.0f it is not visible.
		// 
		// The time between each phase is controlled via 
		// m_Specification.CursorAnimDuration.
		//
		float m_CursorBlinkingTime = 0.0f;
		float m_ScrollX = 0.0f;
	};

}
