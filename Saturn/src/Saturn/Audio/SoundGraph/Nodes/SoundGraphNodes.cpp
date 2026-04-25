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
#include "SoundGraphNodes.h"

#include "Saturn/NodeEditor/Pin.h"
#include "Saturn/NodeEditor/AssetIDPin.h"

#include "Saturn/Audio/SoundGraph/SoundEditorEvaluator.h"
#include "Saturn/Audio/SoundGraph/SoundPin.h"
#include "Saturn/Audio/SoundGraph/Tasks/SoundGraphTasks.h"

#include "Saturn/Audio/AudioSystem.h"
#include "Saturn/Audio/Sound.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// SoundOutput

	SoundOutputNode::SoundOutputNode()
		: NodeEditorBlueprintNode( "Sound Output" )
	{
		CreateNode();
	}

	void SoundOutputNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::SoundOutput;
#if !defined(SAT_DIST)
		Flags |= NodeFlags_Irremovable;
		Color = ImColor( 237, 202, 5, 255 );
//		Color = ImColor( 255, 128, 128 );
#endif

		Inputs.push_back( Ref<SoundPin>::Create( "Final Result", PinKind::Input, PinFlag_RequiredForEvaluation ) );
	}

	SoundOutputNode::~SoundOutputNode()
	{
	}

	NodeEditorTaskBase* SoundOutputNode::ConvertToTask()
	{
		return NewObject<SGraphSoundOutputTask>( nullptr );
	}

	/*
	Saturn::NodeEvaluationState SoundOutputNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		SoundEditorEvaluator* pSoundEditorEvaluator = dynamic_cast< SoundEditorEvaluator* >( evaluator );

		if( !pSoundEditorEvaluator )
			return NodeEvaluationState::Failed;

		// For all currently alive sounds check if they are allowed to play
		// if they are then we play them if not then we'll unload and erase
		size_t index = 0;
		for( auto Itr = pSoundEditorEvaluator->AliveSounds.begin(); Itr != pSoundEditorEvaluator->AliveSounds.end(); )
		{
			Ref<Sound>& rSound = *( Itr );

			if( pSoundEditorEvaluator->SoundsPlaying.count( index ) > 0 )
			{
				rSound->Play();
				++Itr;
			}
			else
			{
				// erase
				rSound->Unload();

				Itr = pSoundEditorEvaluator->AliveSounds.erase( Itr );
			}

			++index;
		}

#if !defined( SAT_DIST )
		SharedPtr<NodeEditor> uiEditor = pSoundEditorEvaluator->GetTargetNodeEditor().As<NodeEditor>();
		const std::string message = std::format( "Playing {0} of out {1} sounds.", pSoundEditorEvaluator->SoundsPlaying.size(), pSoundEditorEvaluator->AliveSounds.size() );

		uiEditor->PushInfoMessage( message );
		SAT_CORE_INFO( message );
#endif

		pSoundEditorEvaluator->SoundsPlaying.clear();

		return NodeEvaluationState::Evaluated;
	}
	*/

	//////////////////////////////////////////////////////////////////////////
	// SoundPitch

	SoundPitchNode::SoundPitchNode()
		: NodeEditorBlueprintNode( "Sound Pitch" )
	{
		CreateNode();
	}

	SoundPitchNode::SoundPitchNode( const std::string& rName )
		: NodeEditorBlueprintNode( rName )
	{
		CreateNode();
	}

	void SoundPitchNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::SoundPitch;
#if !defined(SAT_DIST)
		Color = ImColor( 173, 18, 128 );
#endif

		Inputs.push_back( Ref<SoundPin>::Create( "Sound", PinKind::Input ) );
		Inputs.push_back( Ref<FloatPin>::Create( "Pitch", PinKind::Input ) );

		Outputs.push_back( Ref<SoundPin>::Create( "Result", PinKind::Output ) );

		Inputs[ 1 ].As<FloatPin>()->Data = 1.0f;
	}

	SoundPitchNode::~SoundPitchNode()
	{
	}
	
	//////////////////////////////////////////////////////////////////////////
	// SoundPlayer

	SoundPlayerNode::SoundPlayerNode()
		: NodeEditorBlueprintNode( "Sound Player" )
	{
		CreateNode();
	}

	SoundPlayerNode::SoundPlayerNode( const std::string& rName )
		: NodeEditorBlueprintNode( rName )
	{
		CreateNode();
	}

	void SoundPlayerNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::SoundPlayer;
