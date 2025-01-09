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

#if !defined(SAT_DIST)
#include "Saturn/Audio/SoundNodeEditor/SoundEditorEvaluator.h"
#include "Saturn/NodeEditor/UI/NodeEditor.h"
#endif

namespace Saturn {

	SoundPlayerNode::SoundPlayerNode( const NodeSpecification& rSpec )
		: Node( rSpec )
	{
		ExecutionType = NodeExecutionType::SoundPlayer;
	}

	SoundPlayerNode::~SoundPlayerNode()
	{
	}

	NodeEditorCompilationStatus SoundPlayerNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
#if !defined(SAT_DIST)
		if( SoundAssetID == 0 )
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

	void SoundPlayerNode::OnRenderOutput( Ref<Pin> pin )
	{
		bool showAssetFinder = false;
		
		switch( pin->Type )
		{
			case PinType::Sound:
			{
				std::string name = "Select Asset";

				if( SoundAssetID != 0 )
					name = std::to_string( SoundAssetID );

				if( ImGui::Button( name.c_str() ) )
				{
					showAssetFinder = true;
				}
			} break;
		}

		ed::Suspend();
		Auxiliary::DrawAssetFinder( AssetType::Sound, &showAssetFinder, SoundAssetID );
		ed::Resume();
	}

	void SoundPlayerNode::OnSerialise( std::ofstream& rStream ) const
	{
		UUID::Serialise( SoundAssetID, rStream );
	}

	void SoundPlayerNode::OnDeserialise( std::ifstream& rStream )
	{
		UUID::Deserialise( SoundAssetID, rStream );
	} 
}