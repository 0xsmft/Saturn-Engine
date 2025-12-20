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

#pragma once

#include "AluraElement.h"
#include "AluraStyle.h"

#include "Saturn/Core/Base.h"
#include "Saturn/Core/UUID.h"

#include <glm/glm.hpp>

namespace Saturn {

	class AluraRenderer;

	struct AluraLayout
	{
		// Current emitting position.
		glm::vec2 CursorPos{ 0.0f };
		glm::vec2 CursorPosPrevLine{ 0.0f };

		glm::vec2 CurrLineSize{ 0.0f };
		glm::vec2 PrevLineSize{ 0.0f };
		float     CurrLineTextBaseOffset{ 0.0f };
		float     PrevLineTextBaseOffset{ 0.0f };
		float     CurrentIndent{ 0.0f };

		bool IsSameLine = false;
	};

	class AluraCanvas : public RefTarget
	{
	public:
		AluraCanvas( const std::string& rName, const glm::vec2& rSize, const glm::vec2& rPosition );
		~AluraCanvas();

		void Begin();
		void Draw( Timestep ts );
		void Destory();

		void SetContext( Ref<AluraRenderer> context );

	public:
		AluraElement& AddRect( const glm::vec2& rSize, const glm::vec4& rColor );

		void SetNextItemSize( const glm::vec2& rSize );
		void SetNextItemPosition( const glm::vec2& rPosition );

		void Indent( float width = 0.0f );
		void Unindent( float width = 0.0f );

		[[nodiscard]] bool IsItemHovered();
		[[nodiscard]] bool IsItemClicked( RubyMouseButton mouseBtn );

	public:
		glm::vec2 GetPosition() const { return m_Position; }
		glm::vec2 GetSize() const { return m_Size; }

	public:
		AluraCanvas& SetPosition( const glm::vec2& rPosition ) 
		{
			m_Position = rPosition;
			return *this;
		}

		void NrSetPosition( const glm::vec2& rPosition ) 
		{
			m_Position = rPosition;
		}

		AluraCanvas& SetSize( const glm::vec2& rSize )
		{
			m_Size = rSize;
			return *this;
		}

	private:
		void AdvanceCursor( const glm::vec2& rSize );
		bool IsMouseHoveringRect( const glm::vec2& rMin, const glm::vec2& rMax ) const;

	private:
		std::string m_Name;
		UUID m_ID;
		glm::vec2 m_Size;
		glm::vec2 m_Position;

		glm::vec2 m_PendingNextItemSize{};
		glm::vec2 m_PendingNextItemPosition{};
		bool m_WantToSetItemSize = false, m_WantToSetItemPosition = false;

		// The mouse position relative to this canvas' positions.
		glm::vec2 m_MousePosition{};

		Ref<AluraRenderer> m_Renderer;

		std::vector<AluraElement> m_Elements;

		AluraStyle m_Style{};
		AluraLayout m_Layout{};
	};
	
}
