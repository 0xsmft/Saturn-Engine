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

#include "Saturn/Vulkan/SceneRenderer.h"

#include <imgui.h>

namespace Saturn {

	class SceneRenderer;
	class EditorCamera;

	class SubSceneRendererWindow
	{
	public:
		SubSceneRendererWindow();
		virtual ~SubSceneRendererWindow();

		void RenderViewport( bool end = true );

	public:
		ImVec2 GetViewportSize() const { return m_ViewportSize; }
		ImVec2 GetViewportPosition() const { return m_ViewportPosition; }
		void SetViewportWindowID( UUID id ) { m_WindowID = id; }
		void End();

	protected:
		void Initialise();
		void OnUpdateRenderer( Timestep ts );
		void OnCameraEvent( Event& rEvent );

	protected:
		Ref<SceneRenderer> m_SceneRenderer;
		Ref<Scene> m_Scene;
		EditorCamera m_Camera;

		bool m_AllowCameraEvents = false;
		bool m_StartedRightClickInViewport = false;
		bool m_ViewportFocused = false;
		bool m_MouseOverViewport = false;
		bool m_DisableViewportMovement = false;

	private:
		ImVec2 m_ViewportSize{};
		ImVec2 m_ViewportPosition{};
		UUID m_WindowID = 0llu;
	};
	
}
