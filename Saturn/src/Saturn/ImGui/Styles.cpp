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

#include "sppch.h"
#include "Styles.h"

#include <imgui.h>

namespace Saturn::Styles {

	void Dark()
	{
		ImGuiStyle& rStyle = ImGui::GetStyle();
		ImVec4* pColors = rStyle.Colors;

		// Inspired by My Own custom theme, Geno IDE's theme, and ImGui's own dark theme, and a mixture with some of Hazel's theme.
		pColors[ ImGuiCol_Text ]                   = ImVec4( 1.000f, 1.000f, 1.000f, 1.000f );
		pColors[ ImGuiCol_TextDisabled ]           = ImVec4( 0.500f, 0.500f, 0.500f, 1.000f );
		
		pColors[ ImGuiCol_WindowBg ]               = ImGui::ColorConvertU32ToFloat4( IM_COL32( 21, 21, 21, 255 ) );
		pColors[ ImGuiCol_WindowBg ]               = ImGui::ColorConvertU32ToFloat4( IM_COL32( 26, 26, 26, 255 ) );
		pColors[ ImGuiCol_ChildBg ]                = ImGui::ColorConvertU32ToFloat4( IM_COL32( 34, 34, 34, 255 ) );
		pColors[ ImGuiCol_PopupBg ]				   = pColors[ ImGuiCol_ChildBg ];
		pColors[ ImGuiCol_Border ]                 = ImGui::ColorConvertU32ToFloat4( IM_COL32( 31, 31, 31, 255 ) );
		pColors[ ImGuiCol_BorderShadow ]           = ImVec4( 0.000f, 0.000f, 0.000f, 0.000f );
		
		pColors[ ImGuiCol_FrameBg ]                = ImGui::ColorConvertU32ToFloat4( IM_COL32( 20, 20, 20, 255 ) );
		pColors[ ImGuiCol_FrameBgHovered ]         = ImVec4( 0.200f, 0.200f, 0.200f, 1.000f );
		pColors[ ImGuiCol_FrameBgActive ]		   = pColors[ ImGuiCol_FrameBg ];

		pColors[ ImGuiCol_TitleBg ]                = pColors[ ImGuiCol_WindowBg ];
		pColors[ ImGuiCol_TitleBgActive ]          = pColors[ ImGuiCol_FrameBg ];
		pColors[ ImGuiCol_TitleBgCollapsed ]	   = pColors[ ImGuiCol_FrameBg ];

		pColors[ ImGuiCol_MenuBarBg ]			   = pColors[ ImGuiCol_FrameBg ];
		pColors[ ImGuiCol_ScrollbarBg ]            = ImVec4( 0.160f, 0.160f, 0.160f, 1.000f );
		pColors[ ImGuiCol_ScrollbarGrab ]          = ImVec4( 0.277f, 0.277f, 0.277f, 1.000f );
		pColors[ ImGuiCol_ScrollbarGrabHovered ]   = ImVec4( 0.300f, 0.300f, 0.300f, 1.000f );
		pColors[ ImGuiCol_ScrollbarGrabActive ]    = ImVec4( 1.000f, 0.391f, 0.000f, 1.000f );
		pColors[ ImGuiCol_CheckMark ]              = ImVec4( 1.000f, 1.000f, 1.000f, 1.000f );
		pColors[ ImGuiCol_SliderGrab ]             = ImVec4( 0.391f, 0.391f, 0.391f, 1.000f );
		pColors[ ImGuiCol_SliderGrabActive ]       = ImVec4( 1.000f, 0.391f, 0.000f, 1.000f );
		
		pColors[ ImGuiCol_Button ]                 = ImGui::ColorConvertU32ToFloat4( IM_COL32( 58, 58, 58, 255 ) );
		pColors[ ImGuiCol_ButtonHovered ]          = ImVec4( 1.000f, 1.000f, 1.000f, 0.156f );
		pColors[ ImGuiCol_ButtonActive ]           = ImVec4( 1.000f, 1.000f, 1.000f, 0.391f );
		
		pColors[ ImGuiCol_Header ]                 = ImGui::ColorConvertU32ToFloat4( IM_COL32( 47, 47, 47, 255 ) );
		pColors[ ImGuiCol_HeaderHovered ]          = ImVec4( 0.469f, 0.469f, 0.469f, 1.000f );
		pColors[ ImGuiCol_HeaderActive ]           = pColors[ ImGuiCol_Header ];
		
		pColors[ ImGuiCol_Separator ]			   = ImGui::ColorConvertU32ToFloat4( IM_COL32( 48, 48, 48, 255 ) );
		pColors[ ImGuiCol_SeparatorHovered ]       = ImVec4( 0.391f, 0.391f, 0.391f, 1.000f );
		pColors[ ImGuiCol_SeparatorActive ]        = ImGui::ColorConvertU32ToFloat4( IM_COL32( 76, 121, 141, 255 ) );

		pColors[ ImGuiCol_ResizeGrip ]             = ImVec4( 0.44f, 0.37f, 0.36f, 0.50f );
		pColors[ ImGuiCol_ResizeGripHovered ]      = ImVec4( 1.000f, 1.000f, 1.000f, 0.670f );
		pColors[ ImGuiCol_ResizeGripActive ]       = ImVec4( 1.000f, 0.391f, 0.000f, 1.000f );
		
		pColors[ ImGuiCol_Tab ]                    = ImVec4( 0.23f, 0.22f, 0.22f, 1.00f );
		pColors[ ImGuiCol_TabHovered ]             = ImVec4( 0.352f, 0.352f, 0.352f, 1.000f );
		pColors[ ImGuiCol_TabActive ]              = ImVec4( 0.80f, 0.36f, 0.08f, 1.00f );
		pColors[ ImGuiCol_TabUnfocused ]           = ImVec4( 0.098f, 0.098f, 0.098f, 1.000f );
		pColors[ ImGuiCol_TabUnfocusedActive ]     = ImVec4( 0.195f, 0.195f, 0.195f, 1.000f );

		pColors[ ImGuiCol_DockingPreview ]         = ImVec4( 1.000f, 0.391f, 0.000f, 0.781f );
		pColors[ ImGuiCol_DockingEmptyBg ]         = ImVec4( 0.180f, 0.180f, 0.180f, 1.000f );
		pColors[ ImGuiCol_PlotLines ]              = ImVec4( 0.469f, 0.469f, 0.469f, 1.000f );
		pColors[ ImGuiCol_PlotLinesHovered ]       = ImVec4( 1.000f, 0.391f, 0.000f, 1.000f );
		pColors[ ImGuiCol_PlotHistogram ]          = ImVec4( 0.586f, 0.586f, 0.586f, 1.000f );
		pColors[ ImGuiCol_PlotHistogramHovered ]   = ImVec4( 1.000f, 0.391f, 0.000f, 1.000f );
		pColors[ ImGuiCol_TextSelectedBg ]         = ImVec4( 1.000f, 1.000f, 1.000f, 0.156f );
		pColors[ ImGuiCol_DragDropTarget ]         = ImVec4( 1.000f, 0.391f, 0.000f, 1.000f );
		pColors[ ImGuiCol_NavHighlight ]           = ImVec4( 1.000f, 0.391f, 0.000f, 1.000f );
		pColors[ ImGuiCol_NavWindowingHighlight ]  = ImVec4( 1.000f, 0.391f, 0.000f, 1.000f );
		pColors[ ImGuiCol_NavWindowingDimBg ]      = ImVec4( 0.000f, 0.000f, 0.000f, 0.586f );
		pColors[ ImGuiCol_ModalWindowDimBg ]       = ImVec4( 0.000f, 0.000f, 0.000f, 0.586f );

		rStyle.ChildRounding = 4.0f;
		rStyle.FrameBorderSize = 1.0f;
		rStyle.FrameRounding = 2.0f;
		rStyle.GrabMinSize = 7.0f;
		rStyle.PopupRounding = 2.0f;
		rStyle.ScrollbarRounding = 12.0f;
		rStyle.ScrollbarSize = 13.0f;
		rStyle.TabBorderSize = 1.0f;
		rStyle.TabRounding = 0.0f;
		rStyle.WindowRounding = 4.0f;
		rStyle.TabRounding = 4.0f;
	}

}