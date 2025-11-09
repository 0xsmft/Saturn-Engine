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

#include "Saturn/Core/Event.h"

#include <glm/glm.hpp>

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// This file is stored in the ImGui folder because the majority of the events happen from the Editor UI,
	// only a few do not. 

	class SkylightEntityModifiedEvent : public Event
	{
		SAT_DEFINE_EVENT( SkylightEntityModified, EC_Editor );
	public:
		SkylightEntityModifiedEvent( const glm::vec3& rParams )
			: Event( EventType::SkylightEntityModified, EC_Editor ), m_Params( rParams )
		{
		}

		virtual ~SkylightEntityModifiedEvent() = default;

		[[nodiscard]] inline const glm::vec3& GetParams() const { return m_Params; }

	private:
		glm::vec3 m_Params;
	};
	
	//
	// AssetDeletedEvent
	// Triggers when an Asset is deleted and is NOT replaced
	//
	class AssetDeletedEvent : public Event
	{
		SAT_DEFINE_EVENT( AssetDeleted, EC_Editor );
	public:
		AssetDeletedEvent( AssetID deletedID )
			: Event( EventType::AssetDeleted, EC_Editor ), m_DeletedAssetID( deletedID )
		{
		}

		virtual ~AssetDeletedEvent() = default;

		[[nodiscard]] AssetID GetDeletedAssetID() const { return m_DeletedAssetID; }

	private:
		AssetID m_DeletedAssetID = 0;
	};

	//
	// AssetReplacedEvent
	// Triggers when an Asset is deleted and is replaced
	// NOTE: You may think the event chain will be this:
	//  - AssetDeletedEvent
	//	- AssetReplacedEvent
	// 
	// However, AssetReplacedEvent will fire when as asset is replaced and is exclusive to AssetDeletedEvent meaning that only one can fire at a time!
	//
	class AssetReplacedEvent : public Event
	{
		SAT_DEFINE_EVENT( AssetReplaced, EC_Editor );
	public:
		AssetReplacedEvent( AssetID original, AssetID newID )
			: Event( EventType::AssetReplaced, EC_Editor ), m_OldAssetID( original ), m_NewAssetID( newID )
		{
		}

		virtual ~AssetReplacedEvent() = default;

		// @returns The ID that will replaced by GetNewAssetID()
		[[nodiscard]] AssetID GetOldAssetID() const { return m_OldAssetID; }
		[[nodiscard]] AssetID GetNewAssetID() const { return m_NewAssetID; }

	private:
		AssetID m_OldAssetID = 0;
		AssetID m_NewAssetID = 0;
	};

	//
	// EntitySelectedEvent
	// 
	// Triggers when an entity is selected, this event will trigger regardless of the fact if it was selected/deselected from UI or not.
	//
	class EntitySelectedEvent : public Event
	{
		SAT_DEFINE_EVENT( EntitySelected, EC_Editor );
	public:
		EntitySelectedEvent( UUID id )
			: Event( EventType::EntitySelected, EC_Editor ), m_EntityID( id )
		{
		}

		virtual ~EntitySelectedEvent() = default;

		const UUID GetID() const { return m_EntityID; }

	private:
		UUID m_EntityID = 0llu;
	};

	//
	// EntitiesSelectionChangedEvent
	// 
	// Triggers when multiple entities are selected/deselected, this event will trigger regardless of the fact if it was selected/deselected from UI or not.
	//
	// Warning: Calls to GetStaticType() are invalid, as this event can have multiple types
	//          it can either be EntitySelected/EntityDeselected!
	//
	class EntityDeselectedEvent : public Event
	{
		SAT_DEFINE_EVENT( EntityDeselected, EC_Editor );
	public:
		EntityDeselectedEvent( std::vector<UUID>&& ids )
			: Event( EventType::EntityDeselected, EC_Editor ), m_EntityIDs( std::move( ids ) )
		{
		}

		EntityDeselectedEvent( const std::vector<UUID>& ids, bool selected = true )
			: Event( EventType::EntityDeselected, EC_Editor ), m_EntityIDs( ids )
		{
		}

		virtual ~EntityDeselectedEvent() = default;
	
		const std::vector<UUID>& GetIDs() const { return m_EntityIDs; }

	private:
		std::vector<UUID> m_EntityIDs;
	};
}
