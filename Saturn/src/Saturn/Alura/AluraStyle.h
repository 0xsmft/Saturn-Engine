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

#include <glm/glm.hpp>

namespace Saturn {

	enum class AluraStyleVar 
	{
		Alpha,
		DisabledAlpha,
		WindowPadding,
		IndentSpacing,
		WindowBorderSize,
		Count,
	};

	enum AluraColor
	{
		AluraColor_Text,
		AluraColor_TextDisabled,
		AluraColor_Button,
		AluraColor_ButtonHovered,
		AluraColor_FrameBorder,
		AluraColor_FrameBackground,
		AluraColor_ProgressColor,
		AluraColor_Count,
	};

	class AluraStyle
	{
	public:
		// Note when adding new styling options ditto to the serialiser as well!

		// Default alpha for items.
		float Alpha;

		// Default disabled alpha for items.
		float DisabledAlpha;
		
		// Spacing between the first element and the canvas.
		glm::vec2 WindowPadding;

		// Spacing between each item.
		glm::vec2 ItemSpacing;

		// Indent offset
		float IndentSpacing;

		// Border size of the canvas.
		float WindowBorderSize;

		// Current font size.
		float CurrentFontSize;

		// Fonts array
		std::array<glm::vec4, ( std::underlying_type_t<AluraColor> )AluraColor_Count> Colors;

	public:
		AluraStyle() 
		{
			Colors.fill( glm::one<glm::vec4>() );
			Default();
		}

	public:
		void Default() 
		{
			Alpha = 1.0f;
			DisabledAlpha = 0.5f;
			WindowPadding = glm::vec2( 8.0f, 8.0f );
			ItemSpacing = glm::vec2( 8.0f, 4.0f );
			IndentSpacing = 21.0f;
			WindowBorderSize = 1.0f;
			CurrentFontSize = 16.0f; // == 1 em

			Colors[ AluraColor_Text ] = glm::one<glm::vec4>();
			Colors[ AluraColor_TextDisabled ] = glm::vec4( 0.5f, 0.5f, 0.5f, 1.0f );
			Colors[ AluraColor_Button ] = glm::vec4( 0.26f, 0.59f, 0.98f, 0.40f );
			Colors[ AluraColor_ButtonHovered ] = glm::vec4( 0.26f, 0.59f, 0.98f, 1.0f );
			Colors[ AluraColor_FrameBorder ] = glm::vec4( 0.0f, 0.0f, 0.0f, 1.0f );
			Colors[ AluraColor_FrameBackground ] = glm::vec4( 0.2f, 0.2f, 0.20f, 1.0f );
			Colors[ AluraColor_ProgressColor ] = glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f );
		}
	};
	
}
