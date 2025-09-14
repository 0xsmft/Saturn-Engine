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

#include "Saturn/GameFramework/SObject.h"

#include "Pin.h"
#include "NodeEditorCompilationStatus.h"

#include "Saturn/Core/Buffer.h"
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
		BehaviourTreeGeneralTaskNode,
		HintNode, // Comment node
		AnimGraphOutputNode,
		AnimGraphStateMachinePlayerNode,
		AnimGraphStateMachineStateNode,
		AnimGraphStateMachineOutNode,
		AnimGraphStateMachinePlayAnimNode,
		AnimGraphStateMachineTransitionNode,
		//^^^^ add new node types here ^^^^
		None
	};

	class NodeEditor;
	class NodeEditorBase;
	class NodeEditorRuntime;

	SCLASS()
	class NodeEditorNodeBase : public SObject, public EnabledSharedFromThis<NodeEditorNodeBase>
	{
		// NOTE: SAT_DECLARE_CLASS expanded !! ABSTRACT CLASS !! would handled by the HeaderTool normally
	private: 
		NodeEditorNodeBase& operator=( NodeEditorNodeBase&& ); 
		NodeEditorNodeBase& operator=( const NodeEditorNodeBase& ); 
		static SClass* GetStaticClassInternal(); 

	public: 
		inline static [[nodiscard]] SClass* StaticClass() 
		{
			return GetStaticClassInternal();
		}
	public: 
		typedef NodeEditorNodeBase ThisClass; 
		typedef SObject Super; 

		//////////////////////////////////////////////////////////////////////////

	public:
		UUID ID;
		std::string Name;
		std::vector<Ref<Pin>> Inputs;
		std::vector<Ref<Pin>> Outputs;
		NodeRenderType Type = NodeRenderType::Blueprint;
		NodeExecutionType ExecutionType = NodeExecutionType::None;
		size_t EvaluationOrder = 0;
		NodeEditorBase* pOuter = nullptr;

#if !defined(SAT_DIST)
		std::string ActiveState;
		std::string SavedState;

		ImColor Color;
		ImVec2 Size;

		bool CanBeDeleted = true;
#endif

	public:
		NodeEditorNodeBase() = default;
		NodeEditorNodeBase( const std::string& rName );
		virtual ~NodeEditorNodeBase();

		void Destroy();

		virtual void Render( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder ) = 0;
		virtual NodeEvaluationState EvaluateNode( NodeEditorRuntime* evaluator ) = 0;
		
#if !defined(SAT_DIST)
		// This function is not pure virtual because not every Node needs a special right click menu
		// Unlike Render and Evaluate where every Node needs to have some sort of implementation for those functions
		// The same rule applies for OnRenderOutput, OnRenderInput, OnSerialise, OnDeserialise
		virtual void RenderContextWindow() {}
#endif

	public:
		// Serialise/Deserialise NodeCache (NC)
		virtual void Serialise( std::ofstream& rStream, bool isForDist ) const;
		virtual void Deserialise( FDependentIStream& rStream );
	};

}
