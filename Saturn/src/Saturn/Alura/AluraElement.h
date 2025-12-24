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

#include "Saturn/Core/Base.h"
#include "Saturn/Core/UUID.h"

#include <glm/glm.hpp>

namespace Saturn {

	class AluraRenderer;
	class AluraCanvas;

	enum class AluraRenderType
	{
		Quad,
		Circle,
		Line,
		Text
	};

	class AluraElement
	{
	public:
		AluraElement() = default;
		AluraElement( const std::string& rName, const glm::vec2& rPosition, const glm::vec2& rSize, const glm::vec4& rColor ) 
			: m_Name( rName ), m_Position( rPosition ), m_Size( rSize ), m_Color( rColor )
		{
		}

		~AluraElement() = default;

	public:
		AluraElement& SetPosition( const glm::vec2& rPosition ) 
		{
			m_Position = rPosition;
			return *this;
		}

		AluraElement& Nudge( const glm::vec2& rPosition )
		{
			m_Position += rPosition;
			return *this;
		}

		AluraElement& Poke( const glm::vec2& rPosition )
		{
			m_Position -= rPosition;
			return *this;
		}

		AluraElement& SetColor( const glm::vec4& rColor )
		{
			m_Color = rColor;
			return *this;
		}

		AluraElement& SetSize( const glm::vec2& rSize )
		{
			m_Size = rSize;
			return *this;
		}

		AluraElement& Expand( const glm::vec2& rSize )
		{
			m_Size += rSize;
			return *this;
		}

		AluraElement& Shrink( const glm::vec2& rSize )
		{
			m_Size -= rSize;
			return *this;
		}

	public:
		UUID GetID() const { return m_ElementID; }

	private:
		UUID m_ElementID;

	protected:
		std::string m_Name;
		glm::vec2 m_Position{};
		glm::vec2 m_Size{};
		glm::vec4 m_Color{};
		AluraRenderType m_RenderType = AluraRenderType::Quad;
		bool m_WantToSetPosition = false;

	private:
		friend class AluraCanvas;
	};

}
