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

#include "Components.h"
#include "EntityVisibility.h"

#include "Saturn/GameFramework/SObject.h"
#include "Saturn/GameFramework/Core/GameScript.h"

#include <glm/glm.hpp>
#include "entt.hpp"

namespace Saturn {

	class Scene;

	class Entity : public SObject, public EnabledSharedFromThis<Entity>
	{
		//////////////////////////////////////////////////////////////////////////
		// Needed for game class.

		SAT_DECLARE_CLASS_MOVE( Entity, SObject )
	public:
		Entity();
		Entity( Scene* scene );
		Entity( const std::string& rName, UUID Id );
		Entity( const Entity& other );

		virtual ~Entity();

		// Called when the Runtime begins or when this entity is spawned
		virtual void BeginPlay() {}

		// Called every frame with a potentially variable timestep 
		virtual void OnUpdate( Saturn::Timestep ts ) {}

		// Called every frame with a fixed timestep 
		virtual void OnPhysicsUpdate( Saturn::Timestep ts ) {}

		// TODO: Not sure if I like having the physics events in every single entity....
		//		 we could create a component like PhysicsCallbackComponent, that has two function
		//		 pointers to events... or some other inheritance way.
	
		// When this entity hits another entity with a physics body, or a trigger.
		virtual void OnEntityHit( Entity* pOther, bool isTrigger ) {}

		// When this entity is no longer in the trigger or the entity.
		virtual void OnEntityLeave( Entity* pOther, bool isTrigger ) {}

	public:
		template<typename T, typename... Args>
		T& AddComponent( Args&&... args );

		template<typename T>
		[[nodiscard]] T& GetComponent();

		template<typename T>
		[[nodiscard]] const T& GetComponent() const;

		template<typename T>
		[[nodiscard]] bool HasComponent() const;

		template<typename... T>
		[[nodiscard]] bool HasComponents() const;

		template<typename T>
		void RemoveComponent();

		template<typename... T>
		void RemoveComponents();

		template<typename T>
		[[nodiscard]] T* TryGetComponent();

		template<typename T>
		[[nodiscard]] const T* TryGetComponent() const;

	public:
		[[nodiscard]] bool Valid();
		[[nodiscard]] bool Valid() const;

		Scene* GetScene() { return m_Scene; }
		const Scene* GetScene() const { return m_Scene; }

		glm::mat4 Transform() const { return GetComponent<TransformComponent>().GetTransform(); }
		
		glm::vec3 GetLocalPosition() const { return GetComponent<TransformComponent>().Position; }
		glm::vec3 GetLocalRotation() const { return GetComponent<TransformComponent>().GetRotationEuler(); }
		glm::quat GetLocalRotationQuat() const { return GetComponent<TransformComponent>().GetRotation(); }
		glm::vec3 GetLocalScale() const { return GetComponent<TransformComponent>().Scale; }

		void SetPosition( const glm::vec3& rPosition ) { GetComponent<TransformComponent>().Position = rPosition; }
		void SetRotation( const glm::vec3& rRotatonEuler ) { GetComponent<TransformComponent>().SetRotation( rRotatonEuler ); }
		void SetScale( const glm::vec3& rScale ) { GetComponent<TransformComponent>().Scale = rScale; }

		void SetName( const std::string& rName );

		const entt::entity GetHandle()       { return m_EntityHandle; }
		const entt::entity GetHandle() const { return m_EntityHandle; }

		// Show this entity and it's children.
		void Show();

		// Hide this entity and it's children.
		void Hide();
		
		// Show/Hide this entity and it's children.
		inline void ShowOrHide() 
		{ 
			if( IsVisible() )
				Hide();
			else 
				Show(); 
		}

		// Set the visibility of _only_ this entity!
		void SetVisibility( EntityVisibility flag ) { m_VisibilityFlag = flag; }

		// Is this entity visible
		[[nodiscard]] bool IsVisible() const { return m_VisibilityFlag == EntityVisibility::Visible; }

		UUID GetUUID() const { return GetComponent<IdComponent>().ID; }
		[[nodiscard]] const std::string& GetName() const { return GetComponent<TagComponent>().Tag; }

