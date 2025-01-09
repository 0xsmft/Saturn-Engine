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

#include "Saturn/NodeEditor/NodeEditorBase.h"
#include "Saturn/Audio/SoundNodeEditor/SoundEditorEvaluator.h"

#include "Saturn/Audio/AudioSystem.h"
#include "Saturn/Audio/Sound.h"

#include <unordered_map>

namespace Saturn {

	SoundOutputNode::SoundOutputNode( const NodeSpecification& rSpec )
		: Node( rSpec )
	{
		ExecutionType = NodeExecutionType::SoundOutput;
#if !defined(SAT_DIST)
		CanBeDeleted = false;
		Color = ImColor( 237, 202, 5, 255 );
#endif
	}

	SoundOutputNode::~SoundOutputNode()
	{
	}

	NodeEditorCompilationStatus SoundOutputNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		SoundEditorEvaluator* pSoundEditorEvaluator = dynamic_cast<SoundEditorEvaluator*>( evaluator );
		
		if( !pSoundEditorEvaluator )
			return NodeEditorCompilationStatus::Failed;

		std::stack<UUID>& soundStack = pSoundEditorEvaluator->SoundStack;
		
		std::vector<UUID> soundVector;
		std::vector<UUID> playerID;

		while( !soundStack.empty() )
		{
			const UUID soundAssetID = soundStack.top();
			soundStack.pop();

			soundVector.push_back( soundAssetID );
			playerID.push_back( UUID() );
		}

		AudioSystem::Get().RequestNewSounds( soundVector, playerID, 
			[=]( Ref<Sound> sound ) 
			{
				pSoundEditorEvaluator->AliveSounds.push_back( sound );
			} );

		return NodeEditorCompilationStatus::Success;
	}
}