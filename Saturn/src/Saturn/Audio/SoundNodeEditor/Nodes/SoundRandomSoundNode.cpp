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
#include "SoundRandomSoundNode.h"

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

	SoundRandomSoundNode::SoundRandomSoundNode()
		: NodeEditorBlueprintNode( "Random Sound" )
	{
		CreateNode();
	}

	SoundRandomSoundNode::SoundRandomSoundNode( const std::string& rName )
		: NodeEditorBlueprintNode( rName )
	{
		CreateNode();
	}

	void SoundRandomSoundNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::SoundRandomSound;
#if !defined(SAT_DIST)
		Color = ImColor( 173, 18, 128 );
#endif

		Inputs.push_back( Ref<SoundPin>::Create( "Sound 1", PinKind::Input ) );
		Inputs.push_back( Ref<SoundPin>::Create( "Sound 2", PinKind::Input ) );
		
		Outputs.push_back( Ref<SoundPin>::Create( "Result", PinKind::Output ) );
	}

	SoundRandomSoundNode::~SoundRandomSoundNode()
	{
	}

	Saturn::NodeEvaluationState SoundRandomSoundNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		SoundEditorEvaluator* pSoundEditorEvaluator = dynamic_cast< SoundEditorEvaluator* >( evaluator );

		if( !pSoundEditorEvaluator )
			return NodeEvaluationState::Failed;

		std::map<UUID, UUID> PinToSoundMap;

		const auto ids = pSoundEditorEvaluator->GetTargetNodeEditor()->FindNeighborsRight( SharedFromThis() );

#if !defined( SAT_DIST )
		const auto count = std::count_if( Inputs.begin(), Inputs.end(), 
			[=](const auto& pin)
			{
				return pSoundEditorEvaluator->GetTargetNodeEditor()->IsLinked( pin->ID );
			} );

		if( count != Inputs.size() )
		{
			SharedPtr<NodeEditor> uiEditor = pSoundEditorEvaluator->GetTargetNodeEditor().As<NodeEditor>();

			uiEditor->ThrowError( "Not all pins are linked to the random node!" );

			return NodeEvaluationState::Failed;
		}
#endif
		// Read inputs for sound indexes.
		const size_t pin = Random::RandomElementInRange( 0, Inputs.size() - 1 );
		const size_t index = ( size_t ) Inputs[ pin ].As<SoundPin>()->Data;

		// Register this sound to be played
		pSoundEditorEvaluator->RegisterSound( index );

		// Unregister any other sounds.
		for( size_t i = 0; i < Inputs.size(); i++ )
		{
			const Ref<SoundPin> soundInputPin = Inputs[ i ].As<SoundPin>();
			if( soundInputPin->Data != index )
			{
				// Don't use i because thats just the index in our Inputs array
				// Use the actual data stored in the pin so we can the sound index.
				pSoundEditorEvaluator->UnregisterSound( soundInputPin->Data );

#if	!defined( SAT_DIST )
				auto links = pSoundEditorEvaluator->GetTargetEditor()->FindLinksByPin( soundInputPin->ID );
				for( auto& link : links )
				{
					if( link->EndPinID == soundInputPin->ID )
					{
						pSoundEditorEvaluator->EvaluatedPath[ link->ID ] = NodeEvaluationState::WasEvaluated;
					}
				}
#endif
			}
		}

		// Write our chosen index to the output pin
		Ref<SoundPin> outPin = Outputs[ 0 ].As<SoundPin>();

		const auto links = pSoundEditorEvaluator->GetTargetEditor()->FindLinksByPin( outPin->ID );
		for( const auto& link : links )
		{
			// Find input node
			Ref<SoundPin> inputPin = pSoundEditorEvaluator->GetTargetEditor()->FindPin( link->EndPinID ).As<SoundPin>();
			inputPin->Data = outPin->Data = ( int ) index;

#if !defined(SAT_DIST)
			pSoundEditorEvaluator->EvaluatedPath[ link->ID ] = NodeEvaluationState::Evaluated;
#endif
		}

		return NodeEvaluationState::Evaluated;
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( SoundRandomSoundNode );