		//
		// Set the parent ID only! (semi-internal function!)
		// 
		// For a more easy to use function see ChangeToNewParent, 
		// which will tell the parent that their child has be removed.
		// This function does not.
		//
		inline void SetParent( const UUID& rID )
		{
			GetComponent<RelationshipComponent>().Parent = rID;
		}

		inline UUID GetParent() const
		{
			return GetComponent<RelationshipComponent>().Parent;
		}

		inline const std::vector<UUID>& GetChildren() const { return GetComponent<RelationshipComponent>().ChildrenID; }

		inline std::vector<UUID>& GetChildren() { return GetComponent<RelationshipComponent>().ChildrenID; }
		
		inline void AddChild( UUID id ) 
		{
			auto& children = GetComponent<RelationshipComponent>().ChildrenID;

			const auto itr = std::find( children.begin(), children.end(), id );
			if( itr == children.end() )
			{
				children.push_back( id );
			}
		}

		[[nodiscard]] bool HasParent()   const { return GetComponent<RelationshipComponent>().Parent != 0; }
		[[nodiscard]] bool HasChildren() const { return GetComponent<RelationshipComponent>().ChildrenID.size() > 0; }
		
		[[nodiscard]] inline  bool IsSibling( const SharedPtr<Entity>& rOther ) const
		{
			auto& rRc = GetComponent<RelationshipComponent>(); 
			auto& rOtherRc = rOther->GetComponent<RelationshipComponent>();
		
			return rRc.Parent == rOtherRc.Parent; 
		}

		[[nodiscard]] inline bool IsDescendant( const SharedPtr<Entity>& rOther ) const
		{
			auto& rOtherRc = rOther->GetComponent<RelationshipComponent>();

			return rOtherRc.Parent == GetUUID();
		}

	public:
		//
		// @returns the physics material ID.
		// 
		// Priority is given to the RigidBody then the Static/Skeletal Meshes.
		// May return zero if no ID is set or could be found.
		//
		UUID GetPhysicsMaterialID();

		//
		// Helper to remove this entity from it's parent.
		//
		void RemoveFromParent();

		//
		// Move to a new parent and remove from the old parents list.
		//
		void ChangeToNewParent( SharedPtr<Entity> parent );

		//
		// Try to get the parent.
		// 
		// @returns -- the parent (if any, may be null if not found.)
		//
		[[nodiscard]] SharedPtr<Entity> TryGetParent();
		[[nodiscard]] const SharedPtr<Entity> TryGetParent() const;

		//
		// Attach this entity to a bone in it's parent.
		//
		void AttachToBone( const std::string& rAttachmentName );

		//
		// Attach this entity to a bone in a new parent.
		//
		void AttachToBone( SharedPtr<Entity> parent, const std::string& rAttachmentName );

		//
		// Remove this entity from a bone attachment.
		//
		void DetachFromBone();

		//
		// Calculate forward vector based from the
		// entity's _local_ rotation.
		//
		glm::vec3 CalculateForwardVectorFromRotation();

	public:
		operator entt::entity() const { return m_EntityHandle; }
		operator uint32_t() const { return ( uint32_t ) m_EntityHandle; }

		bool operator==( const Entity& other ) const
		{
			return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
		}

		bool operator!=( const Entity& other ) const
		{
			return !( *this == other );
		}

	public:
		static void Serialise( const SharedPtr<Entity>& rObject, std::ofstream& rStream );
		static void Deserialise( SharedPtr<Entity>& rObject, std::istream& rStream );

	private:
		// EnTT internal handle.
		entt::entity m_EntityHandle{ entt::null };

		// Deciding weather this should be in a component (e.g. VisibilityComponent) or the entity doesn't matter right now...
		// I just want the fucking visibility of the Entity.
		EntityVisibility m_VisibilityFlag = EntityVisibility::Visible;

		// Pointer to the Scene who owns us.
		Scene* m_Scene = nullptr;

	private:
		inline void Invalidate()
		{
			m_EntityHandle = entt::null;
		}
		
	private:
		friend class Scene;
		friend class Prefab;
	};

}
