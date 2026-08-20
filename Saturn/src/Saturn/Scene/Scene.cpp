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

#include "Saturn/Vulkan/Renderer2D.h"
#include "Saturn/Vulkan/SceneRenderer.h"
#include "Saturn/Vulkan/AluraRenderer.h"

#include "Saturn/Asset/Prefab.h"
#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Asset/TextureSourceAsset.h"

#include "Saturn/Core/Profiler.h"
#include "Saturn/Core/VirtualFS.h"
#include "Saturn/Core/MemoryStream.h"

#include "Saturn/Physics/PhysicsScene.h"
#include "Saturn/Physics/PhysicsRigidBody.h"
#include "Saturn/Physics/PhysicsCharacterController.h"

#include "Saturn/Project/Project.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"
#include "Saturn/GameFramework/PlayerInputController.h"

#include "Saturn/Audio/AudioSystem.h"

#include "Saturn/Animation/SkeletonAsset.h"
#include "Saturn/Animation/BoneJoint.h"

#include "Saturn/Alura/AluraCanvas.h"

#if !defined(SAT_DIST)
#include "Saturn/ImGui/EditorIcons.h"
#include "Saturn/ImGui/EntitySelectionManager.h"
#include "Saturn/ImGui/EditorEvents.h"
#include "Saturn/ImGui/ImGuiWindowManager.h"

#include "Saturn/ImGui/UndoRedo/GlobalUndoRedoGroup.h"
#include "Saturn/ImGui/UndoRedo/EntityUndoRedoActions.h"

#include "Saturn/Audio/SoundGraph/GraphSoundAssetViewer.h"

#include "Saturn/Physics/PhysicsDebugMeshes.h"

#include "Saturn/AI/BehaviourTree/BehaviourTree.h"
#include "Saturn/AI/BehaviourTree/BehaviourTreeTaskHandler.h"
#endif

#include "Saturn/AI/Navigation/NavBoundsEntity.h"
#include "Saturn/AI/AIAgentEntity.h"

