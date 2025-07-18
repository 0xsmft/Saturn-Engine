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
#include "SoundMixerNode.h"

#if !defined(SAT_DIST)
#include "Saturn/NodeEditor/UI/NodeEditor.h"
#else
#include "Saturn/NodeEditor/NodeEditorBase.h"
#endif

#include "SoundPlayerNode.h"
#include "SoundRandomSoundNode.h"

#include "Saturn/Audio/SoundNodeEditor/SoundPin.h"
#include "Saturn/Audio/SoundNodeEditor/SoundEditorEvaluator.h"

namespace Saturn {

	SoundMixerNode::SoundMixerNode()
		: NodeEditorBlueprintNode()
	{
		Name = "Mixer";
		CreateNode();
	}

	SoundMixerNode::SoundMixerNode( const std::string& rName )
		: NodeEditorBlueprintNode( rName )
	{
		CreateNode();
	}

	void SoundMixerNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::SoundMixer;
		Color = ImColor( 173, 18, 128 );

		Inputs.push_back( Ref<SoundPin>::Create( "Sound 1", PinKind::Input ) );
		Inputs.push_back( Ref<SoundPin>::Create( "Sound 2", PinKind::Input ) );
		
		Outputs.push_back( Ref<SoundPin>::Create( "Result", PinKind::Output ) );
	}

	SoundMixerNode::~SoundMixerNode()
	{
	}

	Saturn::NodeEvaluationState SoundMixerNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		SoundEditorEvaluator* pSoundEditorEvaluator = dynamic_cast< SoundEditorEvaluator* >( evaluator );

		if( !pSoundEditorEvaluator )
			return NodeEvaluationState::Failed;

#if !defined( SAT_DIST )
		auto count = std::count_if( Inputs.begin(), Inputs.end(),
			[=]( const auto& pin )
			{
				return pSoundEditorEvaluator->GetTargetNodeEditor()->IsLinked( pin->ID );
			} );

		if( count != Inputs.size() )
		{
			Ref<NodeEditor> uiEditor = pSoundEditorEvaluator->GetTargetNodeEditor().As<NodeEditor>();

			uiEditor->ThrowError( "Not all pins are linked to the mixer node!" );

			return NodeEvaluationState::Failed;
		}
#endif

		for( size_t i = 0; i < Inputs.size(); i++ )
		{
			pSoundEditorEvaluator->RegisterSound( i );
		}

		return NodeEvaluationState::Evaluated;
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( SoundMixerNode );
