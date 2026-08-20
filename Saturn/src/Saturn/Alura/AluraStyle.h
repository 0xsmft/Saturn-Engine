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

	enum AluraColour
	{
		AluraColour_Text,
		AluraColour_TextDisabled,
		AluraColour_Button,
		AluraColour_ButtonHovered,
		AluraColour_Border,
		AluraColour_FrameBackground,
		AluraColour_ProgressColour,
		AluraColour_Separator,
		AluraColour_RegionBackground,
		AluraColour_TextSelected,
		AluraColour_SliderGrab,
		AluraColour_SliderGrabActive,
		AluraColour_Count,
	};

	class AluraStyle
	{
	public:
		// Note when adding new styling options ditto to the serialiser as well!

		// Default alpha for items.
		float Alpha;

		// Default disabled alpha for items.
		float DisabledAlpha;
	
		// Region Rounding, 0.0f by default.
		float RegionRounding;

		// Spacing between the first element and the canvas.
		glm::vec2 WindowPadding;

		// Spacing between each item.
		glm::vec2 ItemSpacing;

		// The spacing between items and their inner elements
		// e.g. button and the text inside of the button.
		glm::vec2 ItemInnerSpacing;

		// Indent offset
		float IndentSpacing;

		// Border size of the canvas.
		float WindowBorderSize;

		// Current font size.
		float CurrentFontSize;

		// Colors
		std::array<glm::vec4, ( std::underlying_type_t<AluraColour> )AluraColour_Count> Colours;

	public:
		AluraStyle() 
		{
			Colours.fill( glm::one<glm::vec4>() );
			Default();
		}

	public:
		void Default() 
		{
			Alpha = 1.0f;
			DisabledAlpha = 0.5f;
			RegionRounding = 10.0f;
			WindowPadding = glm::vec2( 8.0f, 8.0f );
			ItemSpacing = glm::vec2( 8.0f, 4.0f );
			ItemInnerSpacing = glm::vec2( 4.0f, 4.0f );
			IndentSpacing = 21.0f;
			WindowBorderSize = 1.0f;
			CurrentFontSize = 16.0f; // == 1 em

			Colours[ AluraColour_Text ] = glm::one<glm::vec4>();
			Colours[ AluraColour_TextDisabled ] = glm::vec4( 0.5f, 0.5f, 0.5f, 1.0f );
			Colours[ AluraColour_Button ] = glm::vec4( 0.26f, 0.59f, 0.98f, 0.40f );
			Colours[ AluraColour_ButtonHovered ] = glm::vec4( 0.26f, 0.59f, 0.98f, 1.0f );
			Colours[ AluraColour_Border ] = glm::vec4( 0.1216f, 0.1216f, 0.1216f, 1.0f );
			Colours[ AluraColour_FrameBackground ] = glm::vec4( 0.2f, 0.2f, 0.20f, 1.0f );
			Colours[ AluraColour_ProgressColour ] = glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f );
			Colours[ AluraColour_Separator ] = glm::vec4( 0.1882f, 0.1882f, 0.1882f, 1.0f );
			Colours[ AluraColour_RegionBackground ] = glm::vec4( 0.0824f, 0.0824f, 0.0824f, 1.0f );
			Colours[ AluraColour_TextSelected ] = glm::vec4( 1.0f, 1.0f, 1.0f, 0.4f );
			Colours[ AluraColour_SliderGrab ] = glm::vec4( 0.3922f, 0.3922f, 0.3922f, 1.0f );
			Colours[ AluraColour_SliderGrabActive ] = glm::vec4( 1.0f, 0.3922f, 0.0f, 1.0f );
		}
	};
	
}