#include "Saturn/Runtime/RuntimeEvents.h"

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace Saturn {

	static std::tuple<glm::vec3, glm::quat, glm::vec3> GetTransformDecomposition( const glm::mat4& transform )
	{
		glm::vec3 scale, translation, skew;
		glm::vec4 perspective;
		glm::quat orientation;
		glm::decompose( transform, scale, orientation, translation, skew, perspective );

		return { translation, orientation, scale };
	}
	
	//////////////////////////////////////////////////////////////////////////
	// Prefab2

	template<typename V>
	struct ComponentRefl
	{
		static void Add( entt::registry* pReg, const entt::entity e ) 
		{
			pReg->emplace_or_replace<V>( e );
		}

		static void CopyFromPrefab( 
			entt::registry* pReg, 
			entt::registry* pPrefReg, 
			const entt::entity e, 
			const entt::entity ep )
		{
			if( pPrefReg->any_of<V>( ep ) )
			{
				V& rOgComponent = pPrefReg->get<V>( ep );
				pReg->emplace_or_replace<V>( e, rOgComponent );
			}
		}

		static void Remove( entt::registry* pReg, const entt::entity e )
		{
			pReg->remove<V>( e );
		}

		static void Break() 
		{
			Core::BreakDebug();
		}
	};

	template<typename... V>
	static void BuildComponentRefl()
	{
		// Fold expression and unpack V, gain meta types for every component in the component group.
		( [ & ]()
		{
			entt::meta<V>()
				.type( entt::type_id<V>().hash() )
				.func<&ComponentRefl<V>::Add>( entt::hashed_string( "Add" ) )
				.func<&ComponentRefl<V>::CopyFromPrefab>( entt::hashed_string( "CopyFromPrefab" ) )
				.func<&ComponentRefl<V>::Break>( entt::hashed_string( "Break" ) )
				.func<&ComponentRefl<V>::Remove>( entt::hashed_string( "Remove" ) );
		}( ), ... );
	}

	template<typename... V>
	static void BuildComponentRefl(
		ComponentGroup<V...> )
	{
		BuildComponentRefl<V...>();
	}

	//////////////////////////////////////////////////////////////////////////

	Scene::Scene()
	{
		m_SceneEntity = m_Registry.create();
		m_Registry.emplace<SceneComponent>( m_SceneEntity, m_InternalID );

		BuildComponentRefl( AllComponents{} );
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
			// The physics scene will cleanup all of the rigid bodies, shapes, etc
			// however, on Debug double we'll check that.
#if defined(SAT_DEBUG)
			const auto rigidBodies = GetAllEntitiesWith<RigidbodyComponent>();
			for( auto& entity : rigidBodies )
			{
				SAT_CORE_ASSERT( !entity->GetComponent<RigidbodyComponent>().Rigidbody );
			}
#endif

			DestroyAudioPlayers();
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
			DestroyPendingEntities();

			// Simulate the physics scene.
			m_PhysicsScene->Simulate( ts );
			OnUpdatePhysics( ts );

			OnUpdateEntities( ts );
			
			OnUpdateAnimators( ts );

			UpdateAudioListeners();
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

		const auto boneAttachments = GetAllEntitiesWith<BoneAttachmentInfoComponent>();
		for( const auto& entity : boneAttachments )
		{
			auto& boneAttachment = entity->GetComponent<BoneAttachmentInfoComponent>();
			auto& tc = entity->GetComponent<TransformComponent>();

			const auto parent = FindEntityByID( entity->GetParent() );
			if( parent )
			{
				const auto& rSk = parent->GetComponent<SkeletalMeshComponent>();

				const auto skeleton = rSk.Mesh->GetSkeletonAsset();
				if( skeleton )
				{
					if( const auto* pBoneJoint = skeleton->FindBoneJoint( boneAttachment.AttachmentName ) ) 
					{
						// Update transform.
						tc.SetTransform( pBoneJoint->GetBoneMatrix( rSk.LocalAnimator ) );
					}
				}
			}
		}
	}

	void Scene::OnEvent( Event& rEvent )
	{
		// Other states do not need to be handled because anything other than Running or Suspended should get through here.
		// TODO: Handle this better.
		if( IsPausedOrSuspended() )
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

	void Scene::OnUpdateAnimators_Preview( Timestep ts )
	{
		const auto boneAttachments = GetAllEntitiesWith<BoneAttachmentInfoComponent>();
		for( const auto& entity : boneAttachments )
		{
			auto& boneAttachment = entity->GetComponent<BoneAttachmentInfoComponent>();
			auto& tc = entity->GetComponent<TransformComponent>();

			const auto parent = FindEntityByID( entity->GetParent() );
			if( parent )
			{
				const auto& rSk = parent->GetComponent<SkeletalMeshComponent>();

				const auto skeleton = rSk.Mesh->GetSkeletonAsset();
				if( skeleton )
				{
					if( const auto* pBoneJoint = skeleton->FindBoneJoint( boneAttachment.AttachmentName ) )
					{
						// Update transform.
						tc.SetTransform( pBoneJoint->GetBoneMatrixPreview( rSk.Mesh ) );
					}
				}
			}
		}
	}

	void Scene::OnRenderEditor( Camera* pCamera, const glm::mat4& rViewMartix, Ref<SceneRenderer> sceneRenderer, Timestep ts )
	{
		SAT_PF_EVENT();

		m_RendererCamera.pCamera = pCamera;
		m_RendererCamera.ViewMatrix = rViewMartix;

		sceneRenderer->SetCamera( m_RendererCamera );
		sceneRenderer->PreRender();

		//////////////////////////////////////////////////////////////////////////

		// Lights
		RtSetupLights( sceneRenderer );

		// Renderer2D 
		RtBuildRenderer2DCommands( sceneRenderer );

		// Scene Renderer (main geometry)
		RtBuildSceneRendererCommands( sceneRenderer );

#if !defined(SAT_DIST)
		RtDrawSkDebug( sceneRenderer );
#endif
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

		g_AluraCanvas->NewFrame();
		g_AluraCanvas->DrawAllDrawers( ts );
		g_AluraCanvas->EndFrame();

		// Lights
		RtSetupLights( sceneRenderer );

		// AI debug visualisation happens during runtime...
#if !defined(SAT_DIST)
		RtDrawAIDebug( sceneRenderer );
		RtDrawSkDebug( sceneRenderer );
#endif

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
				flip = textureAsset->IsLoadFlagSet( TextureLoadFlags_FlipVertically );
			}
			
			// An extra step to counteract if the texture is the wrong way around,
			// this allows the billboard to always display correct.
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

		const auto textEntities = GetAllEntitiesWith<TextComponent>();
		for( const auto& rEntity : textEntities )
		{
			const TextComponent& rTextComp = rEntity->GetComponent<TextComponent>();
			const TransformComponent& rTc = rEntity->GetComponent<TransformComponent>();

			Ref<AluraFont> font = AssetManager::Get()->GetAssetAs<AluraFont>( rTextComp.FontAssetID );
			if( !font )
			{
				font = AssetManager::Get()->GetAssetAs<AluraFont>( Project::GetActiveProject()->GetDefaultFontAsset() );

				// If we get here then I'll need to add an editor backup font!
				SAT_CORE_ASSERT( font );
			}

			sceneRenderer->GetRenderer2D()->SubmitString( rTextComp.Text, font, rTc.GetTransform(), rTextComp.Color );
		}

		if( m_RuntimeState == RuntimeState::Suspended )
		{
			RtDrawAIDebug( sceneRenderer );
		}
#endif
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
		RtBuildSelectedMeshesCmds( sceneRenderer );
#endif
	}

#if !defined(SAT_DIST)
	void Scene::RtBuildSelectedMeshesCmds( Ref<SceneRenderer> sceneRenderer )
	{
		//////////////////////////////////////////////////////////////////////////
		// PhysColliders

		auto submitBoxCollider = [ this, &sceneRenderer ]( SharedPtr<Entity> entity, Ref<StaticMesh> dbgMesh, Ref<MaterialRegistry> materialRegistry )
		{
			glm::mat4 transform = GetTransformRelativeToParent( entity );
			const auto& rComponent = entity->GetComponent<BoxColliderComponent>();

			auto colliderTransform = glm::translate( glm::mat4( 1.0f ),rComponent.Offset )
				* glm::scale( glm::mat4( 1.0f ), rComponent.HalfExtents * 2.0f );

			sceneRenderer->SubmitPhysicsCollider( entity, dbgMesh, materialRegistry, transform * colliderTransform );
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
					else if( rEntity->GetClass() == NavBoundsEntity::StaticClass() )
					{
						sceneRenderer->GetRenderer2D()->SubmitAABB( m_NavBoundsEntity->GetBoundingBox(), glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f } );
						m_NavBoundsEntity->DebugDraw( sceneRenderer->GetRenderer2D().Get() );
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

				if( m_NavBoundsEntity )
				{
					sceneRenderer->GetRenderer2D()->SubmitAABB( m_NavBoundsEntity->GetBoundingBox(), glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f } );
					m_NavBoundsEntity->DebugDraw( sceneRenderer->GetRenderer2D().Get() );
				}
			} break;
		}

