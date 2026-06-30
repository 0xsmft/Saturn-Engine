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
#include "Entity.h"
#include "Scene.h"

#include "Saturn/Serialisation/Raw/RawEntitySerialisation.h"

namespace Saturn {

	Entity::Entity()
	{
		m_Scene = g_ActiveScene;
		m_EntityHandle = m_Scene->CreateHandle();
		
		AddComponent<IdComponent>();
		AddComponent<RelationshipComponent>();
		AddComponent<TransformComponent>();
		AddComponent<TagComponent>().Tag = "Unnamed Entity";
	}

	Entity::Entity( const std::string& rName, UUID Id )
	{
		m_Scene = g_ActiveScene;
		m_EntityHandle = m_Scene->CreateHandle();

		AddComponent<IdComponent>().ID = Id;
		AddComponent<RelationshipComponent>();
		AddComponent<TransformComponent>();
		AddComponent<TagComponent>().Tag = rName;
	}

	Entity::Entity( const Entity& other )
	{
		this->m_Scene = other.m_Scene;
		this->m_EntityHandle = other.m_EntityHandle;
	}

	Entity::Entity( Scene* scene )
	{
		m_Scene = scene;
		m_EntityHandle = m_Scene->CreateHandle();

		AddComponent<IdComponent>();
		AddComponent<RelationshipComponent>();
		AddComponent<TransformComponent>();
		AddComponent<TagComponent>().Tag = "Unnamed Entity";
	}

	Entity::~Entity()
	{
		m_Scene->RemoveHandle( m_EntityHandle );
		m_EntityHandle = entt::null;

		m_Scene = nullptr;
	}

	bool Entity::Valid()
	{
		return m_EntityHandle != entt::null || m_Scene->m_Registry.valid( m_EntityHandle );
	}

	bool Entity::Valid() const
	{
		return m_EntityHandle != entt::null || m_Scene->m_Registry.valid( m_EntityHandle );
	}

	void Entity::SetName( const std::string& rName )
	{
		GetComponent<TagComponent>().Tag = rName;
	}

	void Entity::Show()
	{
		m_VisibilityFlag = EntityVisibility::Visible;
	
		// Propagate down to children.
		for( auto& rChildID : GetChildren() )
		{
			auto child = m_Scene->FindEntityByID( rChildID );
			if( child )
			{
				child->Show();
			}
		}
	}

	void Entity::Hide()
	{
		m_VisibilityFlag = EntityVisibility::Hidden;

		// Propagate down to children.
		for( auto& rChildID : GetChildren() )
		{
			auto child = m_Scene->FindEntityByID( rChildID );
			if( child )
			{
				child->Hide();
			}
		}
	}

	UUID Entity::GetPhysicsMaterialID()
	{
		if( const auto* pRb = TryGetComponent<RigidbodyComponent>() )
		{
			return pRb->MaterialAssetID;
		}
		else if( const auto* pStaticMesh = TryGetComponent<StaticMeshComponent>(); pStaticMesh && pStaticMesh->Mesh )
		{
			return pStaticMesh->Mesh->GetPhysicsMaterial();
		}
		else if( const auto* pSkMesh = TryGetComponent<SkeletalMeshComponent>(); pSkMesh && pSkMesh->Mesh )
		{
			return pSkMesh->Mesh->GetPhysicsMaterial();
		}
		else
		{
			return 0llu;
		}
	}

	void Entity::Serialise( const SharedPtr<Entity>& rObject, std::ofstream& rStream )
	{
		RawEntitySerialisation::SerialiseEntity( rObject, rStream );
	}

	void Entity::Deserialise( SharedPtr<Entity>& rObject, std::istream& rStream )
	{
		RawEntitySerialisation::DeserialiseEntity( rObject, rStream );
	}
}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG_SPWN( Entity );
