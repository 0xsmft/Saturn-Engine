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

#include "AssetViewer.h"
#include "SubSceneRendererWindow.h"

namespace Saturn {

	class SceneRenderer;
	class EditorCamera;

	class StaticMeshAssetViewer : public AssetViewer, public SubSceneRendererWindow
	{
	public:
		StaticMeshAssetViewer( AssetID id );
		~StaticMeshAssetViewer();

		virtual void OnImGuiRender() override;
		virtual void OnUpdate( Timestep ts ) override;
		virtual void OnEvent( Event& rEvent ) override;

	private:
		void AddMesh();
		void DrawNoFallbackPopup();
		void DrawCookingErrorPopup();

	private:
		Ref<StaticMesh> m_Mesh;
		AssetID m_AssetFinderOut = 0;
		AssetID m_AssetFinderOutPhys = 0;

		// The following variables are for the Popup that comes up when
		// the user tries to reset a material but we don't know what to
		// fall back on.
		bool m_ShowNoFallbackPopup = false;
		bool m_ShowCookingErrorPopup = false;
		uint64_t m_CookingError = 0;
		UUID m_FallbackID = 0;
	
		std::queue<uint64_t> m_ResetIndices;
	};
}