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
#include "AnimGraphStateMachinePlayAnimNode.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

#include "Saturn/Animation/AssetViewer/Graph/Animation/AnimGraphAnimationPin.h"
#include "Saturn/Animation/AssetViewer/Graph/Tasks/AnimGraphPlayAnimTask.h"

namespace Saturn {

	AnimGraphStateMachinePlayAnimNode::AnimGraphStateMachinePlayAnimNode()
		: NodeEditorBlueprintNode( "Play Animation" )
	{
		CreateNode();
	}

	AnimGraphStateMachinePlayAnimNode::AnimGraphStateMachinePlayAnimNode( const std::string& rName )
		: NodeEditorBlueprintNode( rName )
	{
		CreateNode();
	}

	void AnimGraphStateMachinePlayAnimNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::AnimGraphStateMachinePlayAnimNode;

#if !defined(SAT_DIST)
		Color = ImColor( 48, 128, 255, 100 );
		RenderType = NodeRenderType::Blueprint;
#endif

		Outputs.push_back( Ref<AnimGraphAnimationPin>::Create( "Out Animation", PinKind::Output, AnimGraphAnimationPinFlags::Animation ) );
	}

	AnimGraphStateMachinePlayAnimNode::~AnimGraphStateMachinePlayAnimNode()
	{
	}

	NodeEditorTaskBase* AnimGraphStateMachinePlayAnimNode::ConvertToTask()
	{
		return NewObject<AnimGraphPlayAnimTask>();
	}

	void AnimGraphStateMachinePlayAnimNode::Serialise( std::ofstream& rStream, bool isForDist ) const
	{
		Super::Serialise( rStream, isForDist );
	}

	void AnimGraphStateMachinePlayAnimNode::Deserialise( FDependentIStream& rStream )
	{
		Super::Deserialise( rStream );
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( AnimGraphStateMachinePlayAnimNode );
