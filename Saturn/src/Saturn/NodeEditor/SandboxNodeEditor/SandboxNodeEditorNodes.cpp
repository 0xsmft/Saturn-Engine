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

#include "sppch.h"
#include "SandboxNodeEditorNodes.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

#include "SandboxNodeEditorTasks.h"

namespace Saturn {	

	//////////////////////////////////////////////////////////////////////////
	// SandboxNodeEditorOutputNode

	SandboxNodeEditorOutputNode::SandboxNodeEditorOutputNode()
		: Super()
	{
		CreateNode();
	}
	
	void SandboxNodeEditorOutputNode::CreateNode()
	{
		Name = "Output Node";
		CanBeDeleted = false;

		Inputs.push_back( Ref<Pin>::Create( "Result", PinType::Int, PinKind::Input ) );
	}

	NodeEditorTaskBase* SandboxNodeEditorOutputNode::ConvertToTask()
	{
		return NewObject<SandboxNodeEditorOutputTask>( GetParentObject() );
	}
	
	//////////////////////////////////////////////////////////////////////////
	// SandboxNodeEditorNode
	
	SandboxNodeEditorNode::SandboxNodeEditorNode()
		: Super()
	{
		CreateNode();
	}

	void SandboxNodeEditorNode::CreateNode()
	{
		Name = "Sandbox Node";

		Outputs.push_back( Ref<Pin>::Create( "Result", PinType::Int, PinKind::Output ) );
	}
	
	NodeEditorTaskBase* SandboxNodeEditorNode::ConvertToTask()
	{
		return NewObject<SandboxNodeEditorNodeTask>( GetParentObject() );
	}

	//////////////////////////////////////////////////////////////////////////
	// SandboxNodeExampleNode

	SandboxNodeExampleNode::SandboxNodeExampleNode()
		: Super()
	{
		CreateNode();
	}

	void SandboxNodeExampleNode::CreateNode()
	{
		Name = "Example Node";
	}

	NodeEditorTaskBase* SandboxNodeExampleNode::ConvertToTask()
	{
		return NewObject<SandboxNodeEditorNodeTask>( GetParentObject() );
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( SandboxNodeEditorOutputNode );
SAT_X31_CREATE_AUTO_REG( SandboxNodeEditorNode );
SAT_X31_CREATE_AUTO_REG( SandboxNodeExampleNode );