#if !defined(SAT_DIST)
		Color = ImColor( 173, 18, 128 );
#endif

		Outputs.push_back( Ref<AssetIDPin>::Create( "Sound Player", PinKind::Output, AssetType::Sound ) );
		Outputs[ 0 ]->Type = PinType::Sound;

		Inputs.push_back( Ref<BoolPin>::Create( "Spatialisation", PinKind::Input ) );
	}

	SoundPlayerNode::~SoundPlayerNode()
	{
	}

	NodeEditorTaskBase* SoundPlayerNode::ConvertToTask()
	{
		return NewObject<SGraphSoundPlayerTask>( nullptr );
	}

	/*
	Saturn::NodeEvaluationState SoundPlayerNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		SoundEditorEvaluator* pSoundEditorEvaluator = dynamic_cast< SoundEditorEvaluator* >( evaluator );

		if( !pSoundEditorEvaluator )
			return NodeEvaluationState::Failed;

		Ref<AssetIDPin> outPin = Outputs[ 0 ].As<AssetIDPin>();

#if !defined(SAT_DIST)
		if( outPin->GetAssetID() == 0 )
		{
			auto uiEditor = pSoundEditorEvaluator->GetTargetNodeEditor().As<NodeEditor>();
			uiEditor->ThrowError( "No Asset was chosen in the sound player node!" );

			return NodeEvaluationState::Failed;
		}
#endif

		auto links = pSoundEditorEvaluator->GetTargetEditor()->FindLinksByPin( outPin->ID );
		for( const auto& link : links )
		{
			// Find input node
			Ref<Pin> inputPin = pSoundEditorEvaluator->GetTargetEditor()->FindPin( link->EndPinID );

			// Submit to the evaluator
			{
				pSoundEditorEvaluator->AddNewSound( outPin->GetAssetID() );
#if !defined(SAT_DIST)
				pSoundEditorEvaluator->EvaluatedPath[ link->ID ] = NodeEvaluationState::Evaluated;
#endif

				inputPin.As<SoundPin>()->Data = ( int ) pSoundEditorEvaluator->AliveSounds.size() - 1;
			}
		}

		return NodeEvaluationState::Evaluated;
	}
	*/

	uint64_t SoundPlayerNode::GetAssetID() const
	{
		return Outputs[ 0 ].As<AssetIDPin>()->GetAssetID();
	}

	/*
	Saturn::NodeEvaluationState SoundPitchNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		SoundEditorEvaluator* pSoundEditorEvaluator = dynamic_cast< SoundEditorEvaluator* >( evaluator );

		if( !pSoundEditorEvaluator )
			return NodeEvaluationState::Failed;

		Ref<SoundPin> soundPin = Inputs[ 0 ].As<SoundPin>();

		if( pSoundEditorEvaluator->GetTargetEditor()->IsLinked( soundPin->ID ) )
		{
			pSoundEditorEvaluator->AliveSounds[ soundPin->Data ]->SetPitch( Inputs[ 1 ].As<FloatPin>()->Data );
		}

		Ref<SoundPin> Outpin = Outputs[ 0 ].As<SoundPin>();

		auto links = pSoundEditorEvaluator->GetTargetEditor()->FindLinksByPin( Outpin->ID );
		for( const auto& link : links )
		{
			// Find input node
			Ref<SoundPin> inputPin = pSoundEditorEvaluator->GetTargetEditor()->FindPin( link->EndPinID );
			inputPin->Data = Outpin->Data = soundPin->Data;

#if !defined(SAT_DIST)
			pSoundEditorEvaluator->EvaluatedPath[ link->ID ] = NodeEvaluationState::Evaluated;
#endif
		}

		pSoundEditorEvaluator->RegisterSound( Outpin->Data );

		return NodeEvaluationState::Evaluated;
	}
	*/

	//////////////////////////////////////////////////////////////////////////
	// SoundRandomPitch
	
	SoundRandomPitchNode::SoundRandomPitchNode()
		: NodeEditorBlueprintNode( "Random Pitch" )
	{
		CreateNode();
	}

	SoundRandomPitchNode::SoundRandomPitchNode( const std::string& rName )
		: NodeEditorBlueprintNode( rName )
	{
		CreateNode();
	}

	void SoundRandomPitchNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::SoundRandomPitch;
#if !defined(SAT_DIST)
		Color = ImColor( 173, 18, 128 );
