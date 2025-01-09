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
#include "GlobalNodesList.h"

#include "Saturn/ImGui/MaterialAssetViewer/MaterialViewerNodes.h"

#include "Saturn/Audio/SoundNodeEditor/Nodes/SoundOutputNode.h"
#include "Saturn/Audio/SoundNodeEditor/Nodes/SoundPlayerNode.h"
#include "Saturn/Audio/SoundNodeEditor/Nodes/SoundRandomNode.h"
#include "Saturn/Audio/SoundNodeEditor/Nodes/SoundMixerNode.h"

#include "Saturn/Audio/SoundNodeEditor/SoundNodeLibrary.h"

#include "NodeEditorBase.h"

namespace Saturn {

	Ref<Node> GlobalNodesList::ConvertExecutionTypeToNode( NodeExecutionType executionType, Ref<NodeEditorBase> nodeEditorBase )
	{
		switch( executionType )
		{
			// TODO: Value is no longer used.
			case NodeExecutionType::Value:
				return nullptr;

			case NodeExecutionType::AssetID:
				return MaterialNodeLibrary::SpawnGetAsset( nodeEditorBase );

			case NodeExecutionType::Sampler2D:
				return MaterialNodeLibrary::SpawnSampler2D( nodeEditorBase );
			case NodeExecutionType::MaterialOutput:
				return MaterialNodeLibrary::SpawnOutputNode( nodeEditorBase );
			case NodeExecutionType::ColorPicker:
				return MaterialNodeLibrary::SpawnColorPicker( nodeEditorBase );
			case NodeExecutionType::MaterialMixColors:
				return MaterialNodeLibrary::SpawnMixColors( nodeEditorBase );

			// Default nodes
			case NodeExecutionType::Add:
				return DefaultNodeLibrary::SpawnAddFloats( nodeEditorBase );
			case NodeExecutionType::Subtract:
				return DefaultNodeLibrary::SpawnSubFloats( nodeEditorBase );
			case NodeExecutionType::Multiply:
				return DefaultNodeLibrary::SpawnMulFloats( nodeEditorBase );
			case NodeExecutionType::Divide:
				return DefaultNodeLibrary::SpawnDivFloats( nodeEditorBase );

			case NodeExecutionType::SoundOutput:
				return SoundNodeLibrary::SpawnOutputNode( nodeEditorBase );
			case NodeExecutionType::SoundPlayer:
				return SoundNodeLibrary::SpawnPlayerNode( nodeEditorBase );
			case NodeExecutionType::RandomSound:
				return SoundNodeLibrary::SpawnRandomNode( nodeEditorBase );
			case NodeExecutionType::SoundMixer:
				return SoundNodeLibrary::SpawnMixerNode( nodeEditorBase );

			case NodeExecutionType::None:
			default:
				return nullptr;
		}

		return nullptr;
	}
}