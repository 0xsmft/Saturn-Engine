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

#include "Saturn/Core/Base.h"
#include "Saturn/Scene/Entity.h"
#include "Saturn/Scene/Scene.h"

#include "ImGuiWindow.h"
#include "EntitySelectionReason.h"

#include "Saturn/Vulkan/Texture.h"

#include <functional>
// TODO: Remove this include (need to find a way to define ImGuiTextFilter without including the imgui header)
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

namespace Saturn {

	class SceneHierarchyPanel : public ImGuiWindow
	{
	public:
		SceneHierarchyPanel();
		SceneHierarchyPanel( const std::string& rWindowName );
		virtual ~SceneHierarchyPanel();

		void SetContext( const Ref<Scene>& scene );

		void SetSelected( SharedPtr<Entity> entity );

		static const char* GetStaticName() 
		{
			return "Scene Hierarchy";
		}

		inline void SetIsPrefabScene( bool value ) { m_IsPrefabScene = value; }
		inline void SetCustomID( UUID ID ) { m_CustomID = ID; }

		[[nodiscard]] bool IsFocused() const { return m_WindowFocused; }

	public:
		//////////////////////////////////////////////////////////////////////////
		// ImGuiWindow
	
		virtual void OnImGuiRender() override;
		virtual void OnEvent( Event& rEvent ) {}
		virtual void OnUpdate( Timestep ts ) {}

	protected:
		void DrawComponents( SharedPtr<Entity> entity );
		void DrawEntityNode( SharedPtr<Entity> entity );
		void DrawEntityProperties( SharedPtr<Entity> entity );
		void DrawEntityComponents( SharedPtr<Entity> entity );
		void DrawEntities();

		template<typename Ty>
		void DrawAddComponents( const char* pName, SharedPtr<Entity> entity );

		template<typename T, typename UIFunction>
		void DrawComponent( const std::string& name, SharedPtr<Entity> entity, UIFunction uiFunction );

		void PopupContextMenuNormal();
		void SelectedEntityPopup();

	private:
		UUID m_CustomID = 0;

		Ref<Texture2D> m_EditIcon;

		// Asset Finder
		AssetID m_CurrentAssetID = 0;
		AssetType m_CurrentFinderType = AssetType::Unknown;

		// Entity Finder
		UUID m_CurrentEntityID = 0;

		bool m_OpenEntityFinderPopup = false;
		bool m_IsPrefabScene = false;
		bool m_Searching = false;
		bool m_WindowFocused = false;

		struct CopyComponentData
		{
			Buffer Buffer;
			uint32_t Hash = 0u;
		};

		CopyComponentData m_CopyComponentData{};

		// Searching text filter
		ImGuiTextFilter m_EntityTextFilter{};

		Ref<Scene> m_Context;
	};
}