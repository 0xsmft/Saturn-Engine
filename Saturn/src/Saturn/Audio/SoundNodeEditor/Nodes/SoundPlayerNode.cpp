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
#include "SoundPlayerNode.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include "Saturn/NodeEditor/AssetIDPin.h"

#include "Saturn/Audio/SoundNodeEditor/SoundPin.h"

#include "Saturn/Audio/SoundNodeEditor/SoundEditorEvaluator.h"

#if !defined(SAT_DIST)
#include "Saturn/NodeEditor/UI/NodeEditor.h"
#else
#include "Saturn/NodeEditor/NodeEditorBase.h"
#endif

namespace Saturn {

	SoundPlayerNode::SoundPlayerNode()
		: NodeEditorBlueprintNode()
	{
		Name = "Sound Player";
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
		Color = ImColor( 173, 18, 128 );

		Outputs.push_back( Ref<AssetIDPin>::Create( "Sound Player", PinKind::Output, AssetType::Sound ) );
		Outputs[ 0 ]->Type = PinType::Sound;
	}

	SoundPlayerNode::~SoundPlayerNode()
	{
	}

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

		// BUG: FindLinkByPin only returns the first link!
		// Create the sound now, if possible
		Ref<Link> link = pSoundEditorEvaluator->GetTargetEditor()->FindLinkByPin( outPin->ID );
		if( link )
		{
			// Find input node
			Ref<Pin> inputPin = pSoundEditorEvaluator->GetTargetEditor()->FindPin( link->EndPinID );

			Ref<NodeEditorNodeBase> node = inputPin->Node;

			// Submit to the evaluator
			{
				pSoundEditorEvaluator->AddNewSound( outPin->GetAssetID() );
#if !defined(SAT_DIST)
				pSoundEditorEvaluator->EvaluatedPath[ link->ID ] = NodeEvaluationState::Evaluated;
#endif

				inputPin.As<SoundPin>()->Data = (int)pSoundEditorEvaluator->AliveSounds.size() - 1;
			}
		}

		return NodeEvaluationState::Evaluated;
	}

	AssetID SoundPlayerNode::GetAssetID() const
	{
		return Outputs[ 0 ].As<AssetIDPin>()->GetAssetID();
	}

}
