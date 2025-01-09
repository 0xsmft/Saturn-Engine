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
#include "Nodes/SoundRandomNode.h" 
#include "Nodes/SoundMixerNode.h" 

namespace Saturn {

	Ref<SoundRandomNode> SoundNodeLibrary::SpawnRandomNode( Ref<NodeEditorBase> nodeEditor )
	{
		NodeSpecification nodeSpec;
		nodeSpec.Color = ImColor( 173, 18, 128 );
		nodeSpec.Name = "Random";

		PinSpecification pinSpec;
		pinSpec.Name = "Sound 1";
		pinSpec.Type = PinType::Sound;

		nodeSpec.Inputs.push_back( pinSpec );
		pinSpec.Name = "Sound 2";
		nodeSpec.Inputs.push_back( pinSpec );

		pinSpec.Name = "Out Sound";
		nodeSpec.Outputs.push_back( pinSpec );

		Ref<SoundRandomNode> node = Ref<SoundRandomNode>::Create( nodeSpec );
		nodeEditor->AddNode( node );

		return node;
	}

	Ref<SoundMixerNode> SoundNodeLibrary::SpawnMixerNode( Ref<NodeEditorBase> nodeEditor )
	{
		NodeSpecification nodeSpec;
		nodeSpec.Color = ImColor( 173, 18, 128 );
		nodeSpec.Name = "Mixer";

		PinSpecification pinSpec;
		pinSpec.Name = "Sound 1";
		pinSpec.Type = PinType::Sound;

		nodeSpec.Inputs.push_back( pinSpec );
		pinSpec.Name = "Sound 2";
		nodeSpec.Inputs.push_back( pinSpec );

		pinSpec.Name = "Out Sound";
		nodeSpec.Outputs.push_back( pinSpec );

		Ref<SoundMixerNode> node = Ref<SoundMixerNode>::Create( nodeSpec );
		nodeEditor->AddNode( node );

		return node;
	}

	Ref<SoundPlayerNode> SoundNodeLibrary::SpawnPlayerNode( Ref<NodeEditorBase> nodeEditor )
	{
		PinSpecification inputPin;
		inputPin.Name = "Sound";
		inputPin.Type = PinType::Sound;

		NodeSpecification nodeSpec;
		nodeSpec.Color = ImColor( 173, 18, 128 );
		nodeSpec.Name = "Sound Player";
		nodeSpec.Outputs.push_back( inputPin );

		Ref<SoundPlayerNode> node = Ref<SoundPlayerNode>::Create( nodeSpec );
		nodeEditor->AddNode( node );

		return node;
	}

	Ref<SoundOutputNode> SoundNodeLibrary::SpawnOutputNode( Ref<NodeEditorBase> nodeEditor )
	{
		PinSpecification inputPin;
		inputPin.Name = "Final Result";
		inputPin.Type = PinType::Sound;

		NodeSpecification nodeSpec;
		nodeSpec.Color = ImColor( 255, 128, 128 );
		nodeSpec.Name = "Sound Output";
		nodeSpec.Inputs.push_back( inputPin );

		Ref<SoundOutputNode> node = Ref<SoundOutputNode>::Create( nodeSpec );
		nodeEditor->AddNode( node );

		return node;
	}
}