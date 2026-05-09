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
#include "Pin.h"

#include "NodeEditorBase.h"

#include "Saturn/Serialisation/Raw/RawSerialisation.h"
#include "Saturn/NodeEditor/NodeEditorBlueprintNode.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"

#include "NodeEditorAux.h"
#include "builders.h"

namespace Saturn {

	Pin::Pin( const std::string& rName, PinType type, PinKind kind, PinFlag flags /*= PinFlag_DefaultSet*/ )
		: Name( rName ), Type( type ), Kind( kind ), Node( nullptr ), ID(), PinFlags( flags )
	{
	}

	Pin::Pin( UUID id, const std::string& rName, PinType type, UUID nodeID, PinFlag flags /*= PinFlag_DefaultSet*/ )
		: Node( nullptr ), Name( rName ), Type( type ), Kind( PinKind::Input ), ID( id ), PinFlags( flags )
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
			case PinType::Class:			  
			case PinType::Function:			  
			case PinType::Material_Color:
			case PinType::Material_TextureColor:
			case PinType::AssetID:            return PinIconType::Circle;
		
			default:
				return PinIconType::Circle;
		}
	}

	ImColor Pin::GetPinColor() const
	{
		switch( Type )
		{
			default:
			case PinType::AnimGraphAnimation:
			case PinType::Flow:                  return ImColor( 255, 255, 255 ); // Pure White
			case PinType::Bool:                  return ImColor( 220, 48, 48 );   // Red, not fully
			case PinType::U32:
			case PinType::U64:
			case PinType::Int:                   return ImColor( 68, 201, 156 );  // Light Green
			case PinType::Float:                 return ImColor( 28, 158, 63 );   // Slightly darker green
			case PinType::String:                return ImColor( 124, 21, 153 );  // Purple
			case PinType::Class:                 return ImColor( 51, 150, 215 );  // Light Blue
			case PinType::Function:              return ImColor( 218, 0, 183 );   // Pink
			case PinType::Delegate:              return ImColor( 255, 48, 48 );   // Red, slightly darker than Bool
			case PinType::AssetID:               return ImColor( 0, 0, 255 );     // Pure Blue
			case PinType::Material_Color:        return ImColor( 142, 61, 186 );  // Purple-ish
			case PinType::Material_TextureColor: return ImColor( 142, 61, 186 );  // Purple-ish
			case PinType::Vec2:					 return ImColor( 237, 120, 9 );   // Orange
			case PinType::Vec3:					 return ImColor( 230, 147, 69 );  // Light Orange
			case PinType::Vec4:					 return ImColor( 255, 124, 0 );   // Lighter Orange
			case PinType::Sound:				 return ImColor( 173, 18, 128 );  // Dark pink
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

	void Pin::RenderInput( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked )
	{
		switch( RenderType )
		{
			case PinRenderType::Blueprint:
			{
				RenderBlueprintInput( rBuilder, linked );
			} break;

			case PinRenderType::Tree:
			{
				RenderTreeInput( rBuilder, linked );
			} break;

			case PinRenderType::Custom:
			{
				OnRenderInput();
			} break;

			default: break;
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

			case PinRenderType::Custom:
			{
				OnRenderOutput();
			} break;

			default: break;
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

	void Pin::RenderBlueprintInput( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked )
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

		// Hand off to children only if not linked
		if( !linked )
			OnRenderInput();

		ImGui::Spring( 0 );

		ImGui::PopStyleVar();

		rBuilder.EndInput();
	}

	void Pin::RenderTreeInput( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked )
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

	void Pin::Serialise( std::ofstream& rStream ) const
	{
		RawSerialisation::WriteObject( ID, rStream );

		RawSerialisation::WriteString( Name, rStream );
		RawSerialisation::WriteObject( Type, rStream );
		RawSerialisation::WriteObject( Kind, rStream );
	}

	void Pin::Deserialise( FDependentIStream& rStream )
	{
		RawSerialisation::ReadObject( ID, rStream );

		Name = RawSerialisation::ReadString( rStream );

		auto* pEditor = dynamic_cast<NodeEditor*>( Node->GetParentObject() );
		if( pEditor->GetVersion() >= NodeEditorVersion::PinClassSizeChange )
		{
			RawSerialisation::ReadObject( Type, rStream );
			RawSerialisation::ReadObject( Kind, rStream );
		}
		else
		{
			RawSerialisation::ReadObject( Type, rStream );
			rStream.ignore( 3 );

			RawSerialisation::ReadObject( Kind, rStream );
			rStream.ignore( 3 );
		}
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

	void FloatPin::Serialise( std::ofstream& rStream ) const
	{
		Pin::Serialise( rStream );

		RawSerialisation::WriteObject( Data, rStream );
	}

	void FloatPin::Deserialise( FDependentIStream& rStream )
	{
		Pin::Deserialise( rStream );

		RawSerialisation::ReadObject( Data, rStream );
	}

	//////////////////////////////////////////////////////////////////////////
	// INT PIN

	IntPin::IntPin( UUID id, const std::string& rName, PinType type, UUID nodeID, PinFlag flags )
		: Pin( id, rName, type, nodeID, flags )
	{
	}

	IntPin::IntPin( const std::string& rName, PinKind kind, PinFlag flags )
		: Pin( rName, PinType::Int, kind, flags )
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

	void IntPin::Serialise( std::ofstream& rStream ) const
	{
		Pin::Serialise( rStream );

		RawSerialisation::WriteObject( Data, rStream );
	}

	void IntPin::Deserialise( FDependentIStream& rStream )
	{
		Pin::Deserialise( rStream );

		RawSerialisation::ReadObject( Data, rStream );
	}

	//////////////////////////////////////////////////////////////////////////
	// UINT PIN
	
	UInt64Pin::UInt64Pin( const std::string& rName, PinKind kind, PinFlag flags )
		: Pin( rName, PinType::U64, kind, flags )
	{
	}

	UInt64Pin::UInt64Pin( UUID id, const std::string& rName, PinType type, UUID nodeID, PinFlag flags )
		: Pin( id, rName, type, nodeID, flags )
	{
	}

	void UInt64Pin::OnRenderInput()
	{
		ImGui::SetNextItemWidth( 25.0f );

		ImGui::PushID( static_cast< int >( ID ) );
		ImGui::DragScalar( "##input", ImGuiDataType_U64, &Data );
		ImGui::PopID();

		ImGui::Spring( 0 );
	}

	void UInt64Pin::Serialise( std::ofstream& rStream ) const
	{
		Pin::Serialise( rStream );
		RawSerialisation::WriteObject( Data, rStream );
	}

	void UInt64Pin::Deserialise( FDependentIStream& rStream )
	{
		Pin::Deserialise( rStream );
		RawSerialisation::ReadObject( Data, rStream );
	}

	//////////////////////////////////////////////////////////////////////////
	// UINT32 PIN
	
	UInt32Pin::UInt32Pin( const std::string& rName, PinKind kind, PinFlag flags /*= PinFlag_DefaultSet*/ )
		: Pin( rName, PinType::U32, kind, flags )
	{
	}

	UInt32Pin::UInt32Pin( UUID id, const std::string& rName, PinType type, UUID nodeID, PinFlag flags /*= PinFlag_DefaultSet*/ )
		: Pin( id, rName, type, nodeID, flags )
	{
	}

	void UInt32Pin::OnRenderInput()
	{
		ImGui::SetNextItemWidth( 25.0f );

		ImGui::PushID( static_cast< int >( ID ) );
		ImGui::DragScalar( "##input", ImGuiDataType_U32, &Data );
		ImGui::PopID();

		ImGui::Spring( 0 );
	}

	void UInt32Pin::Serialise( std::ofstream& rStream ) const
	{
		Pin::Serialise( rStream );
		RawSerialisation::WriteObject( Data, rStream );
	}

	void UInt32Pin::Deserialise( FDependentIStream& rStream )
	{
		Pin::Deserialise( rStream );
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

	void BoolPin::Serialise( std::ofstream& rStream ) const
	{
		Pin::Serialise( rStream );

		RawSerialisation::WriteObject( Data, rStream );
	}

	void BoolPin::Deserialise( FDependentIStream& rStream )
	{
		Pin::Deserialise( rStream );

		RawSerialisation::ReadObject( Data, rStream );
	}

	//////////////////////////////////////////////////////////////////////////
	// VEC2 PIN

	Vec2Pin::Vec2Pin( const std::string& rName, PinKind kind )
		: Pin( rName, PinType::Vec2, kind )
	{
	}

	Vec2Pin::Vec2Pin( UUID id, const std::string& rName, PinType type, UUID nodeID )
		: Pin( id, rName, type, nodeID )
	{
	}

	void Vec2Pin::Serialise( std::ofstream& rStream ) const
	{
		Pin::Serialise( rStream );
		RawSerialisation::WriteObject( Data, rStream );
	}

	void Vec2Pin::Deserialise( FDependentIStream& rStream )
	{
		Pin::Deserialise( rStream );
		RawSerialisation::ReadObject( Data, rStream );
	}

	//////////////////////////////////////////////////////////////////////////
	// VEC3 PIN

	Vec3Pin::Vec3Pin( const std::string& rName, PinKind kind )
		: Pin( rName, PinType::Vec3, kind )
	{
	}

	Vec3Pin::Vec3Pin( UUID id, const std::string& rName, PinType type, UUID nodeID )
		: Pin( id, rName, type, nodeID )
	{
	}

	void Vec3Pin::Serialise( std::ofstream& rStream ) const
	{
		Pin::Serialise( rStream );
		RawSerialisation::WriteObject( Data, rStream );
	}

	void Vec3Pin::Deserialise( FDependentIStream& rStream )
	{
		Pin::Deserialise( rStream );
		RawSerialisation::ReadObject( Data, rStream );
	}

	//////////////////////////////////////////////////////////////////////////
	// VEC4 PIN

	Vec4Pin::Vec4Pin( const std::string& rName, PinKind kind )
		: Pin( rName, PinType::Vec4, kind )
	{
	}

	Vec4Pin::Vec4Pin( UUID id, const std::string& rName, PinType type, UUID nodeID )
		: Pin( id, rName, type, nodeID )
	{
	}

	void Vec4Pin::Serialise( std::ofstream& rStream ) const
	{
		Pin::Serialise( rStream );
		RawSerialisation::WriteObject( Data, rStream );
	}

	void Vec4Pin::Deserialise( FDependentIStream& rStream )
	{
		Pin::Deserialise( rStream );
		RawSerialisation::ReadObject( Data, rStream );
	}

}
