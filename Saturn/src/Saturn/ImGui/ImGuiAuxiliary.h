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

#include "Saturn/Vulkan/Image2D.h"
#include "Saturn/Vulkan/Texture.h"

#include "Saturn/Asset/AssetManager.h"

#include <string>
#include <glm/glm.hpp>

#include <imgui.h>
#include <imgui_internal.h>
#include <set>

namespace Saturn::Auxiliary {
	
	template<typename Ty>
	class ScopedStyleVar
	{
	public:
		ScopedStyleVar( ImGuiStyleVar_ styleVar, const Ty& rValue )
		{
			static_assert( std::is_same<Ty, float>() || std::is_same<Ty, ImVec2>(), "Ty must be float or ImVec2!" );
			
			ImGui::PushStyleVar( styleVar, rValue );
		}

		~ScopedStyleVar()
		{
			ImGui::PopStyleVar();
		}
	};

	class ScopedStyleColor
	{
	public:
		ScopedStyleColor( ImGuiCol_ color, const ImVec4& rValue )
		{
			ImGui::PushStyleColor( color, rValue );
		}

		ScopedStyleColor( ImGuiCol_ color, ImU32 value )
		{
			ImGui::PushStyleColor( color, value );
		}

		~ScopedStyleColor()
		{
			ImGui::PopStyleColor();
		}
	};

	class ScopedItemFlag
	{
	public:
		ScopedItemFlag( ImGuiItemFlags_ itemFlag, bool value )
		{
			ImGui::PushItemFlag( itemFlag, value );
		}

		~ScopedItemFlag()
		{
			ImGui::PopItemFlag();
		}
	};

	class ScopedDisabledFlag 
	{
	public:
		ScopedDisabledFlag( bool value )
		{
			ImGui::PushStyleVar( ImGuiStyleVar_Alpha, value ? 0.5f : 1.0f );
			ImGui::PushItemFlag( ImGuiItemFlags_Disabled, value );
		}

		~ScopedDisabledFlag()
		{
			ImGui::PopStyleVar();
			ImGui::PopItemFlag();
		}
	};

	class DisabledFlag
	{
	public:
		DisabledFlag( bool value )
		{
			ImGui::PushStyleVar( ImGuiStyleVar_Alpha, value ? 0.5f : 1.0f );
			ImGui::PushItemFlag( ImGuiItemFlags_Disabled, value );
		}

		void Pop()
		{
			ImGui::PopStyleVar();
			ImGui::PopItemFlag();
		}
	};

	// NOTE: When not using columns you MUST draw the text before calling this function!
	extern bool DrawVec3Control( const std::string& rLabel, glm::vec3& values, float resetValue = 0.0f, bool useColumns = true, float columnWidth = 100.0f );
	extern bool DrawVec2Control( const std::string& rLabel, glm::vec2& values, float resetValue = 0.0f, bool useColumns = true, float columnWidth = 100.0f );

	// NOTE: When not using columns you MUST draw the text before calling this function!
	extern bool DrawColorVec3Control( const std::string& rLabel, glm::vec3& values, float resetValue = 0.0f, bool useColumns = true, float columnWidth = 100.0f );

	// NOTE: When not using columns you MUST draw the text before calling this function!
	extern bool DrawFloatControl( const std::string& rLabel, float& values, float min = 0.0f, float max = 500.0f, bool useColumns = true, float columnWidth = 125.0f );
	extern bool DrawDisabledFloatControl( const std::string& rLabel, float& values, float min = 0.0f, float max = 500.0f, bool useColumns = true, float columnWidth = 125.0f );

	// NOTE: When not using columns you MUST draw the text before calling this function!
	extern bool DrawIntControl( const std::string& rLabel, int& values, int min = 0.0f, int max = 500.0f, bool useColumns = true, float columnWidth = 125.0f );
	extern bool DrawDisabledIntControl( const std::string& rLabel, int& values, int min = 0.0f, int max = 500.0f, bool useColumns = true, float columnWidth = 125.0f );
	
	// NOTE: When not using columns you MUST draw the text before calling this function!
	extern bool DrawBoolControl( const std::string& rLabel, bool& value, bool useColumns = true, float columnWidth = 125.0f );
	extern bool DrawDisabledBoolControl( const std::string& rLabel, bool& value, bool useColumns = true, float columnWidth = 125.0f );
	
	extern bool DrawInt8Control( const std::string& rLabel, uint8_t& values, int min = 0.0f, int max = 500.0f, float columnWidth = 125.0f );
	extern bool DrawInt16Control( const std::string& rLabel, uint16_t& values, int min = 0.0f, int max = 500.0f, float columnWidth = 125.0f );
	extern bool DrawInt64Control( const std::string& rLabel, uint64_t& values, int min = 0.0f, int max = 500.0f, float columnWidth = 125.0f );

	extern bool DrawUInt8Control( const std::string& rLabel, int8_t& values, int min = 0.0f, int max = 500.0f, float columnWidth = 125.0f );
	extern bool DrawUInt16Control( const std::string& rLabel, int16_t& values, int min = 0.0f, int max = 500.0f, float columnWidth = 125.0f );
	extern bool DrawUInt64Control( const std::string& rLabel, int32_t& values, int min = 0.0f, int max = 500.0f, float columnWidth = 125.0f );
	
	// NOTE: When not using columns you MUST draw the text before calling this function!
	extern bool DrawDoubleControl( const std::string& rLabel, double& values, int min = 0.0f, int max = 500.0f, bool useColumns = true, float columnWidth = 125.0f );

	extern void DrawDisabledButton( const std::string& rLabel );

