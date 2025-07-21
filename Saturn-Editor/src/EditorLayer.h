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

#include <Saturn/ImGui/ImGuiWindowManager.h>
#include <Saturn/ImGui/SceneHierarchyPanel.h>
#include <Saturn/ImGui/ContentBrowserPanel/ContentBrowserPanel.h>
#include <Saturn/ImGui/JobProgress.h>
#include <Saturn/ImGui/TitleBar.h>
#include <Saturn/ImGui/UndoRedo/GlobalUndoRedoGroup.h>

#include <Saturn/Asset/AssetManager.h>

#include <Saturn/Scene/Scene.h>

#include <Saturn/Physics/PhysicsFoundation.h>

#include <Saturn/Core/Layer.h>
#include <Saturn/Core/Renderer/SceneFlyCamera.h>

#include <queue>

namespace Saturn {
	
	class GameModule;

	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		~EditorLayer();

		void OnUpdate( Timestep time ) override;
		void OnImGuiRender() override;
		void OnEvent( RubyEvent& rEvent ) override;
		void OnAttach() override;
		void OnDetach() override;
		
	private:
		void SaveFileAs();
		void OpenFile( AssetID id );

		void SaveFile();
		void SaveFileAuto();

		void SaveProject();

		void SelectionChanged( Ref<Entity> e );
		void ViewportSizeCallback( uint32_t Width, uint32_t Height );
		bool OnKeyPressed( RubyKeyEvent& rEvent );

		// UI Functions.
		void DrawProjectSettingsWindow();
		void HotReloadGame();

		void DrawAssetRegistryDebug();
		void DrawLoadedAssetsDebug();
		void DrawEditorSettings();
		void DrawVFSDebug();
		void DrawTitlebarOptions();
		void DrawAboutWindow();
		void DrawSceneRendererWindow();
		void DrawRendererWindow();
		void DrawMetadataDebug();
		void DrawAssetDependencies();
		void DrawSceneDirtyPopup();

		// Viewport
		void DrawViewport();
		void Viewport_GizmoControl();
		void Viewport_RTControls();
		void Viewport_RTSettings();
		
		void Viewport_RTControls_Running();
		void Viewport_RTControls_Default();

		void ProjectSettings_DrawSoundGroupEdit( Ref<SoundGroup>& rSoundGroup );

		// Close editor and open the project browser.
		void CloseEditorAndOpenPB();
		bool OnTitlebarExit();

		void CheckMissingEnv();
		bool BuildShaderBundle();
		
		[[nodiscard]] bool ValidateProjectDefaults();

		void CreateShaderBundleJob();
		void CreateAssetBundleJob();

		void ShowOrHideContentBrowserPanel();
		void ShowOrHideSceneHierarchyPanel();

		glm::vec2 ConvertMouseToViewport();

	private:
		enum MessageBoxButtons_
		{
			MessageBoxButtons_Ok     = BIT( 0 ),
			MessageBoxButtons_Cancel = BIT( 1 ),
			MessageBoxButtons_Retry  = BIT( 2 ),
			MessageBoxButtons_Exit   = BIT( 3 ),
			MessageBoxButtons_Yes    = BIT( 4 ),
			MessageBoxButtons_No     = BIT( 5 ),
		};

		enum class MessageBoxType 
		{
			Information,
			InformationNoIcon,
			Warning,
			Error
		};

		struct MessageBoxInfo
		{
			std::string Title = "Error";
			std::string Text;

			// enum MessageBoxButtons_
			uint32_t Buttons = MessageBoxButtons_Ok;
			MessageBoxType Type = MessageBoxType::Error;
		};

		void PushMessageBox( MessageBoxInfo& rInfo );
		void PopMessageBox();
		void HandleMessageBoxes();
		void DrawMessageBox( const MessageBoxInfo& rInfo );

	private:
		struct EditorNotification
		{
			std::string Text{};
			float Lifetime = 0.0f;
			MessageBoxType NotificationType = MessageBoxType::InformationNoIcon;

