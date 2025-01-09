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

#pragma once

#include "Saturn/ImGui/AssetViewer.h"
#include "Saturn/NodeEditor/UI/NodeEditor.h"
#include "Saturn/Audio/Sound.h"

namespace Saturn {

	class SoundEditorEvaluator;

	class GraphSoundAssetViewer : public AssetViewer
	{
	public:
		GraphSoundAssetViewer( AssetID id );
		~GraphSoundAssetViewer();

		virtual void OnImGuiRender() override;
		virtual void OnUpdate( Timestep ts ) override;
		virtual void OnEvent( RubyEvent& rEvent ) override;

	private:
		void AddSoundAsset();
		void SetupNewNodeEditor();
		void SetupNodeEditorCallbacks();
		void ShowDirtyModal();

	private:
		Ref<Asset> m_Asset = nullptr;
		Ref<NodeEditor> m_NodeEditor = nullptr;
		Ref<SoundEditorEvaluator> m_Runtime = nullptr;

		bool m_ShowDirtyModal = false;

		UUID m_OutputNodeID = 0;
	};
}