#if SAT_FEATURE_SHOW_SELECTED_CAMERA_FRUSTUM
		//////////////////////////////////////////////////////////////////////////
		// Camera Frustum

		for( const auto& rEntity : EntitySelectionManager::Get()->GetSelectionContexts( this ) )
		{
			if( auto* pCameraComp = rEntity->TryGetComponent<CameraComponent>(); pCameraComp )
			{
				auto renderer2D = sceneRenderer->GetRenderer2D();
				pCameraComp->Camera->RenderDebugFrustum( renderer2D );

				break;
			}
		}
#endif

		for( const auto& rEntity : EntitySelectionManager::Get()->GetSelectionContexts( this ) )
		{
			if( auto* pStaticMeshComp = rEntity->TryGetComponent<StaticMeshComponent>() )
			{
				if( !pStaticMeshComp->Mesh )
					continue;

				const auto transform = GetTransformRelativeToParent( rEntity );

				Ref<MaterialRegistry> targetMaterialRegistry = pStaticMeshComp->Mesh->GetMaterialRegistry();

				if( pStaticMeshComp->MaterialRegistry && pStaticMeshComp->MaterialRegistry->HasAnyOverrides() )
					targetMaterialRegistry = pStaticMeshComp->MaterialRegistry;

				sceneRenderer->SubmitSelectedStaticMesh( rEntity, pStaticMeshComp->Mesh, targetMaterialRegistry, transform );
			}

			if( auto* pSkMeshComp = rEntity->TryGetComponent<SkeletalMeshComponent>() )
			{
				if( !pSkMeshComp->Mesh )
					continue;

				const auto transform = GetTransformRelativeToParent( rEntity );

				Ref<MaterialRegistry> targetMaterialRegistry = pSkMeshComp->Mesh->GetMaterialRegistry();

				if( pSkMeshComp->MaterialRegistry && pSkMeshComp->MaterialRegistry->HasAnyOverrides() )
					targetMaterialRegistry = pSkMeshComp->MaterialRegistry;

				sceneRenderer->SubmitSelectedDynamicMesh( rEntity, pSkMeshComp->Mesh, targetMaterialRegistry, transform );
			}
		}
	}

	void Scene::RtDrawAIDebug( Ref<SceneRenderer> sceneRenderer )
	{
		if( ( m_VisualisationOptions.AIVisualisationOptions & AIVisualisationOptions_NavPaths ) != 0 )
		{
			m_NavigationSystem.DebugDraw( sceneRenderer->GetRenderer2D().Get() );
		}
		
		if( ( m_VisualisationOptions.AIVisualisationOptions & AIVisualisationOptions_BehaviourTreeInfo ) != 0 )
		{
			const auto aiAgentTexture = EditorIcons::GetIcon( "Billboard_AIAgent" );

			auto behaviourTreeEntites = GetAllEntitiesWith<BehaviourTreeComponent>();
			for( const auto& rEntity : behaviourTreeEntites )
			{
				const BehaviourTreeComponent& rBt = rEntity->GetComponent<BehaviourTreeComponent>();
				
				const TransformComponent& rTc = rEntity->GetComponent<TransformComponent>();
				const glm::vec3 position( rTc.Position.x, rTc.Position.y + 2.5f, rTc.Position.z );

				sceneRenderer->GetRenderer2D()->SubmitBillboardTextured(
					position,
					glm::vec4( 1.0f ),
					aiAgentTexture, glm::vec2( 1.0f ) );

				if( const auto aiAgent = rEntity.As<AIAgentEntity>() )
				{
					const auto currentBTTask = aiAgent->GetBehaviourTree()->GetTaskHandler()->GetCurrentTask();
					if( currentBTTask )
					{
						const std::string text = std::format( "BehaviourTree/{0}", currentBTTask->GetDebugName() );

						sceneRenderer->GetRenderer2D()->SubmitString(
							text,
							g_AluraCanvas->GetEditorFont(),
							rTc.GetTransform(),
							glm::one<glm::vec4>() );
					}
				}
			}
		}
	}

	void Scene::RtDrawSkDebug( Ref<SceneRenderer> sceneRenderer )
	{
		if( ( m_VisualisationOptions.SkeletonVisualisationOptions & SkeletonVisualisationOptions_BoneLines ) != 0 )
		{
			const auto skeletalMeshEntites = GetAllEntitiesWith<SkeletalMeshComponent>();
			for( const auto& rEntity : skeletalMeshEntites )
			{
				const auto& tc = rEntity->GetComponent<TransformComponent>();
				const auto& mc = rEntity->GetComponent<SkeletalMeshComponent>();
				const auto skMesh = mc.Mesh;

				if( skMesh )
				{
					const auto sk = skMesh->GetSkeletonAsset();
					if( sk )
					{
						const auto& rBonePositions = sk->GetBonePositions();
						const auto& rBoneNames	   = sk->GetBoneNames();

						const auto& rBoneInfos = sk->GetBoneInfo();
						for( size_t i = 0; i < rBoneInfos.size(); ++i )
						{

							const auto& rCurrent = rBoneInfos[ i ];
							const auto parentIndex = skMesh->GetSkeletonAsset()->GetParentIndex( i );

							if( parentIndex == ~0u )
							{
								const auto& rTransformCurrent = skMesh->GetDefaultBoneTransforms().at( i );
								sceneRenderer->GetRenderer2D()->SubmitLine(
									tc.Position,
									glm::vec3( rTransformCurrent[ 3 ] ) + tc.Position,
									glm::one<glm::vec4>(), true );

							}
							else
							{
								const auto& rTransformCurrent = skMesh->GetDefaultBoneTransforms().at( i );
								const auto& rTransformParent = skMesh->GetDefaultBoneTransforms().at( parentIndex );

								sceneRenderer->GetRenderer2D()->SubmitLine(
									glm::vec3( rTransformParent[ 3 ] ) + tc.Position,
									glm::vec3( rTransformCurrent[ 3 ] ) + tc.Position,
									glm::one<glm::vec4>(), true );
							}
						}
					}
				}
			}
		}
	}

