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

#include "Saturn/NodeEditor/NodeEditorNodeBase.h"

namespace Saturn {

	SCLASS()
	class AnimGraphStateMachineStateNode : public NodeEditorNodeBase
	{
		SAT_DECLARE_CLASS( AnimGraphStateMachineStateNode, NodeEditorNodeBase );
	public:
		AnimGraphStateMachineStateNode();
		AnimGraphStateMachineStateNode( const std::string& rName );

		virtual ~AnimGraphStateMachineStateNode();

		void PostPlace();

	public:
		virtual void Serialise( std::ofstream& rStream, bool isForDist ) const override;
		virtual void Deserialise( FDependentIStream& rStream ) override;
		virtual NodeEditorTaskBase* ConvertToTask() override;
		virtual void Render( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder ) override;
		virtual NodeEvaluationState EvaluateNode( NodeEditorRuntime* evaluator ) override;

	public:
		UUID GetOutputNodeID() const { return m_OutputNodeID; }

	private:
		void CreateNode();
		UUID m_OutputNodeID = 0;
	};

}
