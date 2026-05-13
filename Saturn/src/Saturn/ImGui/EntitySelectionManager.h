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

#include "Saturn/Core/Ref.h"
#include "Saturn/Core/UUID.h"

#include "SingletonStorage.h"

#include "EntitySelectionReason.h"

namespace Saturn {

	class Entity;	
	class Scene;
	
	//
	// EntitySelectionManager
	// This class holds the global list for the currently selected entities, 
	// selections mainly change from the Scene Hierarchy Panel however, the list is always stored in this class
	//
	// NOTE: Selections are stored only for the current scene, so when the scene changes this list will be cleared,
	//       In addition, the selected entities are held with a SharedPtr (which is an authoritative ref) and will stop the deletion if it's not cleared prior to calling DeleteEntity or equiv. however, in most cases this step is already handled, but just keep that in mind.
	// 
	// Ownership:
	//  - EditorLayer
	//
	class EntitySelectionManager
	{
	public:
		static inline EntitySelectionManager* Get() { return SingletonStorage::GetSingleton<EntitySelectionManager>(); }
	public:
		EntitySelectionManager();
		~EntitySelectionManager();

		void EnableMultiSelection() { m_IsMultiSelecting = true; }
		void DisableMultiSelection() { m_IsMultiSelecting = false; }

		void Select( const SharedPtr<Entity> entity );
		void Remove( const SharedPtr<Entity> entity );

		void ClearSelection( Scene* pScene, bool skipEvent = false );
		void ClearAllSections( bool skipEvent = false );

		[[nodiscard]] bool IsSelected( const SharedPtr<Entity> entity ) const;
		[[nodiscard]] bool IsMultiSelecting() const { return m_IsMultiSelecting; }

		[[nodiscard]] std::vector<SharedPtr<Entity>> GetSelectionContexts( Scene* pScene );

		[[nodiscard]] size_t GetSelectionCount( Scene* pScene );

	public:
		[[nodiscard]] std::unordered_map<UUID, std::vector<SharedPtr<Entity>>>& GetAllSelectionContexts() { return m_SelectedEntities; }
		[[nodiscard]] const std::unordered_map<UUID, std::vector<SharedPtr<Entity>>>& GetAllSelectionContexts() const { return m_SelectedEntities; }

	public:
		EntitySelectionReason GetSelectionReason() const { return m_LastSelectionReason; }

		// NOTE: The reason will ONLY be modified by the _main_ viewport and the _main_ SceneHierarchyPanel.
		void SetSelectionReason( EntitySelectionReason reason ) { m_LastSelectionReason = reason; }

	private:
		// Selection map
		//					SCENE ID -> ENTITES
		std::unordered_map<UUID, std::vector<SharedPtr<Entity>>> m_SelectedEntities;

		bool m_IsMultiSelecting = false;

		// NOTE: The reason will ONLY be modified by the _main_ viewport and the _main_ SceneHierarchyPanel.
		// Please do not modify this reason from anything other than the above.
		EntitySelectionReason m_LastSelectionReason = ESR_Other;
	};
}
