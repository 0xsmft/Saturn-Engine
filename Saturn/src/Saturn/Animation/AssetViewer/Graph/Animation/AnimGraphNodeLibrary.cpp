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

#include "sppch.h"
#include "AnimGraphNodeLibrary.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

#include "AnimGraphOutputNode.h"
#include "Saturn/Animation/AssetViewer/Graph/Animation/AnimGraphStateMachinePlayerNode.h"

namespace Saturn {

	SharedPtr<AnimGraphStateMachinePlayerNode> AnimGraphNodeLibrary::SpawnStateMachinePlayerNode( SharedPtr<NodeEditorBase> nodeEditor )
	{
		AnimGraphStateMachinePlayerNode* pNode = ( AnimGraphStateMachinePlayerNode* ) ClassMetadataHandler::Get().CreateClassObject( AnimGraphStateMachinePlayerNode::StaticClass() );

		// Create shared pointer here, to avoid creating a new one when we call AddNode and then another new one when this function returns, would result in two control blocks.
		SharedPtr<AnimGraphStateMachinePlayerNode> sp = pNode;
		nodeEditor->AddNode( sp );

		return sp;
	}

	SharedPtr<AnimGraphOutputNode> AnimGraphNodeLibrary::SpawnOutputNode( SharedPtr<NodeEditorBase> nodeEditor )
	{
		AnimGraphOutputNode* pNode = ( AnimGraphOutputNode* ) ClassMetadataHandler::Get().CreateClassObject( AnimGraphOutputNode::StaticClass() );

		// Create shared pointer here, to avoid creating a new one when we call AddNode and then another new one when this function returns, would result in two control blocks.
		SharedPtr<AnimGraphOutputNode> sp = pNode;

		nodeEditor->AddNode( sp );
		return sp;
	}

}