#endif

	SharedPtr<Entity> Scene::CreateEntityWithIDScript( UUID uuid, const std::string& name /*= "" */, const std::string& rScriptName, bool externalData )
	{
		VariableGuard<Scene*> sceneGuard( g_ActiveScene, this );

		// UNSAFE! We just assume that rScriptName will be a subclass of an entity, could lead to UB
		SharedPtr<Entity> entity( ( Entity* ) ClassMetadataHandler::Get().CreateClassObject( rScriptName, nullptr ) );

		entity->SetName( name );
		entity->GetComponent<IdComponent>().ID = uuid;

		OnEntityCreated( entity );

		return entity;
	}

	SharedPtr<Entity> Scene::CreateEntity( const std::string& name /*= "" */ )
	{
		VariableGuard<Scene*> sceneGuard( g_ActiveScene, this );

		SharedPtr<Entity> entity( NewObject<Entity>( nullptr ) );
		entity->SetName( name );

		OnEntityCreated( entity );

		return entity;
	}

	SharedPtr<Entity> Scene::CreateEntity( CreateEntityParameters& rParams )
	{
		VariableGuard<Scene*> sceneGuard( g_ActiveScene, this );

		if( !rParams.pClass->IsChildOfOrIs( Entity::StaticClass() ) || rParams.pClass == nullptr ) 
			return nullptr;

		SharedPtr<Entity> entity( dynamic_cast< Entity* >( ClassMetadataHandler::Get().CreateClassObject( rParams.pClass ) ) );
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

	SharedPtr<Entity> Scene::FindEntityByTag( const std::string& tag ) const
	{
		for( auto&& [handle, entity] : m_EntityIDMap )
		{
			if( entity->GetComponent<TagComponent>().Tag == tag )
				return entity;
		}

		return nullptr;
	}

	SharedPtr<Entity> Scene::FindEntityByID( const UUID& id ) const
	{
		for( auto&& [handle, entity] : m_EntityIDMap )
		{
			if( entity->GetUUID() == id )
				return entity;
		}

		return nullptr;
	}

	SharedPtr<Entity> Scene::FindEntityByHandle( entt::entity handle ) const
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

	bool Scene::RaycastIgnore( SharedPtr<Entity> entityIgnore, const glm::vec3& Origin, const glm::vec3& Direction, float MaxDistance, RaycastHitResult* pOut )
	{
		if( m_PhysicsScene )
		{
			return m_PhysicsScene->RaycastIgnoringSelf( entityIgnore, Origin, Direction, MaxDistance, pOut );
		}

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
		return DuplicateEntityBetweenScene( this, entity, parent );
	}

	SharedPtr<Entity> Scene::DuplicateEntityBetweenScene( 
		Ref<Scene> targetScene,
		const SharedPtr<Entity> entity, 
		const SharedPtr<Entity> parent /*= nullptr*/ )
	{
		SharedPtr<Entity> newEntity( dynamic_cast< Entity* >( ClassMetadataHandler::Get().CreateClassObject( ( SClass* ) entity->GetClass() ) ) );
		newEntity->SetName( entity->GetComponent<TagComponent>().Tag );

		targetScene->OnEntityCreated( newEntity );

		CopyComponentIfExists( AllDuplicatableComponents{}, newEntity->GetHandle(), entity->GetHandle(), targetScene->GetRegistry() );

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
			SharedPtr<Entity> xparent = targetScene->FindEntityByID( entity->GetParent() );
			SharedPtr<Entity> newParent = targetScene->DuplicateEntity( xparent, nullptr );

			newEntity->SetParent( newParent->GetUUID() );
		}

		for( const auto& rID : sourceRelationship.ChildrenID )
		{
			const SharedPtr<Entity> child = targetScene->FindEntityByID( rID );
			SharedPtr<Entity> newChild = targetScene->DuplicateEntity( child, newEntity );

			newEntity->GetChildren().push_back( newChild->GetUUID() );
		}

		return newEntity;
	}

	void Scene::DeleteEntity( SharedPtr<Entity> entity, bool deleteChildren /*=true*/, UUID orphanParentID /*=0*/ )
	{
#if !defined(SAT_DIST)
		GlobalUndoRedoGroup::Get()->RemoveIfActionHasIdentifier( ( uint64_t ) entity->GetHandle() );
#endif

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

	void Scene::DeleteEntity( const entt::entity handle, bool deleteChildren /*= true*/, UUID orphanParentID /*= 0 */ )
	{
		auto entity = FindEntityByHandle( handle );
		if( entity )
		{
			DeleteEntity( entity, deleteChildren, orphanParentID );
		}
	}

	void Scene::DestroyEntity( Entity* entity )
	{
		m_EntitiesToDestroy.push_back( entity );
	}

	template<typename... V>
	static void BuildComponentHashInternal(
		entt::registry& rReg,
		entt::entity entity,
		std::vector<entt::id_type>& rMap )
	{
		// Most readable C++ code be like:
		// But this is a fold expression.
		( [ & ]()
		{
			if( rReg.any_of<V>( entity ) )
			{
				rMap.push_back( entt::type_id<V>().hash() );
			}
		}( ), ... );
	}

	template<typename... V>
	static void BuildComponentHashInternal( 
		ComponentGroup<V...>, 
		entt::registry& rReg, 
		entt::entity entity, 
		std::vector<entt::id_type>& rMap ) 
	{
		// Ensure no realloc needs to be done by simply reserving the max size of the component group.
		rMap.reserve( sizeof...( V ) );

		BuildComponentHashInternal<V...>( rReg, entity, rMap );
	}

	// Builds a list of all of given component hashes that "entity" has.
	std::vector<entt::id_type> Scene::BuildComponentHash( SharedPtr<Entity> entity )
	{
		std::vector<entt::id_type> res;
		BuildComponentHashInternal( AllComponents{}, m_Registry, entity->GetHandle(), res );
		return res;
	}

	void Scene::OnModifyPrefab_AddNewlyAddedComponents( 
		const Ref<Prefab> prefabAsset, 
		SharedPtr<Entity> entity, 
		const SharedPtr<Entity> entityInPrefab )
	{
		const auto& rPrefabComponent = entity->GetComponent<PrefabComponent>();
		const auto& rComponentMap = prefabAsset->GetComponentMap();

		const auto& rPrefabComponentHashesItr = rComponentMap.find( rPrefabComponent.EntityIDInPrefab );
		if( rPrefabComponentHashesItr != rComponentMap.end() )
		{
			const auto& rPrefabComponentHashes = rPrefabComponentHashesItr->second;

			// Preform merge...
			// Right so we need to figure out first if any components where added,
			// if so we also respect that change and add it to ourself.

			// Build component hash list for comparison.
			const std::vector<entt::id_type> componentHashes = BuildComponentHash( entity );

			// Find any missing in our entity.
			for( const auto& rPrefabHash : rPrefabComponentHashes )
			{
				// Doesn't exist? Add it.
				if( std::find( componentHashes.begin(), componentHashes.end(), rPrefabHash ) == componentHashes.end() )
				{
					// Add component via hash using the lovely entt::meta system.
					if( const auto reflectedType = entt::resolve( rPrefabHash ) )
					{
						auto& rPrefabRegistry = prefabAsset->GetScene()->GetRegistry();

						const auto copyFunc = reflectedType.func( entt::hashed_string( "CopyFromPrefab" ) );
						if( copyFunc )
						{
							// Copy component from master instance in prefab asset into our local entity.
							copyFunc.invoke( {},
								&m_Registry,
								&rPrefabRegistry,
								entity->GetHandle(),
								entityInPrefab->GetHandle() );
						}
					}
					else
						SAT_CORE_ERROR( "[Prefab2]: Internal Error: EnTT was unable to find the meta type for a component with hash: {0}", rPrefabHash );
				}
			}
		}
	}

	void Scene::OnModifyPrefab( AssetID prefabAssetID )
	{
		SAT_CORE_INFO( "[Prefab2]: Prefab modified -- OnModifyPrefab -- {0}", prefabAssetID );

		const Ref<Prefab> prefabAsset = AssetManager::Get()->GetAssetAs<Prefab>( prefabAssetID );
		if( !prefabAsset ) return;

		const auto allPrefabEntities = GetAllEntitiesWith<PrefabComponent>();
		for( auto& rEntity : allPrefabEntities )
		{
			const auto& rPrefabComponent = rEntity->GetComponent<PrefabComponent>();

			// We only want entities with the same ID as the modified one.
			if( rPrefabComponent.AssetID != prefabAssetID )
				continue;

			// Find the OG entity in the prefab itself.
			const SharedPtr<Entity> entityInPrefab = prefabAsset->FindEntityInPrefab( rPrefabComponent.EntityIDInPrefab );
			if( !entityInPrefab )
			{
				SAT_CORE_ERROR( "[Prefab2]: Unable to find the original entity stored in the prefab! Looking for {0}, skipping and will not update.", rPrefabComponent.EntityIDInPrefab );
				break;
			}

			// Add newly added components from the asset into the local entity in our scene.
			if( ( rPrefabComponent.Flags & PrefabUpdateFlag_DoNotAddAddedComponents ) != 0 )
			{
				OnModifyPrefab_AddNewlyAddedComponents( prefabAsset, rEntity, entityInPrefab );
			}
		}
	}

	void Scene::TransferModifiedProperties( 
		const SharedPtr<Entity> sourceEntity, 
		SharedPtr<Entity> targetEntity )
	{
		SAT_CORE_ASSERT( sourceEntity->GetClass() == targetEntity->GetClass() );

		const SClass* pSClass = sourceEntity->GetClass();

		for( size_t i = 0; i < pSClass->GetPropertyCount(); ++i )
		{
			const SProperty* pProperty = pSClass->GetProperties()[ i ];
			pProperty->RtCopyFromOther( sourceEntity.Get(), targetEntity.Get() );
		}
	}

	void Scene::DestroyPendingEntities()
	{
		while( !m_EntitiesToDestroy.empty() )
		{
			Entity* pEntity = m_EntitiesToDestroy.back();
			DeleteEntityChecked( pEntity );

			m_EntitiesToDestroy.pop_back();
		}
	}

	void Scene::DeleteEntityChecked( Entity* pEntity )
	{
		if( !m_Registry.valid( pEntity->GetHandle() ) )
			return;

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
		// Destroy via the shared ptr
		m_EntityIDMap.erase( pEntity->GetHandle() );
	}

	void Scene::CopyScene( Ref<Scene>& NewScene )
	{
		// Copy entities
		// I know we can just use the "=" operator, 
		// but we need to recreate the entities from the game,
		// and not just copy them.
		for( auto&& [hnd, originalEntity] : m_EntityIDMap )
		{
			NewScene->m_EntityIDMap[ hnd ] = NewScene->CreateEntityWithIDScript( originalEntity->GetUUID(), originalEntity->GetName(), originalEntity->GetClass()->GetName(), false );

			TransferModifiedProperties( 
				originalEntity, 
				NewScene->m_EntityIDMap[ hnd ] );
		}

		NewScene->m_Lights = m_Lights;
		
		// Asset props
		NewScene->ID = ID;
		NewScene->Name = Name;
		
#if !defined(SAT_DIST)
		// Visualisation options
		NewScene->m_VisualisationOptions = m_VisualisationOptions;
#endif

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

	bool Scene::TravelToScene( const std::string& rSceneName )
	{
		Ref<Asset> sceneAsset = AssetManager::Get()->FindAsset( rSceneName, AssetType::Scene );
		if( sceneAsset )
		{
			TravelToScene( sceneAsset->ID );
			return true;
		}

		return false;
	}

	bool Scene::IsPausedOrSuspended() const
	{
		return m_RuntimeState == RuntimeState::Suspended || m_RuntimeState == RuntimeState::Paused;
	}

	bool Scene::OnRuntimeStart()
	{
		m_RuntimeState = RuntimeState::Starting;

		CreatePhysicsScene();

		m_NavigationSystem.Initialise();

		StartAudioPlayers();
		StartAnimations();
		StartBehaviourTrees();

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

	void Scene::PauseGame()
	{
		m_RuntimeState = RuntimeState::Paused;

		if( auto entity = m_pMainCameraEntity.Access() )
		{
			entity->GetComponent<CameraComponent>().Camera->SetActive( false );
		}
	}

	void Scene::UnpauseGame()
	{
		m_RuntimeState = RuntimeState::Running;

		if( auto entity = m_pMainCameraEntity.Access() )
		{
			entity->GetComponent<CameraComponent>().Camera->SetActive( true );
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

			if( entity->GetClass()->IsChildOfOrIs( AIAgentEntity::StaticClass() ) )
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
				Ref<GraphSound> sound = AudioSystem::Get().PlayGraphSound( rComp.SpecAssetID, rComp.UniqueID, rComp.Spatialisation );

				sound->WaitUntilLoaded();

				if( rComp.Spatialisation )
				{
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

		AudioSystem::Get().StopSoundInSet( 1 );
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

		AudioSystem::Get().DestroySoundsInSet( 1 );
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
				rAnimator->Destroy();
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
		SAT_CORE_ASSERT( !rEntityParameters.pClass, "It is invalid for the creation parameters to have a valid SClass, you must not change the SClass that is controlled by the Prefab asset!" );

		SharedPtr<Entity> prefabEntity = prefabAsset->InstantiatePrefab( this );

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
		if( prefabEntity->GetClass()->IsChildOfOrIs( AIAgentEntity::StaticClass() ) )
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
		// A map of the old entities that will be deleted.
		std::unordered_map<entt::entity, SharedPtr<Entity>> replace;

		for( auto&& [hnd, entity] : m_EntityIDMap )
		{
			if( entity->GetClass() != Entity::StaticClass() )
			{
				const auto newEntity = HotReloadReplaceOldEntity( entity );
				
				SAT_CORE_INFO( "[Hot-Reload]: Will replace Handle {0}", ( ENTT_ID_TYPE ) hnd );

				replace[ hnd ] = newEntity;
			}
		}

		for( auto& [hnd, entity] : replace )
		{
			// Get the old entity.
			auto& rOldEntity = m_EntityIDMap[ hnd ];

			// Copy over components.
			CopyComponentIfExists( AllDuplicatableComponents{}, entity->GetHandle(), rOldEntity->GetHandle(), m_Registry );
			CopyComponentIfExists<RelationshipComponent>( entity->GetHandle(), rOldEntity->GetHandle(), m_Registry );

			// Delete old
			DeleteEntity( rOldEntity, false, entity->GetUUID() );
		
			// And insert the new entity into the map with it's new handle.
			m_EntityIDMap[ entity->GetHandle() ] = entity;

			SAT_CORE_INFO( "[Hot-Reload]: Replacing Handle {0}", ( ENTT_ID_TYPE ) hnd );
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
		const auto entities = GetAllEntitiesWith<NavigationMeshSpecificationComponent>();

		SAT_CORE_ASSERT( entities.size() <= 1, "There can only be one entity with a NavigationMeshSpecificationComponent in the scene!" );

		for( const auto& rEntity : entities )
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
		SharedPtr<Entity> entity( ( Entity* ) ClassMetadataHandler::Get().CreateClassObject( source->GetClass()->GetHash() ) );

		// and we give the new entity the same name and ID
		entity->SetName( source->GetName() );
		entity->GetComponent<IdComponent>().ID = source->GetUUID();

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
		RawSerialisation::WriteObject( m_EntityIDMap.size(), rStream );

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
		VariableGuard<Scene*> activeScene( g_ActiveScene, this );

		Lights::Deserialise( m_Lights, rStream );

		// Read the map manually.
		size_t mapSize = 0;
		RawSerialisation::ReadObject( mapSize, rStream );

		for( size_t i = 0; i < mapSize; ++i )
		{
			uint64_t classHash = 0llu;
			RawSerialisation::ReadObject( classHash, rStream );

			SharedPtr<Entity> V( ( Entity* )ClassMetadataHandler::Get().CreateClassObject( classHash ) );

			// V is always non-trivial
			Entity::Deserialise( V, rStream );

			m_EntityIDMap[ V->GetHandle() ] = V;
		}

		Name = Path.stem().string();
		PostDeserialise();
	}

}
