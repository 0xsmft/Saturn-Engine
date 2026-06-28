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
#include "RawEntitySerialisation.h"

#include "RawSerialisation.h"

#include "Saturn/Audio/SoundGroup.h"

#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Asset/MemoryAssetDependency.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

namespace Saturn {

	template<typename Component, typename Func>
	static void WriteComponent( const SharedPtr<Entity>& rEntity, std::ofstream& rStream, Func Function )
	{
		bool hasT = rEntity->HasComponent<Component>();

		RawSerialisation::WriteObject( hasT, rStream );

		if( hasT )
		{
			Function();
		}
	}

	template<typename Component, typename IStream, typename Func>
	static void ReadComponent( SharedPtr<Entity>& rEntity, IStream& rStream, Func Function )
	{
		bool hadT = false;
		RawSerialisation::ReadObject( hadT, rStream );

		// If the entity ever had Component before then add it and invoke function.
		if( hadT )
		{
			rEntity->AddComponent<Component>();

			Function();
		}
	}

	// Write a MemoryAssetDependency
	template<AssetType... Types>
	static void WriteAssetDependency( const MemoryAssetDependency<Types...>& rDep, std::ofstream& rStream )
	{
		RawSerialisation::WriteObject( rDep.AssetID, rStream );
	}

	// Read a MemoryAssetDependency
	template<AssetType... Types, typename IStream>
	static void ReadAssetDependency( MemoryAssetDependency<Types...>& rDep, IStream& rStream )
	{
		AssetID id = 0;
		RawSerialisation::ReadObject( id, rStream );

		rDep = id;
	}

