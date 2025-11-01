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

#include "Saturn/Core/Base.h"
#include "Saturn/Core/Log.h"

#include "Saturn/Asset/Asset.h"

#include "SharedGlobals.h"

#include "Saturn/AI/Navigation/NavigationSystem.h"

#include "Saturn/GameFramework/Core/GameScript.h"

#include "Saturn/Core/Renderer/EditorCamera.h"

#include "Saturn/Core/UUID.h"
#include "Saturn/Core/Timestep.h"

#include "RuntimeState.h"

#include "entt.hpp"

#if defined( SAT_ENABLE_GAMETHREAD )
#include <shared_mutex>
#endif

class dtNavMeshQuery;

namespace Saturn {

	class Entity;
	class Prefab;
	class PhysicsScene;
	class SClass;
	class SceneRenderer;
	class NavBoundsEntity;
	class PhysicsRigidBody;
	class PlayerInputController;

	struct TransformComponent;
	struct RaycastHitResult;

	struct SceneComponent
	{
		UUID SceneID;
	};

	struct DirectionalLight
	{
		glm::vec3 Direction = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Radiance = { 0.0f, 0.0f, 0.0f };

		float Intensity = 1.0f;

		static void Serialise( const DirectionalLight& rObject, std::ofstream& rStream )
		{
			RawSerialisation::WriteVec3( rObject.Direction, rStream );
			RawSerialisation::WriteVec3( rObject.Radiance, rStream );

			RawSerialisation::WriteObject( rObject.Intensity, rStream );
		}

		template<typename IStream>
		static void Deserialise( DirectionalLight& rObject, IStream& rStream )
		{
			RawSerialisation::ReadVec3( rObject.Direction, rStream );
			RawSerialisation::ReadVec3( rObject.Radiance, rStream );

			RawSerialisation::ReadObject( rObject.Intensity, rStream );
		}
	};

	struct PointLight
	{
		PointLight() = default;

		PointLight( const glm::vec3& rPosition, const glm::vec3& rRadiance, float multiplier, float lightSize, float radius, float minRadius, float fallOff )
			: Position( rPosition ), Radiance( rRadiance ), Multiplier( multiplier ), LightSize( lightSize ), Radius( radius ), MinRadius( minRadius ), Falloff( fallOff ) 
		{
		}

		alignas( 16 ) glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
		alignas( 16 ) glm::vec3 Radiance = { 0.0f, 0.0f, 0.0f };

		alignas( 4 ) float Multiplier = 1.0f;
		alignas( 4 ) float LightSize = 0.5f;
		alignas( 4 ) float Radius = 10.0f;
		alignas( 4 ) float MinRadius = 0.001f;
		alignas( 4 ) float Falloff = 1.f;

		static void Serialise( const PointLight& rObject, std::ofstream& rStream )
		{
			RawSerialisation::WriteVec3( rObject.Position, rStream );
			RawSerialisation::WriteVec3( rObject.Radiance, rStream );

			RawSerialisation::WriteObject( rObject.Multiplier, rStream );
			RawSerialisation::WriteObject( rObject.LightSize, rStream );
			RawSerialisation::WriteObject( rObject.Radius, rStream );
			RawSerialisation::WriteObject( rObject.MinRadius, rStream );
			RawSerialisation::WriteObject( rObject.Falloff, rStream );
		}

		template<typename IStream>
		static void Deserialise( PointLight& rObject, IStream& rStream )
		{
			RawSerialisation::ReadVec3( rObject.Position, rStream );
			RawSerialisation::ReadVec3( rObject.Radiance, rStream );

			RawSerialisation::ReadObject( rObject.Multiplier, rStream );
			RawSerialisation::ReadObject( rObject.LightSize, rStream );
			RawSerialisation::ReadObject( rObject.Radius, rStream );
			RawSerialisation::ReadObject( rObject.MinRadius, rStream );
			RawSerialisation::ReadObject( rObject.Falloff, rStream );
		}
	};

	struct Lights
	{
		DirectionalLight DirectionalLights[ 4 ];
		std::vector<PointLight> PointLights;

		[[nodiscard]] uint32_t GetPointLightSize() const { return static_cast<uint32_t>( sizeof( PointLight ) * PointLights.size() ); };

