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

#include "Pin.h"
#include "NodeEditorCompilationStatus.h"

#include "Saturn/Core/Memory/Buffer.h"
#include "Saturn/Core/UUID.h"

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
		Tree,
		Comment
	};

	// NOTE: When adding new execution types, make sure to be careful with the order of the enum values.
	// If you change the order, you will break the serialisation of the nodes.
	// So, make sure to add new execution types at the end of the enum
	enum class NodeExecutionType
	{
		Value,
		AssetID,
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
		SoundFloatConst,
		BehaviourTreeRootNode,
		BehaviourTreeSelectorNode,
		BehaviourTreeSequenceNode,
		BehaviourTreeWaitNode,
		BehaviourTreePlaySoundNode,
		BehaviourTreeMoveTo,
		HintNode, // Comment node
		None
	};

	class NodeEditor;
	class NodeEditorBase;
	class NodeEditorRuntime;

#define SAT_NODE_EDITOR_NODE_BODY( ExecutionType ) \
public: \
static inline NodeExecutionType GetStaticExecutionType() { return ExecutionType; }

	// NOTE: Tips when adding a new node
	// 1) You must add a custom execution type
	// 2) You must add the node to its NodeLibrary
	// 3) Refer to the NodeEditor for the Node you are trying to add, for example, BehaviourTrees need a Task counterpart when adding a Task node so you'll need to refer to that
	class NodeEditorNodeBase : public RefTarget
	{
		SAT_NODE_EDITOR_NODE_BODY( NodeExecutionType::None );
	public:
		UUID ID;
		std::string Name;
		std::vector<Ref<Pin>> Inputs;
		std::vector<Ref<Pin>> Outputs;
		NodeRenderType Type = NodeRenderType::Blueprint;
		NodeExecutionType ExecutionType = NodeExecutionType::None;
		bool CanBeDeleted = true;

#if !defined(SAT_DIST)
		ImColor Color;
		ImVec2 Size;
		ImVec2 Position;

		size_t EvaluationOrder = 0;

		std::string ActiveState;
		std::string SavedState;
#endif

	public:
#if !defined(SAT_DIST)
		using IStream = std::ifstream;
#else
		// In Dist, we read from a VFS file which is not an actual file so we can't use std::ifstream
		using IStream = std::istream;
#endif
	public:
		NodeEditorNodeBase() = default;
		NodeEditorNodeBase( const std::string& rName );
		virtual ~NodeEditorNodeBase();

		void Destroy();

		virtual void Render( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, NodeEditorBase* pBase ) = 0;
		virtual NodeEvaluationState EvaluateNode( NodeEditorRuntime* evaluator ) = 0;
		
#if !defined(SAT_DIST)
		// This function is not pure virtual because not every Node needs a special right click menu
		// Unlike Render and Evaluate where every Node needs to have some sort of implementation for those functions
		// The same rule applies for OnRenderOutput, OnRenderInput, OnSerialise, OnDeserialise
		virtual void RenderContextWindow() {}
#endif

	public:
		// Static Serialise/Deserialise called by NodeCache
		static void Serialise( const Ref<NodeEditorNodeBase>& rObject, std::ofstream& rStream );
		static void Deserialise( Ref<NodeEditorNodeBase>& rObject, IStream& rStream );

	public:
		virtual void OnRenderOutput( Ref<Pin> pin ) {}
		virtual void OnRenderInput( Ref<Pin> pin ) {}

	protected:
		// A helper function to all child classes to write their data when Serialising/Deserialising
		virtual void OnSerialise( std::ofstream& rStream ) const {}
		virtual void OnDeserialise( IStream& rStream ) {}
	};

}
