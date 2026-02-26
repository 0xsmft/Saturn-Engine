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
#include "ImGuiAuxiliary.h"

#include <backends/imgui_impl_vulkan.h>
#include <glm/gtc/type_ptr.hpp>

namespace Saturn::Auxiliary {

	static inline ImVec2  operator*( const ImVec2& lhs, const float rhs ) { return ImVec2( lhs.x * rhs, lhs.y * rhs ); }
	static inline ImVec2  operator/( const ImVec2& lhs, const float rhs ) { return ImVec2( lhs.x / rhs, lhs.y / rhs ); }
	static inline ImVec2  operator+( const ImVec2& lhs, const ImVec2& rhs ) { return ImVec2( lhs.x + rhs.x, lhs.y + rhs.y ); }
	static inline ImVec2  operator-( const ImVec2& lhs, const ImVec2& rhs ) { return ImVec2( lhs.x - rhs.x, lhs.y - rhs.y ); }
	static inline ImVec2  operator*( const ImVec2& lhs, const ImVec2& rhs ) { return ImVec2( lhs.x * rhs.x, lhs.y * rhs.y ); }
	static inline ImVec2  operator/( const ImVec2& lhs, const ImVec2& rhs ) { return ImVec2( lhs.x / rhs.x, lhs.y / rhs.y ); }
	static inline ImVec2  operator-( const ImVec2& lhs ) { return ImVec2( -lhs.x, -lhs.y ); }
	static inline ImVec2& operator*=( ImVec2& lhs, const float rhs ) { lhs.x *= rhs; lhs.y *= rhs; return lhs; }
	static inline ImVec2& operator/=( ImVec2& lhs, const float rhs ) { lhs.x /= rhs; lhs.y /= rhs; return lhs; }
	static inline ImVec2& operator+=( ImVec2& lhs, const ImVec2& rhs ) { lhs.x += rhs.x; lhs.y += rhs.y; return lhs; }
	static inline ImVec2& operator-=( ImVec2& lhs, const ImVec2& rhs ) { lhs.x -= rhs.x; lhs.y -= rhs.y; return lhs; }
	static inline ImVec2& operator*=( ImVec2& lhs, const ImVec2& rhs ) { lhs.x *= rhs.x; lhs.y *= rhs.y; return lhs; }
	static inline ImVec2& operator/=( ImVec2& lhs, const ImVec2& rhs ) { lhs.x /= rhs.x; lhs.y /= rhs.y; return lhs; }

