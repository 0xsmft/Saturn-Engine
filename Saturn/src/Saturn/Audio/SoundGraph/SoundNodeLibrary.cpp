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
#include "SoundNodeLibrary.h"

#include "Nodes/SoundGraphNodes.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

namespace Saturn {

	SharedPtr<SoundRandomSoundNode> SoundNodeLibrary::SpawnRandomNode( SharedPtr<NodeEditor> nodeEditor )
	{
		SharedPtr<SoundRandomSoundNode> node( NewObject<SoundRandomSoundNode>( nodeEditor.Get() ) );
		nodeEditor->AddNode( node );

		return node;
	}

	SharedPtr<SoundPlayerNode> SoundNodeLibrary::SpawnPlayerNode( SharedPtr<NodeEditor> nodeEditor )
	{
		SharedPtr<SoundPlayerNode> node( NewObject<SoundPlayerNode>( nodeEditor.Get() ) );
		nodeEditor->AddNode( node );

		return node;
	}

	SharedPtr<SoundPitchNode> SoundNodeLibrary::SpawnPitchNode( SharedPtr<NodeEditor> nodeEditor )
	{
		SharedPtr<SoundPitchNode> node( NewObject<SoundPitchNode>( nodeEditor.Get() ) );
		nodeEditor->AddNode( node );

		return node;
	}

	SharedPtr<SoundRandomPitchNode> SoundNodeLibrary::SpawnRandPitch( SharedPtr<NodeEditor> nodeEditor )
	{
		SharedPtr<SoundRandomPitchNode> node( NewObject<SoundRandomPitchNode>( nodeEditor.Get() ) );
		nodeEditor->AddNode( node );

		return node;
	}

	SharedPtr<SoundOutputNode> SoundNodeLibrary::SpawnOutputNode( SharedPtr<NodeEditor> nodeEditor )
	{
		SharedPtr<SoundOutputNode> node( NewObject<SoundOutputNode>( nodeEditor.Get() ) );
		nodeEditor->AddNode( node );

		return node;
	}
}
