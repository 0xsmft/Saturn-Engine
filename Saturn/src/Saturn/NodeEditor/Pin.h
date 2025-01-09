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

#include "Saturn/Core/Ref.h"
#include "Saturn/Core/Memory/Buffer.h"
#include "Saturn/Core/UUID.h"

#include "Link.h"

#include <string>
#include <imgui_node_editor.h>

namespace ed = ax::NodeEditor;

namespace ax {
	namespace NodeEditor::Utilities {
		struct BlueprintNodeBuilder;
	}
}

namespace Saturn {

	enum class PinIconType : unsigned int;

	enum class PinType
	{
		Flow,
		Bool,
		Int,
		Float,
		String,
		Object,
		Function,
		Delegate,
		Material_Sampler2D,
		Sound,
		AssetID
	};

	enum class PinKind
	{
		Output,
		Input
	};

	inline std::string_view PinTypeToString( PinType type )
	{
		switch( type )
		{
			case Saturn::PinType::Flow:
				return "Flow";
			case Saturn::PinType::Bool:
				return "Bool";
			case Saturn::PinType::Int:
				return "Int";
			case Saturn::PinType::Float:
				return "Float";
			case Saturn::PinType::String:
				return "String";
			case Saturn::PinType::Object:
				return "Object";
			case Saturn::PinType::Function:
				return "Function";
			case Saturn::PinType::Delegate:
				return "Delegate";
			case Saturn::PinType::Material_Sampler2D:
				return "Material_Sampler2D";
			case Saturn::PinType::AssetID:
				return "AssetHandle";
			default:
				break;
		}

		return "";
	}

	inline PinType StringToPinType( const std::string& rString )
	{
		if( rString == "Flow" )
			return PinType::Flow;
		else if( rString == "Bool" )
			return PinType::Bool;
		else if( rString == "Int" )
			return PinType::Int;
		else if( rString == "Float" )
			return PinType::Float;
		else if( rString == "String" )
			return PinType::String;
		else if( rString == "Object" )
			return PinType::Object;
		else if( rString == "Function" )
			return PinType::Function;
		else if( rString == "Material_Sampler2D" )
			return PinType::Material_Sampler2D;
		else if( rString == "AssetHandle" )
			return PinType::AssetID;
		else
			return PinType::Object;
	}

	class Node;

	class Pin : public RefTarget
	{
	public:
		Pin() = default;

		Pin( UUID id, 
			const std::string& rName, 
			PinType type, 
			UUID nodeID ) 
			: Node( nullptr ), Name( rName ), Type( type ), Kind( PinKind::Input )
		{
			ID = std::move( id );

			ExtraData = Buffer();
		}

		~Pin() 
		{
			ExtraData.Free();
		}

		PinIconType GetIconType() const;
		ImColor GetPinColor() const;

		void DrawIcon( bool connected, int alpha );
		void Render( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked, uint32_t pinIndex );

	public:
		static void Serialise( const Ref<Pin>& rObject, std::ofstream& rStream );
		static void Deserialise( Ref<Pin>& rObject, std::ifstream& rStream );

	public:
		UUID        ID;
		Ref<Node>   Node;
		std::string Name;
		PinType     Type = PinType::Flow;
		PinKind     Kind = PinKind::Input;
		Buffer      ExtraData;
		bool        AcceptMultipleLinks = false;

	private:
		void RenderInput( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked, uint32_t pinIndex );
		void RenderOutput( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked );

		bool CanCreateLink( const Ref<Pin>& rOther );
	};
}