#endif

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

	/*
	Saturn::NodeEvaluationState SoundRandomPitchNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		SoundEditorEvaluator* pSoundEditorEvaluator = dynamic_cast< SoundEditorEvaluator* >( evaluator );

		if( !pSoundEditorEvaluator )
			return NodeEvaluationState::Failed;

		// Get random number in range
		const float pitch = Random::RandomFloatInRange( Inputs[ 1 ].As<FloatPin>()->Data, Inputs[ 2 ].As<FloatPin>()->Data );

#if !defined( SAT_DIST )
		SharedPtr<NodeEditor> uiEditor = pSoundEditorEvaluator->GetTargetNodeEditor().As<NodeEditor>();
		uiEditor->PushInfoMessage( std::format( "Random pitch is: {0} (NC/{1})", pitch, ( uint64_t ) ID ) );
#endif

		// Set pitch
		const Ref<SoundPin> soundPin = Inputs[ 0 ].As<SoundPin>();
		pSoundEditorEvaluator->AliveSounds[ soundPin->Data ]->SetPitch( pitch );

		// Write data (pass from our input pin)
		Ref<SoundPin> Outpin = Outputs[ 0 ].As<SoundPin>();

		const auto links = pSoundEditorEvaluator->GetTargetEditor()->FindLinksByPin( Outpin->ID );
		for( const auto& link : links )
		{
			// Find input node
			Ref<SoundPin> inputPin = pSoundEditorEvaluator->GetTargetEditor()->FindPin( link->EndPinID );
			inputPin->Data = Outpin->Data = soundPin->Data;

#if !defined(SAT_DIST)
			pSoundEditorEvaluator->EvaluatedPath[ link->ID ] = NodeEvaluationState::Evaluated;
#endif
		}

		pSoundEditorEvaluator->RegisterSound( soundPin->Data );

		return NodeEvaluationState::Evaluated;
	}
	*/

	//////////////////////////////////////////////////////////////////////////
	// SoundRandomSound

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

		Inputs.push_back( Ref<SoundPin>::Create( "Sound A", PinKind::Input, PinFlag_RequiredForEvaluation ) );
		Inputs.push_back( Ref<SoundPin>::Create( "Sound B", PinKind::Input, PinFlag_RequiredForEvaluation ) );

		Outputs.push_back( Ref<SoundPin>::Create( "Result", PinKind::Output ) );
	}

	SoundRandomSoundNode::~SoundRandomSoundNode()
	{
	}

	NodeEditorTaskBase* SoundRandomSoundNode::ConvertToTask()
	{
		return NewObject<SGraphSoundRandomSoundTask>( nullptr );
	}

	/*
	NodeEvaluationState SoundRandomSoundNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		SoundEditorEvaluator* pSoundEditorEvaluator = dynamic_cast< SoundEditorEvaluator* >( evaluator );

		if( !pSoundEditorEvaluator )
			return NodeEvaluationState::Failed;

		std::map<UUID, UUID> PinToSoundMap;

		const auto ids = pSoundEditorEvaluator->GetTargetNodeEditor()->FindNeighborsRight( SharedFromThis() );

#if !defined( SAT_DIST )
		const auto count = std::count_if( Inputs.begin(), Inputs.end(),
			[ = ]( const auto& pin )
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
	*/

	//////////////////////////////////////////////////////////////////////////
	// SoundFloatConst

	SoundFloatConst::SoundFloatConst()
		: NodeEditorBlueprintNode( "Constant Float" )
	{
		CreateNode();
	}

	SoundFloatConst::SoundFloatConst( const std::string& rName )
		: NodeEditorBlueprintNode( rName )
	{
		CreateNode();
	}

	void SoundFloatConst::CreateNode()
	{
		ExecutionType = NodeExecutionType::SoundFloatConst;
#if !defined(SAT_DIST)
		Color = ImColor( 173, 18, 128 );
#endif

		Outputs.push_back( Ref<FloatPin>::Create( "Constant", PinKind::Output ) );
	}

	SoundFloatConst::~SoundFloatConst()
	{
	}
	
}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( SoundOutputNode );
SAT_X31_CREATE_AUTO_REG( SoundPitchNode );
SAT_X31_CREATE_AUTO_REG( SoundPlayerNode );
SAT_X31_CREATE_AUTO_REG( SoundRandomPitchNode );
SAT_X31_CREATE_AUTO_REG( SoundRandomSoundNode );
SAT_X31_CREATE_AUTO_REG( SoundFloatConst );
