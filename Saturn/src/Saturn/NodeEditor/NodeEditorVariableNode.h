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

#include "NodeEditorBlueprintNode.h"
#include "NodeEditorVariable.h"

namespace Saturn {

	//
	// NodeEditorVariableNode
	//
	// The node that represents a NodeEditorVariable
	//
	SCLASS();
	class NodeEditorVariableNode : public NodeEditorNodeBase
	{
		SAT_DECLARE_CLASS( NodeEditorVariableNode, NodeEditorNodeBase );
	public:
		NodeEditorVariableNode();
		NodeEditorVariableNode( const std::string& rName, Ref<NodeEditorVariable> var );
		virtual ~NodeEditorVariableNode();

		Ref<NodeEditorVariable> GetVariable() const { return m_Variable; }

	public:
		//////////////////////////////////////////////////////////////////////////
		// NodeEditorNodeBase

		virtual void Serialise( std::ofstream& rStream ) const override;
		virtual void Deserialise( FDependentIStream& rStream ) override;

		virtual void Render( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder ) override;
		virtual NodeEditorTaskBase* ConvertToTask() override;

	public:
		static SharedPtr<NodeEditorVariableNode> SpawnVariableNode( Ref<NodeEditorVariable> var, SharedPtr<NodeEditor> nodeEditor );

	private:
		void CreateNode();
		void InitPinsForVariable();

	private:
		// Non-owning, used so we can properly create this node with the correct PinType.
		Ref<NodeEditorVariable> m_Variable;
	};

	SCLASS()
	class NodeEditorSetVariableNode : public NodeEditorBlueprintNode
	{
		SAT_DECLARE_CLASS( NodeEditorSetVariableNode, NodeEditorBlueprintNode );
	public:
		NodeEditorSetVariableNode();
		NodeEditorSetVariableNode( const std::string& rName, Ref<NodeEditorVariable> var );
		~NodeEditorSetVariableNode();

	public:
		static SharedPtr<NodeEditorSetVariableNode> SpawnSetVariableNode( Ref<NodeEditorVariable> var, SharedPtr<NodeEditor> nodeEditor );

	private:
		void CreateNode();

	private:
		Ref<NodeEditorVariable> m_Variable;
	};
}