			float AnimationTime = 0.0f;

			UUID ID;
		};

		void PushNotification( EditorNotification& rInfo );
		void PopNotification();
		float DrawSingleNotification( EditorNotification& rInfo, float lastYOffset );
		void DrawNotifications();

	private:
		GameModule* m_GameModule = nullptr;
		Ref<AssetManager> m_AssetManager;

		Ref<GlobalUndoRedoGroup> m_GlobalUndoRedoGroup = nullptr;

	private:
		TitleBar m_TitleBar;
		
		Ref< Texture2D > m_CheckerboardTexture = nullptr;
		Ref< Texture2D > m_StartRuntimeTexture = nullptr;
		Ref< Texture2D > m_StartErrorRuntimeTexture = nullptr;
		Ref< Texture2D > m_EndRuntimeTexture = nullptr;
		Ref< Texture2D > m_PauseRuntimeTexture = nullptr;

		Ref< Texture2D > m_TranslationTexture = nullptr;
		Ref< Texture2D > m_RotationTexture = nullptr;
		Ref< Texture2D > m_ScaleTexture = nullptr;
		Ref< Texture2D > m_SyncTexture = nullptr;
		Ref< Texture2D > m_PointLightTexture = nullptr;
		Ref< Texture2D > m_ExclamationTexture = nullptr;

		Ref<ImGuiWindowManager> m_ImGuiWindowManager = nullptr;

		// Used to be called BlockingOperation hence the name.
		Ref<JobProgress> m_BlockingOperation = nullptr;

		EditorCamera m_EditorCamera;
		EditorCamera m_SuspendedEditorCamera;
		Camera* m_pRuntimeCamera = nullptr;

		bool m_AllowCameraEvents = false;
		bool m_StartedRightClickInViewport = false;
		bool m_ViewportFocused = false;
		bool m_MouseOverViewport = false;
		bool m_OpenEditorSettings = false;
		bool m_ShowImGuiDemoWindow = false;
		bool m_ShowVFSDebug = false;
		bool m_HasPremakePath = false;
		bool m_OpenAssetRegistryDebug = false;
		bool m_OpenLoadedAssetDebug = false;
		bool m_JobModalOpen = false;
		bool m_OpenAboutWindow = false;
		bool m_ShowMetadataDebug = false;
		bool m_ShowAssetDependencies = false;
		bool m_ShowRendererWindow = true;
		bool m_ShowSceneRendererWindow = true;
		bool m_ShowSceneDirtyModal = false;
		bool m_ShowUserSettings = false;
		bool m_RequestRuntime = false;
		bool m_ShowCameraFrustum = false;
		bool m_ShowMeshAABB = false;
		bool m_ShowCBThumbnailDebug = false;
		bool m_ShowUndoRedoDebug = false;
		// JobProgress
		bool m_ShowOperation = false;

		bool m_WasGizmoUsed = false;
		bool m_LastRuntimeAttemptFailed = false;

		// JobProgress
		float m_OperationPercent = 0.0f;
		// Translate as default
		int m_GizmoOperation = 7 /* ImGuizmo::OPERATION::TRANSLATE */;

		float m_LastAutoSaveTime = 0.0f;
		uint32_t m_AutoSaveCount = 0u;

		ImVec2 m_ViewportSize;

		std::queue<MessageBoxInfo> m_MessageBoxes;
		std::vector<EditorNotification> m_Notifications;
		std::unordered_map<entt::entity, glm::mat4> m_GizmoOrignalTransforms;
		std::unordered_map<entt::entity, std::tuple<glm::vec3, glm::vec3, glm::vec3>> m_GizmoModifiedTransforms;

		Ref<Scene> m_EditorScene = nullptr;
		Ref<Scene> m_RuntimeScene = nullptr;

		PhysicsFoundation m_PhysicsFoundation;
	};
}
