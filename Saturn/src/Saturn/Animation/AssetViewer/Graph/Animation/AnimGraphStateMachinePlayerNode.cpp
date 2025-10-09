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
#include "AnimGraphStateMachinePlayerNode.h"

#include "AnimGraphAnimationPin.h"
#include "AnimGraph.h"

#include "Saturn/Animation/AssetViewer/Graph/StateMachine/StateMachineNodeLibrary.h"
#include "Saturn/Animation/AssetViewer/Graph/StateMachine/AnimGraphStateMachineStateNode.h"


namespace Saturn {

	AnimGraphStateMachinePlayerNode::AnimGraphStateMachinePlayerNode()
		: NodeEditorBlueprintNode( "STATE MACHINE" )
	{
		CreateNode();
	}

	AnimGraphStateMachinePlayerNode::AnimGraphStateMachinePlayerNode( const std::string& rName )
		: NodeEditorBlueprintNode( rName )
	{
		CreateNode();
	}

	void AnimGraphStateMachinePlayerNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::AnimGraphStateMachinePlayerNode;

#if !defined(SAT_DIST)
		CanBeDeleted = false;
		Color = ImColor( 48, 128, 255, 100 );
		RenderType = NodeRenderType::Blueprint;
#endif

		Outputs.emplace_back( Ref<AnimGraphAnimationPin>::Create( "Out", PinKind::Output, AnimGraphAnimationPinFlags::StateMachine ) );
	}

	AnimGraphStateMachinePlayerNode::~AnimGraphStateMachinePlayerNode()
	{
	}

	void AnimGraphStateMachinePlayerNode::PostPlace() 
	{
		// Spawn entry node
		auto entryNode = StateMachineNodeLibrary::SpawnStateNode( pOuter->SharedFromThis() );
		entryNode->pParentObject = this;

		auto* AG = dynamic_cast< AnimGraph* >( pOuter );
		if( AG )
		{
			AG->MarkNodeAsEntry( entryNode );
		}

		entryNode->PostPlace();
	}
}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( AnimGraphStateMachinePlayerNode );