	extern bool DrawVec2Control(const std::string& rLabel, glm::vec2& values, float resetValue /*= 0.0f*/, bool useColumns /*= true*/, float columnWidth /*= 100.0f */)
{
		bool modified = false;

		ImGuiIO& io = ImGui::GetIO();

		// This is not the bold font this is normal one, I just like it like that.
		auto boldFont = io.Fonts->Fonts[ 0 ];

		ImGui::PushID( rLabel.c_str() );

		if( useColumns )
		{
			ImGui::Columns( 2 );
			ImGui::SetColumnWidth( 0, columnWidth );
			ImGui::Text( rLabel.c_str() );
			ImGui::NextColumn();
		}
		else
		{
			ImGui::SetNextItemWidth( columnWidth );
		}

		ImGui::PushMultiItemsWidths( 3, ImGui::CalcItemWidth() );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 } );

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize ={ lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f } );
		ImGui::PushFont( boldFont );
		if( ( ImGui::Button( "X", buttonSize ) ) )
		{
			values.x = resetValue;
			modified = true;
		}
		ImGui::PopFont();
		ImGui::PopStyleColor( 3 );

		ImGui::SameLine();
		modified |= ImGui::DragFloat( "##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f" );
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f } );
		ImGui::PushFont( boldFont );
		if( ( ImGui::Button( "Y", buttonSize ) ) )
		{
			values.y = resetValue;
			modified = true;
		}
		ImGui::PopFont();
		ImGui::PopStyleColor( 3 );

		ImGui::SameLine();
		modified |= ImGui::DragFloat( "##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f" );
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PopStyleVar();

		if( useColumns )
			ImGui::Columns( 1 );

		ImGui::PopID();

		return modified;
	}

	extern bool DrawVec3Control( const std::string& rLabel, glm::vec3& values, float resetValue /*= 0.0f*/, bool useColumns /*= true*/, float columnWidth /*= 100.0f */ )
	{
		bool modified = false;

		ImGuiIO& io = ImGui::GetIO();

		ImGui::PushID( rLabel.c_str() );


		if /*constexpr*/( useColumns )
		{
			ImGui::Columns( 2 );
			ImGui::SetColumnWidth( 0, columnWidth );
			ImGui::Text( rLabel.c_str() );
			ImGui::NextColumn();
		}
		else
		{
			ImGui::SetNextItemWidth( columnWidth );
		}

		ImGui::PushMultiItemsWidths( 3, ImGui::CalcItemWidth() );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 } );

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize ={ lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f } );
		if( ( ImGui::Button( "X", buttonSize ) ) )
		{
			values.x = resetValue;
			modified = true;
		}
		ImGui::PopStyleColor( 3 );

		ImGui::SameLine();
		modified |= ImGui::DragFloat( "##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f" );
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f } );
		if( ( ImGui::Button( "Y", buttonSize ) ) )
		{
			values.y = resetValue;
			modified = true;
		}
		ImGui::PopStyleColor( 3 );

		ImGui::SameLine();
		modified |= ImGui::DragFloat( "##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f" );
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f } );
		if( ( ImGui::Button( "Z", buttonSize ) ) )
		{
			values.z = resetValue;
			modified = true;
		}
		ImGui::PopStyleColor( 3 );

		ImGui::SameLine();
		modified |= ImGui::DragFloat( "##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f" );
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		if( useColumns )
			ImGui::Columns( 1 );

		ImGui::PopID();

		return modified;
	}

	extern bool DrawColorVec3Control( const std::string& rLabel, glm::vec3& values, float resetValue /*= 0.0f*/, bool useColumns /*= true*/, float columnWidth /*= 100.0f */ )
	{
		bool modified = false;

		ImGui::PushID( rLabel.c_str() );

		ImGui::Text( rLabel.c_str() );

		ImGui::SameLine();

		modified |= ImGui::ColorEdit3( "##picker", glm::value_ptr( values ) );

		ImGui::PopID();

		return modified;
	}

	extern bool DrawFloatControl( const std::string& rLabel, float& values, float min /*= 0.0f*/, float max /*= 500.0f*/, bool useColumns /*= true*/, float columnWidth /*= 125.0f */ )
	{
		bool modified = false;

		ImGuiIO& io = ImGui::GetIO();

		ImGui::PushID( rLabel.c_str() );
		
		if( useColumns )
		{
			ImGui::Columns( 2 );
			ImGui::SetColumnWidth( 0, columnWidth );

			ImGui::Text( rLabel.c_str() );

			ImGui::NextColumn();
		}
		else
		{
			ImGui::SetNextItemWidth( columnWidth );
		}

		ImGui::PushMultiItemsWidths( 1, ImGui::CalcItemWidth() );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 } );

		modified |= ImGui::DragFloat( "##floatx", &values, 0.1f, max, min, "%.2f", ImGuiSliderFlags_AlwaysClamp );
		values = glm::clamp( values, min, max );

		ImGui::PopItemWidth();
		ImGui::PopStyleVar();

		if( useColumns )
			ImGui::Columns( 1 );

		ImGui::PopID();

		return modified;
	}

	extern bool DrawDisabledFloatControl( const std::string& rLabel, float& values, float min /*= 0.0f*/, float max /*= 500.0f*/, bool useColumns /*= true*/, float columnWidth /*= 125.0f */ )
	{
		ScopedItemFlag disabled( ImGuiItemFlags_Disabled, true );
		ScopedStyleVar alpha( ImGuiStyleVar_Alpha, 0.5f );

		return DrawFloatControl( rLabel, values, min, max, useColumns, columnWidth );
	}

	extern bool DrawIntControl( const std::string& rLabel, int& values, int min /*= 0.0f*/, int max /*= 500.0f*/, bool useColumns /*= true*/, float columnWidth /*= 125.0f */ )
	{
		bool modified = false;

		ImGuiIO& io = ImGui::GetIO();

		ImGui::PushID( rLabel.c_str() );

		if( useColumns )
		{
			ImGui::Columns( 2 );
			ImGui::SetColumnWidth( 0, columnWidth );

			ImGui::Text( rLabel.c_str() );
			ImGui::NextColumn();
		}
		else
		{
			ImGui::SetNextItemWidth( columnWidth );
		}

		ImGui::PushMultiItemsWidths( 1, ImGui::CalcItemWidth() );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 } );

		modified |= ImGui::DragInt( "##intx", &values, 0.0f, 5000, 0, "%.2f", ImGuiSliderFlags_AlwaysClamp );
		values = glm::clamp( values, min, max );

		ImGui::PopItemWidth();
		ImGui::PopStyleVar();
		
		if( useColumns )
			ImGui::Columns( 1 );

		ImGui::PopID();

		return modified;
	}

	extern bool DrawDisabledIntControl( const std::string& rLabel, int& values, int min /*= 0.0f*/, int max /*= 500.0f*/, bool useColumns /*= true*/, float columnWidth /*= 125.0f */ )
	{
		ScopedItemFlag disabled( ImGuiItemFlags_Disabled, true );
		ScopedStyleVar alpha( ImGuiStyleVar_Alpha, 0.5f );

		return DrawIntControl( rLabel, values, min, max, useColumns, columnWidth );
	}

	extern bool DrawBoolControl( const std::string& rLabel, bool& value, bool useColumns /*= true*/, float columnWidth /*= 125.0f */ )
	{
		bool modified = false;

		ImGuiIO& io = ImGui::GetIO();

		ImGui::PushID( rLabel.c_str() );

		if( useColumns )
		{
			ImGui::Columns( 2 );
			ImGui::SetColumnWidth( 0, columnWidth );

			ImGui::Text( rLabel.c_str() );

			ImGui::NextColumn();
		}
		else
		{
			ImGui::SetNextItemWidth( columnWidth );
		}

		ImGui::PushMultiItemsWidths( 1, ImGui::CalcItemWidth() );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 } );

		modified |= ImGui::Checkbox( "##boolean", &value );

		ImGui::PopItemWidth();
		ImGui::PopStyleVar();

		if( useColumns )
			ImGui::Columns( 1 );

		ImGui::PopID();

		return modified;
	}

	void DrawDisabledButton( const std::string& rLabel )
	{
		ImGui::PushItemFlag( ImGuiItemFlags_Disabled, true );
		ImGui::PushStyleVar( ImGuiStyleVar_Alpha, 0.5f );
		ImGui::Button( rLabel.c_str() );
		ImGui::PopStyleVar( 1 );
		ImGui::PopItemFlag();
	}

	extern bool DrawDisabledBoolControl( const std::string& rLabel, bool& value, bool useColumns /*= true*/, float columnWidth /*= 125.0f */ )
	{
		ScopedItemFlag disabled( ImGuiItemFlags_Disabled, true );
		ScopedStyleVar alpha( ImGuiStyleVar_Alpha, 0.5f );

		return DrawBoolControl( rLabel, value, useColumns, columnWidth );
	}

	void Image( Ref<Image2D> Image, const ImVec2& Size, const ImVec2& UV0, const ImVec2& UV1, const ImVec4& TintColor, const ImVec4& BorderColor )
	{
		void* TextureID = ImGui_ImplVulkan_AddTexture( Image->GetSampler(), Image->GetImageView(), Image->GetDescriptorInfo().imageLayout );

		ImGui::Image( TextureID, Size, UV0, UV1, TintColor, BorderColor );
	}

	void Image( Ref<Image2D> Image, uint32_t ImageLayer, const ImVec2& Size, const ImVec2& UV0, const ImVec2& UV1, const ImVec4& TintColor, const ImVec4& BorderColor )
	{
		void* TextureID = ImGui_ImplVulkan_AddTexture( Image->GetSampler(), Image->GetImageView( ImageLayer ), Image->GetDescriptorInfo().imageLayout );

		ImGui::Image( TextureID, Size, UV0, UV1, TintColor, BorderColor );
	}

	void Image( Ref<Texture2D> Image, const ImVec2& Size, const ImVec2& UV0, const ImVec2& UV1, const ImVec4& TintColor, const ImVec4& BorderColor )
	{
		void* TextureID = ImGui_ImplVulkan_AddTexture( Image->GetSampler(), Image->GetImageView(), Image->GetDescriptorInfo().imageLayout );

		ImGui::Image( TextureID, Size, UV0, UV1, TintColor, BorderColor );
	}

	void Image( Ref<Texture2D> Image, uint32_t Mip, const ImVec2& Size, const ImVec2& UV0, const ImVec2& UV1, const ImVec4& TintColor, const ImVec4& BorderColor )
	{
		void* TextureID = ImGui_ImplVulkan_AddTexture( Image->GetSampler(), Image->GetOrCreateMipImageView( Mip ), Image->GetDescriptorInfo().imageLayout );

		ImGui::Image( TextureID, Size, UV0, UV1, TintColor, BorderColor );
	}

	bool ImageButton( Ref< Image2D > Image, const ImVec2& Size, const ImVec2& UV0, const ImVec2& UV1, int FramePadding, const ImVec4& BackgroundColor, const ImVec4& TintColor )
	{
		void* TextureID = ImGui_ImplVulkan_AddTexture( Image->GetSampler(), Image->GetImageView(), Image->GetDescriptorInfo().imageLayout );

		return ImGui::ImageButton( TextureID, Size, UV0, UV1, FramePadding, BackgroundColor, TintColor );
	}

	bool ImageButton( Ref< Texture2D > Image, const ImVec2& Size, const ImVec2& UV0, const ImVec2& UV1, int FramePadding, const ImVec4& BackgroundColor, const ImVec4& TintColor )
	{
		void* TextureID = ImGui_ImplVulkan_AddTexture( Image->GetSampler(), Image->GetImageView(), Image->GetDescriptorInfo().imageLayout );

		return ImGui::ImageButton( TextureID, Size, UV0, UV1, FramePadding, BackgroundColor, TintColor );
	}

	bool TreeNode( const std::string& rLabel, bool open /*= true */ )
	{
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;

		if( open )
			flags |= ImGuiTreeNodeFlags_DefaultOpen;

		return ImGui::TreeNodeEx( rLabel.c_str(), flags );
	}

	void EndTreeNode()
	{
		ImGui::TreePop();
	}

	bool ButtonRd( const char* rLabel, const ImRect& bb, bool rounded /*= false */ )
	{
		using namespace ImGui;

		ImGuiButtonFlags flags = ImGuiButtonFlags_None;

		ImGuiWindow* window = GetCurrentWindow();
		if( window->SkipItems )
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID( rLabel );
		const ImVec2 label_size = CalcTextSize( rLabel, NULL, true );

		ImVec2 pos = window->DC.CursorPos;
		if( ( flags & ImGuiButtonFlags_AlignTextBaseLine ) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset ) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
			pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;

		ImVec2 size = CalcItemSize( bb.Min, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f );

		ItemSize( size, style.FramePadding.y );
		if( !ItemAdd( bb, id ) )
			return false;

		if( g.LastItemData.InFlags & ImGuiItemFlags_ButtonRepeat )
			flags |= ImGuiButtonFlags_Repeat;

		bool hovered, held;
		bool pressed = ButtonBehavior( bb, id, &hovered, &held, flags );

		// Render
		const ImU32 col = GetColorU32( ( held && hovered ) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button );
		RenderNavHighlight( bb, id );

		// RenderFrame
		{
			if( rounded )
			{
				window->DrawList->AddRect( bb.Min, bb.Max, col, 5.0f, ImDrawFlags_RoundCornersAll );
			}
			else
				RenderFrame( bb.Min, bb.Max, col, true, style.FrameRounding );
		}

		if( g.LogEnabled )
			LogSetNextTextDecoration( "[", "]" );
		RenderTextClipped( bb.Min + style.FramePadding, bb.Max - style.FramePadding, rLabel, NULL, &label_size, style.ButtonTextAlign, &bb );

		// Automatically close popups
		//if (pressed && !(flags & ImGuiButtonFlags_DontClosePopups) && (window->Flags & ImGuiWindowFlags_Popup))
		//    CloseCurrentPopup();

		IMGUI_TEST_ENGINE_ITEM_INFO( id, rLabel, g.LastItemData.StatusFlags );
		return pressed;
	}

	void DrawColoredRect( const ImVec2& size, const ImVec4& color )
	{
		// Context
		ImGuiContext& g = *GImGui;

		const float DefualtSize = ImGui::GetFrameHeight();
		const ImVec2 RealSize( size.x == 0.0f ? DefualtSize : size.x, size.y == 0.0f ? DefualtSize : size.y );

		const ImRect BoundingBox( ImGui::GetCursorPos(), ImGui::GetCursorPos() + size );

		ImGui::ItemSize( BoundingBox, (size.y >= DefualtSize) ? g.Style.FramePadding.y : 0.0f );
		ImGui::ItemAdd( BoundingBox, ImGui::GetID( "colored_rect" ) );

		float grid_step = ImMin( size.x, size.y ) / 2.99f;
		float rounding = ImMin( g.Style.FrameRounding, grid_step * 0.5f );

		// Draw the rect
		ImDrawList* pDrawList = ImGui::GetWindowDrawList();

		ImVec4 ColorNoAlpha = ImVec4( color.x, color.y, color.z, 1.0f );
		pDrawList->AddRectFilled( BoundingBox.Min, BoundingBox.Max, ImGui::GetColorU32( ColorNoAlpha ), rounding );
	}

	bool DrawAssetFinder( AssetType type, bool* rOpen, AssetID& rOut, AssetID ignoreID )
	{
		bool Modified = false;

		if( *rOpen == true )
		{
			ImGui::OpenPopup( "AssetFinderPopup" );
			*rOpen = false;
		}

		ImGui::SetNextWindowSize( { 250.0f, 0.0f } );
		if( ImGui::BeginPopup( "AssetFinderPopup", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize ) )
		{
			bool PopupModified = false;

			if( ImGui::BeginListBox( "##ASSETLIST", ImVec2( -FLT_MIN, 0.0f ) ) )
			{
				for( const auto& [assetID, rAsset] : AssetManager::Get()->GetCombinedAssetMap() )
				{
					bool Selected = ( rOut == assetID );

					ImGui::PushID( static_cast< int >( assetID ) );

					if( ( rAsset->Type == type || type == AssetType::Unknown ) && assetID != ignoreID )
					{
						if( ImGui::Selectable( rAsset->Name.c_str(), Selected ) )
						{
							rOut = assetID;

							PopupModified = true;
						}
					}

					ImGui::PopID();

					if( Selected )
						ImGui::SetItemDefaultFocus();
				}

				ImGui::EndListBox();
			}

			if( PopupModified )
			{
				ImGui::CloseCurrentPopup();

				Modified = true;
				*rOpen = false;
			}

			ImGui::EndPopup();
		}

		return Modified;
	}

	bool DrawAssetFinder( const std::set<AssetType>& rAllowedTypes, bool* rOpen, AssetID& rOut )
	{
		bool Modified = false;

		if( *rOpen == true )
		{
			ImGui::OpenPopup( "AssetFinderPopup" );
			*rOpen = false;
		}

		ImGui::SetNextWindowSize( { 250.0f, 0.0f } );
		if( ImGui::BeginPopup( "AssetFinderPopup", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize ) )
		{
			bool PopupModified = false;

			if( ImGui::BeginListBox( "##ASSETLIST", ImVec2( -FLT_MIN, 0.0f ) ) )
			{
				// TODO: Change with editor.
				for( const auto& [assetID, rAsset] : AssetManager::Get()->GetCombinedAssetMap() )
				{
					bool Selected = ( rOut == assetID );

					ImGui::PushID( static_cast< int >( assetID ) );

					if( rAllowedTypes.find( rAsset->Type ) != rAllowedTypes.end() )
					{
						if( ImGui::Selectable( rAsset->Name.c_str(), Selected ) )
						{
							rOut = assetID;

							PopupModified = true;
						}
					}

					ImGui::PopID();

					if( Selected )
						ImGui::SetItemDefaultFocus();
				}

				ImGui::EndListBox();
			}

			if( PopupModified )
			{
				ImGui::CloseCurrentPopup();

				Modified = true;
				*rOpen = false;
			}

			ImGui::EndPopup();
		}

		return Modified;
	}

	static bool DrawScalarControl( const std::string& rLabel, void* values, int min /*= 0.0f*/, int max /*= 500.0f*/, bool useColumns, float columnWidth /*= 125.0f */, ImGuiDataType_ Type )
	{
		bool modified = false;

		ImGuiIO& io = ImGui::GetIO();

		ImGui::PushID( rLabel.c_str() );

		if( useColumns )
		{
			ImGui::Columns( 2 );
			ImGui::SetColumnWidth( 0, columnWidth );

			ImGui::Text( rLabel.c_str() );

			ImGui::NextColumn();
		}
		else
		{
			ImGui::SetNextItemWidth( columnWidth );
		}

		ImGui::PushMultiItemsWidths( 1, ImGui::CalcItemWidth() );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 } );

		modified |= ImGui::DragScalarN( "##sclx", Type, &values, 1, 0.0f, &min, &max, "%.2f", ImGuiSliderFlags_AlwaysClamp );

		ImGui::PopItemWidth();
		ImGui::PopStyleVar();

		if( useColumns )
			ImGui::Columns( 1 );

		ImGui::PopID();

		return modified;
	}

	bool DrawUInt8Control( const std::string& rLabel, uint8_t& values, int min /*= 0.0f*/, int max /*= 500.0f*/, float columnWidth /*= 125.0f */ )
	{
		bool modified = DrawScalarControl( rLabel, &values, min, max, true, columnWidth, ImGuiDataType_U8 );
		values = glm::clamp( values, ( uint8_t ) min, ( uint8_t ) max );

		return modified;
	}

	bool DrawUInt16Control( const std::string& rLabel, uint16_t& values, int min /*= 0.0f*/, int max /*= 500.0f*/, float columnWidth /*= 125.0f */ )
	{
		bool modified = DrawScalarControl( rLabel, &values, min, max, true, columnWidth, ImGuiDataType_U16 );
		values = glm::clamp( values, ( uint16_t ) min, ( uint16_t ) max );

		return modified;
	}

	bool DrawUInt64Control( const std::string& rLabel, uint64_t& values, int min /*= 0.0f*/, int max /*= 500.0f*/, float columnWidth /*= 125.0f */ )
	{
		bool modified = DrawScalarControl( rLabel, &values, min, max, true, columnWidth, ImGuiDataType_U64 );
		values = glm::clamp( values, ( uint64_t ) min, ( uint64_t ) max );

		return modified;
	}

	//////////////////////////////////////////////////////////////////////////
	
	bool DrawInt8Control( const std::string& rLabel, int8_t& values, int min /*= 0.0f*/, int max /*= 500.0f*/, float columnWidth /*= 125.0f */ )
	{
		bool modified = DrawScalarControl( rLabel, &values, min, max, true, columnWidth, ImGuiDataType_S8 );
		values = glm::clamp( values, ( int8_t ) min, ( int8_t ) max );

		return modified;
	}

	bool DrawInt16Control( const std::string& rLabel, int16_t& values, int min /*= 0.0f*/, int max /*= 500.0f*/, float columnWidth /*= 125.0f */ )
	{
		bool modified = DrawScalarControl( rLabel, &values, min, max, true, columnWidth, ImGuiDataType_S16 );
		values = glm::clamp( values, ( int16_t ) min, ( int16_t ) max );

		return modified;
	}

	bool DrawInt64Control( const std::string& rLabel, int64_t& values, int min /*= 0.0f*/, int max /*= 500.0f*/, float columnWidth /*= 125.0f */ )
	{
		bool modified = DrawScalarControl( rLabel, &values, min, max, true, columnWidth, ImGuiDataType_S64 );
		values = glm::clamp( values, ( int64_t ) min, ( int64_t ) max );

		return modified;
	}

	//////////////////////////////////////////////////////////////////////////

	extern bool DrawDoubleControl( const std::string& rLabel, double& values, int min /*= 0.0f*/, int max /*= 500.0f*/, bool useColumns /*= true*/, float columnWidth /*= 125.0f */ )
	{
		bool modified = DrawScalarControl( rLabel, &values, min, max, useColumns, columnWidth, ImGuiDataType_Double );
		values = glm::clamp( values, ( double ) min, ( double ) max );

		return modified;
	}

	//////////////////////////////////////////////////////////////////////////

	ImVec2 GetClosestPointOnRectBorder( const ImRect& rRect, const ImVec2& rPoint )
	{
		ImVec2 const points[ 4 ] =
		{
			ImLineClosestPoint( rRect.GetTL(), rRect.GetTR(), rPoint ),
			ImLineClosestPoint( rRect.GetBL(), rRect.GetBR(), rPoint ),
			ImLineClosestPoint( rRect.GetTL(), rRect.GetBL(), rPoint ),
			ImLineClosestPoint( rRect.GetTR(), rRect.GetBR(), rPoint )
		};

		float distancesSq[ 4 ] =
		{
			ImLengthSqr( points[ 0 ] - rPoint ),
			ImLengthSqr( points[ 1 ] - rPoint ),
			ImLengthSqr( points[ 2 ] - rPoint ),
			ImLengthSqr( points[ 3 ] - rPoint )
		};

		float lowestDistance = FLT_MAX;
		int32_t closestPointIdx = -1;
		for( auto i = 0; i < 4; i++ )
		{
			if( distancesSq[ i ] < lowestDistance )
			{
				closestPointIdx = i;
				lowestDistance = distancesSq[ i ];
			}
		}

		return points[ closestPointIdx ];
	}

	ImVec2 GetConnectionPointBetweenRectAndPoint( const ImRect& rRect, const ImVec2& rEndPoint )
	{
		ImVec2 startPoint = rRect.GetCenter();
		ImVec2 const midPoint = startPoint + ( ( rEndPoint - startPoint ) * 0.5f );

		return GetClosestPointOnRectBorder( rRect, midPoint );
	}

	void GetConnectionPointsBetweenRects( const ImRect& rStartRect, const ImRect& rEndRect, ImVec2& rStartPoint, ImVec2& rEndPoint )
	{
		rStartPoint = rStartRect.GetCenter();
		rEndPoint = rEndRect.GetCenter();
		ImVec2 const midPoint = rStartPoint + ( ( rEndPoint - rStartPoint ) * 0.5f );

		rStartPoint = GetClosestPointOnRectBorder( rStartRect, midPoint );
		rEndPoint = GetClosestPointOnRectBorder( rEndRect, midPoint );
	}

	void DrawArrow( const ImVec2& rStartPoint, const ImVec2& rEndPoint, ImU32 color, float thinkness, float headSize )
	{
		auto* pDrawList = ImGui::GetWindowDrawList();

		ImVec2 arrowDir = ImVec2( rEndPoint - rStartPoint );
		const float length = glm::sqrt( ImLengthSqr( arrowDir ) );

		if( length > 0.0 )
		{
			// Normalise.
			arrowDir /= length;
		}

		const ImVec2 ortho = ImVec2( arrowDir.y, -arrowDir.x );
		const ImVec2 arrowBegin = ImVec2( rEndPoint.x - arrowDir.x * headSize, rEndPoint.y - arrowDir.y * headSize );

		const float headWidthTri = headSize * 0.6f * 0.5f;
		const ImVec2 l( arrowBegin.x + ortho.x * headWidthTri, arrowBegin.y + ortho.y * headWidthTri );
		const ImVec2 r( arrowBegin.x - ortho.x * headWidthTri, arrowBegin.y - ortho.y * headWidthTri );

		// Draw shaft
		pDrawList->AddLine( rStartPoint, arrowBegin, color, thinkness );
		pDrawList->AddTriangleFilled( rEndPoint, l, r, color );
	}

	void DrawArrowOffset( ImVec2 rStartPoint, ImVec2 rEndPoint, ImU32 color, float thinkness, float headSize, float offset )
	{
		auto* pDrawList = ImGui::GetWindowDrawList();

		ImVec2 arrowDir = ImVec2( rEndPoint - rStartPoint );
		float arrowLength = glm::sqrt( ImLengthSqr( arrowDir ) );
		if( arrowLength <= 1e-6f )
			return;

		// Normalize direction
		arrowDir.x /= arrowLength;
		arrowDir.y /= arrowLength;

		// Perpendicular
		const ImVec2 orthogonal = { arrowDir.y, -arrowDir.x };

		const ImVec2 offsetVec = orthogonal * offset;
		rStartPoint += offsetVec;
		rEndPoint += offsetVec;

		const ImVec2 base = rEndPoint - arrowDir * headSize;

		// Left and right points of arrowhead
		const ImVec2 left = base + orthogonal * headSize;
		const ImVec2 right = base - orthogonal * headSize;

		// Draw shaft
		pDrawList->AddLine( rStartPoint, base, color, thinkness );
		pDrawList->AddTriangleFilled( rEndPoint, left, right, color );
	}

	//////////////////////////////////////////////////////////////////////////
	// PORTED FROM imgui_stblib.cpp

	struct InputTextCallback_UserData
	{
		std::string*			pStr;
		ImGuiInputTextCallback  ChainCallback;
		void*					pChainCallbackUserData;
	};

	static int InputTextCallback( ImGuiInputTextCallbackData* pData )
	{
		InputTextCallback_UserData* pUserData = ( InputTextCallback_UserData* ) pData->UserData;

		if( pData->EventFlag == ImGuiInputTextFlags_CallbackResize )
		{
			// Resize string callback
			// If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
			std::string* pStr = pUserData->pStr;
			IM_ASSERT( pData->Buf == pStr->c_str() );

			pStr->resize( pData->BufTextLen );
			pData->Buf = ( char* ) pStr->c_str();
		}
		else if( pUserData->ChainCallback )
		{
			// Forward to user callback, if any
			pData->UserData = pUserData->pChainCallbackUserData;
			return pUserData->ChainCallback( pData );
		}

		return 0;
	}

	bool InputText( const char* label, std::string* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data )
	{
		IM_ASSERT( ( flags & ImGuiInputTextFlags_CallbackResize ) == 0 );
		flags |= ImGuiInputTextFlags_CallbackResize;

		InputTextCallback_UserData userDataCB{};
		userDataCB.pStr = str;
		userDataCB.ChainCallback = callback;
		userDataCB.pChainCallbackUserData = user_data;
		
		return ImGui::InputText( label, ( char* ) str->c_str(), str->capacity() + 1, flags, InputTextCallback, &userDataCB );
	}

}
