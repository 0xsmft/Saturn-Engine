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
#include "Prefab.h"

#include "Saturn/Asset/AssetManager.h"

#include "Saturn/Scene/Scene.h"
#include "Saturn/Scene/Entity.h"

namespace Saturn {

	template<typename... V>
	static void CopyComponentIfExists( entt::entity dst, entt::entity src, entt::registry& rRegistry, entt::registry& rDstRegistry )
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
	static void CopyComponentIfExists( ComponentGroup<V...>, entt::entity dst, entt::entity src, entt::registry& rRegistry, entt::registry& rDstRegistry )
	{
		CopyComponentIfExists<V...>( dst, src, rRegistry, rDstRegistry );
	}

	static void SwapActiveScene( Scene* pScene ) 
	{
		if( GActiveScene != pScene )
			GActiveScene = pScene;
	}

	static void RestoreActiveScene( Scene* pNewScene ) 
	{
		if( GActiveScene != pNewScene )
			GActiveScene = pNewScene;
	}

	Prefab::Prefab()
	{
	}

	Prefab::~Prefab()
	{
		m_Scene = nullptr;
	}

	void Prefab::Create( const Ref<Entity>& srcEntity )
	{
		m_Scene = Ref<Scene>::Create();

		Scene* CurrentScene = GActiveScene;
		SwapActiveScene( m_Scene.Get() );

		// TODO: (Entities using refs) Fix this
		if( srcEntity->Valid() )
			m_Entity = CreateFromEntity( srcEntity );

		RestoreActiveScene( CurrentScene );
	}

	void Prefab::Create()
	{
		m_Scene = Ref<Scene>::Create();
	}

	void Prefab::CreateScene()
	{
		m_Scene = nullptr;
		m_Scene = Ref<Scene>::Create();
	}

	void Prefab::SerialisePrefab( std::ofstream& rStream )
	{
		m_Scene->SerialiseInternal( rStream );
	}

	void Prefab::DeserialisePrefab( std::istream& rStream )
	{
		Create();

		m_Scene->DeserialiseInternal( rStream );
	}

	Ref<Entity> Prefab::PrefabToEntity( Ref<Scene> Scene )
	{
		Ref<Entity> result = Scene->CreateEntity();
		result->AddComponent<PrefabComponent>().AssetID = ID;

		// Now we need to find the root entity of the prefab.

		auto entities = m_Scene->GetAllEntitiesWith<RelationshipComponent>();

		Ref<Entity> RootEntity = nullptr;

		for( auto& entity : entities )
		{
			if( entity->GetParent() == 0 )
			{
				RootEntity = entity;
				break;
			}
		}

		if( !RootEntity )
			RootEntity = m_Entity;

		CopyComponentIfExists( AllComponents{}, 
			result->m_EntityHandle, RootEntity->m_EntityHandle,
			m_Scene->m_Registry, Scene->m_Registry );

		// We don't want the same id, what if we spawn this prefab and it has the same id?
		result->GetComponent<IdComponent>().ID = {};

		for( auto& childId : RootEntity->GetChildren() )
		{
			Ref<Entity> child = CreateChildren( m_Scene->FindEntityByID( childId ), Scene );

			child->SetParent( result->GetComponent<IdComponent>().ID );
		}

		return result;
	}

	Ref<Entity> Prefab::CreateFromEntity( Ref<Entity> srcEntity )
	{
		Ref<Entity> result = GActiveScene->CreateEntity();
		result->AddComponent<PrefabComponent>().AssetID = ID;
		
		auto& rc = srcEntity->GetComponent<RelationshipComponent>();

		CopyComponentIfExists( AllComponents{}, 
			result->m_EntityHandle, srcEntity->m_EntityHandle,
			srcEntity->m_Scene->m_Registry, m_Scene->m_Registry );

		for( auto& childId : srcEntity->GetChildren() )
		{
			Ref<Entity> child = CreateFromEntity( srcEntity->m_Scene->FindEntityByID( childId ) );

			auto& rc = result->GetComponent<RelationshipComponent>();

			child->SetParent( result->GetComponent<IdComponent>().ID );
			rc.ChildrenID.push_back( child->GetComponent<IdComponent>().ID );
		}

		return result;
	}

	Ref<Entity> Prefab::CreateChildren( const Ref<Entity>& parent, Ref<Scene> Scene )
	{
		// Create the child in the new scene.
		Ref<Entity> child = GActiveScene->CreateEntity();
		
		// Copy Components, from our child in the scene.
		CopyComponentIfExists( AllComponents{}, 
			child->m_EntityHandle, parent->m_EntityHandle, 
			m_Scene->m_Registry, Scene->m_Registry );

		// Check if this entity has any children.
		for( auto& childId : child->GetChildren() )
		{
			Ref<Entity> c = CreateChildren( child, Scene );

			c->SetParent( child->GetComponent<IdComponent>().ID );
			
			child->GetComponent<RelationshipComponent>().ChildrenID.push_back( c->GetComponent<IdComponent>().ID );
		}

		return child;
	}

}