	void RawEntitySerialisation::SerialiseEntity( const SharedPtr<Entity>& rEntity, std::ofstream& rStream )
	{
		RawSerialisation::WriteObject( rEntity->GetComponent<IdComponent>().ID, rStream );
		RawSerialisation::WriteObject( rEntity->GetHandle(), rStream );

		bool isPrefab = rEntity->HasComponent<PrefabComponent>();
		
		// Tag Component.
		WriteComponent<TagComponent>( rEntity, rStream, [&]() 
			{
				RawSerialisation::WriteString( rEntity->GetComponent<TagComponent>().Tag, rStream );
			} );

		// Transform Component.
		WriteComponent<TransformComponent>( rEntity, rStream, [&]()
			{
				auto& tc = rEntity->GetComponent<TransformComponent>();

				RawSerialisation::WriteVec3( tc.Position, rStream );
				RawSerialisation::WriteVec3( tc.GetRotationEuler(), rStream );
				RawSerialisation::WriteVec3( tc.Scale, rStream );
			} );


		// Relationship Component.
		WriteComponent<RelationshipComponent>( rEntity, rStream, [&]()
			{
				auto& rc = rEntity->GetComponent< RelationshipComponent >();

				RawSerialisation::WriteObject( rc.Parent, rStream );
				RawSerialisation::WriteVector( rc.ChildrenID, rStream );
			} );
		
		
		// Prefab Component
		WriteComponent<PrefabComponent>( rEntity, rStream, [&]()
			{
				auto& pc = rEntity->GetComponent< PrefabComponent >();

				WriteAssetDependency( pc.AssetID, rStream );
				RawSerialisation::WriteObject( pc.Modified, rStream );
			} );

		// Mesh Component
		WriteComponent<StaticMeshComponent>( rEntity, rStream, [&]()
			{
				auto& mc = rEntity->GetComponent< StaticMeshComponent >();

				AssetID ID = 0;

				if( mc.Mesh )
					ID = mc.Mesh->ID;

				RawSerialisation::WriteObject( ID, rStream );

				bool HasRegistry = mc.MaterialRegistry != nullptr;
				RawSerialisation::WriteObject( HasRegistry, rStream );

				// Write local material registry
				// So only write what material have been overridden
				if( HasRegistry ) 
				{
					RawSerialisation::WriteObject( mc.MaterialRegistry->GetMaterialAssets().size(), rStream );

					int i = 0;
					for( const auto& rMaterial : mc.MaterialRegistry->GetMaterialAssets() )
					{
						if( mc.MaterialRegistry->HasOverrides( i ) )
						{
							RawSerialisation::WriteObject( rMaterial->ID, rStream );
						}
						else
						{
							const AssetID matID = 0;
							RawSerialisation::WriteObject( matID, rStream );
						}

						++i;
					}
				}
				//	MaterialRegistry::Serialise( mc.MaterialRegistry, rStream );
			} );

		// Dynamic Mesh Component
		WriteComponent<SkeletalMeshComponent>( rEntity, rStream, [ & ]()
			{
				auto& mc = rEntity->GetComponent< SkeletalMeshComponent >();

				AssetID ID = 0;

				if( mc.Mesh )
					ID = mc.Mesh->ID;

				RawSerialisation::WriteObject( ID, rStream );

				bool HasRegistry = mc.MaterialRegistry != nullptr;
				RawSerialisation::WriteObject( HasRegistry, rStream );

				// Write local material registry
				// So only write what material have been overridden
				if( HasRegistry )
				{
					RawSerialisation::WriteObject( mc.MaterialRegistry->GetMaterialAssets().size(), rStream );

					int i = 0;
					for( const auto& rMaterial : mc.MaterialRegistry->GetMaterialAssets() )
					{
						if( mc.MaterialRegistry->HasOverrides( i ) )
						{
							RawSerialisation::WriteObject( rMaterial->ID, rStream );
						}
						else
						{
							const AssetID matID = 0;
							RawSerialisation::WriteObject( matID, rStream );
						}

						++i;
					}
				}

				RawSerialisation::WriteObject( mc.AnimationControllerAssetID, rStream );
				RawSerialisation::WriteObject( ( uint8_t ) mc.AnimatorType, rStream );
			} );

		// Sky light component
		WriteComponent<SkylightComponent>( rEntity, rStream, [&]()
			{
				auto& slc = rEntity->GetComponent< SkylightComponent >();

				RawSerialisation::WriteObject( slc.DynamicSky, rStream );

				if( slc.DynamicSky )
				{
					RawSerialisation::WriteObject( slc.Turbidity, rStream );
					RawSerialisation::WriteObject( slc.Azimuth, rStream );
					RawSerialisation::WriteObject( slc.Inclination, rStream );
				}
			} );

		// Directional Light Component
		WriteComponent<DirectionalLightComponent>( rEntity, rStream, [&]()
			{
				auto& dlc = rEntity->GetComponent< DirectionalLightComponent>();

				RawSerialisation::WriteVec3( dlc.Radiance, rStream );
				RawSerialisation::WriteObject( dlc.Intensity, rStream );
				RawSerialisation::WriteObject( dlc.CastShadows, rStream );
			} );

		// Point Light Component
		WriteComponent<PointLightComponent>( rEntity, rStream, [&]()
			{
				auto& plc = rEntity->GetComponent< PointLightComponent >();

				RawSerialisation::WriteVec3( plc.Radiance, rStream );
				RawSerialisation::WriteObject( plc.Intensity, rStream );
				RawSerialisation::WriteObject( plc.Falloff, rStream );
				RawSerialisation::WriteObject( plc.LightSize, rStream );
				RawSerialisation::WriteObject( plc.MinRadius, rStream );
				RawSerialisation::WriteObject( plc.Multiplier, rStream );
				RawSerialisation::WriteObject( plc.Radius, rStream );
			} );

		// Box collider
		WriteComponent<BoxColliderComponent>( rEntity, rStream, [&]()
			{
				auto& bcc = rEntity->GetComponent< BoxColliderComponent >();

				RawSerialisation::WriteVec3( bcc.HalfExtents, rStream );
				RawSerialisation::WriteVec3( bcc.Offset, rStream );
				RawSerialisation::WriteObject( bcc.IsTrigger, rStream );
				RawSerialisation::WriteObject( bcc.AutoAdjustExtent, rStream );
			} );
		
		// Sphere collider
		WriteComponent<SphereColliderComponent>( rEntity, rStream, [&]()
			{
				auto& scc = rEntity->GetComponent< SphereColliderComponent >();

				RawSerialisation::WriteObject( scc.Radius, rStream );
				RawSerialisation::WriteVec3( scc.Offset, rStream );
				RawSerialisation::WriteObject( scc.IsTrigger, rStream );
			} );

		// Capsule collider
		WriteComponent<CapsuleColliderComponent>( rEntity, rStream, [&]()
			{
				auto& ccc = rEntity->GetComponent< CapsuleColliderComponent >();

				RawSerialisation::WriteObject( ccc.HalfHeight, rStream );
				RawSerialisation::WriteObject( ccc.Radius, rStream );
				RawSerialisation::WriteVec3( ccc.Offset, rStream );
				RawSerialisation::WriteObject( ccc.IsTrigger, rStream );
			} );

		// Rigid body
		WriteComponent<RigidbodyComponent>( rEntity, rStream, [&]()
			{
				auto& rbc = rEntity->GetComponent< RigidbodyComponent >();

				RawSerialisation::WriteObject( rbc.BodyType, rStream );
				RawSerialisation::WriteObject( rbc.Mass, rStream );
				RawSerialisation::WriteObject( rbc.LockFlags, rStream );
			} );

		// Character Movement Component
		WriteComponent<CharacterMovementComponent>( rEntity, rStream, [ & ]()
			{
				auto& cmc = rEntity->GetComponent< CharacterMovementComponent >();

				RawSerialisation::WriteObject( cmc.StepOffset, rStream );
				RawSerialisation::WriteObject( cmc.NoGravity, rStream );
				RawSerialisation::WriteObject( cmc.ControlMovementInAir, rStream );
				RawSerialisation::WriteObject( cmc.ControlRotationInAir, rStream );
			} );

		// Camera Component
		WriteComponent<CameraComponent>( rEntity, rStream, [&]()
			{
				auto& cc = rEntity->GetComponent< CameraComponent >();

				RawSerialisation::WriteObject( cc.MainCamera, rStream );
			} );

		// Audio Player Component
		WriteComponent<AudioPlayerComponent>( rEntity, rStream, [&]()
			{
				auto& spc = rEntity->GetComponent< AudioPlayerComponent >();

				WriteAssetDependency( spc.SpecAssetID, rStream );
				RawSerialisation::WriteObject( spc.Loop, rStream );
				RawSerialisation::WriteObject( spc.Mute, rStream );
				RawSerialisation::WriteObject( spc.Spatialisation, rStream );
				RawSerialisation::WriteObject( spc.Volume, rStream );
				RawSerialisation::WriteObject( spc.Pitch, rStream );
			} );

		// Audio Listener Component
		WriteComponent<AudioListenerComponent>( rEntity, rStream, [&]()
			{
				auto& alc = rEntity->GetComponent< AudioListenerComponent >();

				RawSerialisation::WriteObject( alc.Primary, rStream );
				RawSerialisation::WriteVec3( alc.Direction, rStream );
				RawSerialisation::WriteObject( alc.ConeInnerAngle, rStream );
				RawSerialisation::WriteObject( alc.ConeOuterAngle, rStream );
			} );

		// Navigation Mesh Specification Component
		WriteComponent<NavigationMeshSpecificationComponent>( rEntity, rStream, [ & ]()
			{
				auto& nmsc = rEntity->GetComponent< NavigationMeshSpecificationComponent >();
				
				RawSerialisation::WriteVec3( nmsc.Extent, rStream );

				uint8_t bit = nmsc.HasBuilt;
				rStream.write( reinterpret_cast< const char* >( &bit ), sizeof( bit ) );
			} );

		// Behaviour Tree Component
		WriteComponent<BehaviourTreeComponent>( rEntity, rStream, [ & ]()
			{
				auto& btc = rEntity->GetComponent< BehaviourTreeComponent >();
				WriteAssetDependency( btc.BehaviourTreeAssetID, rStream );
			} );

		// Text Component
		WriteComponent<TextComponent>( rEntity, rStream, [ & ]()
			{
				auto& rTextComponent = rEntity->GetComponent< TextComponent>();
				WriteAssetDependency( rTextComponent.FontAssetID, rStream );
				RawSerialisation::WriteVec4( rTextComponent.Color, rStream );
				RawSerialisation::WriteString( rTextComponent.Text, rStream );
			} );
	}

