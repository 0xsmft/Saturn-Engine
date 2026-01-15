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

#include "sppch.h"
#include "EntitySelectionManager.h"

#include "Saturn/Core/App.h"

#include "Saturn/Asset/Asset.h"
#include "Saturn/ImGui/EditorEvents.h"

#include "Saturn/Scene/Entity.h"

namespace Saturn {

	EntitySelectionManager::EntitySelectionManager()
	{
		SAT_CORE_ASSERT( !SingletonStorage::GetSingleton<EntitySelectionManager>(), "A EntitySelectionManager already exists!" );

		SingletonStorage::AddSingleton<EntitySelectionManager>( this );
	}

	EntitySelectionManager::~EntitySelectionManager()
	{
		ClearSelection();
	}

	void EntitySelectionManager::Select( const SharedPtr<Entity> entity, EntitySelectionReason reason )
	{
		if( !IsSelected( entity ) ) 
		{
			m_SelectedEntities.push_back( entity );
			m_LastReason = reason;

			Application::Get().DispatchEvent<EntitySelectedEvent>( entity->GetUUID() );
		}
	}

	void EntitySelectionManager::Remove( const SharedPtr<Entity> entity )
	{
		m_SelectedEntities.erase( std::remove( m_SelectedEntities.begin(), m_SelectedEntities.end(), entity ) );
		
		std::vector<UUID> ids( entity->GetUUID() );
		Application::Get().DispatchEvent<EntityDeselectedEvent>( ids );
	}

	void EntitySelectionManager::ClearSelection( bool skipEvent )
	{
		if( const auto size = m_SelectedEntities.size(); size && !skipEvent )
		{
			std::vector<UUID> ids;
			ids.reserve( size );

			m_LastReason = EntitySelectionReason::Other;

			std::ranges::transform( m_SelectedEntities, std::back_inserter( ids ), &Entity::GetUUID );
			Application::Get().DispatchEvent<EntityDeselectedEvent>( ids );
		}

		m_SelectedEntities.clear();
	}

	bool EntitySelectionManager::IsSelected( const SharedPtr<Entity> entity ) const
	{
		return std::find( m_SelectedEntities.begin(), m_SelectedEntities.end(), entity ) != m_SelectedEntities.end();
	}

}
