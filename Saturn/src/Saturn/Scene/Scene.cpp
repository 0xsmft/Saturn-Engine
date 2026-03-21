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
#include "Scene.h"

#include "Entity.h"
#include "Components.h"

// TOOD: #FixSceneRendererIncludes
#include "Saturn/Vulkan/AluraRenderer.h"
#include "Saturn/Vulkan/Renderer2D.h"
#include "Saturn/Vulkan/SceneRenderer.h"
#include "Saturn/Vulkan/VulkanContext.h"

#include "Saturn/Asset/Prefab.h"
#include "Saturn/Asset/AssetManager.h"

#include "Saturn/Core/Profiler.h"
#include "Saturn/Core/VirtualFS.h"
#include "Saturn/Core/MemoryStream.h"
#include "Saturn/Core/Renderer/SceneFlyCamera.h"
#include "Saturn/Core/VariableGuard.h"

#include "Saturn/Physics/PhysicsScene.h"
#include "Saturn/Physics/PhysicsRigidBody.h"
#include "Saturn/Physics/PhysicsCharacterController.h"

#include "Saturn/Project/Project.h"

#include "Saturn/GameFramework/Core/GameModule.h"
#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"
#include "Saturn/GameFramework/PlayerInputController.h"

#include "Saturn/Serialisation/YAML/SceneSerialiser.h"

#include "Saturn/Audio/AudioSystem.h"

#include "Saturn/Animation/BoneJoint.h"

#include "Saturn/Alura/AluraCanvas.h"

#if !defined(SAT_DIST)
#include "Saturn/ImGui/EditorIcons.h"
#include "Saturn/ImGui/EntitySelectionManager.h"
#include "Saturn/ImGui/ImGuiWindow.h"
#include "Saturn/ImGui/EditorEvents.h"
#include "Saturn/ImGui/ImGuiWindowManager.h"

#include "Saturn/Audio/SoundNodeEditor/GraphSoundAssetViewer.h"

#include "Saturn/Physics/PhysicsDebugMeshes.h"
#endif

#include "Saturn/AI/Navigation/NavBoundsEntity.h"
#include "Saturn/AI/AIAgentEntity.h"

#include "Saturn/Runtime/RuntimeEvents.h"