		static void Serialise( const Lights& rObject, std::ofstream& rStream )
		{
			RawSerialisation::WriteVector( rObject.PointLights, rStream );

			DirectionalLight::Serialise( rObject.DirectionalLights[ 0 ], rStream );
			DirectionalLight::Serialise( rObject.DirectionalLights[ 1 ], rStream );
			DirectionalLight::Serialise( rObject.DirectionalLights[ 2 ], rStream );
			DirectionalLight::Serialise( rObject.DirectionalLights[ 3 ], rStream );
		}

		template<typename IStream>
		static void Deserialise( Lights& rObject, IStream& rStream )
		{
			RawSerialisation::ReadVector( rObject.PointLights, rStream );

			DirectionalLight::Deserialise( rObject.DirectionalLights[ 0 ], rStream );
			DirectionalLight::Deserialise( rObject.DirectionalLights[ 1 ], rStream );
			DirectionalLight::Deserialise( rObject.DirectionalLights[ 2 ], rStream );
			DirectionalLight::Deserialise( rObject.DirectionalLights[ 3 ], rStream );
		}
	};

	struct CreateEntityParameters
	{
		UUID ID;
		std::string Tag;
		SharedPtr<Entity> Parent;
		// TODO: We could use Entity::StaticClass but then we have to include Entity.h, not really ideal
		SClass* pClass = nullptr;

		// TODO: We could use TransformComponent but then we have to include Components.h, not really ideal
		glm::vec3 Position;
		glm::vec3 Rotation;
		glm::vec3 Scale = { 1.0F, 1.0F, 1.0F };
	};

	class Scene : public Asset
	{
		SAT_DECLARE_CLASS_NO_INTER( Scene, Asset )
	public:
		Scene();
		~Scene();
		
		// Spawn an entity, this is the most basic function for spawning an Entity
		[[nodiscard]] SharedPtr<Entity> CreateEntity( const std::string& name = "" );

		// Spawn an entity, with custom parameters
		[[nodiscard]] SharedPtr<Entity> CreateEntity( CreateEntityParameters& rParams );

		[[nodiscard]] SharedPtr<Entity> CreateEntityWithIDScript( UUID uuid, const std::string& name = "", const std::string& rScriptName = "", bool externalData = true );

		template<typename Ty>
		[[nodiscard]] SharedPtr<Ty> CreateEntityFromClass( const std::string& rEntityName = "" )
		{
			static_assert( std::is_base_of<Entity, Ty>::value, "Ty must be based from an entity!" );

			SharedPtr<Ty> entity = dynamic_cast<Ty*>( ClassMetadataHandler::Get().CreateClassObject( Ty::StaticClass() ) );
			entity->SetName( rEntityName );

			OnEntityCreated( entity );

			return entity;
		}

	public:
		void OnRenderEditor( const EditorCamera& rCamera, Timestep ts, SceneRenderer& rSceneRenderer );
		void OnRenderRuntime( Timestep ts, SceneRenderer& rSceneRenderer );

		SharedPtr<Entity> DuplicateEntity( const SharedPtr<Entity> entity, const SharedPtr<Entity> parent = nullptr );
		void DeleteEntity( SharedPtr<Entity> entity, bool deleteChildren = true, UUID orphanParentID = 0 );
		
		void DestroyEntity( Entity* entity );

		void OnModifyPrefab( Ref<Prefab> prefabAsset );

		void TravelToScene( AssetID newSceneID );

		void OnUpdate( Timestep ts );
		void OnUpdatePhysics( Timestep ts );
		void OnUpdateAnimators( Timestep ts );
		void OnEvent( Event& rEvent );

	public:
		template<typename T>
		std::vector<SharedPtr<Entity>> GetAllEntitiesWith( void )
		{
			std::vector<SharedPtr<Entity>> result;

			for( const auto& [ id, entity ] : m_EntityIDMap )
			{
				if( entity->HasComponent<T>() )
					result.push_back( entity );
			}

			return result;
		}

		template<typename T>
		std::vector<SharedPtr<Entity>> GetAllEntitiesWithClass( void )
		{
			std::vector<SharedPtr<Entity>> result;

			for( const auto& [id, entity] : m_EntityIDMap )
			{
				if( entity->GetClass() == T::StaticClass() )
					result.push_back( entity );
			}

			return result;
		}

		template<typename Func>
		void Each( Func Function )
		{
			for( auto&& [id, entity] : m_EntityIDMap )
			{
				Function( entity );
			}
		}

		// #ReplaceRawPtrOrRefWithWeakRef
		[[nodiscard]] WeakRef<Entity> GetMainCameraEntity( bool force = false );
		
