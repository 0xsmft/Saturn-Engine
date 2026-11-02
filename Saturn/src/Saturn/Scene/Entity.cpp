/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2026 0xsmft https://0xsmft.dev                                       *
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

#include "Saturn/Physics/PhysicsShapes.h"
#include "Saturn/Physics/PhysicsCharacterController.h"

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

		AddComponent<IdComponent>( Id );
		AddComponent<RelationshipComponent>();
		AddComponent<TransformComponent>();
		AddComponent<TagComponent>( rName );
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

	void Entity::SetPosition( const glm::vec3& rPosition )
	{
		if( auto* pMovementComp = TryGetComponent<CharacterMovementComponent>(); pMovementComp )
		{
			// The movement component may not have a valid physics movement
			// this is because the component may exist in the editor but 
			// the physics part only exists during runtime.
			if( auto* pPhysMovement = pMovementComp->CharacterMovement; pPhysMovement )
			{
				pPhysMovement->StopMovementImmediately();
				pPhysMovement->Teleport( rPosition );
			}
			else
			{
				// Yes, this could be removed and we could always call this
				// even if we are a physics object but doing so is pointless
				// because the position of this object will be set when the
				// PhysicsScene updates.
				GetComponent<TransformComponent>().Position = rPosition;
			}
		}
		else
		{
			GetComponent<TransformComponent>().Position = rPosition;
		}
	}

	void Entity::SetRotation( const glm::vec3& rRotatonEuler )
	{
		if( auto* pMovementComp = TryGetComponent<CharacterMovementComponent>(); pMovementComp )
		{
			if( auto* pPhysMovement = pMovementComp->CharacterMovement; pPhysMovement )
			{
				pPhysMovement->Rotate( glm::quat( rRotatonEuler ) );
			}
			else
			{
				GetComponent<TransformComponent>().Position = rRotatonEuler;
			}
		}
		else
		{
			GetComponent<TransformComponent>().SetRotation( rRotatonEuler );
		}
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

	void Entity::RemoveFromParent()
	{
		auto parent = GetScene()->FindEntityByID( GetParent() );
		if( !parent )
			return;

		auto& rChildren = parent->GetChildren();
		rChildren.erase( std::remove( rChildren.begin(), rChildren.end(), GetUUID() ), rChildren.end() );

		SetParent( 0 );
	}

	void Entity::ChangeToNewParent( SharedPtr<Entity> parent )
	{
		RemoveFromParent();
		SetParent( parent->GetUUID() );

		parent->AddChild( GetUUID() );
	}

	SharedPtr<Entity> Entity::TryGetParent()
	{
		return GetScene()->FindEntityByID( GetParent() );
	}

	const SharedPtr<Entity> Entity::TryGetParent() const
	{
		return GetScene()->FindEntityByID( GetParent() );
	}

	void Entity::AttachToBone( const std::string& rAttachmentName )
	{
		AddComponent<BoneAttachmentInfoComponent>( rAttachmentName );
	}

	void Entity::AttachToBone( SharedPtr<Entity> parent, const std::string& rAttachmentName )
	{
		RemoveFromParent();
		ChangeToNewParent( parent );

		AttachToBone( rAttachmentName );
	}

	void Entity::DetachFromBone()
	{
		RemoveComponent<BoneAttachmentInfoComponent>();
	}

	glm::vec3 Entity::CalculateForwardVectorFromRotation()
	{
		return GetLocalRotationQuat() * TransformComponent::Forward;
	}

	//////////////////////////////////////////////////////////////////////////

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
