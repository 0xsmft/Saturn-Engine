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

	struct AluraTextInputSpecification
	{
		std::string* pString = nullptr;
		
		uint64_t ItemID = 0llu;
		size_t MaxCharacters = 1024llu;
		float CursorAnimDuration = 0.5f;

		bool AcceptUnicode = true;
	};

	class AluraTextInputA
	{
	public:
		AluraTextInputA();
		~AluraTextInputA();

		void Init( const AluraTextInputSpecification& rSpecification );

		void OnCharacter( uint32_t wc );
		void OnKeyPressed( RubyKey key );

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

		void ResetCursorTime() { m_CursorBlinkingTime = 0.0f; }

		bool IsModifiedAndAcknowledgeModification() { bool modified = m_ModifiedSinceLastRender; m_ModifiedSinceLastRender = false; return modified; }
		bool CursorIsVisible() const { return m_CursorBlinkingTime <= 0.0f; }

	public:
		uint64_t GetItemID() const { return m_Specification.ItemID; }
		size_t GetCursorIndex() const { return m_CursorIndex; }
		float GetBlinkingTime() const { return m_CursorBlinkingTime; }

	private:
		void EraseAtCursor();

	private:
		bool m_EnterPressed = false;
		bool m_ModifiedSinceLastRender = false;

		AluraTextInputSpecification m_Specification{};
		size_t m_CursorIndex = std::u32string::npos;

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
	};

}