	void RawEntitySerialisation::DeserialiseEntity( SharedPtr<Entity>& rEntity, std::istream& rStream )
	{
		RawSerialisation::ReadObject( rEntity->GetComponent<IdComponent>().ID, rStream );
		
		entt::entity handle{ entt::null };
		RawSerialisation::ReadObject( handle, rStream );

		// Tag Component
		ReadComponent<TagComponent>( rEntity, rStream, [&]()
			{
				rEntity->GetComponent<TagComponent>().Tag = RawSerialisation::ReadString( rStream );
			} );

		// Transform Component
		ReadComponent<TransformComponent>( rEntity, rStream, [&]()
			{
				auto& tc = rEntity->GetComponent<TransformComponent>();

				glm::vec3 rotation{};

				RawSerialisation::ReadVec3( tc.Position, rStream );
				RawSerialisation::ReadVec3( rotation, rStream );
				RawSerialisation::ReadVec3( tc.Scale, rStream );

				tc.SetRotation( rotation );
			} );

		// Relationship Component
		ReadComponent<RelationshipComponent>( rEntity, rStream, [&]()
			{
				auto& rc = rEntity->GetComponent< RelationshipComponent >();

				RawSerialisation::ReadObject( rc.Parent, rStream );
				RawSerialisation::ReadVector( rc.ChildrenID, rStream );
			} );

		// Prefab Component
		ReadComponent<PrefabComponent>( rEntity, rStream, [&]()
			{
				auto& pc = rEntity->GetComponent< PrefabComponent >();

				ReadAssetDependency( pc.AssetID, rStream );
				RawSerialisation::ReadObject( pc.Modified, rStream );
			} );

		// Mesh Component
		ReadComponent<StaticMeshComponent>( rEntity, rStream, [&]()
			{
				auto& mc = rEntity->GetComponent< StaticMeshComponent >();

				AssetID ID = 0;
				RawSerialisation::ReadObject( ID, rStream );

				bool HasRegistry = false;
				RawSerialisation::ReadObject( HasRegistry, rStream );

				mc.MaterialRegistry = Ref<MaterialRegistry>::Create();

				// Load the mesh before materials
				if( ID != 0 )
				{
					// Load Mesh 
					// Hand off to RawStaticMeshAssetSerialiser
					auto mesh = AssetManager::Get()->GetAssetAs<StaticMesh>( ID );
					mc.Mesh = mesh;
				}

				// Now, build local material registry
				if( HasRegistry )
				{
					size_t materials = 0;
					RawSerialisation::ReadObject( materials, rStream );

					for( size_t i = 0; i < materials; ++i )
					{
						AssetID materialID = 0;
						RawSerialisation::ReadObject( materialID, rStream );

						// Load material asset
						// Will call RawMaterialAssetSerialiser
						Ref<MaterialAsset> asset = AssetManager::Get()->GetAssetAs<MaterialAsset>( materialID );

						if( asset )
						{
							mc.MaterialRegistry->AddAsset( asset );
							mc.MaterialRegistry->SetOverrides( ( uint32_t ) i, true );
						}
					}
				}

				// If no overrides then copy the master registry
				if( !mc.MaterialRegistry->HasAnyOverrides() )
				{
					mc.MaterialRegistry->Copy( mc.Mesh->GetMaterialRegistry() );
				}
			} );

		// Dynamic Mesh Component
		ReadComponent<SkeletalMeshComponent>( rEntity, rStream, [ & ]()
			{
				auto& mc = rEntity->GetComponent< SkeletalMeshComponent >();

				AssetID ID = 0;
				RawSerialisation::ReadObject( ID, rStream );

				bool HasRegistry = false;
				RawSerialisation::ReadObject( HasRegistry, rStream );

				mc.MaterialRegistry = Ref<MaterialRegistry>::Create();

				// Load the mesh before materials
				if( ID != 0 )
				{
					// Load Mesh 
					// Hand off to RawSkeletalMeshAssetSerialiser
					auto mesh = AssetManager::Get()->GetAssetAs<SkeletalMesh>( ID );
					mc.Mesh = mesh;
				}

				// Now, build local material registry
				if( HasRegistry )
				{
					size_t materials = 0;
					RawSerialisation::ReadObject( materials, rStream );

					for( size_t i = 0; i < materials; ++i )
					{
						AssetID materialID = 0;
						RawSerialisation::ReadObject( materialID, rStream );

						// Load material asset
						// Will call RawMaterialAssetSerialiser
						Ref<MaterialAsset> asset = AssetManager::Get()->GetAssetAs<MaterialAsset>( materialID );

						if( asset )
						{
							mc.MaterialRegistry->AddAsset( asset );
							mc.MaterialRegistry->SetOverrides( ( uint32_t ) i, true );
						}
					}
				}

				// If no overrides then copy the master registry
				if( !mc.MaterialRegistry->HasAnyOverrides() )
				{
					mc.MaterialRegistry->Copy( mc.Mesh->GetMaterialRegistry() );
				}
				
				// TODO: For some reason we don't read the same amount of data that we wrote...
				//		 we are off by 8-bytes.
				uint64_t x = 0;
				RawSerialisation::ReadObject( x, rStream );

				RawSerialisation::ReadObject( mc.AnimationControllerAssetID, rStream );
				
				uint8_t type = 0u;
				RawSerialisation::ReadObject( type, rStream );

				mc.AnimatorType = ( AnimatorType ) type;
			} );

		// Sky light component
		ReadComponent<SkylightComponent>( rEntity, rStream, [&]()
			{
				auto& slc = rEntity->GetComponent< SkylightComponent >();

				RawSerialisation::ReadObject( slc.DynamicSky, rStream );

				if( slc.DynamicSky )
				{
					RawSerialisation::ReadObject( slc.Turbidity, rStream );
					RawSerialisation::ReadObject( slc.Azimuth, rStream );
					RawSerialisation::ReadObject( slc.Inclination, rStream );
				}
			} );
		
		// Directional Light Component
		ReadComponent<DirectionalLightComponent>( rEntity, rStream, [&]()
			{
				auto& dlc = rEntity->GetComponent< DirectionalLightComponent>();

				RawSerialisation::ReadVec3( dlc.Radiance, rStream );
				RawSerialisation::ReadObject( dlc.Intensity, rStream );
				RawSerialisation::ReadObject( dlc.CastShadows, rStream );
			} );

		// Point Light Component
		ReadComponent<PointLightComponent>( rEntity, rStream, [&]()
			{
				auto& plc = rEntity->GetComponent< PointLightComponent >();

				RawSerialisation::ReadVec3( plc.Radiance, rStream );
				RawSerialisation::ReadObject( plc.Intensity, rStream );
				RawSerialisation::ReadObject( plc.Falloff, rStream );
				RawSerialisation::ReadObject( plc.LightSize, rStream );
				RawSerialisation::ReadObject( plc.MinRadius, rStream );
				RawSerialisation::ReadObject( plc.Multiplier, rStream );
				RawSerialisation::ReadObject( plc.Radius, rStream );
			} );

		// Box collider
		ReadComponent<BoxColliderComponent>( rEntity, rStream, [&]()
			{
				auto& bcc = rEntity->GetComponent< BoxColliderComponent >();

				RawSerialisation::ReadVec3( bcc.HalfExtents, rStream );
				RawSerialisation::ReadVec3( bcc.Offset, rStream );
				RawSerialisation::ReadObject( bcc.IsTrigger, rStream );
				RawSerialisation::ReadObject( bcc.AutoAdjustExtent, rStream );
			} );
		
		// Sphere collider
		ReadComponent<SphereColliderComponent>( rEntity, rStream, [&]()
			{
				auto& scc = rEntity->GetComponent< SphereColliderComponent >();

				RawSerialisation::ReadObject( scc.Radius, rStream );
				RawSerialisation::ReadVec3( scc.Offset, rStream );
				RawSerialisation::ReadObject( scc.IsTrigger, rStream );
			} );
		
		// Capsule collider
		ReadComponent<CapsuleColliderComponent>( rEntity, rStream, [&]()
			{
				auto& ccc = rEntity->GetComponent< CapsuleColliderComponent >();

				RawSerialisation::ReadObject( ccc.HalfHeight, rStream );
				RawSerialisation::ReadObject( ccc.Radius, rStream );
				RawSerialisation::ReadVec3( ccc.Offset, rStream );
				RawSerialisation::ReadObject( ccc.IsTrigger, rStream );
			} );
		
		// Rigid body
		ReadComponent<RigidbodyComponent>( rEntity, rStream, [&]()
			{
				auto& rbc = rEntity->GetComponent< RigidbodyComponent >();

				RawSerialisation::ReadObject( rbc.BodyType, rStream );
				RawSerialisation::ReadObject( rbc.Mass, rStream );
				RawSerialisation::ReadObject( rbc.LockFlags, rStream );
			} );
		
		// Character Movement Component
		ReadComponent<CharacterMovementComponent>( rEntity, rStream, [&]()
			{
				auto& cmc = rEntity->GetComponent< CharacterMovementComponent >();

				RawSerialisation::ReadObject( cmc.StepOffset, rStream );
				RawSerialisation::ReadObject( cmc.NoGravity, rStream );
				RawSerialisation::ReadObject( cmc.ControlMovementInAir, rStream );
				RawSerialisation::ReadObject( cmc.ControlRotationInAir, rStream );
			} );

		// Camera Component
		ReadComponent<CameraComponent>( rEntity, rStream, [&]()
			{
				auto& cc = rEntity->GetComponent< CameraComponent >();

				RawSerialisation::ReadObject( cc.MainCamera, rStream );
			} );

		// Audio Player Component
		ReadComponent<AudioPlayerComponent>( rEntity, rStream, [&]()
			{
				auto& spc = rEntity->GetComponent< AudioPlayerComponent >();

				ReadAssetDependency( spc.SpecAssetID, rStream );
				RawSerialisation::ReadObject( spc.Loop, rStream );
				RawSerialisation::ReadObject( spc.Mute, rStream );
				RawSerialisation::ReadObject( spc.Spatialisation, rStream );
				RawSerialisation::ReadObject( spc.Volume, rStream );
				RawSerialisation::ReadObject( spc.Pitch, rStream );
			} );

		// Audio Listener Component
		ReadComponent<AudioListenerComponent>( rEntity, rStream, [&]()
			{
				auto& alc = rEntity->GetComponent< AudioListenerComponent >();

				RawSerialisation::ReadObject( alc.Primary, rStream );
				RawSerialisation::ReadVec3( alc.Direction, rStream );
				RawSerialisation::ReadObject( alc.ConeInnerAngle, rStream );
				RawSerialisation::ReadObject( alc.ConeOuterAngle, rStream );
			} );

		// Navigation Mesh Specification Component
		ReadComponent<NavigationMeshSpecificationComponent>( rEntity, rStream, [ & ]()
			{
				auto& nmsc = rEntity->GetComponent< NavigationMeshSpecificationComponent >();
				RawSerialisation::ReadVec3( nmsc.Extent, rStream );
			
				uint8_t bit = 0;
				rStream.read( reinterpret_cast< char* >( &bit ), sizeof( bit ) );
				nmsc.HasBuilt = bit ? true : false;
			} );

		// Behaviour Tree Component
		ReadComponent<BehaviourTreeComponent>( rEntity, rStream, [ & ]()
			{
				auto& btc = rEntity->GetComponent< BehaviourTreeComponent>();
				ReadAssetDependency( btc.BehaviourTreeAssetID, rStream );
			} );

		// Text Component
		ReadComponent<TextComponent>( rEntity, rStream, [ & ]()
			{
				auto& rTextComponent = rEntity->GetComponent< TextComponent>();
				ReadAssetDependency( rTextComponent.FontAssetID, rStream );
				RawSerialisation::ReadVec4( rTextComponent.Color, rStream );
				rTextComponent.Text = RawSerialisation::ReadString( rStream );
			} );
	}

}
