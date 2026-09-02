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

#pragma once

#include "Saturn/MaterialGraph/Graph/MatGraph2_Graph.h"
#include "Saturn/MaterialGraph/MaterialGraph2.h"
#include "Saturn/ImGui/AssetViewer.h"

namespace Saturn {

	class MaterialGraph2AssetViewer : public AssetViewer
	{
	public:
		MaterialGraph2AssetViewer( AssetID id );
		virtual ~MaterialGraph2AssetViewer();

		virtual void OnImGuiRender() override;
		virtual void OnUpdate( Timestep ts ) override;
		virtual void OnEvent( Event& rEvent ) override;
		virtual void OnRuntimeStateChanged( RuntimeState newState, RuntimeState oldState ) override;

	private:
		void AddMaterialGraph();
		void SetupNewNodeEditor();
		void SetupNodeEditorCallbacks();

	private:
		// MaterialGraph asset.
		Ref<Asset> m_Asset;

		// Current graph that we are drawing.
		SharedPtr<MatGraph2_Graph> m_MaterialGraph = nullptr;
	
		bool m_ShowDirtyPopup = false;
		UUID m_OutputNodeID = 0;
	};
	
}
