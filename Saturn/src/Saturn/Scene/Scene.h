/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2024 BEAST                                                           *
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

#include "Saturn/GameFramework/Core/GameScript.h"

#include "Saturn/Core/Renderer/EditorCamera.h"

#include "Saturn/Core/UUID.h"
#include "Saturn/Core/Timestep.h"

#include "entt.hpp"

#if defined( SAT_ENABLE_GAMETHREAD )
#include <shared_mutex>
#endif

namespace Saturn {

	class Entity;
	class Prefab;
	class PhysicsScene;
	class SClass;
	class SceneRenderer;

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

		[[nodiscard]] uint32_t GetPointLightSize() { return static_cast<uint32_t>( sizeof( PointLight ) * PointLights.size() ); };

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

	class Scene : public Asset
	{
		SAT_DECLARE_CLASS_NO_INTER( Scene, Asset )
	public:
		Scene();
		~Scene();

		[[nodiscard]] Ref<Entity> CreateEntityWithIDScript( UUID uuid, const std::string& name = "", const std::string& rScriptName = "" );
	public:
		void OnRenderEditor( const EditorCamera& rCamera, Timestep ts, SceneRenderer& rSceneRenderer );
		void OnRenderRuntime( Timestep ts, SceneRenderer& rSceneRenderer );

		Ref<Entity> DuplicateEntity( Ref<Entity> entity, Ref<Entity> parent = nullptr );
		void DeleteEntity( Ref<Entity> entity, bool deleteChildren = true, UUID orphanParentID = 0 );
		
		void OnUpdate( Timestep ts );
		void OnUpdatePhysics( Timestep ts );
		void OnEvent( RubyEvent& rEvent );

	public:
		template<typename T>
		std::vector<Ref<Entity>> GetAllEntitiesWith( void )
		{
			std::vector<Ref<Entity>> result;

			for( const auto& [ id, entity ] : m_EntityIDMap )
			{
				if( entity->HasComponent<T>() )
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

		[[nodiscard]] Ref<Entity> GetMainCameraEntity( bool force = false );

		void AddSelectedEntity( Ref<Entity> entity );
		void DeselectEntity( Ref<Entity> entity );
		void ClearSelectedEntities();
		
		[[nodiscard]] Ref<Entity> FindEntityByTag( const std::string& tag );
		[[nodiscard]] Ref<Entity> FindEntityByID( const UUID& id );

		glm::mat4 GetTransformRelativeToParent( Ref<Entity> entity );
		TransformComponent GetWorldSpaceTransform( Ref<Entity> entity );

		[[nodiscard]] bool Raycast( const glm::vec3& Origin, const glm::vec3& Direction, float MaxDistance, RaycastHitResult* pOut );

	public:
		void CopyScene( Ref<Scene>& NewScene );
		void Empty();

		bool RuntimeRunning = false;

		void OnRuntimeStart();
		void OnRuntimeEnd();

		entt::registry& GetRegistry() { return m_Registry; }
		const entt::registry& GetRegistry() const { return m_Registry; }

		// This transfers a prefab to an entity.
		// The prefabs holds an entity however that entity is local to it's scene and we want that entity to be our scene.
		Ref<Entity> CreatePrefab( Ref<Prefab> prefabAsset );

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

		void StartAudioPlayers();
		void StopAudioPlayers();
		void DestroyAudioPlayers();
		void UpdateAudioListeners();

#if defined(SAT_DEBUG) || defined(SAT_RELEASE)
		void MarkDirty() { m_Dirty = true; }
		void CleanDirty() { m_Dirty = false; }
		bool IsDirty() const { return m_Dirty; }
#else
		void MarkDirty() {}
		void CleanDirty() {}
		bool IsDirty() const { return false; }
#endif

		void AcknowledgeHotReload();

		void AddController( const Ref<PlayerInputController>& rPlayerInputController );
		void RemoveController( const Ref<PlayerInputController>& rPlayerInputController );
		static void   SetActiveScene( Scene* pScene );
		static Scene* GetActiveScene();

	public:
		//////////////////////////////////////////////////////////////////////////
		// #WARNING This should not be confused with AssetSerialisers. This is for raw binary serialisation!

		void SerialiseData();
		void DeserialiseData();

	private:
		void SerialiseInternal( std::ofstream& rStream );

		template<typename IStream>
		void DeserialiseInternal( IStream& rStream );

		Ref<Entity> HotReloadReplaceOldEntity( Ref<Entity> source );

		void TransferModifiedProperties( const Ref<Entity>& rSourceEntity, Ref<Entity>& rEntity, const std::string& rMetadataName );

	protected:
		void OnEntityCreated( Ref<Entity> entity );

	private:

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
		[[nodiscard]] bool HasComponent( entt::entity entity )
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

	private:
		std::unordered_map<entt::entity, Ref<Entity>> m_EntityIDMap;
		entt::registry m_Registry;

		entt::entity m_SceneEntity{ entt::null };
		
#if !defined(SAT_DIST)
		std::vector< Ref<Entity> > m_SelectedEntities;
#endif

		std::vector<Ref<PlayerInputController>> m_Controllers{};

		Lights m_Lights;
		Ref<Entity> m_MainCameraEntity = nullptr;

#if defined( SAT_ENABLE_GAMETHREAD )
		std::mutex m_Mutex;
#endif

		RendererCamera m_RendererCamera;

		// TODO: Change raw pointer to Ref?
		PhysicsScene* m_PhysicsScene = nullptr;

		UUID m_InternalID;
		bool m_Dirty = false;

	private:

		friend class Entity;
		friend class Prefab;
		friend class SceneHierarchyPanel;
		friend class SceneSerialiser;
		friend class SceneRenderer;
	};
}