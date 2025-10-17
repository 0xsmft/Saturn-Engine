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

#include "Saturn/NodeEditor/NodeEditorBlueprintNode.h"

namespace Saturn {

	SCLASS()
	class AnimGraphStateMachinePlayerNode : public NodeEditorBlueprintNode
	{
		SAT_DECLARE_CLASS( AnimGraphStateMachinePlayerNode, NodeEditorBlueprintNode );
	public:
		AnimGraphStateMachinePlayerNode();
		AnimGraphStateMachinePlayerNode( const std::string& rName );
		virtual ~AnimGraphStateMachinePlayerNode();

		virtual NodeEvaluationState EvaluateNode( NodeEditorRuntime* pEvaluator ) { return NodeEvaluationState::Evaluated; }

		// PostPlace is only ever called after the node as been "placed", placed means it was clicked from the right-click options and the created.
		// This function does not get called when the node deserialises as this information would already exist.
		// In the case of a state machine player, it will create the entry node and call it's PostPlace.
		void PostPlace();

	private:
		void CreateNode();
	};

}
