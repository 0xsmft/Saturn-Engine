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
#include "SoundRandomPitchNode.h"

#if !defined(SAT_DIST)
#include "Saturn/NodeEditor/UI/NodeEditor.h"
#else
#include "Saturn/NodeEditor/NodeEditorBase.h"
#endif

#include "Saturn/Audio/Sound.h"

#include "Saturn/Audio/SoundNodeEditor/SoundPin.h"
#include "Saturn/Audio/SoundNodeEditor/SoundEditorEvaluator.h"

#include "Saturn/Core/Random.h"

namespace Saturn {

	SoundRandomPitchNode::SoundRandomPitchNode()
		: Node()
	{
		Name = "Random Pitch";
		CreateNode();
	}

	SoundRandomPitchNode::SoundRandomPitchNode( const std::string& rName )
		: Node( rName )
	{
		CreateNode();
	}

	void SoundRandomPitchNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::SoundRandomPitch;
		Color = ImColor( 173, 18, 128 );

		Inputs.push_back( Ref<SoundPin>::Create( "Sound", PinKind::Input ) );

		Inputs.push_back( Ref<FloatPin>::Create( "Min", PinKind::Input ) );
		Inputs.push_back( Ref<FloatPin>::Create( "Max", PinKind::Input ) );

		Outputs.push_back( Ref<SoundPin>::Create( "Result", PinKind::Output ) );

		Inputs[ 1 ].As<FloatPin>()->Data = 1.0f;
		Inputs[ 2 ].As<FloatPin>()->Data = 2.0f;
	}

	SoundRandomPitchNode::~SoundRandomPitchNode()
	{
	}

	NodeEditorCompilationStatus SoundRandomPitchNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		SoundEditorEvaluator* pSoundEditorEvaluator = dynamic_cast< SoundEditorEvaluator* >( evaluator );

		if( !pSoundEditorEvaluator )
			return NodeEditorCompilationStatus::Failed;

		// Get random number in range
		float pitch = Random::RandomFloatInRange( Inputs[ 1 ].As<FloatPin>()->Data, Inputs[ 2 ].As<FloatPin>()->Data );

#if !defined( SAT_DIST )
		Ref<NodeEditor> uiEditor = pSoundEditorEvaluator->GetTargetNodeEditor().As<NodeEditor>();
		uiEditor->PushInfoMessage( std::format( "Random pitch is: {0} (NC/{1})", pitch, (uint64_t)ID ) );
#endif

		// Set pitch
		Ref<SoundPin> soundPin = Inputs[ 0 ].As<SoundPin>();
		pSoundEditorEvaluator->AliveSounds[ soundPin->Data ]->SetPitch( pitch );

		// Write data (pass from our input pin)
		Ref<SoundPin> Outpin = Outputs[ 0 ].As<SoundPin>();

		Ref<Link> link = pSoundEditorEvaluator->GetTargetEditor()->FindLinkByPin( Outpin->ID );
		if( link )
		{
			// Find input node
			Ref<SoundPin> inputPin = pSoundEditorEvaluator->GetTargetEditor()->FindPin( link->EndPinID );
			inputPin->Data = Outpin->Data = soundPin->Data;
		}
		
		pSoundEditorEvaluator->RegisterSound( soundPin->Data );

		return NodeEditorCompilationStatus::Success;
	}

}
