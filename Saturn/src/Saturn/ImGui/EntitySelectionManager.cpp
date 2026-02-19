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

#include "sppch.h"
#include "EntitySelectionManager.h"

#include "Saturn/Core/App.h"

#include "Saturn/Asset/Asset.h"
#include "Saturn/ImGui/EditorEvents.h"

#include "Saturn/Scene/Entity.h"

// define this if you want to see the callstack and why selections are getting removed/added quickly,
// this was used during debugging of the Camera Preview...
//#define SAT_ESM_STACKTRACE_ONCHANGE
#if defined(SAT_ESM_STACKTRACE_ONCHANGE)
#include <stacktrace>
#endif

namespace Saturn {

	EntitySelectionManager::EntitySelectionManager()
	{
		SAT_CORE_ASSERT( !SingletonStorage::GetSingleton<EntitySelectionManager>(), "A EntitySelectionManager already exists!" );

		SingletonStorage::AddSingleton<EntitySelectionManager>( this );
	}

	EntitySelectionManager::~EntitySelectionManager()
	{
		ClearAllSections( true );
	}

	void EntitySelectionManager::Select( const SharedPtr<Entity> entity )
	{
		if( !IsSelected( entity ) ) 
		{
			if( !m_IsMultiSelecting )
				ClearSelection( entity->GetScene() );

			m_SelectedEntities[ entity->GetScene()->GetInternalID() ].push_back( entity );

#if defined(SAT_ESM_STACKTRACE_ONCHANGE)
			SAT_CORE_INFO( std::stacktrace::current() );
#endif
			Application::Get()->DispatchEvent<EntitySelectedEvent>( entity->GetUUID() );
		}
	}

	void EntitySelectionManager::Remove( const SharedPtr<Entity> entity )
	{
		for( auto& [sceneID, rSelections] : m_SelectedEntities )
		{
			if( sceneID != entity->GetScene()->GetInternalID() )
				continue;

			for( const auto& rEntity : rSelections )
			{
				if( rEntity != entity )
					continue;

				rSelections.erase( std::remove( rSelections.begin(), rSelections.end(), rEntity ), rSelections.end() );

				break;
			}
		}

#if defined(SAT_ESM_STACKTRACE_ONCHANGE)
		SAT_CORE_INFO( std::stacktrace::current() );
#endif
		std::vector<UUID> ids( entity->GetUUID() );
		Application::Get()->DispatchEvent<EntityDeselectedEvent>( ids );
	}

	void EntitySelectionManager::ClearSelection( Scene* pScene, bool skipEvent )
	{
		if( const auto size = m_SelectedEntities.size(); size && !skipEvent )
		{
			std::vector<UUID> ids;
			ids.reserve( size );

#if defined(SAT_ESM_STACKTRACE_ONCHANGE)
			SAT_CORE_INFO( std::stacktrace::current() );
#endif
			for( const auto& [sceneID, rSelections] : m_SelectedEntities )
			{
				for( const auto& rEntity : rSelections )
				{
					if( rEntity->GetScene() == pScene )
					{
						ids.push_back( rEntity->GetUUID() );
					}
				}
			}

			Application::Get()->DispatchEvent<EntityDeselectedEvent>( ids );
		}

		if( pScene )
			m_SelectedEntities.erase( pScene->GetInternalID() );
	}

	void EntitySelectionManager::ClearAllSections( bool skipEvent /*= false */ )
	{
		if( const auto size = m_SelectedEntities.size(); size && !skipEvent )
		{
			std::vector<UUID> ids;
			ids.reserve( size );

			for( const auto& [sceneID, rSelections] : m_SelectedEntities )
			{
				for( const auto& rEntity : rSelections )
				{
					ids.push_back( rEntity->GetUUID() );
				}
			}

			Application::Get()->DispatchEvent<EntityDeselectedEvent>( ids );
		}

		m_SelectedEntities.clear();
	}

	bool EntitySelectionManager::IsSelected( const SharedPtr<Entity> entity ) const
	{
		for( const auto& [sceneID, rSelection] : m_SelectedEntities )
		{
			if( sceneID != entity->GetScene()->GetInternalID() )
				continue;

			for( const auto& rEntity : rSelection )
			{
				if( rEntity == entity )
					return true;
			}
		}

		return false;
	}

	std::vector<SharedPtr<Entity>> EntitySelectionManager::GetSelectionContexts( Scene* pScene )
	{
		std::vector<SharedPtr<Entity>> selections;

		for( const auto& [sceneID, rSelections] : m_SelectedEntities )
		{
			if( sceneID != pScene->GetInternalID() )
				continue;

			for( const auto& rEntity : rSelections )
			{
				selections.push_back( rEntity );
			}
		}

		return selections;
	}

	size_t EntitySelectionManager::GetSelectionCount( Scene* pScene )
	{
		for( const auto& [sceneID, rSelections] : m_SelectedEntities )
		{
			if( sceneID != pScene->GetInternalID() )
				continue;

			return rSelections.size();
		}

		return 0;
	}

}
