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
#include "AluraStyleEditor.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace Saturn {

	void AluraStyleEditor::Draw( AluraStyle& rStyle, bool readOnly )
	{
		Auxiliary::ScopedDisabledFlag disabledIfReadOnly( readOnly );

		ImGui::SeparatorText( "General Style" );

		Auxiliary::DrawFloatControl( "Alpha", rStyle.Alpha );
		Auxiliary::DrawFloatControl( "Disabled Alpha", rStyle.DisabledAlpha );
		Auxiliary::DrawFloatControl( "Region Rounding", rStyle.RegionRounding );

		Auxiliary::DrawVec2Control( "Window Padding", rStyle.WindowPadding );
		Auxiliary::DrawVec2Control( "Item Spacing", rStyle.ItemSpacing );
		Auxiliary::DrawVec2Control( "Inner Item Spacing", rStyle.ItemInnerSpacing );

		Auxiliary::DrawFloatControl( "Indent Spacing", rStyle.IndentSpacing );
		Auxiliary::DrawFloatControl( "Window Border Size", rStyle.WindowBorderSize );
		Auxiliary::DrawFloatControl( "Font Size", rStyle.CurrentFontSize );

		ImGui::SeparatorText( "Colours" );
		ImGui::ColorEdit4( "Text", glm::value_ptr( rStyle.Colours[ 0 ] ) );
		ImGui::ColorEdit4( "Text Disabled", glm::value_ptr( rStyle.Colours[ 1 ] ) );
		ImGui::ColorEdit4( "Button", glm::value_ptr( rStyle.Colours[ 2 ] ) );
		ImGui::ColorEdit4( "Button Hovered", glm::value_ptr( rStyle.Colours[ 3 ] ) );
		ImGui::ColorEdit4( "Border", glm::value_ptr( rStyle.Colours[ 4 ] ) );
		ImGui::ColorEdit4( "Frame Background", glm::value_ptr( rStyle.Colours[ 5 ] ) );
		ImGui::ColorEdit4( "Progress bar Color", glm::value_ptr( rStyle.Colours[ 6 ] ) );
		ImGui::ColorEdit4( "Separator", glm::value_ptr( rStyle.Colours[ 7 ] ) );
		ImGui::ColorEdit4( "Region Background", glm::value_ptr( rStyle.Colours[ 8 ] ) );
		ImGui::ColorEdit4( "Text Selected", glm::value_ptr( rStyle.Colours[ 9 ] ) );
		ImGui::ColorEdit4( "Slider Grab", glm::value_ptr( rStyle.Colours[ 10 ] ) );
		ImGui::ColorEdit4( "Slider Grab Active", glm::value_ptr( rStyle.Colours[ 11 ] ) );
	}
}
