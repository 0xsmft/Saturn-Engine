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

#include "Saturn/Core/Maths.h"
#include "Saturn/Core/UUID.h"
#include "Saturn/Core/Renderer/SceneCamera.h"

#include "Saturn/Asset/MemoryAssetDependency.h"

#include "Saturn/Vulkan/EnvironmentMap.h"
#include "Saturn/Vulkan/Mesh.h"

#include "Saturn/Physics/PhysicsBodyType.h"

// TODO: Should not be included...
#include "Saturn/Animation/Animator.h"

#include "Saturn/Animation/BoneJoint.h"

#include <string>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Saturn {

	struct TransformComponent
	{
	private:
		friend class SceneSerialiser;
	private:
		// NOTE: Quaternions in GLM are WXYZ!
		// It would be better to have them as XYZW, however, this causes many issues
		// such as issues with physics even though PhysX uses XYZW and other issues with transform decomposition.
		glm::quat  RotationQuat ={ 1.0f, 0.0f, 0.0f, 0.0f };
		glm::vec3  Rotation = { 0.0f, 0.0f, 0.0f };
	public:
		glm::vec3  Position ={ 0.0f , 0.0f, 0.0f };
		glm::vec3  Scale	={ 1.0f , 1.0f, 1.0f };

		static constexpr glm::vec3 Up ={ 0.0f, 1.0f, 0.0f };
		static constexpr glm::vec3 Right ={ 1.0f, 0.0f, 0.0f };
		static constexpr glm::vec3 Forward ={ 0.0f, 0.0f, -1.0f };

		TransformComponent( void ) = default;
		TransformComponent( const TransformComponent& ) = default;
		TransformComponent( const glm::vec3& rPosition )
			: Position( rPosition )
		{
		}

		glm::mat4 GetTransform() const
		{
			return glm::translate( glm::mat4( 1.0f ), Position )
				* glm::toMat4( RotationQuat )
				* glm::scale( glm::mat4( 1.0f ), Scale );
		}
		
		void SetTransform( const glm::mat4& rTransfrom )
		{
			Maths::DecomposeTransform( rTransfrom, Position, RotationQuat, Scale );
			Rotation = glm::eulerAngles( RotationQuat );
		}

		// NOTE: Rotation should be specified in euler angles.
		void SetPositionRotationScale( const glm::vec3& rPosition, const glm::vec3& rRotation, const glm::vec3& rScale )
		{
			Position = rPosition;
			SetRotation( Rotation );
			Scale = rScale;
		}

		// NOTE: Rotation should be specified in quaternion form.
		void SetPositionRotationScale( const glm::vec3& rPosition, const glm::quat& rRotation, const glm::vec3& rScale )
		{
			Position = rPosition;
			SetRotation( Rotation );
			Scale = rScale;
		}

		// Where rotation is a euler angle.
		// Rotational values must be radians.
		void SetRotation( const glm::vec3& rotation ) 
		{
			Rotation = rotation;
			RotationQuat = glm::quat( rotation );
		}

		// Rotational values must be radians.
		void SetRotation( const glm::quat& rotation )
		{
			RotationQuat = rotation;
			Rotation = glm::eulerAngles( rotation );
		}

		// Rotational values will be in radians.
		glm::quat GetRotation() const
		{
			return RotationQuat;
		}

		// Rotational values will be in radians.
		glm::vec3 GetRotationEuler() const
		{
			return Rotation;
		}

		operator glm::mat4 ( ) { return GetTransform(); }
		operator const glm::mat4& ( ) const { return GetTransform(); }
	};

	struct TagComponent
	{
		std::string Tag;

		TagComponent() = default;
		TagComponent( const TagComponent& ) = default;
		TagComponent( const std::string& tag ) : Tag( tag ) {}
	};

	struct IdComponent
	{
		UUID ID;

		IdComponent() = default;
		IdComponent( const IdComponent& ) = default;
		IdComponent( const UUID& uuid ) : ID( uuid ) {}
	};

	struct StaticMeshComponent
	{
		MemoryAssetDependency<AssetType::StaticMesh> AssetID;

		// TODO: Change to Asset ID
		Ref<Saturn::StaticMesh> Mesh;
		// We always want to store our own material registry because there will be one in the asset however that is global for all of the same meshes in the scene and what if we want to just locally change one asset.
		Ref<Saturn::MaterialRegistry> MaterialRegistry;

		StaticMeshComponent() = default;
		StaticMeshComponent( const StaticMeshComponent& other ) = default;
		StaticMeshComponent( Ref<Saturn::StaticMesh>& rMesh )
			: Mesh( rMesh ) {}

		operator Ref<Saturn::StaticMesh>() { return Mesh; }
	};

	struct SkeletalMeshComponent
	{
		MemoryAssetDependency<AssetType::SkeletalMesh> AssetID;

		// TODO: Change to Asset ID
		Ref<Saturn::SkeletalMesh> Mesh;
		// We always want to store our own material registry because there will be one in the asset however that is global for all of the same meshes in the scene and what if we want to just locally change one asset.
		Ref<Saturn::MaterialRegistry> MaterialRegistry;
		
		// Animation
		MemoryAssetDependency<AssetType::AnimationController> AnimationControllerAssetID;

		// LocalAnimator is only valid for runtime objects
		Ref<Animator> LocalAnimator;

		AnimatorType AnimatorType = AnimatorType::Single;

		SkeletalMeshComponent() = default;
		SkeletalMeshComponent( const SkeletalMeshComponent& other ) = default;
		SkeletalMeshComponent( Ref<Saturn::SkeletalMesh>& rMesh )
			: Mesh( rMesh ) 
		{
		}

		operator Ref<Saturn::SkeletalMesh>() { return Mesh; }
	};
	
	struct DirectionalLightComponent
	{
		glm::vec3 Radiance = { 1.0f, 1.0f, 1.0f };
		
		float Intensity = 1.0f;
		bool CastShadows = true;
	};

	struct CameraComponent
	{
		Ref<SceneCamera> Camera;
		bool MainCamera = false;
		float Fov = 45.0f;

		CameraComponent() 
		{
			Camera = Ref<SceneCamera>::Create( Fov, 1280.0f, 720.0f );
		}
	};

	// Preetham sky
	struct SkylightComponent 
	{
		EnvironmentMap Map;

		bool DynamicSky = true;

		float Turbidity = 2.0f;
		float Azimuth = 0.0f;
		float Inclination = 0.0f;

		SkylightComponent() = default;
		SkylightComponent( const SkylightComponent& other ) = default;
	};

	struct BoxColliderComponent
	{
		glm::vec3 HalfExtents = { 0.5f, 0.5f, 0.5f };
		glm::vec3 Offset = { 0.0f, 0.0f, 0.0f };

		bool IsTrigger = false;
#if !defined(SAT_DIST) /* SAT_WITH_EDITOR */
		bool AutoAdjustExtent = false;
#endif

		BoxColliderComponent() = default;
		BoxColliderComponent( const glm::vec3& extents ) : HalfExtents( extents ) { }
	};

	struct SphereColliderComponent
	{
		glm::vec3 Offset = { 0.0f, 0.0f, 0.0f };
		float Radius = 1.0f;

		bool IsTrigger = false;

		SphereColliderComponent() = default;
		SphereColliderComponent( float radius ) : Radius( radius ) { }
	};

	struct CapsuleColliderComponent
	{
		glm::vec3 Offset = { 0.0f, 0.0f, 0.0f };

		float Radius = 1.0f;
		float HalfHeight = 1.0f;

		bool IsTrigger = false;

		CapsuleColliderComponent() = default;
		CapsuleColliderComponent( float radius, float height ) : Radius( radius ), HalfHeight( height ) { }
	};

	class PhysicsCharacterController;
	struct CharacterMovementComponent
	{
		PhysicsCharacterController* CharacterMovement = nullptr;

		float StepOffset = 0.3f;
		bool NoGravity = false;
		bool ControlMovementInAir = false;
		bool ControlRotationInAir = false;

		CharacterMovementComponent() = default;
		CharacterMovementComponent( float stepOffset, bool noGravity, bool controlMovementInAir, bool controlRotationInAir ) : StepOffset( stepOffset ), NoGravity( noGravity ), ControlMovementInAir( controlMovementInAir ), ControlRotationInAir( controlRotationInAir ) {}
	};

	// TODO: Do we really want to store the rigid body here?
	class PhysicsRigidBody;
	struct RigidbodyComponent
	{
		PhysicsRigidBody* Rigidbody = nullptr;

		float Mass = 2.0f;
		float LinearDrag = 1.0f;

		PhysicsRigidBodyType BodyType = PhysicsRigidBodyType::Dynamic;
		uint8_t LockFlags = 0;

		MemoryAssetDependency<AssetType::PhysicsMaterial> MaterialAssetID;

		RigidbodyComponent() = default;
		RigidbodyComponent( bool ccd, float mass, float linearDrag, uint8_t lockFlags )
			: Mass( mass ), LinearDrag( linearDrag ), LockFlags( lockFlags )
		{
		}
	};

	struct PointLightComponent
	{
		glm::vec3 Radiance = { 1.0f, 1.0f, 1.0f };
		float Intensity = 1.0f;
		float Multiplier = 1.0f;
		float LightSize = 0.5f;
		float Radius = 10.0f;
		float MinRadius = 1.f;
		float Falloff = 1.f;
	};

	// DEPRECATED IN 0.2.1
	// This component was deprecated in 0.2.1 in favour of using GetStaticClass() and GetClass()
	// There is not a direct replacement to this component. Try use GetStaticClass() or GetClass()
	// ----
	// This is an internal component use for identification
	// This component cannot be added/removed from the Editor
	// The usage of this class is so that we know what class this entity is based from and from this when we are loading and/or spawning in entities we know what class to create
	// Example:
	//  ClassName = Character
	//  ExternalData = 0 (false)
	// 
	// NOTE: This class uses a bitfield!
	//       - Serialising, you must convert ExternalData to byte
	//       - Deserialising, you must convert it from a byte back into a bitfield (var ? 1 : 0)
	// Saturn::ScriptComponent is deprecated and will be removed. There is no direct replacement to the ScriptComponent, try to use SClass instead.
	struct DScriptComponent
	{
		std::string ClassName;
		// Does the class (script) come from the game or the engine, true if comes from the Game
		unsigned int ExternalData : 1 = false;

		DScriptComponent() = default;
		DScriptComponent( const DScriptComponent& other ) = default;
		DScriptComponent( unsigned int externalData ) : ExternalData( externalData ) {}
	};

	struct RelationshipComponent
	{
		UUID Parent = 0;

		std::vector<UUID> ChildrenID;

		RelationshipComponent() = default;
		RelationshipComponent( const RelationshipComponent& other ) = default;
		RelationshipComponent( UUID parent ) : Parent( parent ) {}
	};

	struct PrefabComponent
	{
		MemoryAssetDependency<AssetType::Prefab> AssetID;

		bool Modified = false;

		PrefabComponent() = default;
		PrefabComponent( PrefabComponent& other ) = default;
		PrefabComponent( Saturn::AssetID id ) : AssetID( id ) {}
	};

	class SoundGroup;
	struct AudioPlayerComponent
	{
		MemoryAssetDependency<AssetType::Sound, AssetType::GraphSound> SpecAssetID;
		UUID UniqueID;
		bool Loop = false;
		bool Mute = false;
		bool Spatialisation = false;
		float Volume = 1.0f;
		float Pitch = 1.0f;
		Ref<SoundGroup> SoundGroup = nullptr;
	};

	struct AudioListenerComponent
	{
		bool Primary = false;
		glm::vec3 Direction = TransformComponent::Forward;

		// Radians not degrees
		float ConeInnerAngle = 0.0f;
		float ConeOuterAngle = 0.0f;
	};

	// NOTE: This component only exists in the Editor.
	struct BillboardComponent
	{
		MemoryAssetDependency<AssetType::Texture> AssetID;

		BillboardComponent() = default;
		BillboardComponent( const BillboardComponent& other ) = default;
		BillboardComponent( UUID id ) : AssetID( id ) {}
	};

	struct NavigationMeshSpecificationComponent
	{
		glm::vec3 Extent{};
		unsigned int HasBuilt : 1 = 0;

		NavigationMeshSpecificationComponent() = default;
		NavigationMeshSpecificationComponent( const NavigationMeshSpecificationComponent& other ) = default;
	};

	struct AttachmentPointComponent
	{
		BoneJoint* pBoneJoint = nullptr;
	};

	struct BehaviourTreeComponent
	{
		MemoryAssetDependency<AssetType::BehaviourTree> BehaviourTreeAssetID;

		BehaviourTreeComponent() = default;
		BehaviourTreeComponent( const BehaviourTreeComponent& other ) = default;
		BehaviourTreeComponent( AssetID assetID ) : BehaviourTreeAssetID( assetID ) {}
	};

	struct TextComponent
	{
		std::string Text;
		MemoryAssetDependency<AssetType::Font> FontAssetID;
		glm::vec4 Color = glm::one<glm::vec4>();
	};

	template<typename... V>
	struct ComponentGroup {};

	using AllComponents = ComponentGroup<TransformComponent, TagComponent, IdComponent, RelationshipComponent, PrefabComponent,
		StaticMeshComponent, SkeletalMeshComponent,
		DirectionalLightComponent, SkylightComponent, PointLightComponent,
		CameraComponent,
		BoxColliderComponent, SphereColliderComponent, CapsuleColliderComponent, RigidbodyComponent, CharacterMovementComponent,
		AudioPlayerComponent, AudioListenerComponent,
		BillboardComponent,
		NavigationMeshSpecificationComponent,
		AttachmentPointComponent, 
		BehaviourTreeComponent,
		TextComponent>;

	// Without TagComponent, IdComponent, RelationshipComponent
	// We could use templates and concepts for this however that will add a new layer of complexity and ambiguity.
	using AllDuplicatableComponents = ComponentGroup<TransformComponent, PrefabComponent,
		StaticMeshComponent, SkeletalMeshComponent,
		DirectionalLightComponent, SkylightComponent, PointLightComponent,
		CameraComponent,
		BoxColliderComponent, SphereColliderComponent, CapsuleColliderComponent, RigidbodyComponent, CharacterMovementComponent,
		AudioPlayerComponent, AudioListenerComponent,
		BillboardComponent,
		NavigationMeshSpecificationComponent,
		AttachmentPointComponent, 
		BehaviourTreeComponent,
		TextComponent>;
}
