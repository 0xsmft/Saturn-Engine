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

#if !defined(SAT_DIST)
#include "Saturn/Audio/SoundNodeEditor/SoundEditorEvaluator.h"
#include "Saturn/NodeEditor/UI/NodeEditor.h"
#endif

namespace Saturn {

	SoundPlayerNode::SoundPlayerNode()
		: Node()
	{
		Name = "Sound Player";
		CreateNode();
	}

	SoundPlayerNode::SoundPlayerNode( const std::string& rName )
		: Node()
	{
		Name = rName;
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

	NodeEditorCompilationStatus SoundPlayerNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
#if !defined(SAT_DIST)
		if( Outputs[ 0 ].As<AssetIDPin>()->GetAssetID() == 0 )
		{
			SoundEditorEvaluator* pSoundEvaluator = dynamic_cast<SoundEditorEvaluator*>( evaluator );

			if( pSoundEvaluator )
			{
				auto uiEditor = pSoundEvaluator->GetTargetNodeEditor().As<NodeEditor>();

				uiEditor->ThrowError( "No Asset was chosen in the sound player node!" );
			}

			return NodeEditorCompilationStatus::Failed;
		}
#endif

		return NodeEditorCompilationStatus::Success;
	}

	AssetID SoundPlayerNode::GetAssetID() const
	{
		return Outputs[ 0 ].As<AssetIDPin>()->GetAssetID();
	}

}
