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

#include "Components.h"
#include "Scene.h"

#include "Saturn/GameFramework/SObject.h"
#include "Saturn/GameFramework/Core/GameScript.h"

#include <glm/glm.hpp>
#include "entt.hpp"

namespace Saturn {

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

	public:
		template<typename T, typename... Args>
		inline T& AddComponent( Args&&... args )
		{
			return m_Scene->AddComponent<T>( m_EntityHandle, std::forward<Args>( args )... );
		}

		template<typename T>
		[[nodiscard]] inline T& GetComponent()
		{
			return m_Scene->GetComponent<T>( m_EntityHandle );
		}

		template<typename T>
		[[nodiscard]] inline const T& GetComponent() const
		{
			return m_Scene->GetComponent<T>( m_EntityHandle );
		}

		template<typename T>
		[[nodiscard]] inline bool HasComponent() const
		{
			return m_Scene->HasComponent<T>( m_EntityHandle );
		}

		template<typename... T>
		[[nodiscard]] inline bool HasComponents() const
		{
			return m_Scene->template HasComponents<T...>( m_EntityHandle );
		}

		template<typename T>
		inline void RemoveComponent()
		{
			m_Scene->RemoveComponent<T>( m_EntityHandle );
		}

		template<typename... T>
		inline void RemoveComponents()
		{
			m_Scene->template RemoveComponents<T...>( m_EntityHandle );
		}

		template<typename T>
		[[nodiscard]] inline T* TryGetComponent() 
		{
			return m_Scene->TryGetComponent<T>( m_EntityHandle );
		}

		template<typename T>
		[[nodiscard]] inline const T* TryGetComponent() const
		{
			return m_Scene->TryGetComponent<T>( m_EntityHandle );
		}

	public:
		[[nodiscard]] bool Valid()
		{
			return m_EntityHandle != entt::null || m_Scene->m_Registry.valid( m_EntityHandle );
		}

		[[nodiscard]] bool Valid() const
		{
			return m_EntityHandle != entt::null || m_Scene->m_Registry.valid( m_EntityHandle );
		}

		Scene* GetScene() { return m_Scene; }
		const Scene* GetScene() const { return m_Scene; }

		glm::mat4 Transform() const { return m_Scene->m_Registry.get<TransformComponent>( m_EntityHandle ).GetTransform(); }
		
		glm::vec3 GetLocalPosition() const { return m_Scene->m_Registry.get<TransformComponent>( m_EntityHandle ).Position; }
		glm::quat GetLocalRotation() const { return m_Scene->m_Registry.get<TransformComponent>( m_EntityHandle ).GetRotationEuler(); }
		glm::vec3 GetLocalScale() const { return m_Scene->m_Registry.get<TransformComponent>( m_EntityHandle ).Scale; }

		void SetName( const std::string& rName );

		const entt::entity GetHandle()       { return m_EntityHandle; }
		const entt::entity GetHandle() const { return m_EntityHandle; }

		UUID GetUUID() const { return GetComponent<IdComponent>().ID; }
		[[nodiscard]] const std::string& GetName() const { return GetComponent<TagComponent>().Tag; }

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
		entt::entity m_EntityHandle{ entt::null };
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
