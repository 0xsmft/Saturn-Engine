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
#include "MatGraph2_Nodes.h"

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// SMatGraph2_OutputNode (OutputNode)

	SMatGraph2_OutputNode::SMatGraph2_OutputNode()
		: NodeEditorBlueprintNode( "Material Output" )
	{
		CreateNode();
	}

	void SMatGraph2_OutputNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::MaterialOutput;

#if !defined(SAT_DIST)
		Flags |= NodeFlags_Irremovable | NodeFlags_RejectCopyPaste;
		Color = ImColor( 255, 128, 128 );
#endif

		Inputs.reserve( 7llu );

		// Inputs
		Inputs.push_back( Ref<Vec3Pin>::Create( "Albedo Color", PinKind::Input ) );
		Inputs.push_back( Ref<Vec3Pin>::Create( "Normal Color", PinKind::Input ) );
		Inputs.push_back( Ref<Vec3Pin>::Create( "Metallic Color", PinKind::Input ) );
		Inputs.push_back( Ref<Vec3Pin>::Create( "Roughness Color", PinKind::Input ) );

		// Float inputs
		Inputs.push_back( Ref<FloatPin>::Create( "Emission", PinKind::Input ) );
		Inputs.push_back( Ref<FloatPin>::Create( "Roughness", PinKind::Input ) );
		Inputs.push_back( Ref<FloatPin>::Create( "Metalness", PinKind::Input ) );
	}

	SMatGraph2_OutputNode::~SMatGraph2_OutputNode()
	{
	}

	NodeEditorTaskBase* SMatGraph2_OutputNode::ConvertToTask()
	{
		return nullptr;
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( SMatGraph2_OutputNode );
