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

#include "NodeEditorNodeBase.h"

namespace Saturn {

	SCLASS()
	class NodeEditorTreeNode : public NodeEditorNodeBase
	{
		SAT_DECLARE_CLASS( NodeEditorTreeNode, NodeEditorNodeBase );
	public:
		NodeEditorTreeNode() = default;
		NodeEditorTreeNode( const std::string& rName );
		virtual ~NodeEditorTreeNode();

	public:
#if !defined(SAT_DIST)
		virtual void OnRenderExtra() {}
		virtual void OnRenderNextSection() {}
#endif

	public:
		//////////////////////////////////////////////////////////////////////////
		// NodeEditorNodeBase
		
		virtual void Render( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder ) override final;
		virtual NodeEvaluationState EvaluateNode( NodeEditorRuntime* evaluator ) override { return NodeEvaluationState::Failed; }
	};
	
}
