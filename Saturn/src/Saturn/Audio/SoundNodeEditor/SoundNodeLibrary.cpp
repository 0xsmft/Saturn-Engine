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
#include "SoundNodeLibrary.h"

#include "Nodes/SoundOutputNode.h"
#include "Nodes/SoundPlayerNode.h"
#include "Nodes/SoundRandomSoundNode.h" 
#include "Nodes/SoundMixerNode.h" 
#include "Nodes/SoundPitchNode.h" 
#include "Nodes/SoundRandomPitchNode.h"
#include "Nodes/SoundFloatConstNode.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

namespace Saturn {

	SharedPtr<SoundRandomSoundNode> SoundNodeLibrary::SpawnRandomNode( SharedPtr<NodeEditorBase> nodeEditor )
	{
		SharedPtr<SoundRandomSoundNode> node = ( SoundRandomSoundNode* ) ClassMetadataHandler::Get().CreateClassObject( SoundRandomSoundNode::StaticClass() );
		nodeEditor->AddNode( node );

		return node;
	}

	SharedPtr<SoundMixerNode> SoundNodeLibrary::SpawnMixerNode( SharedPtr<NodeEditorBase> nodeEditor )
	{
		SharedPtr<SoundMixerNode> node = ( SoundMixerNode* ) ClassMetadataHandler::Get().CreateClassObject( SoundMixerNode::StaticClass() );
		nodeEditor->AddNode( node );

		return node;
	}

	SharedPtr<SoundPlayerNode> SoundNodeLibrary::SpawnPlayerNode( SharedPtr<NodeEditorBase> nodeEditor )
	{
		SharedPtr<SoundPlayerNode> node = ( SoundPlayerNode* ) ClassMetadataHandler::Get().CreateClassObject( SoundPlayerNode::StaticClass() );
		nodeEditor->AddNode( node );

		return node;
	}

	SharedPtr<SoundPitchNode> SoundNodeLibrary::SpawnPitchNode( SharedPtr<NodeEditorBase> nodeEditor )
	{
		SharedPtr<SoundPitchNode> node = ( SoundPitchNode* ) ClassMetadataHandler::Get().CreateClassObject( SoundPitchNode::StaticClass() );
		nodeEditor->AddNode( node );

		return node;
	}

	SharedPtr<SoundRandomPitchNode> SoundNodeLibrary::SpawnRandPitch( SharedPtr<NodeEditorBase> nodeEditor )
	{
		SharedPtr<SoundRandomPitchNode> node = ( SoundRandomPitchNode* ) ClassMetadataHandler::Get().CreateClassObject( SoundRandomPitchNode::StaticClass() );
		nodeEditor->AddNode( node );

		return node;
	}

	SharedPtr<SoundFloatConst> SoundNodeLibrary::SpawnFloatConst( SharedPtr<NodeEditorBase> nodeEditor )
	{
		SharedPtr<SoundFloatConst> node = ( SoundFloatConst* ) ClassMetadataHandler::Get().CreateClassObject( SoundFloatConst::StaticClass() );
		nodeEditor->AddNode( node );

		return node;
	}

	SharedPtr<SoundOutputNode> SoundNodeLibrary::SpawnOutputNode( SharedPtr<NodeEditorBase> nodeEditor )
	{
		SharedPtr<SoundOutputNode> node = ( SoundOutputNode* ) ClassMetadataHandler::Get().CreateClassObject( SoundOutputNode::StaticClass() );
		nodeEditor->AddNode( node );

		return node;
	}
}