		[[nodiscard]] SharedPtr<Entity> FindEntityByTag( const std::string& tag );
		[[nodiscard]] SharedPtr<Entity> FindEntityByID( const UUID& id );
		[[nodiscard]] SharedPtr<Entity> FindEntityByHandle( entt::entity handle );

		// Convert the local space transformation into world space
		glm::mat4 GetTransformRelativeToParent( const SharedPtr<Entity> entity );

		// Convert the local space transformation into world space
		TransformComponent GetWorldSpaceTransform( const SharedPtr<Entity> entity );

		[[nodiscard]] bool Raycast( const glm::vec3& Origin, const glm::vec3& Direction, float MaxDistance, RaycastHitResult* pOut );

	public:
		void CopyScene( Ref<Scene>& NewScene );
		void Empty();

		[[nodiscard]] bool IsRuntimeActive() const { return m_RuntimeState != RuntimeState::NoState && m_RuntimeState != RuntimeState::Ending; }

		[[nodiscard]] bool IsRuntimeRunning() const { return m_RuntimeState == RuntimeState::Running; }

		[[nodiscard]] RuntimeState GetRuntimeState() const { return m_RuntimeState; }

		[[nodiscard]] bool OnRuntimeStart();

		void SuspendRuntime();
		void ResumeRuntime();
		
		void SuspendOrResumeRuntime();

		void OnRuntimeEnd();

		entt::registry& GetRegistry() { return m_Registry; }
		const entt::registry& GetRegistry() const { return m_Registry; }

		SharedPtr<NavBoundsEntity> GetNavBoundsEntity() const;

		// This transfers a prefab to an entity.
		// The prefabs holds an entity however that entity is local to it's scene and we want that entity to be our scene.
		[[nodiscard]] SharedPtr<Entity> CreatePrefab( Ref<Prefab> prefabAsset );

		[[nodiscard]] entt::entity CreateHandle()
		{
			return m_Registry.create();
		}

		void RemoveHandle( entt::entity handle ) 
		{
			if( m_Registry.valid( handle ) ) 
			{
				m_Registry.destroy( handle );
			}
		}

		// Start NEW audio players
		void StartAudioPlayers();

		// Pause audio players
		void StopAudioPlayers();

		// Stop and unload audio players
		void DestroyAudioPlayers();
		
		void UpdateAudioListeners();

#if !defined(SAT_DIST)
		void MarkDirty() { m_Dirty = true; }
		void CleanDirty() { m_Dirty = false; }
		bool IsDirty() const { return m_Dirty; }
#else
		void MarkDirty() {}
		void CleanDirty() {}
		bool IsDirty() const { return false; }
#endif

		void AcknowledgeHotReload();

		void AddController( Ref<PlayerInputController> playerInputController );
		void RemoveController( Ref<PlayerInputController> playerInputController );
		static void   SetActiveScene( Scene* pScene );
		static Scene* GetActiveScene();

		void PostDeserialise();

	public:
		//////////////////////////////////////////////////////////////////////////
		// #WARNING This should not be confused with AssetSerialisers. This is for raw binary serialisation!

		void SerialiseData();
		void DeserialiseData();

	private:
		void SerialiseInternal( std::ofstream& rStream );

		template<typename IStream>
		void DeserialiseInternal( IStream& rStream );

		SharedPtr<Entity> HotReloadReplaceOldEntity( SharedPtr<Entity> source );

		void TransferModifiedProperties( const SharedPtr<Entity>& rSourceEntity, SharedPtr<Entity>& rEntity, const std::string& rMetadataName );

		void DestroyPendingEntities();
		void DeleteEntityChecked( Entity* pEntity );

	protected:
		void OnEntityCreated( SharedPtr<Entity> entity );

	public:

		//////////////////////////////////////////////////////////////////////////
		// TODO: Rework this is as locking a mutex every frame can be bad for performance.

		template<typename Ty, typename... Args>
		Ty& AddComponent(entt::entity entity, Args&&... args )
		{
#if defined( SAT_ENABLE_GAMETHREAD )
			std::unique_lock<std::mutex> Lock( m_Mutex, std::try_to_lock );
#endif
			if( !HasComponent<Ty>( entity ) ) 
			{
				return m_Registry.emplace<Ty>( entity, std::forward<Args>( args )... );
			}
			else
				return GetComponent<Ty>( entity );
		}

