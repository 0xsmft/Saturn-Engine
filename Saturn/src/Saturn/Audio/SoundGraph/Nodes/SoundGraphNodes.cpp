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

		Inputs.push_back( Ref<SoundPin>::Create( "Sound", PinKind::Input, PinFlag_RequiredForEvaluation ) );
		Inputs.push_back( Ref<FloatPin>::Create( "Pitch", PinKind::Input ) );

		Outputs.push_back( Ref<SoundPin>::Create( "Result", PinKind::Output ) );

		Inputs[ 1 ].As<FloatPin>()->Data = 1.0f;
	}

	SoundPitchNode::~SoundPitchNode()
	{
	}
	
	NodeEditorTaskBase* SoundPitchNode::ConvertToTask()
	{
		return NewObject<SGraphSoundPitchTask>( nullptr );
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

	uint64_t SoundPlayerNode::GetAssetID() const
	{
		return Outputs[ 0 ].As<AssetIDPin>()->GetAssetID();
	}

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

		Inputs.push_back( Ref<SoundPin>::Create( "Sound", PinKind::Input, PinFlag_RequiredForEvaluation ) );

		Inputs.push_back( Ref<FloatPin>::Create( "Min", PinKind::Input ) );
		Inputs.push_back( Ref<FloatPin>::Create( "Max", PinKind::Input ) );

		Outputs.push_back( Ref<SoundPin>::Create( "Result", PinKind::Output ) );

		Inputs[ 1 ].As<FloatPin>()->Data = 1.0f;
		Inputs[ 2 ].As<FloatPin>()->Data = 2.0f;
	}

	SoundRandomPitchNode::~SoundRandomPitchNode()
	{
	}

	NodeEditorTaskBase* SoundRandomPitchNode::ConvertToTask()
	{
		return NewObject<SGraphSoundRandomPitchTask>( nullptr );
	}

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

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( SoundOutputNode );
SAT_X31_CREATE_AUTO_REG( SoundPitchNode );
SAT_X31_CREATE_AUTO_REG( SoundPlayerNode );
SAT_X31_CREATE_AUTO_REG( SoundRandomPitchNode );
SAT_X31_CREATE_AUTO_REG( SoundRandomSoundNode );
