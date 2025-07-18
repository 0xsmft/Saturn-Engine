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

//------------------------------------------------------------------------------
// LICENSE
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
//
// CREDITS
//   Written by Michal Cichon
//------------------------------------------------------------------------------

#include "sppch.h"
#include "NodeEditorAux.h"

#include <imgui_internal.h>

namespace Saturn::Auxiliary {

	void DrawPinIconInternal( ImDrawList* pDrawList, const ImVec2& a, const ImVec2& b, PinIconType type, bool filled, ImU32 color, ImU32 innerColor )
	{
		auto rect = ImRect( a, b );
		auto rect_x = rect.Min.x;
		auto rect_y = rect.Min.y;
		auto rect_w = rect.Max.x - rect.Min.x;
		auto rect_h = rect.Max.y - rect.Min.y;
		auto rect_center_x = ( rect.Min.x + rect.Max.x ) * 0.5f;
		auto rect_center_y = ( rect.Min.y + rect.Max.y ) * 0.5f;
		auto rect_center = ImVec2( rect_center_x, rect_center_y );
		const auto outline_scale = rect_w / 24.0f;
		const auto extra_segments = static_cast< int >( 2 * outline_scale ); // for full circle

		if( type == PinIconType::Flow )
		{
			const auto origin_scale = rect_w / 24.0f;

			const auto offset_x = 1.0f * origin_scale;
			const auto offset_y = 0.0f * origin_scale;
			const auto margin = ( filled ? 2.0f : 2.0f ) * origin_scale;
			const auto rounding = 0.1f * origin_scale;
			const auto tip_round = 0.7f; // percentage of triangle edge (for tip)
			//const auto edge_round = 0.7f; // percentage of triangle edge (for corner)
			const auto canvas = ImRect(
				rect.Min.x + margin + offset_x,
				rect.Min.y + margin + offset_y,
				rect.Max.x - margin + offset_x,
				rect.Max.y - margin + offset_y );
			const auto canvas_x = canvas.Min.x;
			const auto canvas_y = canvas.Min.y;
			const auto canvas_w = canvas.Max.x - canvas.Min.x;
			const auto canvas_h = canvas.Max.y - canvas.Min.y;

			const auto left = canvas_x + canvas_w * 0.5f * 0.3f;
			const auto right = canvas_x + canvas_w - canvas_w * 0.5f * 0.3f;
			const auto top = canvas_y + canvas_h * 0.5f * 0.2f;
			const auto bottom = canvas_y + canvas_h - canvas_h * 0.5f * 0.2f;
			const auto center_y = ( top + bottom ) * 0.5f;
			//const auto angle = AX_PI * 0.5f * 0.5f * 0.5f;

			const auto tip_top = ImVec2( canvas_x + canvas_w * 0.5f, top );
			const auto tip_right = ImVec2( right, center_y );
			const auto tip_bottom = ImVec2( canvas_x + canvas_w * 0.5f, bottom );

			pDrawList->PathLineTo( ImVec2( left, top ) + ImVec2( 0, rounding ) );
			pDrawList->PathBezierCubicCurveTo(
				ImVec2( left, top ),
				ImVec2( left, top ),
				ImVec2( left, top ) + ImVec2( rounding, 0 ) );
			pDrawList->PathLineTo( tip_top );
			pDrawList->PathLineTo( tip_top + ( tip_right - tip_top ) * tip_round );
			pDrawList->PathBezierCubicCurveTo(
				tip_right,
				tip_right,
				tip_bottom + ( tip_right - tip_bottom ) * tip_round );
			pDrawList->PathLineTo( tip_bottom );
			pDrawList->PathLineTo( ImVec2( left, bottom ) + ImVec2( rounding, 0 ) );
			pDrawList->PathBezierCubicCurveTo(
				ImVec2( left, bottom ),
				ImVec2( left, bottom ),
				ImVec2( left, bottom ) - ImVec2( 0, rounding ) );

			if( !filled )
			{
				if( innerColor & 0xFF000000 )
					pDrawList->AddConvexPolyFilled( pDrawList->_Path.Data, pDrawList->_Path.Size, innerColor );

				pDrawList->PathStroke( color, true, 2.0f * outline_scale );
			}
			else
				pDrawList->PathFillConvex( color );
		}
		else
		{
			auto triangleStart = rect_center_x + 0.32f * rect_w;

			auto rect_offset = -static_cast< int >( rect_w * 0.25f * 0.25f );

			rect.Min.x += rect_offset;
			rect.Max.x += rect_offset;
			rect_x += rect_offset;
			rect_center_x += rect_offset * 0.5f;
			rect_center.x += rect_offset * 0.5f;

			if( type == PinIconType::Circle )
			{
				const auto c = rect_center;

				if( !filled )
				{
					const auto r = 0.5f * rect_w / 2.0f - 0.5f;

					if( innerColor & 0xFF000000 )
						pDrawList->AddCircleFilled( c, r, innerColor, 12 + extra_segments );
					pDrawList->AddCircle( c, r, color, 12 + extra_segments, 2.0f * outline_scale );
				}
				else
				{
					pDrawList->AddCircleFilled( c, 0.5f * rect_w / 2.0f, color, 12 + extra_segments );
				}
			}

			if( type == PinIconType::Square )
			{
				if( filled )
				{
					const auto r = 0.5f * rect_w / 2.0f;
					const auto p0 = rect_center - ImVec2( r, r );
					const auto p1 = rect_center + ImVec2( r, r );

#if IMGUI_VERSION_NUM > 18101
					pDrawList->AddRectFilled( p0, p1, color, 0, ImDrawFlags_RoundCornersAll );
#else
					pDrawList->AddRectFilled( p0, p1, color, 0, 15 );
#endif
				}
				else
				{
					const auto r = 0.5f * rect_w / 2.0f - 0.5f;
					const auto p0 = rect_center - ImVec2( r, r );
					const auto p1 = rect_center + ImVec2( r, r );

					if( innerColor & 0xFF000000 )
					{
#if IMGUI_VERSION_NUM > 18101
						pDrawList->AddRectFilled( p0, p1, innerColor, 0, ImDrawFlags_RoundCornersAll );
#else
						pDrawList->AddRectFilled( p0, p1, innerColor, 0, 15 );
#endif
					}

#if IMGUI_VERSION_NUM > 18101
					pDrawList->AddRect( p0, p1, color, 0, ImDrawFlags_RoundCornersAll, 2.0f * outline_scale );
#else
					pDrawList->AddRect( p0, p1, color, 0, 15, 2.0f * outline_scale );
#endif
				}
			}

			if( type == PinIconType::Grid )
			{
				const auto r = 0.5f * rect_w / 2.0f;
				const auto w = ceilf( r / 3.0f );

				const auto baseTl = ImVec2( floorf( rect_center_x - w * 2.5f ), floorf( rect_center_y - w * 2.5f ) );
				const auto baseBr = ImVec2( floorf( baseTl.x + w ), floorf( baseTl.y + w ) );

				auto tl = baseTl;
				auto br = baseBr;
				for( int i = 0; i < 3; ++i )
				{
					tl.x = baseTl.x;
					br.x = baseBr.x;
					pDrawList->AddRectFilled( tl, br, color );
					tl.x += w * 2;
					br.x += w * 2;
					if( i != 1 || filled )
						pDrawList->AddRectFilled( tl, br, color );
					tl.x += w * 2;
					br.x += w * 2;
					pDrawList->AddRectFilled( tl, br, color );

					tl.y += w * 2;
					br.y += w * 2;
				}

				triangleStart = br.x + w + 1.0f / 24.0f * rect_w;
			}

			if( type == PinIconType::RoundSquare )
			{
				if( filled )
				{
					const auto r = 0.5f * rect_w / 2.0f;
					const auto cr = r * 0.5f;
					const auto p0 = rect_center - ImVec2( r, r );
					const auto p1 = rect_center + ImVec2( r, r );

#if IMGUI_VERSION_NUM > 18101
					pDrawList->AddRectFilled( p0, p1, color, cr, ImDrawFlags_RoundCornersAll );
#else
					pDrawList->AddRectFilled( p0, p1, color, cr, 15 );
#endif
				}
				else
				{
					const auto r = 0.5f * rect_w / 2.0f - 0.5f;
					const auto cr = r * 0.5f;
					const auto p0 = rect_center - ImVec2( r, r );
					const auto p1 = rect_center + ImVec2( r, r );

					if( innerColor & 0xFF000000 )
					{
#if IMGUI_VERSION_NUM > 18101
						pDrawList->AddRectFilled( p0, p1, innerColor, cr, ImDrawFlags_RoundCornersAll );
#else
						pDrawList->AddRectFilled( p0, p1, innerColor, cr, 15 );
#endif
					}

#if IMGUI_VERSION_NUM > 18101
					pDrawList->AddRect( p0, p1, color, cr, ImDrawFlags_RoundCornersAll, 2.0f * outline_scale );
#else
					pDrawList->AddRect( p0, p1, color, cr, 15, 2.0f * outline_scale );
#endif
				}
			}
			else if( type == PinIconType::Diamond )
			{
				if( filled )
				{
					const auto r = 0.607f * rect_w / 2.0f;
					const auto c = rect_center;

					pDrawList->PathLineTo( c + ImVec2( 0, -r ) );
					pDrawList->PathLineTo( c + ImVec2( r, 0 ) );
					pDrawList->PathLineTo( c + ImVec2( 0, r ) );
					pDrawList->PathLineTo( c + ImVec2( -r, 0 ) );
					pDrawList->PathFillConvex( color );
				}
				else
				{
					const auto r = 0.607f * rect_w / 2.0f - 0.5f;
					const auto c = rect_center;

					pDrawList->PathLineTo( c + ImVec2( 0, -r ) );
					pDrawList->PathLineTo( c + ImVec2( r, 0 ) );
					pDrawList->PathLineTo( c + ImVec2( 0, r ) );
					pDrawList->PathLineTo( c + ImVec2( -r, 0 ) );

					if( innerColor & 0xFF000000 )
						pDrawList->AddConvexPolyFilled( pDrawList->_Path.Data, pDrawList->_Path.Size, innerColor );

					pDrawList->PathStroke( color, true, 2.0f * outline_scale );
				}
			}
			else
			{
				const auto triangleTip = triangleStart + rect_w * ( 0.45f - 0.32f );

				pDrawList->AddTriangleFilled(
					ImVec2( ceilf( triangleTip ), rect_y + rect_h * 0.5f ),
					ImVec2( triangleStart, rect_center_y + 0.15f * rect_h ),
					ImVec2( triangleStart, rect_center_y - 0.15f * rect_h ),
					color );
			}
		}
	}

	void DrawPinIcon( const ImVec2& size, PinIconType type, bool filled, const ImVec4& color /*= ImVec4( 1, 1, 1, 1 )*/, const ImVec4& innerColor /*= ImVec4( 0, 0, 0, 0 ) */ )
	{
		if( ImGui::IsRectVisible( size ) )
		{
			ImVec2 cursorPos = ImGui::GetCursorScreenPos();
			ImDrawList* pDrawList = ImGui::GetWindowDrawList();

			DrawPinIconInternal( pDrawList, cursorPos, cursorPos + size, type, filled, ImColor( color ), ImColor( innerColor ) );
		}

		ImGui::Dummy( size );
	}

}
