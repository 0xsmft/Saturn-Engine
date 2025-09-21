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
	class GraphSound;

#if !defined(SAT_DIST)
	struct GraphSoundAssetViewerReference
	{
		Ref<GraphSound> Sound;
	};
#endif

	class GraphSoundAssetViewer : public AssetViewer
	{
	public:
		GraphSoundAssetViewer( AssetID id );
		~GraphSoundAssetViewer();

		virtual void OnImGuiRender() override;
		virtual void OnUpdate( Timestep ts ) override;
		virtual void OnEvent( Event& rEvent ) override;
		virtual void OnRuntimeStateChanged( RuntimeState newState, RuntimeState oldState ) override;

#if !defined(SAT_DIST)
		void AddSoundReference( Ref<GraphSound> sound );
#endif

	private:
		void AddSoundAsset();
		void SetupNewNodeEditor();
		void SetupNodeEditorCallbacks();

	private:
		// Sound specification asset
		Ref<Asset> m_Asset = nullptr;
		// Current graph that we are drawing
		SharedPtr<NodeEditor> m_NodeEditor = nullptr;
		Ref<SoundEditorEvaluator> m_Runtime = nullptr;

		bool m_ShowDirtyModal = false;
		UUID m_OutputNodeID = 0;

#if !defined(SAT_DIST)
		// The asset that is referencing this viewer
		// For example we could have the same sound spec asset being used in different places so which one are we trying to view (only available when in Runtime).
		std::vector<Ref<GraphSound>> m_ReferencingAssets;

		SharedPtr<NodeEditor> m_OriginalNodeEditor = nullptr;
#endif
	};
}