		template<typename Ty>
		[[nodiscard]] bool HasComponent( entt::entity entity ) const
		{
#if defined( SAT_ENABLE_GAMETHREAD )
			std::unique_lock<std::mutex> Lock( m_Mutex, std::try_to_lock );
#endif
			return m_Registry.any_of<Ty>( entity );
		}

		template<typename Ty>
		void RemoveComponent( entt::entity entity )
		{
#if defined( SAT_ENABLE_GAMETHREAD )
			std::unique_lock<std::mutex> Lock( m_Mutex, std::try_to_lock );
#endif
			if( HasComponent<Ty>( entity ) )
			{
				m_Registry.remove<Ty>( entity );
			}
		}

		template<typename Ty>
		[[nodiscard]] Ty& GetComponent( entt::entity entity )
		{
#if defined( SAT_ENABLE_GAMETHREAD )
			std::unique_lock<std::mutex> Lock( m_Mutex, std::try_to_lock );
#endif
			SAT_CORE_ASSERT( HasComponent<Ty>( entity ), "Entity does not have component!" );

			return m_Registry.get<Ty>( entity );
		}

		template<typename Ty>
		[[nodiscard]] const Ty& GetComponent( entt::entity entity ) const
		{
#if defined( SAT_ENABLE_GAMETHREAD )
			std::unique_lock<std::mutex> Lock( m_Mutex, std::try_to_lock );
#endif
			SAT_CORE_ASSERT( HasComponent<Ty>( entity ), "Entity does not have component!" );

			return m_Registry.get<Ty>( entity );
		}

		template<typename Ty>
		[[nodiscard]] Ty* TryGetComponent( entt::entity entity )
		{
#if defined( SAT_ENABLE_GAMETHREAD )
			std::unique_lock<std::mutex> Lock( m_Mutex, std::try_to_lock );
#endif
			return m_Registry.try_get<Ty>( entity );
		}

		template<typename Ty>
		[[nodiscard]] const Ty* TryGetComponent( entt::entity entity ) const
		{
#if defined( SAT_ENABLE_GAMETHREAD )
			std::unique_lock<std::mutex> Lock( m_Mutex, std::try_to_lock );
#endif
			return m_Registry.try_get<Ty>( entity );
		}

	public:
		void PrepareForNavMeshBuilding();

		void OnNavMeshBuildCompleted();
		dtNavMeshQuery* GetNavMeshQuery() { return nullptr; }
		std::shared_ptr<PhysicsScene> GetPhysicsScene() const { return m_PhysicsScene; }
		NavigationSystem& GetNavigationSystem() { return m_NavigationSystem; }

		void DestroyPhysicsScene();
	private:
		void CreatePhysicsScene();

		void OnNavMeshBuildCompAdded( entt::registry& reg, entt::entity entity );
		void OnNavMeshBuildCompRemoved( entt::registry& reg, entt::entity entity );

	private:
		void RtSetupLights();
		void RtBuildRenderer2DCommands();
		void RtBuildSceneRendererCommands( SceneRenderer& rSceneRenderer );

	private:
		std::map<entt::entity, SharedPtr<Entity>> m_EntityIDMap;
		entt::registry m_Registry;
		entt::entity m_SceneEntity{ entt::null };
		
		RuntimeState m_RuntimeState = RuntimeState::NoState;

		std::vector<Ref<PlayerInputController>> m_Controllers;

		// For use by Runtime Scene only
		// Holds a list of the entities that will be destroyed in the next frame
		std::vector<Entity*> m_EntitiesToDestory;

		Lights m_Lights;

		// #ReplaceRawPtrOrRefWithWeakRef
		WeakRef<Entity> m_pMainCameraEntity;
		
		SharedPtr<NavBoundsEntity> m_NavBoundsEntity = nullptr;

#if defined( SAT_ENABLE_GAMETHREAD )
		std::mutex m_Mutex;
#endif

		RendererCamera m_RendererCamera;

		std::shared_ptr<PhysicsScene> m_PhysicsScene;

		NavigationSystem m_NavigationSystem;

		UUID m_InternalID;
#if !defined(SAT_DIST)
		bool m_Dirty = false;
#endif

	private:
		friend class PhysXSceneExporter;
		friend class Entity;
		friend class Prefab;
		friend class SceneHierarchyPanel;
		friend class SceneSerialiser;
		friend class SceneRenderer;
	};

}
