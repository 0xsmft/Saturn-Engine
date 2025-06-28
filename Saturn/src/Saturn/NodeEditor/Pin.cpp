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
#include "Pin.h"

#include "Saturn/Serialisation/RawSerialisation.h"
#include "Saturn/NodeEditor/NodeEditorBlueprintNode.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"

#include "NodeEditorAux.h"
#include "builders.h"

namespace Saturn {

	Pin::Pin( const std::string& rName, PinType type, PinKind kind )
		: Name( rName ), Type( type ), Kind( kind ), Node( nullptr ), ID()
	{
	}

	Pin::Pin( UUID id, const std::string& rName, PinType type, UUID nodeID )
		: Node( nullptr ), Name( rName ), Type( type ), Kind( PinKind::Input ), ID( id )
	{
	}

	PinIconType Pin::GetIconType() const
	{
		switch( Type )
		{
			case PinType::Flow:				  return PinIconType::Flow;
			case PinType::Delegate:           return PinIconType::Square;
			
			case PinType::Bool:				  
			case PinType::Int:				  
			case PinType::Float:			  
			case PinType::String:			  
			case PinType::Object:			  
			case PinType::Function:			  
			case PinType::Material_Color:
			case PinType::Material_TextureColor:
			case PinType::AssetID:            return PinIconType::Circle;
		}

		return PinIconType::Circle;
	}

	ImColor Pin::GetPinColor() const
	{
		switch( Type )
		{
			default:
			case PinType::Flow:                  return ImColor( 255, 255, 255 );
			case PinType::Bool:                  return ImColor( 220, 48, 48 );
			case PinType::Int:                   return ImColor( 68, 201, 156 );
			case PinType::Float:                 return ImColor( 147, 226, 74 );
			case PinType::String:                return ImColor( 124, 21, 153 );
			case PinType::Object:                return ImColor( 51, 150, 215 );
			case PinType::Function:              return ImColor( 218, 0, 183 );
			case PinType::Delegate:              return ImColor( 255, 48, 48 );
			case PinType::AssetID:               return ImColor( 0, 0, 255 );
			case PinType::Material_Color:        return ImColor( 142, 61, 186 );
			case PinType::Material_TextureColor: return ImColor( 142, 61, 186 );
		}

		return ImColor( 0, 0, 255 );
	}

	void Pin::DrawIcon( bool connected, int alpha ) const
	{
		constexpr float PIN_ICON_SIZE = 24.0f;
		constexpr ImVec2 size = ImVec2( PIN_ICON_SIZE, PIN_ICON_SIZE );

		if( ImGui::IsRectVisible( size ) )
		{
			const auto rendererIcon = GetIconType();
			ImColor color = GetPinColor();
			color.Value.w = alpha / 255.0f;

			const auto cursorPos = ImGui::GetCursorScreenPos();
			auto drawList = ImGui::GetWindowDrawList();

			Auxiliary::DrawPinIconInternal( drawList,
				cursorPos,
				cursorPos + size,
				rendererIcon,
				connected,
				color, ImColor( 32, 32, 32, alpha ) );
		}

		ImGui::Dummy( size );
	}

	void Pin::Render( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked, uint32_t pinIndex )
	{
		switch( Kind )
		{
			case PinKind::Output: 
			{
				RenderOutput( rBuilder, linked );
			} break;

			case PinKind::Input:
			{
				RenderInput( rBuilder, linked, pinIndex );
			} break;
		}
	}

	void Pin::RenderInput( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked, uint32_t pinIndex )
	{
		switch( RenderType )
		{
			case PinRenderType::Blueprint:
			{
				RenderBlueprintInput( rBuilder, linked, pinIndex );
			} break;

			case PinRenderType::Tree:
			{
				RenderTreeInput( rBuilder, linked, pinIndex );
			} break;
		}
	}

	void Pin::RenderOutput( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked )
	{
		switch( RenderType )
		{
			case PinRenderType::Blueprint:
			{
				RenderBlueprintOutput( rBuilder, linked );
			} break;

			case PinRenderType::Tree:
			{
				RenderTreeOutput( rBuilder, linked );
			} break;
		}
	}

	void Pin::RenderBlueprintOutput( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked )
	{
		const auto alpha = ImGui::GetStyle().Alpha;

		ImGui::PushStyleVar( ImGuiStyleVar_Alpha, alpha );

		rBuilder.Output( ed::PinId( ID ) );

		if( !Name.empty() )
		{
			ImGui::Spring( 0 );
			ImGui::TextUnformatted( Name.c_str() );

			OnRenderOutput();
		}

		ImGui::Spring( 0 );
		DrawIcon( linked, ( int ) ( alpha * 255 ) );

		rBuilder.EndOutput();
		ImGui::PopStyleVar();
	}

	void Pin::RenderTreeOutput( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked )
	{
		const auto alpha = ImGui::GetStyle().Alpha;
		const ImRect itemRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );

		ed::PushStyleVar( ed::StyleVar_PinCorners, ImDrawFlags_RoundCornersTop );

		ed::BeginPin( ed::PinId( ID ), ed::PinKind::Output );

