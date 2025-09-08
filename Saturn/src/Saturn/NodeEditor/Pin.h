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
		Material_Color,
		Material_TextureColor, // Sampler2D
		Sound,
		AssetID,
		// Internal pin type to allow us to make sure that a composite node can only be linked to the root node.
		BehaviourTreeCompositeLink,
		AnimGraphAnimation
	};

	enum class PinKind
	{
		Output,
		Input
	};

	enum class PinRenderType
	{
		Blueprint,
		Tree
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
			case Saturn::PinType::Material_Color:
				return "Material_Color";
			case Saturn::PinType::Material_TextureColor:
				return "Material_TextureColor";
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
		else if( rString == "Material_Color" )
			return PinType::Material_Color;
		else if( rString == "Material_TextureColor" )
			return PinType::Material_TextureColor;
		else if( rString == "AssetHandle" )
			return PinType::AssetID;
		else
			return PinType::Object;
	}

	class NodeEditorNodeBase;

	class Pin : public RefTarget
	{
	public:
		Pin() = default;
		Pin( const std::string& rName, PinType type, PinKind kind );
		Pin( UUID id, const std::string& rName, PinType type, UUID nodeID );

		virtual ~Pin() = default;

	public:
		UUID           ID;
		// TODO: Use weak ref #ReplaceRawPtrOrRefWithWeakRef
		SharedPtr<NodeEditorNodeBase>      Node;
		std::string    Name;
		PinType        Type = PinType::Flow;
		PinKind        Kind = PinKind::Input;
		PinRenderType  RenderType = PinRenderType::Blueprint;
		bool           AcceptMultipleLinks = false;

	public:
		PinIconType GetIconType() const;
		ImColor GetPinColor() const;

		void Render( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked, uint32_t pinIndex );

	public:
		virtual void Serialise( std::ofstream& rStream ) const;
		virtual void Deserialise( FDependentIStream& rStream );

	protected:
		virtual void OnRenderOutput() {}
		virtual void OnRenderInput() {}

		void DrawIcon( bool connected, int alpha ) const;

	private:
		void RenderInput( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked, uint32_t pinIndex );
		void RenderOutput( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked );

		void RenderBlueprintOutput( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked );
		void RenderTreeOutput( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked );

		void RenderBlueprintInput( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked, uint32_t pinIndex );
		void RenderTreeInput( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked, uint32_t pinIndex );

		bool CanCreateLink( const Ref<Pin>& rOther ) const;
	};

	class FloatPin : public Pin
	{
	public:
		FloatPin() = default;
		FloatPin( const std::string& rName, PinKind kind );
		FloatPin( UUID id, const std::string& rName, PinType type, UUID nodeID );

		~FloatPin() = default;

	public:
		void Serialise( std::ofstream& rStream ) const override;
		void Deserialise( FDependentIStream& rStream ) override;

	protected:
		void OnRenderInput() override;
		void OnRenderOutput() override;

	public:
		float Data = 0.0f;
	};

	class IntPin : public Pin
	{
	public:
		IntPin() = default;
		IntPin( const std::string& rName, PinKind kind );
		IntPin( UUID id, const std::string& rName, PinType type, UUID nodeID );

		~IntPin() = default;

	public:
		void Serialise( std::ofstream& rStream ) const override;
		void Deserialise( FDependentIStream& rStream ) override;

	protected:
		void OnRenderInput() override;

	public:
		int Data = 0;
	};

	class BoolPin : public Pin
	{
	public:
		BoolPin() = default;
		BoolPin( const std::string& rName, PinKind kind );
		BoolPin( UUID id, const std::string& rName, PinType type, UUID nodeID );

		~BoolPin() = default;

	public:
		void Serialise( std::ofstream& rStream ) const override;
		void Deserialise( FDependentIStream& rStream ) override;

	protected:
		void OnRenderInput() override;

	public:
		bool Data = false;
	};
}