#include <Detour/DetourNavMeshQuery.h>

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Saturn {

	static std::tuple<glm::vec3, glm::quat, glm::vec3> GetTransformDecomposition( const glm::mat4& transform )
	{
		glm::vec3 scale, translation, skew;
		glm::vec4 perspective;
		glm::quat orientation;
		glm::decompose( transform, scale, orientation, translation, skew, perspective );

		return { translation, orientation, scale };
	}

	Scene::Scene()
	{
		m_SceneEntity = m_Registry.create();
		m_Registry.emplace<SceneComponent>( m_SceneEntity, m_InternalID );

		/*
		m_Registry.on_construct<NavigationMeshSpecificationComponent>().connect<&Scene::OnNavMeshBuildCompAdded> ( *this );
		m_Registry.on_destroy<NavigationMeshSpecificationComponent>().connect<&Scene::OnNavMeshBuildCompRemoved>( *this );
		*/
	}

	void Scene::OnNavMeshBuildCompAdded( entt::registry& reg, entt::entity entity )
	{
	}

	void Scene::OnNavMeshBuildCompRemoved( entt::registry& reg, entt::entity entity )
	{
	}

	Scene::~Scene()
	{
		Empty();
	}

	void Scene::Empty()
	{
#if !defined(SAT_DIST)
		if( auto* rESM = EntitySelectionManager::Get(); rESM )
			rESM->ClearSelection( this, true );
#endif
		{

			// TODO: Is really needed? As the physics scene will destroy all of this.
			auto rigidBodies = GetAllEntitiesWith<RigidbodyComponent>();
			for( auto& entity : rigidBodies )
			{
				if( entity->GetComponent<RigidbodyComponent>().Rigidbody ) 
				{
					delete entity->GetComponent<RigidbodyComponent>().Rigidbody;
					entity->GetComponent<RigidbodyComponent>().Rigidbody = nullptr;
				}
			}

			StopAudioPlayers();

			// Only true if we are the editor scene or if OnRuntimeEnd was not called.
			DestroyPhysicsScene();
		}

		m_Controllers.clear();
		m_NavBoundsEntity = nullptr;

		// Release weak ref to main camera entity.
		m_pMainCameraEntity.Reset();

		m_EntityIDMap.clear();
		m_Registry.clear();

		m_NavigationSystem.Terminate();
	}

	WeakRef<Entity> Scene::GetMainCameraEntity( bool force )
	{
		if( !m_pMainCameraEntity.Expired() && !force )
			return m_pMainCameraEntity;

		const auto entities = GetAllEntitiesWith<CameraComponent>();
		for( auto& entity : entities )
		{
			if( entity->GetComponent<CameraComponent>().MainCamera )
			{
				m_pMainCameraEntity = entity;
				return m_pMainCameraEntity;
			}
		}

		// If we get here, that means that there is no camera, reset weak ref
		m_pMainCameraEntity.Reset();
		return m_pMainCameraEntity;
	}

	void Scene::OnUpdate( Timestep ts )
	{
		SAT_PF_EVENT();

		// Update Cycle.
		// Step 1: Update and simulate the physics scene.
		// Step 2: Update all entities.

		// TODO: We might want to change the order of this update cycle.
		if( IsRuntimeRunning() ) 
		{
			// Simulate the physics scene.
			m_PhysicsScene->Simulate( ts );
			OnUpdatePhysics( ts );

			OnUpdateEntities( ts );
			
			OnUpdateAnimators( ts );

			UpdateAudioListeners();

			DestroyPendingEntities();
		}
	}
	
	void Scene::OnUpdateEntities( Timestep ts )
	{
		for( auto&& [id, entity] : m_EntityIDMap )
		{
			entity->OnUpdate( ts );
		}
	}

	void Scene::OnUpdatePhysics( Timestep ts )
	{
		SAT_PF_EVENT();

		constexpr float FixedTimestep = 1.0f / 100.0f;
		for( auto&& [id, entity] : m_EntityIDMap )
		{
			entity->OnPhysicsUpdate( FixedTimestep );
		}

		auto rigidBodies = GetAllEntitiesWith<RigidbodyComponent>();
		for( auto& rEntity : rigidBodies )
		{
			auto& rb = rEntity->GetComponent<RigidbodyComponent>();
		
			// rb.Rigidbody will be null if it's just been spawned into the world.
			if( !rb.Rigidbody )
			{
				m_PhysicsScene->InitialiseNewBody( rEntity, rb );
			}
			
			rb.Rigidbody->SyncTransfrom();
		}

		auto charControllers = GetAllEntitiesWith<CharacterMovementComponent>();
		for( auto& rEntity : charControllers )
		{
			auto* pController = rEntity->GetComponent<CharacterMovementComponent>().CharacterMovement;
			if( pController )
			{
				// SyncTransform
				rEntity->GetComponent<TransformComponent>().Position = pController->GetPosition();

				/*
				auto& rb = rEntity->GetComponent<RigidbodyComponent>();

				// rb.Rigidbody will be null if it's just been spawned into the world.
				if( rb.Rigidbody )
				{
					rb.Rigidbody->SetPosition( pController->GetPosition() );
				}
				*/
			}
			else
			{
				// pController will be null if it's just been spawned into the world.
				m_PhysicsScene->AddNewController( rEntity );
			}
		}
	}

	void Scene::OnUpdateAnimators( Timestep ts )
	{
		const auto dynamicMeshEntities = GetAllEntitiesWith<SkeletalMeshComponent>();
		for( const auto& entity : dynamicMeshEntities )
		{
			auto& meshComponent = entity->GetComponent<SkeletalMeshComponent>();
			if( meshComponent.Mesh && meshComponent.LocalAnimator )
			{
				meshComponent.LocalAnimator->TickAnimation( ts );
			}
		}
	}

	void Scene::OnEvent( Event& rEvent )
	{
		// Other states do not need to be handled because anything other than Running or Suspended should get through here.
		// TODO: Handle this better.
		if( m_RuntimeState == RuntimeState::Suspended )
			return;

		switch( rEvent.Type )
		{
			default: break;
			case EventType::KeyPressed:
			case EventType::KeyReleased:
			{
				RubyKeyEvent keyEvent = ( RubyKeyEvent& ) rEvent;

				for( auto& rController : m_Controllers )
				{
					rController->UpdateKeyState( keyEvent );
				}
			} break;

			case EventType::MousePressed:
			case EventType::MouseReleased:
			{
				RubyMouseEvent mouseEvent = ( RubyMouseEvent& ) rEvent;

				for( auto& rController : m_Controllers )
				{
					rController->UpdateMouseState( mouseEvent );
				}
			} break;
		}
	}

	void Scene::OnRenderEditor( Camera* pCamera, const glm::mat4& rViewMartix, Ref<SceneRenderer> sceneRenderer, Timestep ts )
	{
		SAT_PF_EVENT();

		m_RendererCamera.pCamera = pCamera;
		m_RendererCamera.ViewMatrix = rViewMartix;

		sceneRenderer->SetCamera( m_RendererCamera );
		sceneRenderer->GetRenderer2D()->PreRender();
		if( sceneRenderer->GetAluraRenderer() )
			sceneRenderer->GetAluraRenderer()->PreRender();

		//////////////////////////////////////////////////////////////////////////

		// Lights
		RtSetupLights( sceneRenderer );

		// Renderer2D 
		RtBuildRenderer2DCommands( sceneRenderer );

		// Scene Renderer (main geometry)
		RtBuildSceneRendererCommands( sceneRenderer );
	}

	void Scene::OnRenderRuntime( Timestep ts, Ref<SceneRenderer> sceneRenderer )
	{
		SAT_PF_EVENT();

		// Try find new main camera entity if current saved one is null
		if( m_pMainCameraEntity.Expired() )
		{
			m_pMainCameraEntity = GetMainCameraEntity();
		}

		// Check twice because we are always going to have to set the projection
		if( auto entity = m_pMainCameraEntity.Access() )
		{
			const auto tc = GetWorldSpaceTransform( entity );

			const auto view = glm::inverse( tc.GetTransform() );

			auto& rCamera = entity->GetComponent<CameraComponent>().Camera;
			rCamera->SetViewportSize( sceneRenderer->Width(), sceneRenderer->Height() );
			rCamera->SetPosition( tc.Position );

			rCamera->OnUpdate( ts );

			m_RendererCamera.pCamera = ( Camera* ) rCamera.Get();
			m_RendererCamera.ViewMatrix = rCamera->ViewMatrix();
		}
		else
		{
			// TODO: Try to find a new camera or create one or even end runtime
			m_RuntimeState = RuntimeState::Ending;
		}

		sceneRenderer->SetCamera( m_RendererCamera );

		//////////////////////////////////////////////////////////////////////////

		g_AluraCanvas->Begin();

		// Lights
		RtSetupLights( sceneRenderer );

		// Scene Renderer
		RtBuildSceneRendererCommands( sceneRenderer );
	}

	void Scene::RtSetupLights( Ref<SceneRenderer> sceneRenderer )
	{
		m_Lights = Lights();

		// Directional Lights
		{
			const auto lights = m_Registry.group<DirectionalLightComponent>( entt::get<TransformComponent> );
			uint32_t lightCount = 0;
			for( const auto& e : lights )
			{
				const auto [transformComponent, lightComponent] = lights.get<TransformComponent, DirectionalLightComponent>( e );

				const glm::vec3 direction = -glm::normalize( glm::mat3( transformComponent.GetTransform() ) * glm::vec3( 1.0f ) );

				m_Lights.DirectionalLights[ lightCount++ ] = { direction, lightComponent.Radiance, lightComponent.Intensity };
			}
		}

		// Point lights
		{
			const auto points = m_Registry.group<PointLightComponent>( entt::get<TransformComponent> );
			if( points.size() )
			{
#if !defined(SAT_DIST)
				const Ref<Texture2D> pointLightBillboardTex = EditorIcons::GetIcon( "Billboard_PointLight" );
#endif

				m_Lights.PointLights.reserve( points.size() );

				for( const auto& e : points )
				{
					const auto [transformComponent, lightComponent] = points.get<TransformComponent, PointLightComponent>( e );

					/*
					PointLight pl = {
						.Position = transformComponent.Position,
						.Radiance = lightComponent.Radiance,
						.Multiplier = lightComponent.Multiplier,
						.LightSize = lightComponent.LightSize,
						.Radius = lightComponent.Radius,
						.MinRadius = lightComponent.MinRadius,
						.Falloff = lightComponent.Falloff };
					*/

					m_Lights.PointLights.emplace_back( 
						transformComponent.Position, 
						lightComponent.Radiance,
						lightComponent.Multiplier,
						lightComponent.LightSize,
						lightComponent.Radius,
						lightComponent.MinRadius,
						lightComponent.Falloff );

#if !defined(SAT_DIST)
					sceneRenderer->GetRenderer2D()->SubmitBillboardTextured( transformComponent.Position, glm::vec4( 1.0f ), pointLightBillboardTex, glm::vec2( 1.5f ) );
#endif
				}
			}
		}
	}

	void Scene::RtBuildRenderer2DCommands( Ref<SceneRenderer> sceneRenderer )
	{
#if !defined(SAT_DIST)
		// Audio Billboards
		const auto players = m_Registry.group<AudioPlayerComponent>( entt::get<TransformComponent> );
		if( players.size() )
		{
			const Ref<Texture2D> audio = EditorIcons::GetIcon( "Billboard_Audio" );
			const Ref<Texture2D> audioMuted = EditorIcons::GetIcon( "Billboard_AudioMuted" );
			const Ref<Texture2D> audioLooped = EditorIcons::GetIcon( "Billboard_AudioLooping" );

			for( const auto& e : players )
			{
				const auto [transformComponent, playerComponent] = players.get<TransformComponent, AudioPlayerComponent>( e );

				Ref<Texture2D> submissionTexture = audio;

				if( playerComponent.Loop )
					submissionTexture = audioLooped;
				else if( playerComponent.Mute )
					submissionTexture = audioMuted;

				sceneRenderer->GetRenderer2D()->SubmitBillboardTextured(
					transformComponent.Position,
					glm::vec4( 1.0f ),
					submissionTexture, glm::vec2( 1.0f ) );
			}
		}

		// Direction for Audio listeners
		const auto listeners = m_Registry.group<AudioListenerComponent>( entt::get<TransformComponent> );
		if( listeners.size() )
		{
			const Ref<Texture2D> listenTexture = EditorIcons::GetIcon( "Billboard_AudioListen" );

			for( const auto& e : listeners )
			{
				const auto [transformComponent, comp] = listeners.get<TransformComponent, AudioListenerComponent>( e );

				const auto pos = glm::vec3( transformComponent.Position.x, transformComponent.Position.y + 2.5f, transformComponent.Position.z );

				sceneRenderer->GetRenderer2D()->SubmitBillboardTextured(
					pos,
					glm::vec4( 1.0f ),
					listenTexture, glm::vec2( 1.0f ) );

				// Use billboard pos as starting pos
				const auto start = pos;
				const auto end = start + glm::normalize( comp.Direction ) * 2.0f;

				sceneRenderer->GetRenderer2D()->SubmitLine( start, end, glm::vec4( 1.0f ) );
			}
		}

		// Agent billboards
		const auto aiAgents = GetAllEntitiesWithClass<AIAgentEntity>();
		if( aiAgents.size() )
		{
			const auto aiAgentTexture = EditorIcons::GetIcon( "Billboard_AIAgent" );

			for( const auto& rEntity : aiAgents )
			{
				const TransformComponent& rTc = rEntity->GetComponent<TransformComponent>();
				const glm::vec3 position( rTc.Position.x, rTc.Position.y + 2.5f, rTc.Position.z );

				sceneRenderer->GetRenderer2D()->SubmitBillboardTextured(
					position,
					glm::vec4( 1.0f ),
					aiAgentTexture, glm::vec2( 1.0f ) );
			}
		}

		const auto billboards = GetAllEntitiesWith<BillboardComponent>();
		for( const auto& rEntity : billboards )
		{
			const BillboardComponent& bc = rEntity->GetComponent<BillboardComponent>();
			const TransformComponent& rTc = rEntity->GetComponent<TransformComponent>();

			// TODO: We might want to load this texture on the JobSystem.
			Ref<TextureSourceAsset> textureAsset = AssetManager::Get()->GetAssetAs<TextureSourceAsset>( bc.AssetID );
			Ref<Texture2D> textureToSubmit = EditorIcons::GetIcon( "Billboard_Circle" );
			bool flip = false;

			if( textureAsset ) 
			{
				textureToSubmit = textureAsset->GetTexture();
				flip = textureAsset->IsFlagSet( TextureLoadFlags_FlipVertically );
			}
			
			if( flip )
			{
				sceneRenderer->GetRenderer2D()->SubmitBillboardTexturedFlipped(
					rTc.Position,
					glm::vec4( 1.0f ),
					textureToSubmit,
					glm::vec2( 1.0f )
				);
			}
			else
			{
				sceneRenderer->GetRenderer2D()->SubmitBillboardTextured(
					rTc.Position,
					glm::vec4( 1.0f ),
					textureToSubmit,
					glm::vec2( 1.0f )
				);
			}
		}
#endif

		if( m_RuntimeState == RuntimeState::Suspended )
		{
			m_NavigationSystem.DebugDraw( sceneRenderer->GetRenderer2D().Get() );
		}
	}

	void Scene::RtBuildSceneRendererCommands( Ref<SceneRenderer> sceneRenderer )
	{
		const auto staticMeshEntities = GetAllEntitiesWith<StaticMeshComponent>();
		for( const auto& entity : staticMeshEntities )
		{
			if( !entity->IsVisible() )
				continue;

			const auto& meshComponent = entity->GetComponent<StaticMeshComponent>();
			const auto transform = GetTransformRelativeToParent( entity );

			if( meshComponent.Mesh )
			{
				Ref<MaterialRegistry> targetMaterialRegistry = meshComponent.Mesh->GetMaterialRegistry();

				if( meshComponent.MaterialRegistry && meshComponent.MaterialRegistry->HasAnyOverrides() )
					targetMaterialRegistry = meshComponent.MaterialRegistry;

				sceneRenderer->SubmitStaticMesh( entity, meshComponent.Mesh, targetMaterialRegistry, transform );
			}
		}

		const auto dynamicMeshEntities = GetAllEntitiesWith<SkeletalMeshComponent>();
		for( const auto& entity : dynamicMeshEntities )
		{
			if( !entity->IsVisible() )
				continue;

			auto& meshComponent = entity->GetComponent<SkeletalMeshComponent>();
			const auto transform = GetTransformRelativeToParent( entity );

			if( meshComponent.Mesh )
			{
				Ref<MaterialRegistry> targetMaterialRegistry = meshComponent.Mesh->GetMaterialRegistry();

				if( meshComponent.MaterialRegistry && meshComponent.MaterialRegistry->HasAnyOverrides() )
					targetMaterialRegistry = meshComponent.MaterialRegistry;

				std::vector<glm::mat4> boneTransforms = meshComponent.Mesh->GetDefaultBoneTransforms();
				if( meshComponent.LocalAnimator && meshComponent.LocalAnimator->IsPlaying() )
				{
					boneTransforms = meshComponent.LocalAnimator->GetBoneTransforms();
				}

				sceneRenderer->SubmitDynamicMesh( entity, meshComponent.Mesh, targetMaterialRegistry, transform, boneTransforms );
			}
		}

#if !defined(SAT_DIST)
		RtRenderColliderDebug( sceneRenderer );
#endif
	}

#if !defined(SAT_DIST)
	void Scene::RtRenderColliderDebug( Ref<SceneRenderer> sceneRenderer )
	{
		auto submitBoxCollider = [ this, &sceneRenderer ]( SharedPtr<Entity> entity, Ref<StaticMesh> dbgMesh, Ref<MaterialRegistry> materialRegistry )
		{
			const auto& rTransform = GetWorldSpaceTransform( entity );
			glm::mat4 finalTransform = glm::one<glm::mat4>();
			const auto& rComponent = entity->GetComponent<BoxColliderComponent>();

			finalTransform = glm::translate( glm::mat4( 1.0f ), rTransform.Position + rComponent.Offset ) 
				* glm::toMat4( rTransform.GetRotation() ) 
				* glm::scale( glm::mat4( 1.0f ), rComponent.HalfExtents * 2.0f );

			sceneRenderer->SubmitPhysicsCollider( entity, dbgMesh, materialRegistry, finalTransform );
		};

		auto submitSphereCollider = [ this, &sceneRenderer ]( SharedPtr<Entity> entity, Ref<StaticMesh> dbgMesh, Ref<MaterialRegistry> materialRegistry )
		{
			glm::mat4 transform = GetTransformRelativeToParent( entity );
			const auto& rComponent = entity->GetComponent<SphereColliderComponent>();

			auto colliderTransform = glm::translate( glm::mat4( 1.0f ), rComponent.Offset ) * glm::scale( glm::mat4( 1.0f ), glm::vec3( rComponent.Radius * 2.0f ) );
			sceneRenderer->SubmitPhysicsCollider( entity, dbgMesh, materialRegistry, transform * colliderTransform );
		};

		auto submitCapsuleCollider = [ this, &sceneRenderer ]( SharedPtr<Entity> entity, Ref<StaticMesh> dbgMesh, Ref<MaterialRegistry> materialRegistry )
		{
			glm::mat4 transform = GetTransformRelativeToParent( entity );
			const auto& rComponent = entity->GetComponent<CapsuleColliderComponent>();

			auto colliderTransform = glm::translate( glm::mat4( 1.0f ), rComponent.Offset ) * glm::scale( glm::mat4( 1.0f ), glm::vec3( rComponent.Radius * 2.0f, rComponent.HalfHeight * 2.0f, rComponent.Radius * 2.0f ) );
			sceneRenderer->SubmitPhysicsCollider( entity, dbgMesh, materialRegistry, transform * colliderTransform );
		};

		switch( m_VisualisationOptions.PhysColliderOptions )
		{
			case PhysicsColliderVisualisationOptions::Disabled:
			default:
				break;

			case PhysicsColliderVisualisationOptions::SelectedOnly:
			{
				for( const auto& rEntity : EntitySelectionManager::Get()->GetSelectionContexts( this ) )
				{
					if( auto* pBoxColliderComponent = rEntity->TryGetComponent<BoxColliderComponent>(); pBoxColliderComponent )
					{
						auto mesh = PhysicsDebugMeshes::Get().GetBoxMesh();

						const auto& rComponent = rEntity->GetComponent<BoxColliderComponent>();
						submitBoxCollider( rEntity, mesh, mesh->GetMaterialRegistry() );
					}
					else if( auto* pSphereColliderComponent = rEntity->TryGetComponent<SphereColliderComponent>(); pSphereColliderComponent )
					{
						auto mesh = PhysicsDebugMeshes::Get().GetSphereMesh();

						const auto& rComponent = rEntity->GetComponent<SphereColliderComponent>();
						submitSphereCollider( rEntity, mesh, mesh->GetMaterialRegistry() );
					}
					else if( auto* pCapsuleColliderComponent = rEntity->TryGetComponent<CapsuleColliderComponent>(); pCapsuleColliderComponent )
					{
						auto mesh = PhysicsDebugMeshes::Get().GetCapsuleMesh();

						const auto& rComponent = rEntity->GetComponent<CapsuleColliderComponent>();
						submitCapsuleCollider( rEntity, mesh, mesh->GetMaterialRegistry() );
					} 
				}
			} break;

			case PhysicsColliderVisualisationOptions::All: 
			{
				// Box
				auto boxEntities = GetAllEntitiesWith<BoxColliderComponent>();
				for( const auto& rEntity : boxEntities )
				{
					auto mesh = PhysicsDebugMeshes::Get().GetBoxMesh();

					const auto& rComponent = rEntity->GetComponent<BoxColliderComponent>();
					submitBoxCollider( rEntity, mesh, mesh->GetMaterialRegistry() );
				}

				// Sphere
				auto sphereEntities = GetAllEntitiesWith<SphereColliderComponent>();
				for( const auto& rEntity : sphereEntities )
				{
					auto mesh = PhysicsDebugMeshes::Get().GetSphereMesh();

					const auto& rComponent = rEntity->GetComponent<SphereColliderComponent>();
					submitSphereCollider( rEntity, mesh, mesh->GetMaterialRegistry() );
				}

				// Capsule
				auto capsuleEntities = GetAllEntitiesWith<CapsuleColliderComponent>();
				for( const auto& rEntity : capsuleEntities )
				{
					auto mesh = PhysicsDebugMeshes::Get().GetCapsuleMesh();

					const auto& rComponent = rEntity->GetComponent<CapsuleColliderComponent>();
					submitCapsuleCollider( rEntity, mesh, mesh->GetMaterialRegistry() );
				}
			} break;
		}
	}
#endif

	SharedPtr<Entity> Scene::CreateEntityWithIDScript( UUID uuid, const std::string& name /*= "" */, const std::string& rScriptName, bool externalData )
	{
		Scene* ActiveScene = g_ActiveScene;
		if( g_ActiveScene != this )
			g_ActiveScene = this;

		// UNSAFE! We just assume that rScriptName will be a subclass of an entity, could lead to UB
		SharedPtr<Entity> entity = (Entity*)ClassMetadataHandler::Get().CreateClassObject( rScriptName, this );

		entity->SetName( name );
		entity->GetComponent<IdComponent>().ID = uuid;

		OnEntityCreated( entity );

		g_ActiveScene = ActiveScene;

		return entity;
	}

	SharedPtr<Entity> Scene::CreateEntity( const std::string& name /*= "" */ )
	{
		SharedPtr<Entity> entity = NewObject<Entity>( this );
		entity->SetName( name );

		OnEntityCreated( entity );

		return entity;
	}

	SharedPtr<Entity> Scene::CreateEntity( CreateEntityParameters& rParams )
	{
		if( !rParams.pClass->IsChildOf( Entity::StaticClass() ) || rParams.pClass == nullptr ) 
			return nullptr;

		SharedPtr<Entity> entity = dynamic_cast<Entity*>( ClassMetadataHandler::Get().CreateClassObject( rParams.pClass ) );
		entity->SetName( rParams.Tag );
		entity->GetComponent<IdComponent>().ID = rParams.ID;

		if( rParams.Parent )
		{
			entity->SetParent( rParams.Parent->GetUUID() );
			rParams.Parent->AddChild( entity->GetUUID() );
		}

		auto& tc = entity->GetComponent<TransformComponent>();
		tc.Position = rParams.Position;
		tc.SetRotation( rParams.Rotation );
		tc.Scale = rParams.Scale;

		OnEntityCreated( entity );

		return entity;
	}

	SharedPtr<Entity> Scene::FindEntityByTag( const std::string& tag )
	{
		for( auto&& [handle, entity] : m_EntityIDMap )
		{
			if( entity->GetComponent<TagComponent>().Tag == tag )
				return entity;
		}

		return nullptr;
	}

	SharedPtr<Entity> Scene::FindEntityByID( const UUID& id )
	{
		for( auto&& [handle, entity] : m_EntityIDMap )
		{
			if( entity->GetUUID() == id )
				return entity;
		}

		return nullptr;
	}

	SharedPtr<Entity> Scene::FindEntityByHandle( entt::entity handle )
	{
		const auto Itr = m_EntityIDMap.find( handle );
		if( Itr != m_EntityIDMap.end() )
			return Itr->second;

		return nullptr;
	}

	glm::mat4 Scene::GetTransformRelativeToParent( const SharedPtr<Entity> entity )
	{
		SAT_PF_EVENT();

		glm::mat4 transform( 1.0f );

		// TODO: Change RelationshipComponent's to use entt::entity handles as it will increase the proformance greatly.
		const UUID& rParentID = entity->GetParent();

		if( rParentID != 0 )
		{
			SharedPtr<Entity> parent = FindEntityByID( rParentID );
			if( parent )
			{
				transform = GetTransformRelativeToParent( parent );
				
				if( auto* pAttachmentComp = entity->TryGetComponent<AttachmentPointComponent>(); pAttachmentComp && pAttachmentComp->pBoneJoint ) 
				{
					if( const auto& anim = parent->GetComponent<SkeletalMeshComponent>().LocalAnimator ) 
					{
						transform *= pAttachmentComp->pBoneJoint->GetBoneMatrix( anim );
					}
				}
			}
		}

		return transform * entity->GetComponent<TransformComponent>().GetTransform(); /* <- entity local ts */
	}

	TransformComponent Scene::GetWorldSpaceTransform( const SharedPtr<Entity> entity )
	{
		SAT_PF_EVENT();

		TransformComponent tc;

		glm::mat4 worldSpace = GetTransformRelativeToParent( entity );
		glm::quat rotation{};

		Maths::DecomposeTransform( worldSpace, tc.Position, rotation, tc.Scale );

		tc.SetRotation( rotation );

		return tc;
	}

	bool Scene::Raycast( const glm::vec3& Origin, const glm::vec3& Direction, float MaxDistance, RaycastHitResult* pOut )
	{
		if( m_PhysicsScene )
			return m_PhysicsScene->Raycast( Origin, Direction, MaxDistance, pOut );

		return false;
	}

	template<typename ...V>
	static void CopyComponent( entt::registry& dstRegistry, entt::registry& srcRegistry, const std::unordered_map<UUID, entt::entity>& enttMap )
	{
		( [&]() 
		{
			auto components = srcRegistry.view<V>();
			for( auto srcEntity : components )
			{
				// Don't add to the scene entity.
				if( !srcRegistry.any_of<SceneComponent>( srcEntity ) )
				{
					entt::entity destEntity = enttMap.at( srcRegistry.get<IdComponent>( srcEntity ).ID );

					auto& srcComponent = srcRegistry.get<V>( srcEntity );
					auto& destComponent = dstRegistry.emplace_or_replace<V>( destEntity, srcComponent );
				}
			}
		}( ), ... );
	}

	template<typename ...V>
	static void CopyComponent( ComponentGroup<V...>, entt::registry& dstRegistry, entt::registry& srcRegistry, const std::unordered_map<UUID, entt::entity>& enttMap )
	{
		CopyComponent<V...>( dstRegistry, srcRegistry, enttMap );
	}
	
	template<typename... V>
	static void CopyComponentIfExists( entt::entity dst, entt::entity src, entt::registry& rRegistry )
	{
		([&]()
		{
			if( rRegistry.any_of<V>( src ) )
			{
				auto& srcComponent = rRegistry.get<V>( src );
				rRegistry.emplace_or_replace<V>( dst, srcComponent );
			}
		}(), ... );
	}
	
	template<typename... V>
	static void CopyComponentIfExists( ComponentGroup<V...>, entt::entity dst, entt::entity src, entt::registry& rRegistry )
	{
		CopyComponentIfExists<V...>( dst, src, rRegistry );
	}

	SharedPtr<Entity> Scene::DuplicateEntity( const SharedPtr<Entity> entity, const SharedPtr<Entity> parent )
	{
		SharedPtr<Entity> newEntity = dynamic_cast<Entity*>( ClassMetadataHandler::Get().CreateClassObject( (SClass*)entity->GetClass() ) );
		newEntity->SetName( entity->GetComponent<TagComponent>().Tag );

		OnEntityCreated( newEntity );

		CopyComponentIfExists( AllDuplicatableComponents{}, newEntity->GetHandle(), entity->GetHandle(), m_Registry );

		auto& relationshipComponent = newEntity->GetComponent<RelationshipComponent>();
		const auto& sourceRelationship = entity->GetComponent<RelationshipComponent>();
		
		relationshipComponent.ChildrenID.resize( entity->GetChildren().size() );
		
		// parent should only be a valid pointer if we are calling this recursively.
		if( parent )
		{
			newEntity->SetParent( parent->GetUUID() );
		}

		if( entity->HasParent() && !parent )
		{
			SharedPtr<Entity> parent = FindEntityByID( entity->GetParent() );
			SharedPtr<Entity> newParent = DuplicateEntity( parent, nullptr );

			newEntity->SetParent( newParent->GetUUID() );
		}

		for( const auto& rID : sourceRelationship.ChildrenID )
		{
			const SharedPtr<Entity> child = FindEntityByID( rID );
			SharedPtr<Entity> newChild = DuplicateEntity( child, newEntity );

			newEntity->GetChildren().push_back( newChild->GetUUID() );
		}

		return newEntity;
	}

	void Scene::DeleteEntity( SharedPtr<Entity> entity, bool deleteChildren /*=true*/, UUID orphanParentID /*=0*/ )
	{
		for( auto& rChild : entity->GetChildren() )
		{
			auto child = FindEntityByID( rChild );

			if( child )
			{
				if( deleteChildren )
				{
					DeleteEntity( child, true );
				}
				else
				{
					child->SetParent( orphanParentID );
				}
			}
		}

		entt::entity handle = entity->GetHandle();
		entity->Invalidate();

		m_EntityIDMap.erase( handle );
		m_Registry.destroy( handle );
	}

	void Scene::DestroyEntity( Entity* entity )
	{
		m_EntitiesToDestory.push_back( entity );
	}

	void Scene::OnModifyPrefab( Ref<Prefab> prefabAsset )
	{
		std::unordered_map<entt::entity, SharedPtr<Entity>> replace;

		auto entities = GetAllEntitiesWith<PrefabComponent>();
		for( auto& rEntity : entities )
		{
			const auto& rPrefabComp = rEntity->GetComponent<PrefabComponent>();

			if( rPrefabComp.AssetID != prefabAsset->ID )
				continue;

			if( rPrefabComp.Modified )
			{
				// merge
			}
			else
			{
				CreateEntityParameters cep;
				replace[ rEntity->GetHandle() ] = CreatePrefab( prefabAsset, cep );
			}
		}

		for( auto& [id, rEntity] : replace )
		{
			auto& rOldEntity = m_EntityIDMap[ id ];

			// Copy over core data
			CopyComponentIfExists<RelationshipComponent>( rEntity->GetHandle(), rOldEntity->GetHandle(), m_Registry );
			CopyComponentIfExists<IdComponent>( rEntity->GetHandle(), rOldEntity->GetHandle(), m_Registry );
			CopyComponentIfExists<TagComponent>( rEntity->GetHandle(), rOldEntity->GetHandle(), m_Registry );
			CopyComponentIfExists<TransformComponent>( rEntity->GetHandle(), rOldEntity->GetHandle(), m_Registry );

			SAT_CORE_WARN( "Rebasing prefab entity {0}! (ASSET/{1})", rEntity->GetName(), prefabAsset->Name );

			DeleteEntity( rOldEntity, false, rEntity->GetUUID() );
		}
	}

	void Scene::TransferModifiedProperties( const SharedPtr<Entity>& rSourceEntity, SharedPtr<Entity>& rEntity, const std::string& rMetadataName )
	{
		/*
		auto& rProperties = ClassMetadataHandler::Get().GetAllProperties( rMetadataName );

		for( auto& rProperty : rProperties )
		{
			if( rProperty.GetType() == SPropertyType::Entity )
			{
				SharedPtr<Entity>& currentEntity = rProperty.Read<SPropertyType::Entity>( const_cast< Entity* >( rSourceEntity.Get() ) );

				if( currentEntity != nullptr )
				{
					UUID id = currentEntity->GetUUID();

					// Find the same entity but in our scene
					SharedPtr<Entity> ourEntity = FindEntityByID( id );

					rProperty.SetProperty( rEntity.Get(), ourEntity );
				}
			}
			else
			{
				rProperty.RtCopyFromOther( const_cast< Entity* >( rSourceEntity.Get() ), rEntity.Get() );
			}
		}
		*/
	}

	void Scene::DestroyPendingEntities()
	{
		while( !m_EntitiesToDestory.empty() )
		{
			Entity* pEntity = m_EntitiesToDestory.back();
			DeleteEntityChecked( pEntity );

			m_EntitiesToDestory.pop_back();
		}
	}

	void Scene::DeleteEntityChecked( Entity* pEntity )
	{
		// Could use GetClass()
		if( m_NavBoundsEntity->GetUUID() == pEntity->GetUUID() )
			m_NavBoundsEntity = nullptr;

//		if( pEntity == m_pMainCameraEntity )
//			m_pMainCameraEntity = nullptr;

		if( pEntity->HasComponent<StaticMeshComponent>() )
		{
			auto& mc = pEntity->GetComponent<StaticMeshComponent>();
			if( mc.Mesh )
			{
				mc.Mesh.Reset();
				mc.MaterialRegistry.Reset();
			}
		}

		if( pEntity->HasComponent<RigidbodyComponent>() )
		{
			auto& rb = pEntity->GetComponent<RigidbodyComponent>();
			if( rb.Rigidbody )
			{
				delete rb.Rigidbody;
			}
		}

		if( pEntity->HasComponent<CharacterMovementComponent>() )
		{
			auto& movementComponent = pEntity->GetComponent<CharacterMovementComponent>();
			if( movementComponent.CharacterMovement )
			{
				delete movementComponent.CharacterMovement;
			}
		}

		for( auto& rChild : pEntity->GetChildren() )
		{
			auto child = FindEntityByID( rChild );
			if( child )
			{
				DeleteEntityChecked( child.Get() );
			}
		}

		m_Registry.destroy( pEntity->GetHandle() );
		// Destory via the shared ptr
		m_EntityIDMap.erase( pEntity->GetHandle() );
	}

	void Scene::CopyScene( Ref<Scene>& NewScene )
	{
		// Copy entities
		// I know we can just use the "=" operator, but we need to recreate the entities from the game.
		for( auto&& [hnd, originalEntity] : m_EntityIDMap )
		{
			NewScene->m_EntityIDMap[ hnd ] = NewScene->CreateEntityWithIDScript( originalEntity->GetUUID(), originalEntity->GetName(), originalEntity->GetClass()->GetName(), false );

			TransferModifiedProperties( originalEntity, NewScene->m_EntityIDMap[ hnd ], originalEntity->GetClass()->GetName() );
		}

		NewScene->m_Lights = m_Lights;
		
		// Asset props
		NewScene->ID = ID;
		NewScene->Name = Name;

		std::unordered_map< UUID, entt::entity > EntityMap;
		
		const auto IdComponents = NewScene->GetAllEntitiesWith< IdComponent >();
		for( const auto& entity : IdComponents )
			EntityMap[ entity->GetUUID() ] = entity->GetHandle();

		CopyComponent( AllComponents{}, NewScene->m_Registry, m_Registry, EntityMap );

		NewScene->PostDeserialise();
	}

	void Scene::TravelToScene( AssetID newSceneID )
	{
		// There isn't much we can do, we must let the parent layer handle a scene travel.
		Application::Get()->DispatchEvent<SceneTravelEvent>( newSceneID );
	}

	bool Scene::OnRuntimeStart()
	{
		m_RuntimeState = RuntimeState::Starting;

		CreatePhysicsScene();

		for( auto&& [id, entity] : m_EntityIDMap )
		{
			entity->BeginPlay();
		}

		// Init new scene camera
		m_pMainCameraEntity = GetMainCameraEntity( true );

		if( m_pMainCameraEntity.Expired() )
		{
			// Reject runtime, no camera was found after BeginPlay was called
			OnRuntimeEnd();

			SAT_CORE_ERROR( "Runtime request blocked. No camera was found after BeginPlay was called!" );
		
			return false;
		}

		m_NavigationSystem.Initialise();

		StartAudioPlayers();
		StartAnimations();
		StartBehaviourTrees();

		m_RuntimeState = RuntimeState::Running;

		return true;
	}

	void Scene::SuspendRuntime()
	{
		if( m_RuntimeState != RuntimeState::Running )
			return;

		m_RuntimeState = RuntimeState::Suspended;

		AudioSystem::Get().Suspend();
	}

	void Scene::ResumeRuntime()
	{
		if( m_RuntimeState != RuntimeState::Suspended )
			return;

		m_RuntimeState = RuntimeState::Running;

		AudioSystem::Get().Resume();
	}

	void Scene::SuspendOrResumeRuntime()
	{
		if( m_RuntimeState == RuntimeState::Suspended )
		{
			ResumeRuntime();
		}
		else if( m_RuntimeState == RuntimeState::Running ) 
		{
			SuspendRuntime();
		}
	}

	void Scene::UpdateAudioListeners()
	{
		const auto listeners = GetAllEntitiesWith< AudioListenerComponent >();
		for( auto& entity : listeners )
		{
			auto& rComp = entity->GetComponent<AudioListenerComponent>();
			
			if( rComp.Primary )
			{
				auto& rTransform = entity->GetComponent<TransformComponent>();
				AudioSystem::Get().SetPrimaryListenerPos( rTransform.Position );
			}
		}
	}

	void Scene::StartAnimations()
	{
		const auto anims = GetAllEntitiesWith< SkeletalMeshComponent >();
		for( auto& entity : anims )
		{
			auto& rComp = entity->GetComponent<SkeletalMeshComponent>();

			rComp.LocalAnimator = Ref<Animator>::Create();

			rComp.LocalAnimator->InitAnimation( rComp.AnimationControllerAssetID, rComp.Mesh, rComp.AnimatorType );
			rComp.LocalAnimator->Begin();
		}
	}

	void Scene::StartBehaviourTrees()
	{
		const auto anims = GetAllEntitiesWith<BehaviourTreeComponent>();
		for( auto& entity : anims )
		{
			auto& rComp = entity->GetComponent<BehaviourTreeComponent>();
			if( rComp.BehaviourTreeAssetID == 0 ) continue;

			if( entity->GetClass()->IsChildOf( AIAgentEntity::StaticClass() ) )
			{
				auto agent = entity.As<AIAgentEntity>();
				agent->StartBehaviourTree( rComp.BehaviourTreeAssetID );
			}
		}
	}

	void Scene::RemoveRigidBody( PhysicsRigidBody* pBody )
	{
		delete pBody;
	}

	void Scene::StartAudioPlayers()
	{
		const auto sndPlayers = GetAllEntitiesWith< AudioPlayerComponent >();
		for( auto& entity : sndPlayers )
		{
			auto& rComp = entity->GetComponent<AudioPlayerComponent>();
			Ref<Asset> soundSpec = AssetManager::Get()->FindAsset( rComp.SpecAssetID );

			if( !soundSpec )
				continue;
			
			if( soundSpec->Type == AssetType::GraphSound )
			{
				Ref<GraphSound> sound = AudioSystem::Get().PlayGraphSound( rComp.SpecAssetID, rComp.UniqueID );

				sound->WaitUntilLoaded();

				if( rComp.Spatialisation )
				{
					sound->SetSpatialisation( true );
					sound->SetPosition( entity->GetComponent<TransformComponent>().Position );
				}

				sound->SetVolume( rComp.Mute ? 0.0f : rComp.Volume );
				sound->Loop( rComp.Loop );

#if !defined(SAT_DIST)
				// Add reference if a graph sound asset viewer is open
				const std::string name = std::format( "{0}##{1}", soundSpec->Name, ( uint64_t ) soundSpec->ID );
				Ref<GraphSoundAssetViewer> window = ImGuiWindowManager::Get()->GetWindow<GraphSoundAssetViewer>( name );

				if( window )
				{
					window->AddSoundReference( sound );
				}
#endif
			}
			else
			{
				Ref<Sound> sound = nullptr;

				if( rComp.Spatialisation )
				{
					sound = AudioSystem::Get().PlaySoundAtLocation( rComp.SpecAssetID, rComp.UniqueID, entity->GetComponent<TransformComponent>().Position, true, rComp.SoundGroup );
				}
				else
				{
					sound = AudioSystem::Get().RequestNewSound( rComp.SpecAssetID, rComp.UniqueID, true, rComp.SoundGroup );
				}

				sound->Loop( rComp.Loop );
				sound->SetVolume( rComp.Volume );
				sound->SetPitch( rComp.Pitch );
			}
		}

		AudioSystem::Get().StartSoundGroups();
	}

	void Scene::StopAudioPlayers() 
	{
		const auto sndPlayers = GetAllEntitiesWith< AudioPlayerComponent >();
		for( auto& entity : sndPlayers )
		{
			auto& rComp = entity->GetComponent<AudioPlayerComponent>();
			
			AudioSystem::Get().StopSound( rComp.UniqueID );
		}

		AudioSystem::Get().StopSoundGroups();
	}

	void Scene::DestroyAudioPlayers()
	{
		const auto sndPlayers = GetAllEntitiesWith< AudioPlayerComponent >();
		for( auto& entity : sndPlayers )
		{
			auto& rComp = entity->GetComponent<AudioPlayerComponent>();

			AudioSystem::Get().UnloadSound( rComp.UniqueID );
		}

		AudioSystem::Get().StopSoundGroups();
	}

	void Scene::OnRuntimeEnd()
	{
		if( m_RuntimeState == RuntimeState::Suspended )
			ResumeRuntime();

		m_RuntimeState = RuntimeState::Ending;

		DestroyPhysicsScene();

		m_Controllers.clear();

		DestroyAudioPlayers();

		const auto animators = GetAllEntitiesWith<SkeletalMeshComponent>();
		for( auto& entity : animators )
		{
			auto& rAnimator = entity->GetComponent<SkeletalMeshComponent>().LocalAnimator;
			if( rAnimator )
			{
				rAnimator->Destory();
			}

			rAnimator = nullptr;
		}

		m_pMainCameraEntity = nullptr;

		m_NavigationSystem.ReleaseReferenceToNavBounds();
		m_RuntimeState = RuntimeState::NoState;
	}

	SharedPtr<NavBoundsEntity> Scene::GetNavBoundsEntity() const
	{
		return m_NavBoundsEntity;
	}

	SharedPtr<Entity> Scene::CreatePrefab( Ref<Prefab> prefabAsset, CreateEntityParameters& rEntityParameters )
	{
		SharedPtr<Entity> prefabEntity = prefabAsset->PrefabToEntity( this );

		SAT_CORE_ASSERT( !rEntityParameters.pClass, "It is invalid for the creation parameters to have a valid SClass, you must not change the SClass as that is controlled by the Prefab asset!" );

		if( !rEntityParameters.Tag.empty() )
		{
			prefabEntity->GetComponent<TagComponent>().Tag = rEntityParameters.Tag;
		}

		prefabEntity->GetComponent<TransformComponent>().SetPositionRotationScale( rEntityParameters.Position, rEntityParameters.Rotation, rEntityParameters.Scale );
		
		if( rEntityParameters.Parent )
		{
			rEntityParameters.Parent->AddChild( prefabEntity->GetUUID() );
			prefabEntity->SetParent( rEntityParameters.Parent->GetUUID() );
		}

		// TODO: Temp! should create somesort of OnEntitySpawned() function so we can start animations, behaviour trees etc.
		if( prefabEntity->GetClass()->IsChildOf( AIAgentEntity::StaticClass() ) )
		{
			if( const auto* pComp = prefabEntity->TryGetComponent<BehaviourTreeComponent>(); pComp )
			{
				auto agent = prefabEntity.As<AIAgentEntity>();
				agent->StartBehaviourTree( pComp->BehaviourTreeAssetID );
			}
		}

		return prefabEntity;
	}

	void Scene::AcknowledgeHotReload()
	{
		std::unordered_map<entt::entity, SharedPtr<Entity>> replace;

		for( auto&& [id, entity] : m_EntityIDMap )
		{
			if( entity->HasComponent<DScriptComponent>() )
			{
				auto& rScriptComponent = entity->GetComponent<DScriptComponent>();

				SharedPtr<Entity> newEntity = HotReloadReplaceOldEntity(entity);

				replace[ id ] = newEntity;
			}
		}

		for( auto& [id, entity] : replace )
		{
			auto& rOldEntity = m_EntityIDMap[ id ];

			CopyComponentIfExists( AllDuplicatableComponents{}, entity->GetHandle(), rOldEntity->GetHandle(), m_Registry );

			CopyComponentIfExists<RelationshipComponent>( entity->GetHandle(), rOldEntity->GetHandle(), m_Registry );

			DeleteEntity( rOldEntity, false, entity->GetUUID() );
		}

		replace.clear();
	}

	void Scene::AddController( Ref<PlayerInputController> playerInputController )
	{
		m_Controllers.push_back( playerInputController );
	}

	void Scene::RemoveController( Ref<PlayerInputController> playerInputController )
	{
		const auto Itr = std::find( m_Controllers.begin(), m_Controllers.end(), playerInputController );

		if( Itr != m_Controllers.end() )
			m_Controllers.erase( Itr );
	}

	void Scene::SetActiveScene( Scene* pScene )
	{
		g_ActiveScene = pScene;
	}

	Scene* Scene::GetActiveScene()
	{
		return g_ActiveScene;
	}

	void Scene::PostDeserialise()
	{
		// Find and load the nav mesh
		auto entites = GetAllEntitiesWith<NavigationMeshSpecificationComponent>();

		SAT_CORE_ASSERT( entites.size() <= 1, "There can only be one entity with a NavigationMeshSpecificationComponent in the scene!" );

		for( const auto& rEntity : entites )
		{
			if( rEntity->GetClass() != NavBoundsEntity::StaticClass() )
			{
				SAT_CORE_ERROR( "Invalid class type for navigation bounds!" );
				continue;
			}

			// Load initial nav mesh
			SharedPtr<NavBoundsEntity> boundsEntity = rEntity.As<NavBoundsEntity>();
			boundsEntity->LoadNavMeshFromDisk();

			m_NavBoundsEntity = boundsEntity;

#if !defined(SAT_DIST)
			if( !m_NavBoundsEntity->GetComponent<NavigationMeshSpecificationComponent>().HasBuilt )
			{
				Application::Get()->DispatchEvent<SendEditorNotificationEvent>( "The navigation mesh was unable to be loaded from disk or it was never built! Please check the output for more information." );
			}
#endif

//			OnNavMeshBuildCompleted();
		}
	}

	void Scene::OnEntityCreated( SharedPtr<Entity> entity )
	{
		m_EntityIDMap[ entity->GetHandle() ] = entity;

		if( IsRuntimeRunning() )
		{
			entity->BeginPlay();
		}
	}

	void Scene::PrepareForNavMeshBuilding()
	{
		// NOTE: This function is called by both the editor scene and the runtime scene
		//       So if we are the editor scene, we'll create the physics scene here
		//       and keep it alive and add any new bodies to the scene.
		// NOTE: Currently the need for PrepareForNavMeshBuilding to exist is pointless
		//       however we may need to have special functionality here so it will be kept.
		CreatePhysicsScene();
	}

	void Scene::OnNavMeshBuildCompleted()
	{
	}

	void Scene::CreatePhysicsScene()
	{
		m_PhysicsScene = std::make_shared<PhysicsScene>( this );
	}

	void Scene::DestroyPhysicsScene()
	{
		m_PhysicsScene.reset();
	}

	SharedPtr<Entity> Scene::HotReloadReplaceOldEntity( SharedPtr<Entity> source )
	{
		// Create new entity
		SharedPtr<Entity> entity = (Entity*)ClassMetadataHandler::Get().CreateClassObject( source->GetClass()->GetHash() );

		entity->SetName( source->GetName() );
		entity->GetComponent<IdComponent>().ID = source->GetUUID();

		// We don't replace the entt handle as we only entt for the backend
		// So just remove the source entity from entt and we will handle our map.
		//DeleteEntity( source );

		return entity;
	}

	//////////////////////////////////////////////////////////////////////////
	// #WARNING This should not be confused with AssetSerialisers. This is for raw binary serialisation!

	void Scene::SerialiseData()
	{
		std::filesystem::path out = Project::GetActiveProject()->GetTempDir();
		out /= std::to_string( ID );
		out.replace_extension( ".vfs" );

		std::ofstream stream( out, std::ios::binary | std::ios::trunc );

		/////////////////////////////////////

		SerialiseInternal( stream );

		stream.close();
	}
	
	void Scene::SerialiseInternal( std::ofstream& rStream )
	{
		Lights::Serialise( m_Lights, rStream );

		// Serialise the map manually.
		size_t mapSize = m_EntityIDMap.size();
		rStream.write( reinterpret_cast< char* >( &mapSize ), sizeof( size_t ) );

		for( auto& [k, v] : m_EntityIDMap )
		{
			// Write SClass hash
			const uint64_t classHash = v->GetClass()->GetHash();
			RawSerialisation::WriteObject( classHash, rStream );

			// V (Entity) is not trivial
			Entity::Serialise( v, rStream );
		}
	}

	void Scene::DeserialiseData()
	{
		const std::string& rMountBase = Project::GetActiveConfig().Name;
		Ref<VFile> file = VirtualFS::Get().FindFile( rMountBase, Path );

		if( !file )
			return;

		PakFileMemoryBuffer membuf( file->FileContent );

		std::istream stream( &membuf );

		/////////////////////////////////////

		DeserialiseInternal( stream );
	}

	template<typename IStream>
	void Scene::DeserialiseInternal( IStream& rStream )
	{
//		VariableGuard<Scene*> activeScene( g_ActiveScene, this );
		auto* pOldScene = g_ActiveScene;
		g_ActiveScene = this;

		Lights::Deserialise( m_Lights, rStream );

		// Read the map manually.
		size_t mapSize = 0;
		rStream.read( reinterpret_cast< char* >( &mapSize ), sizeof( size_t ) );
		for( size_t i = 0; i < mapSize; ++i )
		{
			uint64_t classHash = 0llu;
			RawSerialisation::ReadObject( classHash, rStream );

			SharedPtr<Entity> V = nullptr;
			V = ( Entity* ) ClassMetadataHandler::Get().CreateClassObject( classHash );

			// V is always non-trivial
			Entity::Deserialise( V, rStream );

			m_EntityIDMap[ V->GetHandle() ] = V;
		}

		Name = Path.stem().string();
		PostDeserialise();

		g_ActiveScene = pOldScene;
	}

}
