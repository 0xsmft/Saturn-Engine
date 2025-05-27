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
#include "SoundOutputNode.h"

#if !defined( SAT_DIST )
#include "Saturn/NodeEditor/UI/NodeEditor.h"
#else
#include "Saturn/NodeEditor/NodeEditorBase.h"
#endif

#include "Saturn/Audio/SoundNodeEditor/SoundEditorEvaluator.h"
#include "Saturn/Audio/SoundNodeEditor/SoundPin.h"

#include "Saturn/Audio/AudioSystem.h"
#include "Saturn/Audio/Sound.h"

namespace Saturn {

	SoundOutputNode::SoundOutputNode()
		: NodeEditorBlueprintNode( "Sound Output" )
	{
		CreateNode();
	}

	void SoundOutputNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::SoundOutput;
#if !defined(SAT_DIST)
		CanBeDeleted = false;
		Color = ImColor( 237, 202, 5, 255 );
//		Color = ImColor( 255, 128, 128 );
#endif

		Inputs.push_back( Ref<SoundPin>::Create( "Final Result", PinKind::Input ) );
	}

	SoundOutputNode::~SoundOutputNode()
	{
	}

	Saturn::NodeEvaluationState SoundOutputNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		SoundEditorEvaluator* pSoundEditorEvaluator = dynamic_cast<SoundEditorEvaluator*>( evaluator );
		
		if( !pSoundEditorEvaluator )
			return NodeEvaluationState::Failed;

		// For all currently alive sounds check if they are allowed to play
		// if they are then we play them if not then we'll unload and erase
		size_t index = 0;
		for( auto Itr = pSoundEditorEvaluator->AliveSounds.begin(); Itr != pSoundEditorEvaluator->AliveSounds.end(); )
		{
			Ref<Sound>& rSound = *(Itr);

			if( pSoundEditorEvaluator->SoundsPlaying.count( index ) > 0 )
			{
				rSound->Play();
				Itr++;
			}
			else
			{
				// erase
				rSound->Unload();
#if !defined( SAT_DIST )
#endif

				Itr = pSoundEditorEvaluator->AliveSounds.erase( Itr );
			}

			index++;
		}

#if !defined( SAT_DIST )
		Ref<NodeEditor> uiEditor = pSoundEditorEvaluator->GetTargetNodeEditor().As<NodeEditor>();
		std::string message = std::format( "Playing {0} of out {1} sounds.", pSoundEditorEvaluator->SoundsPlaying.size(), pSoundEditorEvaluator->AliveSounds.size() );

		uiEditor->PushInfoMessage( message );
		SAT_CORE_INFO( message );
#endif

		pSoundEditorEvaluator->SoundsPlaying.clear();

		return NodeEvaluationState::Evaluated;
	}

}
