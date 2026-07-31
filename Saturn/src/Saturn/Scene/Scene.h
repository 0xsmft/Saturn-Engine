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

#include "SceneVisualisationOptions.h"

#include "Entity.h"

#include "Saturn/Core/Base.h"
#include "Saturn/Core/Log.h"

#include "Saturn/Asset/Asset.h"

#include "SharedGlobals.h"

#include "Saturn/AI/Navigation/NavigationSystem.h"

#include "Saturn/GameFramework/Core/GameScript.h"
#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

#include "Saturn/Core/Renderer/EditorCamera.h"
#include "Saturn/Core/VariableGuard.h"

#include "Saturn/Core/UUID.h"
#include "Saturn/Core/Timestep.h"

#include "Saturn/Runtime/RuntimeState.h"

#include "entt.hpp"

#if defined( SAT_ENABLE_GAMETHREAD )
#include <shared_mutex>
#endif

class dtNavMeshQuery;

namespace Saturn {

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
		std::string Tag = "Empty Entity";
		SharedPtr<Entity> Parent;
		// TODO: We could use Entity::StaticClass but then we have to include Entity.h, not really ideal
		SClass* pClass = nullptr;

		// TODO: We could use TransformComponent but then we have to include Components.h, not really ideal
		glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Scale = { 1.0F, 1.0F, 1.0F };
	};

	class Scene : public Asset
	{
	public:
		Scene();
		~Scene();
		
		// Spawn an entity, this is the most basic function for spawning an Entity
		[[nodiscard]] SharedPtr<Entity> CreateEntity( const std::string& name = "" );

		// Spawn an entity, with custom parameters
		[[nodiscard]] SharedPtr<Entity> CreateEntity( CreateEntityParameters& rParams );

		// Spawn an entity with a set ID, name and a class name, only really used by serialisation.
		[[nodiscard]] SharedPtr<Entity> CreateEntityWithIDScript( UUID uuid, const std::string& name = "", const std::string& rScriptName = "", bool externalData = true );

		// Create class from SClass, providing a name for it.
		template<typename Ty>
		[[nodiscard]] SharedPtr<Ty> CreateEntityFromClass( const std::string& rEntityName = "" )
		{
			VariableGuard<Scene*> sceneGuard( g_ActiveScene, this );

			static_assert( std::is_base_of<Entity, Ty>::value, "Ty must be based from an entity!" );

			SharedPtr<Ty> entity( dynamic_cast<Ty*>( ClassMetadataHandler::Get().CreateClassObject( Ty::StaticClass() ) ) );
			entity->SetName( rEntityName );

			OnEntityCreated( entity );

			return entity;
		}

	public:
		void OnRenderEditor( Camera* pCamera, const glm::mat4& rViewMartix, Ref<SceneRenderer> sceneRenderer, Timestep ts );
		void OnRenderRuntime( Timestep ts, Ref<SceneRenderer> sceneRenderer );

		SharedPtr<Entity> DuplicateEntity( const SharedPtr<Entity> entity, const SharedPtr<Entity> parent = nullptr );

		//
		// Duplicate an Entity but place the new entity into a new scene
		// specified in targetScene
		//
		SharedPtr<Entity> DuplicateEntityBetweenScene( Ref<Scene> targetScene, const SharedPtr<Entity> entity, const SharedPtr<Entity> parent = nullptr );

		//
		// Deletes an entity. 
		// This function is editor only and not to be called during Runtime or in Dist. (Use DestroyEntity)
		// 
		// This function will automatically remove the entity from any undo/redo actions. (Although this is temporary as we do not yet have an undo/redo action for deleting an entity.)
		// 
		// @param entity - target entity to be deleted.
		// @param deleteChildren - should the children of the entity be deleted as well
		// @param orphanParentID - if so, what ID should their new parent be.
		//
		void DeleteEntity( SharedPtr<Entity> entity, bool deleteChildren = true, UUID orphanParentID = 0 );
		
		//
		// For information on this function please look at  DeleteEntity( SharedPtr<Entity> entity, ... )
		//
		void DeleteEntity( const entt::entity handle, bool deleteChildren = true, UUID orphanParentID = 0 );

		//
		// Runtime variant of DeleteEntity.
		// 
		// NB: This will add entity into a differed queue for deletion.
		//
		void DestroyEntity( Entity* entity );

		void OnModifyPrefab( AssetID prefabAssetID );

		void TravelToScene( AssetID newSceneID );

	public:
		void OnUpdate( Timestep ts );
		void OnUpdateEntities( Timestep ts );
		void OnUpdatePhysics( Timestep ts );
		void OnUpdateAnimators( Timestep ts );
		void OnEvent( Event& rEvent );
		
		// Internal use only! For use by SkeletonAssetView only!
		// Update bone joints but as previews.
		void OnUpdateAnimators_Preview( Timestep ts );

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
		std::vector<SharedPtr<T>> GetAllEntitiesWithClass( void )
		{
			std::vector<SharedPtr<T>> result;

			for( const auto& [id, entity] : m_EntityIDMap )
			{
				if( entity->GetClass() == T::StaticClass() )
					result.push_back( entity.As<T>() );
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

		[[nodiscard]] bool IsEmptyScene() const { return m_EntityIDMap.empty(); }

		// #ReplaceRawPtrOrRefWithWeakRef
		[[nodiscard]] WeakRef<Entity> GetMainCameraEntity( bool force = false );
		
		[[nodiscard]] SharedPtr<Entity> FindEntityByTag( const std::string& tag ) const;
		[[nodiscard]] SharedPtr<Entity> FindEntityByID( const UUID& id ) const;
		[[nodiscard]] SharedPtr<Entity> FindEntityByHandle( entt::entity handle ) const;

		// Convert the local space transformation into world space
		glm::mat4 GetTransformRelativeToParent( const SharedPtr<Entity> entity );

		// Convert the local space transformation into world space
		TransformComponent GetWorldSpaceTransform( const SharedPtr<Entity> entity );

		[[nodiscard]] bool Raycast( const glm::vec3& Origin, const glm::vec3& Direction, float MaxDistance, RaycastHitResult* pOut );
		[[nodiscard]] bool RaycastIgnore( SharedPtr<Entity> entityIgnore, const glm::vec3& Origin, const glm::vec3& Direction, float MaxDistance, RaycastHitResult* pOut );

	public:
		void CopyScene( Ref<Scene>& NewScene );
		void Empty();

	public:
		[[nodiscard]] bool IsRuntimeActive() const { return m_RuntimeState != RuntimeState::NoState && m_RuntimeState != RuntimeState::Ending; }
		[[nodiscard]] bool IsRuntimeRunning() const { return m_RuntimeState == RuntimeState::Running; }
		[[nodiscard]] bool IsPaused() const { return m_RuntimeState == RuntimeState::Paused; }
		[[nodiscard]] bool IsPausedOrSuspended() const;
		[[nodiscard]] bool OnRuntimeStart();

		[[nodiscard]] RuntimeState GetRuntimeState() const { return m_RuntimeState; }

		void SuspendRuntime();
		void ResumeRuntime();
		void SuspendOrResumeRuntime();
		void PauseGame();
		void UnpauseGame();
		void OnRuntimeEnd();

	public:
		entt::registry& GetRegistry() { return m_Registry; }
		const entt::registry& GetRegistry() const { return m_Registry; }

		const UUID GetInternalID() const { return m_InternalID; }

		SharedPtr<NavBoundsEntity> GetNavBoundsEntity() const;

		// Covert a prefab asset into an entity within the Scene.
		[[nodiscard]] SharedPtr<Entity> CreatePrefab( Ref<Prefab> prefabAsset, CreateEntityParameters& rEntityParameters );

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

		void RemoveRigidBody( PhysicsRigidBody* pBody );

		// Start NEW audio players
		void StartAudioPlayers();

		// Pause audio players
		void StopAudioPlayers();

		// Stop and unload audio players
		void DestroyAudioPlayers();
		
		void UpdateAudioListeners();

		void StartAnimations();
		void StartBehaviourTrees();

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

		void OnModifyPrefab_AddNewlyAddedComponents( const Ref<Prefab> prefabAsset, SharedPtr<Entity> entity, const SharedPtr<Entity> entityInPrefab );
		std::vector<entt::id_type> BuildComponentHash( SharedPtr<Entity> entity );

	protected:
		void OnEntityCreated( SharedPtr<Entity> entity );

	public:

		//////////////////////////////////////////////////////////////////////////
		// TODO: Rework this is as locking a mutex every frame can be bad for performance.

		template<typename Ty, typename... Args>
		Ty& AddComponent(entt::entity entity, Args&&... args )
		{
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
			return m_Registry.any_of<Ty>( entity );
		}

		template<typename... Ty>
		[[nodiscard]] bool HasComponents( entt::entity entity ) const
		{
			return m_Registry.any_of<Ty...>( entity );
		}

		template<typename Ty>
		void RemoveComponent( entt::entity entity )
		{
			if( HasComponent<Ty>( entity ) )
			{
				m_Registry.remove<Ty>( entity );
			}
		}

		template<typename... Ty>
		void RemoveComponents( entt::entity entity )
		{
			if( HasComponents<Ty...>( entity ) )
			{
				m_Registry.remove<Ty...>( entity );
			}
		}

		template<typename Ty>
		[[nodiscard]] Ty& GetComponent( entt::entity entity )
		{
			SAT_CORE_ASSERT( HasComponent<Ty>( entity ), "Entity does not have component!" );

			return m_Registry.get<Ty>( entity );
		}

		template<typename Ty>
		[[nodiscard]] const Ty& GetComponent( entt::entity entity ) const
		{
			SAT_CORE_ASSERT( HasComponent<Ty>( entity ), "Entity does not have component!" );

			return m_Registry.get<Ty>( entity );
		}

		template<typename Ty>
		[[nodiscard]] Ty* TryGetComponent( entt::entity entity )
		{
			return m_Registry.try_get<Ty>( entity );
		}

		template<typename Ty>
		[[nodiscard]] const Ty* TryGetComponent( entt::entity entity ) const
		{
			return m_Registry.try_get<Ty>( entity );
		}

	public:
		void PrepareForNavMeshBuilding();

		void OnNavMeshBuildCompleted();
		dtNavMeshQuery* GetNavMeshQuery() { return nullptr; }
		std::shared_ptr<PhysicsScene> GetPhysicsScene() const { return m_PhysicsScene; }
		NavigationSystem& GetNavigationSystem() { return m_NavigationSystem; }

#if !defined(SAT_DIST)
		SceneVisualisationOptions& GetVisualisationOptions() { return m_VisualisationOptions; }
		const SceneVisualisationOptions& GetVisualisationOptions() const { return m_VisualisationOptions; }
#endif

		void DestroyPhysicsScene();
	private:
		void CreatePhysicsScene();

		void OnNavMeshBuildCompAdded( entt::registry& reg, entt::entity entity );
		void OnNavMeshBuildCompRemoved( entt::registry& reg, entt::entity entity );

	private:
		void RtSetupLights( Ref<SceneRenderer> sceneRenderer );
		void RtBuildRenderer2DCommands( Ref<SceneRenderer> sceneRenderer );
		void RtBuildSceneRendererCommands( Ref<SceneRenderer> sceneRenderer );
		void RtBuildSelectedMeshesCmds( Ref<SceneRenderer> sceneRenderer );
		void RtDrawAIDebug( Ref<SceneRenderer> sceneRenderer );

	private:
		std::map<entt::entity, SharedPtr<Entity>> m_EntityIDMap;
		entt::registry m_Registry;
		entt::entity m_SceneEntity{ entt::null };
		
		std::vector<Ref<PlayerInputController>> m_Controllers;

		// For use by Runtime Scene only
		// Holds a list of the entities that will be destroyed in the next frame
		std::vector<Entity*> m_EntitiesToDestroy;

		Lights m_Lights;

		// #ReplaceRawPtrOrRefWithWeakRef
		WeakRef<Entity> m_pMainCameraEntity;
		
		SharedPtr<NavBoundsEntity> m_NavBoundsEntity = nullptr;

		RendererCamera m_RendererCamera;

		std::shared_ptr<PhysicsScene> m_PhysicsScene;

		NavigationSystem m_NavigationSystem;

		UUID m_InternalID;

		RuntimeState m_RuntimeState = RuntimeState::NoState;

#if !defined(SAT_DIST)
		bool m_Dirty = false;
		SceneVisualisationOptions m_VisualisationOptions{};
#endif

	private:
		friend class PhysicsSceneExporter;
		friend class Entity;
		friend class Prefab;
		friend class SceneHierarchyPanel;
		friend class SceneSerialiser;
		friend class SceneRenderer;
	};

}

#include "EntityECS.h"
