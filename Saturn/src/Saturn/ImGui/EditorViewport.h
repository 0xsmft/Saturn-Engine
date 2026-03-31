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

namespace Saturn {

	class Scene;

	enum ViewportFlags : uint8_t
	{
		VP_None = 0,
		VP_Primary = BIT( 0 ),
		VP_AluraCanvas = BIT( 1 ),
		VP_DisplayGizmo = BIT( 2 ),
		VP_DisplayRuntimeControl = BIT( 3 ),
		VP_DisplayHotReloadControl = BIT( 4 ),
		VP_DisplayGizmoControl = BIT( 5 ),
		VP_DragNDropTarget = BIT( 6 ),

		// Default flags for main primary viewports
		VP_DefaultPrimary = VP_Primary | VP_AluraCanvas | VP_DisplayGizmo | VP_DisplayRuntimeControl | VP_DisplayHotReloadControl | VP_DisplayGizmoControl | VP_DragNDropTarget,

		// Default secondary viewports
		VP_DefaultSecondary = VP_DisplayGizmo | VP_AluraCanvas | VP_DisplayRuntimeControl | VP_DisplayHotReloadControl | VP_DisplayGizmoControl | VP_DragNDropTarget,

		// Default flags sub viewport i.e. viewports in AssetViewers
		VP_DefaultSub = VP_DisplayGizmo | VP_DisplayGizmoControl
	};

	class EditorViewport
	{
	public:
		EditorViewport( ViewportFlags flags );
		~EditorViewport();

		void Initialise( SceneRendererFlags sceneRendererFlags, Ref<Scene> scene, const std::string& rName, UUID ID, bool* pRequestRuntimeVal = nullptr, bool* pLastRuntimeAttemptFailedVal = nullptr );

		void OnUpdate( Timestep ts );
		void Draw();
		void OnEvent( Event& rEvent );

	public:
		[[nodiscard]] bool IsViewportFlagSet( ViewportFlags flag ) const
		{
			return ( m_ViewportFlags & flag ) != 0;
		}

		Ref<SceneRenderer> GetSceneRenderer() const { return m_SceneRenderer; }
		EditorCamera& GetCamera() { return m_EditorCamera; }

		glm::vec2 GetPosition() const { return m_ViewportBoundsMin; }
		glm::vec2 GetSize() const { return m_ViewportSize; }

	private:
		bool OnKeyPressed( RubyKeyEvent& rEvent );
		bool OnMousePressed( RubyMouseEvent& rEvent );

		void Viewport_DrawGizmo();
		void Viewport_GizmoControl();
		void Viewport_RTControls();
		void Viewport_RTControls_Running();
		void Viewport_RTControls_Default();
		void Viewport_RTSettings();

	private:
		std::string m_ViewportName;
		UUID m_ViewportID = 0;

		// NOTE: You can debate later with me if theses should be ImGui types, (which may give a slight runtime speed advantage due to us not having to convert)
		// the main reason why they are not is because I do not want to expose the whole imgui and imgui_internal headers
		// for any class that needs this class,
		// although many classes that use EditorViewport may already have imgui.h included, it is best to assume that they don't
		// to again avoid leaking imgui into files they shouldn't be in.
		glm::vec2 m_ViewportBoundsMin{}, m_ViewportBoundsMax{};
		glm::vec2 m_ViewportSize{};

		// Used for a fullscreen viewport
		glm::vec2 m_PreVPFullscreenPosition{};
		glm::vec2 m_PreVPFullscreenSize{};

		// NOTE: Scene is created externally, i.e. the viewport does not create it/own it, we just use it
		Ref<Scene> m_Scene;
		Ref<SceneRenderer> m_SceneRenderer;
		Ref< Texture2D > m_StartRuntimeTexture = nullptr;
		Ref< Texture2D > m_StartErrorRuntimeTexture = nullptr;
		Ref< Texture2D > m_EndRuntimeTexture = nullptr;
		Ref< Texture2D > m_PauseRuntimeTexture = nullptr;
		Ref< Texture2D > m_TranslationTexture;
		Ref< Texture2D > m_RotationTexture;
		Ref< Texture2D > m_ScaleTexture;

		bool* m_pRequestRuntime = nullptr;
		bool* m_pLastRuntimeAttemptFailed = nullptr;

		unsigned int m_PreVPDockedNodeID = 0;

		// Translate as default
		int m_GizmoOperation = 7 /* ImGuizmo::OPERATION::TRANSLATE */;

		// NOTE: Separated by 4 bytes... not Separated in an order.
		ViewportFlags m_ViewportFlags = VP_None;
		bool m_PrimaryViewport = false;
		bool m_AllowCameraEvents = false;
		bool m_PendingFullscreenChange = false;

		bool m_FullscreenViewport = false;
		bool m_DisableViewportMovement = false;
		bool m_StartedRightClickInViewport = false;
		bool m_ViewportFocused = false;

		bool m_MouseOverViewport = false;
		bool m_WasGizmoUsed = false;

		EditorCamera m_EditorCamera;
	};
	
}
