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

#include "Saturn/Core/Base.h"
#include "Saturn/Scene/Entity.h"
#include "Saturn/Scene/Scene.h"

#include "ImGuiWindow.h"
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
		~SceneHierarchyPanel();

		void SetContext( const Ref<Scene>& scene );
		void SetSelected( Ref<Entity> entity );
		void ClearSelection();
		void SetSelectionChangedCallback( const std::function<void( Ref<Entity> )>& func ) { m_SelectionChangedCallback = func; }

		std::vector<Ref<Entity>>& GetSelectionContexts() { return m_SelectionContexts; }
		const std::vector< Ref<Entity>>& GetSelectionContexts() const { return m_SelectionContexts; }
		
		Ref<Entity> GetSelectionContext( uint32_t index = 0 ) 
		{
			if( m_SelectionContexts.size() < index || !m_SelectionContexts.size() )
			{
				return nullptr;
			}

			return m_SelectionContexts[ index ]; 
		}
		
		Ref<Entity> GetSelectionContext( uint32_t index = 0 ) const
		{
			if( m_SelectionContexts.size() < index || !m_SelectionContexts.size() )
			{
				return nullptr;
			}
		
			return m_SelectionContexts[ index ]; 
		}

		virtual void OnImGuiRender() override;
		virtual void OnEvent( RubyEvent& rEvent ) {}
		virtual void OnUpdate( Timestep ts ) {}
		
		static const char* GetStaticName() 
		{
			return "Scene Hierarchy Panel";
		}

		void SetIsPrefabScene( bool value ) { m_IsPrefabScene = value; }

		void AddID( UUID ID ) { m_CustomID = ID; }
		void SetName( const std::string& rName ) { m_WindowName = rName; }
		const std::string& GetName() const { return m_WindowName; }

	protected:
		void DrawComponents( Ref<Entity> entity );
		bool IsEntitySelected( Ref<Entity> entity );
		void DrawEntityNode( Ref<Entity> entity );
		void DrawEntityProperties( Ref<Entity> entity );
		void DrawEntityComponents( Ref<Entity> entity );
		void DrawEntities();

		template<typename Ty>
		void DrawAddComponents( const char* pName, Ref<Entity> entity );

		template<typename T, typename UIFunction>
		void DrawComponent( const std::string& name, Ref<Entity> entity, UIFunction uiFunction );

	private:
		UUID m_CustomID = 0;
		std::string m_WindowName = "Scene Hierarchy";

		Ref<Texture2D> m_EditIcon;

		// Asset Finder
		AssetID m_CurrentAssetID = 0;
		AssetType m_CurrentFinderType = AssetType::Unknown;

		// Entity Finder
		UUID m_CurrentEntityID = 0;

		bool m_OpenEntityFinderPopup = false;
		bool m_IsPrefabScene = false;
		bool m_Searching = false;
		bool m_IsMultiSelecting = false;

		struct CopyComponentData
		{
			Buffer Buffer;
			std::string Name;
		};

		CopyComponentData m_CopyComponentData{};

		// Searching text filter
		ImGuiTextFilter m_EntityTextFilter{};

		Ref<Scene> m_Context;
		std::vector< Ref<Entity> > m_SelectionContexts;
		std::function<void( Ref<Entity> )> m_SelectionChangedCallback;
	};
}