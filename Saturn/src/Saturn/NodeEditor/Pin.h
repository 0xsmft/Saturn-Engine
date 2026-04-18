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

#pragma once

#include "Saturn/Core/Ref.h"
#include "Saturn/Core/UUID.h"

#include "Link.h"

#include "Saturn/Serialisation/Raw/RawSerialisationBase.h"

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

	enum class PinIconType : uint32_t;

	enum class PinType : uint32_t
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

	enum class PinKind : uint32_t
	{
		Output,
		Input
	};

	enum class PinRenderType : uint32_t
	{
		Blueprint,
		Tree,
		Custom
	};

	enum PinFlag : uint8_t
	{
		PinFlag_DefaultSet = 0,
		PinFlag_AcceptMultipleLinks = BIT( 0 ),
		PinFlag_RequiredForEvaluation = BIT( 1 ),
	};

	class NodeEditorNodeBase;

	class Pin : public RefTarget
	{
	public:
		Pin() = default;
		Pin( const std::string& rName, PinType type, PinKind kind, PinFlag flags = PinFlag_DefaultSet );
		Pin( UUID id, const std::string& rName, PinType type, UUID nodeID, PinFlag flags = PinFlag_DefaultSet );

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
		uint8_t		   PinFlags = PinFlag_DefaultSet;

	public:
		PinIconType GetIconType() const;
		ImColor GetPinColor() const;

		void RenderInput( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked );
		void RenderOutput( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, bool linked );

		bool IsFlagSet( PinFlag flag ) const { return ( flag & PinFlags ) != 0; }

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
