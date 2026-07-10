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
#include "Prefab.h"

// Fuckass include
#include "Saturn/Audio/SoundGroup.h"

#include "Saturn/Scene/Scene.h"
#include "Saturn/Scene/Entity.h"

#include "Saturn/Core/VariableGuard.h"

#include "Saturn/GameFramework/SClass.h"

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////

	template<typename... V>
	static void CopyComponentIfExists( 
		entt::entity dst, 
		entt::entity src, 

		entt::registry& rRegistry, entt::registry& rDstRegistry )
	{
		( [&]()
			{
				if( rRegistry.any_of<V>( src ) )
				{
					auto& srcComponent = rRegistry.get<V>( src );
					rDstRegistry.emplace_or_replace<V>( dst, srcComponent );
				}
			}( ), ... );
	}

	template<typename... V>
	static void CopyComponentIfExists( ComponentGroup<V...>, 
		entt::entity dst, entt::entity src, 
		entt::registry& rRegistry, entt::registry& rDstRegistry )
	{
		CopyComponentIfExists<V...>( dst, src, rRegistry, rDstRegistry );
	}

	//////////////////////////////////////////////////////////////////////////
	// PREFAB

	Prefab::Prefab()
	{
	}

	Prefab::Prefab( const Ref<Asset>& rBase )
		: Asset( rBase )
	{
		m_Scene = Ref<Scene>::Create();
	}

	Prefab::~Prefab()
	{
	}

	void Prefab::InitPrefab( const SharedPtr<Entity> srcEntity )
	{
		m_Scene = Ref<Scene>::Create();

		VariableGuard<Scene*> sceneGuard( g_ActiveScene, m_Scene.Get() );

		if( srcEntity->Valid() )
			m_Entity = CreateFromEntity( srcEntity );
	}

	SharedPtr<Entity> Prefab::CreateFromEntity( SharedPtr<Entity> srcEntity )
	{
		CreateEntityParameters params{};
		params.pClass = ( SClass* ) srcEntity->GetClass();

		SharedPtr<Entity> result = g_ActiveScene->CreateEntity( params );

		// Make sure that all entities in this prefab have the PrefabComponent.
		auto& rPrefabComp = result->AddComponent<PrefabComponent>();
		rPrefabComp.AssetID = ID;
		rPrefabComp.EntityIDInPrefab = srcEntity->GetUUID();

		CopyComponentIfExists( AllComponents{}, 
			result->m_EntityHandle, srcEntity->m_EntityHandle,
			srcEntity->m_Scene->m_Registry, m_Scene->m_Registry );

		for( auto& childId : srcEntity->GetChildren() )
		{
			SharedPtr<Entity> child = CreateFromEntity( srcEntity->m_Scene->FindEntityByID( childId ) );

			auto& rc = result->GetComponent<RelationshipComponent>();

			child->SetParent( result->GetComponent<IdComponent>().ID );
			rc.ChildrenID.push_back( child->GetComponent<IdComponent>().ID );
		}

		return result;
	}

	SharedPtr<Entity> Prefab::InstantiatePrefab( Ref<Scene> SceneToSpawnIn )
	{
		CreateEntityParameters params{};
		params.pClass = ( SClass* ) m_Entity->GetClass();

		SharedPtr<Entity> result = SceneToSpawnIn->CreateEntity( params );
		auto& pc = result->AddComponent<PrefabComponent>();
		pc.AssetID = ID;
		pc.EntityIDInPrefab = m_Entity->GetUUID();

		// Copy components over.
		CopyComponentIfExists( AllComponents{},
			result->m_EntityHandle, m_Entity->m_EntityHandle,
			m_Scene->m_Registry, SceneToSpawnIn->m_Registry );

		// We don't want the same id, what if we spawn this prefab and it has the same id as an 
		// already existing entity in the target scene?
		result->GetComponent<IdComponent>().ID = {};

		for( auto& childId : m_Entity->GetChildren() )
		{
			SharedPtr<Entity> child = CreateChildren( m_Scene->FindEntityByID( childId ), SceneToSpawnIn );

			child->SetParent( result->GetComponent<IdComponent>().ID );
		}

		return result;
	}

	SharedPtr<Entity> Prefab::CreateChildren( const SharedPtr<Entity>& parent, Ref<Scene> Scene )
	{
		// Create the child in the new scene.
		SharedPtr<Entity> child = g_ActiveScene->CreateEntity();
		auto& pc = child->AddComponent<PrefabComponent>();
		pc.AssetID = ID;
		pc.EntityIDInPrefab = parent->GetUUID();

		// Copy Components, from our child in the scene.
		CopyComponentIfExists( AllComponents{},
			child->m_EntityHandle, parent->m_EntityHandle,
			m_Scene->m_Registry, Scene->m_Registry );

		// Check if this entity has any children.
		for( auto& childId : child->GetChildren() )
		{
			SharedPtr<Entity> c = CreateChildren( child, Scene );

			c->SetParent( child->GetComponent<IdComponent>().ID );

			child->GetComponent<RelationshipComponent>().ChildrenID.push_back( c->GetComponent<IdComponent>().ID );
		}

		return child;
	}

	template<typename... V>
	static void IterateOverAllComponents( entt::entity entity, entt::registry& reg, std::vector<entt::id_type>& rMap )
	{
		( [ & ]()
		{
			if( reg.any_of<V>( entity ) )
			{
				rMap.push_back( entt::type_id<V>().hash() );
			}
		} ( ), ... );
	}

	template<typename... V>
	static void IterateOverAllComponents( 
		ComponentGroup<V...>, 
		entt::entity entity, entt::registry& reg, 
		std::vector<entt::id_type>& rMap )
	{
		IterateOverAllComponents<V...>( entity, reg, rMap );
	}

	void Prefab::RebuildComponentCache()
	{
		m_ComponentCaches.clear();

		m_Scene->Each( [this]( const auto entity ) 
		{
			auto& rCachesList = m_ComponentCaches[ entity->GetUUID() ];

			IterateOverAllComponents( AllComponents{}, entity->GetHandle(), m_Scene->GetRegistry(), rCachesList );
		} );
	}

	void Prefab::SerialisePrefab( std::ofstream& rStream )
	{
		m_Scene->SerialiseInternal( rStream );
	}

	void Prefab::DeserialisePrefab( std::istream& rStream )
	{
		m_Scene->DeserialiseInternal( rStream );
		
		// Find root entity.
		SharedPtr<Entity> RootEntity;

		for( const auto& entity : m_Scene->GetAllEntitiesWith<RelationshipComponent>() )
		{
			if( entity->GetComponent<RelationshipComponent>().Parent != 0 )
				continue;

			SAT_CORE_ASSERT( RootEntity, "A root entity was already found! A prefab can only have one root entity (root entity means an entity with no parent)." );
			RootEntity = entity;
			
			break;
		}

		m_Entity = RootEntity;
	}
}