		ed::PinPivotRect( itemRect.GetTL(), itemRect.GetBR() );
		ed::PinRect( itemRect.GetTL(), itemRect.GetBR() );
		// No need to hand off to children, tree pins don't need extra drawing.
		ed::EndPin();
		ed::PopStyleVar();
	}

	void Pin::RenderBlueprintInput( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked, uint32_t pinIndex )
	{
		const auto alpha = ImGui::GetStyle().Alpha;

		rBuilder.Input( ed::PinId( ID ) );

		ImGui::PushStyleVar( ImGuiStyleVar_Alpha, alpha );

		DrawIcon( linked, ( int ) ( alpha * 255 ) );

		ImGui::Spring( 0 );

		if( !Name.empty() )
		{
			ImGui::TextUnformatted( Name.c_str() );
			ImGui::Spring( 0 );
		}

#if !defined(SAT_DEBUG)
		// Hand off to children only if not linked
		if( !linked )
#endif
			OnRenderInput();

		ImGui::Spring( 0 );

		ImGui::PopStyleVar();

		rBuilder.EndInput();
	}

	void Pin::RenderTreeInput( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked, uint32_t pinIndex )
	{
		const auto alpha = ImGui::GetStyle().Alpha;
		const ImRect itemRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );

		// Set pin style and spacing
		ed::PushStyleVar( ed::StyleVar_PinArrowSize, 10.0f );
		ed::PushStyleVar( ed::StyleVar_PinArrowWidth, 10.0f );
		ed::PushStyleVar( ed::StyleVar_PinCorners, ImDrawFlags_RoundCornersBottom );
	
		ed::BeginPin( ed::PinId( ID ), ed::PinKind::Input );
		ed::PinPivotRect( itemRect.GetTL(), itemRect.GetBR() );
		ed::PinRect( itemRect.GetTL(), itemRect.GetBR() );
		// No need to hand off to children, tree pins don't need extra drawing.
		ed::EndPin();
		ed::PopStyleVar( 3 );
	}

	bool Pin::CanCreateLink( const Ref<Pin>& rOther ) const
	{
		if( !rOther || Kind == rOther->Kind || Type != rOther->Type || Node == rOther->Node )
			return false;

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// SERIAILSATION

	void Pin::Serialise( const Ref<Pin>& rObject, std::ofstream& rStream )
	{
		RawSerialisation::WriteObject( rObject->ID, rStream );

		RawSerialisation::WriteString( rObject->Name, rStream );
		RawSerialisation::WriteObject( rObject->Type, rStream );
		RawSerialisation::WriteObject( rObject->Kind, rStream );

		rObject->OnSerialise( rStream );
	}

	void Pin::Deserialise( Ref<Pin>& rObject, IStream& rStream )
	{
		RawSerialisation::ReadObject( rObject->ID, rStream );

		rObject->Name = RawSerialisation::ReadString( rStream );
		RawSerialisation::ReadObject( rObject->Type, rStream );
		RawSerialisation::ReadObject( rObject->Kind, rStream );

		rObject->OnDeserialise( rStream );
	}

	//////////////////////////////////////////////////////////////////////////
	// FLOAT PIN

	FloatPin::FloatPin( UUID id, const std::string& rName, PinType type, UUID nodeID )
		: Pin( id, rName, type, nodeID )
	{
	}

	FloatPin::FloatPin( const std::string& rName, PinKind kind )
		: Pin( rName, PinType::Float, kind )
	{
	}

	void FloatPin::OnRenderInput()
	{
		ImGui::SetNextItemWidth( 25.0f );

		ImGui::PushID( static_cast< int >( ID ) );

		ImGui::DragFloat( "##input", &Data );

		ImGui::PopID();

		ImGui::Spring( 0 );
	}

	void FloatPin::OnRenderOutput()
	{
		ImGui::SetNextItemWidth( 25.0f );

		ImGui::PushID( static_cast< int >( ID ) );

		ImGui::DragFloat( "##output", &Data );

		ImGui::PopID();

		ImGui::Spring( 0 );
	}

	void FloatPin::OnSerialise( std::ofstream& rStream ) const
	{
		RawSerialisation::WriteObject( Data, rStream );
	}

	void FloatPin::OnDeserialise( IStream& rStream )
	{
		RawSerialisation::ReadObject( Data, rStream );
	}

	//////////////////////////////////////////////////////////////////////////
	// INT PIN

	IntPin::IntPin( UUID id, const std::string& rName, PinType type, UUID nodeID )
		: Pin( id, rName, type, nodeID )
	{
	}

	IntPin::IntPin( const std::string& rName, PinKind kind )
		: Pin( rName, PinType::Int, kind )
	{
	}

	void IntPin::OnRenderInput()
	{
		ImGui::SetNextItemWidth( 25.0f );

		ImGui::PushID( static_cast< int >( ID ) );

		ImGui::DragInt( "##input", &Data );

		ImGui::PopID();

		ImGui::Spring( 0 );
	}

	void IntPin::OnSerialise( std::ofstream& rStream ) const
	{
		RawSerialisation::WriteObject( Data, rStream );
	}

	void IntPin::OnDeserialise( IStream& rStream )
	{
		RawSerialisation::ReadObject( Data, rStream );
	}

	//////////////////////////////////////////////////////////////////////////
	// BOOL PIN

	BoolPin::BoolPin( UUID id, const std::string& rName, PinType type, UUID nodeID )
		: Pin( id, rName, type, nodeID )
	{
	}

	BoolPin::BoolPin( const std::string& rName, PinKind kind )
		: Pin( rName, PinType::Bool, kind )
	{
	}

	void BoolPin::OnRenderInput()
	{
		ImGui::SetNextItemWidth( 25.0f );

		ImGui::PushID( static_cast< int >( ID ) );

		ImGui::Checkbox( "##input", &Data );

		ImGui::PopID();

		ImGui::Spring( 0 );
	}

	void BoolPin::OnSerialise( std::ofstream& rStream ) const
	{
		RawSerialisation::WriteObject( Data, rStream );
	}

	void BoolPin::OnDeserialise( IStream& rStream )
	{
		RawSerialisation::ReadObject( Data, rStream );
	}

}
