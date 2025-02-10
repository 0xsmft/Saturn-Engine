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

#include "Saturn/Core/Memory/Buffer.h"
#include "Saturn/Core/UUID.h"
#include "Pin.h"
#include "NodeEditorCompilationStatus.h"

#include <string>
#include <vector>
#include <imgui_node_editor.h>

namespace ed = ax::NodeEditor;
namespace util = ax::NodeEditor::Utilities;

namespace ax::NodeEditor::Utilities {
	struct BlueprintNodeBuilder;
}

namespace Saturn {

	enum class NodeRenderType
	{
		Blueprint,
		Comment
	};

	enum class NodeExecutionType
	{
		Value,
		AssetID, // Values and Asset IDs are different as values can be added together however AssetIDs can not
		Sampler2D,
		MaterialOutput,
		ColorPicker,
		Add,
		Subtract,
		Multiply,
		Divide,
		LessThan,
		GreaterThan,
		LessThanOrEqu,
		GreaterThanOrEqu,
		MaterialMixColors,
		SoundOutput,
		SoundPlayer,
		SoundRandomSound,
		SoundRandomPitch,
		SoundMixer,
		SoundPitch,
		None
	};

	class NodeEditor;
	class NodeEditorBase;
	class NodeEditorRuntime;

	class Node : public RefTarget
	{
	public:
		Node() = default;
		Node( const std::string& rName );
		virtual ~Node();

		void Destroy();

		void Render( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, NodeEditorBase* pBase );

	public:
		static void Serialise( const Ref<Node>& rObject, std::ofstream& rStream );
		static void Deserialise( Ref<Node>& rObject, std::ifstream& rStream );

		virtual NodeEditorCompilationStatus EvaluateNode( NodeEditorRuntime* evaluator ) { return NodeEditorCompilationStatus::Success; }

		virtual void OnRenderOutput( Ref<Pin> pin ) {}
		virtual void OnRenderInput( Ref<Pin> pin ) {}

	protected:
		virtual void OnSerialise( std::ofstream& rStream ) const {}
		virtual void OnDeserialise( std::ifstream& rStream ) {}

	public:
		UUID ID;
		std::string Name;
		std::vector<Ref<Pin>> Inputs;
		std::vector<Ref<Pin>> Outputs;
		ImColor Color;
		NodeRenderType Type = NodeRenderType::Blueprint;
		NodeExecutionType ExecutionType = NodeExecutionType::None;
		ImVec2 Size;
		ImVec2 Position;
		bool CanBeDeleted = true;

		std::string ActiveState;
		std::string SavedState;
	};

}