	extern void Image( Ref< Image2D > Image, const ImVec2& Size, const ImVec2& UV0 = ImVec2( 0, 1 ), const ImVec2& UV1 = ImVec2( 1, 0 ), const ImVec4& TintColor = ImVec4( 1, 1, 1, 1 ), const ImVec4& BorderColor = ImVec4( 0, 0, 0, 0 ) );
	
	extern void Image( Ref< Texture2D > Image, const ImVec2& Size, const ImVec2& UV0 = ImVec2( 0, 1 ), const ImVec2& UV1 = ImVec2( 1, 0 ), const ImVec4& TintColor = ImVec4( 1, 1, 1, 1 ), const ImVec4& BorderColor = ImVec4( 0, 0, 0, 0 ) );
	
	extern void Image( Ref< Texture2D > Image, uint32_t Mip, const ImVec2& Size, const ImVec2& UV0 = ImVec2( 0, 1 ), const ImVec2& UV1 = ImVec2( 1, 0 ), const ImVec4& TintColor = ImVec4( 1, 1, 1, 1 ), const ImVec4& BorderColor = ImVec4( 0, 0, 0, 0 ) );

	extern void Image( Ref<Image2D> Image, uint32_t ImageLayer, const ImVec2& Size, const ImVec2& UV0 = ImVec2( 0, 1), const ImVec2& UV1 = ImVec2( 1, 0 ), const ImVec4& TintColor = ImVec4( 1, 1, 1, 1 ), const ImVec4& BorderColor = ImVec4( 0, 0, 0, 0 ) );

	extern bool ImageButton( Ref< Image2D > Image, const ImVec2& Size, const ImVec2& UV0 = ImVec2( 0, 1 ), const ImVec2& UV1 = ImVec2( 1, 0 ), int FramePadding = -1, const ImVec4& BackgroundColor = ImVec4( 0, 0, 0, 0 ), const ImVec4& TintColor = ImVec4( 1, 1, 1, 1 ) );
	
	extern bool ImageButton( Ref< Texture2D > Image, const ImVec2& Size, const ImVec2& UV0 = ImVec2( 0, 1 ), const ImVec2& UV1 = ImVec2( 1, 0 ), int FramePadding = -1, const ImVec4& BackgroundColor = ImVec4( 0, 0, 0, 0 ), const ImVec4& TintColor = ImVec4( 1, 1, 1, 1 ) );

	extern bool TreeNode( const std::string& rLabel, bool open = true );
	extern void EndTreeNode();

	extern bool ButtonRd( const char* rLabel, const ImRect& bb, bool rounded = false );

	extern void DrawColoredRect( const ImVec2& size, const ImVec4& color );

	extern bool DrawAssetFinder( AssetType type, bool* rOpen, AssetID& rOut, AssetID ignoreID = 0 );
	extern bool DrawAssetFinder( const std::set<AssetType>& rAllowedTypes, bool* rOpen, AssetID& rOut );

	extern bool InputText( const char* label, std::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr );

	extern ImVec2 GetClosestPointOnRectBorder( const ImRect& rRect, const ImVec2& rPoint );

	extern ImVec2 GetConnectionPointBetweenRectAndPoint( const ImRect& rRect, const ImVec2& rEndPoint );

	extern void GetConnectionPointsBetweenRects( const ImRect& rStartRect, const ImRect& rEndRect, ImVec2& rStartPoint, ImVec2& rEndPoint );

	extern void DrawArrow( const ImVec2& rStartPoint, const ImVec2& rEndPoint, ImU32 color, float thinkness = 2.0f, float headSize = 12.0f );

	extern void DrawArrowOffset( ImVec2 rStartPoint, ImVec2 rEndPoint, ImU32 color, float thinkness = 2.0f, float headSize = 12.0f, float offset = 5.0f );

	template<typename Function>
	inline bool DrawAssetFinder( AssetType allowedTypes, AssetID lastID, bool* rOpen, Function&& rrFunction )
	{
		bool Modified = false;

		if( *rOpen == true && !ImGui::IsPopupOpen( "AssetFinderPopup" ) )
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
				for( const auto& [assetID, rAsset] : AssetManager::Get().GetAssetRegistry()->GetAssetMap() )
				{
					bool Selected = ( lastID == assetID );

					ImGui::PushID( static_cast< int >( assetID ) );

					if( rAsset->GetAssetType() == allowedTypes || allowedTypes == AssetType::Unknown )
					{
						if( ImGui::Selectable( rAsset->Name.c_str(), Selected ) )
						{
							rrFunction( rAsset );

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

	template<typename Ty, typename Fn>
	bool DrawAssetDragDropTarget( const char* rLabel, const char* assetName, UUID& returnID, Fn&& function ) 
	{
		if( returnID == 0 )
			return false;

		bool changed = false;

		ImGui::Text( rLabel );

		std::string ButtonName = "";

		if( auto asset = AssetManager::Get().FindAsset( returnID ) )
			ButtonName = asset->Name;
		else
			ButtonName = "Unknown";

		ImGui::Button( ButtonName.c_str() );

		if( ImGui::BeginDragDropTarget() )
		{
			auto data = ImGui::AcceptDragDropPayload( "asset_payload" );

			if( data )
			{
				const wchar_t* path = ( const wchar_t* ) data->Data;

				Ref<Asset> asset = AssetManager::Get().GetAssetAs<Asset>( AssetManager::Get().PathToID( path ) );

				if( asset )
				{
					function( asset.As<Ty>() );
					changed = true;
				}
			}
		}

		return changed;
	}

}
