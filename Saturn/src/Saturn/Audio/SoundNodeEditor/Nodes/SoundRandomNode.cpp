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
#include "SoundRandomNode.h"

#if !defined(SAT_DIST)
#include "Saturn/NodeEditor/UI/NodeEditor.h"
#else
#include "Saturn/NodeEditor/NodeEditorBase.h"
#endif

#include "SoundPlayerNode.h"

#include "Saturn/Audio/SoundNodeEditor/SoundPin.h"
#include "Saturn/Audio/SoundNodeEditor/SoundEditorEvaluator.h"

#include "Saturn/Core/Random.h"

namespace Saturn {

	SoundRandomNode::SoundRandomNode()
		: Node()
	{
		Name = "Random Sound";
		CreateNode();
	}

	SoundRandomNode::SoundRandomNode( const std::string& rName )
		: Node()
	{
		Name = rName;
		CreateNode();
	}

	void SoundRandomNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::SoundRandomSound;
		Color = ImColor( 173, 18, 128 );

		Inputs.push_back( Ref<SoundPin>::Create( "Sound 1", PinKind::Input ) );
		Inputs.push_back( Ref<SoundPin>::Create( "Sound 2", PinKind::Input ) );
		
		Outputs.push_back( Ref<SoundPin>::Create( "Result", PinKind::Output ) );
	}

	SoundRandomNode::~SoundRandomNode()
	{
	}

	NodeEditorCompilationStatus SoundRandomNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		SoundEditorEvaluator* pSoundEditorEvaluator = dynamic_cast< SoundEditorEvaluator* >( evaluator );

		if( !pSoundEditorEvaluator )
			return NodeEditorCompilationStatus::Failed;

		std::map<UUID, UUID> PinToSoundMap;

		auto ids = pSoundEditorEvaluator->GetTargetNodeEditor()->FindNeighbors( this );

#if !defined( SAT_DIST )
		auto count = std::count_if( Inputs.begin(), Inputs.end(), 
			[=](const auto& pin)
			{
				return pSoundEditorEvaluator->GetTargetNodeEditor()->IsLinked( pin->ID );
			} );

		if( count != Inputs.size() )
		{
			Ref<NodeEditor> uiEditor = pSoundEditorEvaluator->GetTargetNodeEditor().As<NodeEditor>();

			uiEditor->ThrowError( "Not all pins are linked to the random node!" );

			return NodeEditorCompilationStatus::Failed;
		}
#endif
		// Read inputs for sound indexes.
		size_t pin = Random::RandomElementInRange( 0, Inputs.size() - 1 );
		size_t index = ( size_t ) Inputs[ pin ].As<SoundPin>()->Data;

		// Register this sound to be played
		pSoundEditorEvaluator->RegisterSound( index );

		// Unregister any other sounds.
		for( size_t i = 0; i < Inputs.size(); i++ )
		{
			Ref<SoundPin> soundInputPin = Inputs[ i ].As<SoundPin>();

			if( soundInputPin->Data != index )
			{
				// Don't use i because thats just the index in our Inputs array
				// Use the actual data stored in the pin so we can the sound index.
				pSoundEditorEvaluator->UnregisterSound( soundInputPin->Data );
			}
		}

		// Write our chosen index to the output pin
		Ref<SoundPin> outPin = Outputs[ 0 ].As<SoundPin>();

		Ref<Link> link = pSoundEditorEvaluator->GetTargetEditor()->FindLinkByPin( outPin->ID );
		if( link )
		{
			// Find input node
			Ref<SoundPin> inputPin = pSoundEditorEvaluator->GetTargetEditor()->FindPin( link->EndPinID ).As<SoundPin>();
			inputPin->Data = outPin->Data = ( int ) index;
		}

		return NodeEditorCompilationStatus::Success;
	}

}
