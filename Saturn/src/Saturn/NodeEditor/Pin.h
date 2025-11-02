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
#include <glm/glm.hpp>

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
		Class,
		Function,
		Delegate,
		Material_Color,
		Material_TextureColor, // Sampler2D
		Sound,
		AssetID,
		// Internal pin type to allow us to make sure that a composite node can only be linked to the root node.
		BehaviourTreeCompositeLink,
		AnimGraphAnimation,
		Vec2,
		Vec3,
		Vec4
	};

	enum class PinKind
	{
		Output,
		Input
	};

	enum class PinRenderType
	{
		Blueprint,
		Tree,
		Custom
	};

	inline std::string_view PinTypeToString( PinType type )
	{
		switch( type )
		{
			case PinType::Flow:
				return "Flow";
			case PinType::Bool:
				return "Bool";
			case PinType::Int:
				return "Int";
			case PinType::Float:
				return "Float";
			case PinType::String:
				return "String";
			case PinType::Class:
				return "Class";
			case PinType::Function:
				return "Function";
			case PinType::Delegate:
				return "Delegate";
			case PinType::Material_Color:
				return "Material_Color";
			case PinType::Material_TextureColor:
				return "Material_TextureColor";
			case PinType::AssetID:
				return "AssetHandle";
			
			default: return "";
		}
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
		else if( rString == "Class" )
			return PinType::Class;
		else if( rString == "Function" )
			return PinType::Function;
		else if( rString == "Material_Color" )
			return PinType::Material_Color;
		else if( rString == "Material_TextureColor" )
			return PinType::Material_TextureColor;
		else if( rString == "AssetHandle" )
			return PinType::AssetID;
		else
			return PinType::Class;
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
		// The node in which we are a "child" of, it owns us.
		SharedPtr<NodeEditorNodeBase>      Node;
		std::string    Name;
		PinType        Type = PinType::Flow;
		PinKind        Kind = PinKind::Input;
		PinRenderType  RenderType = PinRenderType::Blueprint;
		bool           AcceptMultipleLinks = false;

	public:
		PinIconType GetIconType() const;
		ImColor GetPinColor() const;

		void RenderInput( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked );
		void RenderOutput( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked );

	public:
		virtual void Serialise( std::ofstream& rStream ) const;
		virtual void Deserialise( FDependentIStream& rStream );

	protected:
		virtual void OnRenderOutput() {}
		virtual void OnRenderInput() {}

		void DrawIcon( bool connected, int alpha ) const;

	private:
		void RenderBlueprintOutput( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked );
		void RenderTreeOutput( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked );

		void RenderBlueprintInput( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked );
		void RenderTreeInput( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked );

		bool CanCreateLink( const Ref<Pin>& rOther ) const;
	};

	// 
	// FloatPin, carries across a single floating-point number.
	//
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

	// 
	// IntPin, carries across a single 32-bit signed integer number.
	//
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

	// 
	// BoolPin, carries across a single boolean.
	//
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

	// 
	// Vec2Pin, carries across a single vector2 value.
	//
	class Vec2Pin : public Pin
	{
	public:
		Vec2Pin() = default;
		Vec2Pin( const std::string& rName, PinKind kind );
		Vec2Pin( UUID id, const std::string& rName, PinType type, UUID nodeID );

		~Vec2Pin() = default;

	public:
		void Serialise( std::ofstream& rStream ) const override;
		void Deserialise( FDependentIStream& rStream ) override;

	public:
		glm::vec2 Data{};
	};

	// 
	// Vec3Pin, carries across a single vector3 value.
	//
	class Vec3Pin : public Pin
	{
	public:
		Vec3Pin() = default;
		Vec3Pin( const std::string& rName, PinKind kind );
		Vec3Pin( UUID id, const std::string& rName, PinType type, UUID nodeID );

		~Vec3Pin() = default;

	public:
		void Serialise( std::ofstream& rStream ) const override;
		void Deserialise( FDependentIStream& rStream ) override;

	public:
		glm::vec3 Data{};
	};

	// 
	// Vec4Pin, carries across a single vector4 value.
	//
	class Vec4Pin : public Pin
	{
	public:
		Vec4Pin() = default;
		Vec4Pin( const std::string& rName, PinKind kind );
		Vec4Pin( UUID id, const std::string& rName, PinType type, UUID nodeID );

		~Vec4Pin() = default;

	public:
		void Serialise( std::ofstream& rStream ) const override;
		void Deserialise( FDependentIStream& rStream ) override;

	public:
		glm::vec4 Data{};
	};

}
