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
#include "EntitySerialisation.h"

#include "YamlAux.h"

#include "Saturn/Audio/SoundGroup.h"

#include "Saturn/Scene/Scene.h"
#include "Saturn/Scene/Components.h"

#include "Saturn/Asset/AssetManager.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// ENTITY SERIALSATION

	void EntitySerialisation::SerialiseEntity( YAML::Emitter& rEmitter, const SharedPtr<Entity> entity )
	{
		rEmitter << YAML::BeginMap;
		rEmitter << YAML::Key << "Entity" << YAML::Value << entity->GetComponent< IdComponent >().ID;
		rEmitter << YAML::Key << "Class" << YAML::Value << entity->GetClass()->GetName();

		ComponentSerialisation::SerialiseComponents( rEmitter, entity );

		rEmitter << YAML::EndMap;
	}

	void EntitySerialisation::DeserialiseEntity( const YAML::Node& rNode, Ref<Scene> scene )
	{
		ComponentSerialisation::DeserialiseComponents( rNode, scene );
	}

	//////////////////////////////////////////////////////////////////////////
	// COMPONENT SERIALSATION

	void ComponentSerialisation::SerialiseComponents( YAML::Emitter& rEmitter, const SharedPtr<Entity> entity )
	{
		// Tag Component
		if( entity->HasComponent<TagComponent>() )
		{
			rEmitter << YAML::Key << "TagComponent";
			rEmitter << YAML::BeginMap;

			rEmitter << YAML::Key << "Tag" << YAML::Value << entity->GetComponent< TagComponent >().Tag;

			rEmitter << YAML::EndMap;
		}

		{
			rEmitter << YAML::Key << "ClassInformation";
			rEmitter << YAML::BeginMap;

			const auto propCount = entity->GetClass()->GetPropertyCount();
			rEmitter << YAML::Key << "LastPropertyCount" << YAML::Value << propCount;

			rEmitter << YAML::Key << "Properties";
			rEmitter << YAML::BeginSeq;

			const auto* pProperties = entity->GetClass()->GetProperties();
			for( int i = 0; i < propCount; ++i )
			{
#define SAT_SERIALISE_PROPERTY_YAML( PropertyType ) \
{ \
typename PropertyTypeTraits<Saturn::SPropertyType::PropertyType>::Type value = pProperty->Read<Saturn::SPropertyType::PropertyType>( entity.Get() ); \
rEmitter << YAML::Key << "Value" << YAML::Value << value; \
} break

				const SProperty* pProperty = pProperties[ i ];

				rEmitter << YAML::BeginMap;
				rEmitter << YAML::Key << "Name" << YAML::Value << pProperty->GetName();

				rEmitter << YAML::Key << "ValueType" << YAML::Value << ( int ) pProperty->GetType();

				switch( pProperty->GetType() )
				{
					case SPropertyType::Char:
						SAT_SERIALISE_PROPERTY_YAML( Char );

					case SPropertyType::Float:
						SAT_SERIALISE_PROPERTY_YAML( Float );

					case SPropertyType::Int:
						SAT_SERIALISE_PROPERTY_YAML( Int );

					case SPropertyType::Double:
						SAT_SERIALISE_PROPERTY_YAML( Double );

					case SPropertyType::Uint8:
						SAT_SERIALISE_PROPERTY_YAML( Uint8 );

					case SPropertyType::Uint16:
						SAT_SERIALISE_PROPERTY_YAML( Uint16 );

					case SPropertyType::Uint32:
						SAT_SERIALISE_PROPERTY_YAML( Uint32 );

					case SPropertyType::Uint64:
						SAT_SERIALISE_PROPERTY_YAML( Uint64 );

					case SPropertyType::Int8:
						SAT_SERIALISE_PROPERTY_YAML( Int8 );

					case SPropertyType::Int16:
						SAT_SERIALISE_PROPERTY_YAML( Int16 );

					case SPropertyType::Int64:
						SAT_SERIALISE_PROPERTY_YAML( Int64 );

					case SPropertyType::Vector2:
					{
						auto& temporaryValue = pProperty->Read<SPropertyType::Vector2>( entity.Get() );
						rEmitter << YAML::Key << "Value" << YAML::Value << temporaryValue;
					} break;

					case SPropertyType::Vector3:
					{
						auto& temporaryValue = pProperty->Read<SPropertyType::Vector3>( entity.Get() );
						rEmitter << YAML::Key << "Value" << YAML::Value << temporaryValue;
					} break;

					case SPropertyType::Vector4:
					{
						auto& temporaryValue = pProperty->Read<SPropertyType::Vector4>( entity.Get() );
						rEmitter << YAML::Key << "Value" << YAML::Value << temporaryValue;
					} break;

					case SPropertyType::String:
						SAT_SERIALISE_PROPERTY_YAML( String );

					case SPropertyType::EntityType:
					{
						SharedPtr<Entity>& rEntity = pProperty->Read<Saturn::SPropertyType::EntityType>( entity.Get() );

						if( rEntity )
							rEmitter << YAML::Key << "Value" << YAML::Value << rEntity->GetUUID();
						else
							rEmitter << YAML::Key << "Value" << YAML::Value << 0;
					} break;

					case SPropertyType::Asset:
					{
						const AssetReference& rAssetReference = pProperty->Read<Saturn::SPropertyType::Asset>( entity.Get() );

						rEmitter << YAML::Key << "Value" << YAML::Value << rAssetReference.ID;
						rEmitter << YAML::Key << "ExpectedType" << YAML::Value << ( int ) rAssetReference.ExpectedType;
					} break;
				}

				rEmitter << YAML::EndMap;
			}

			rEmitter << YAML::EndSeq;
			rEmitter << YAML::EndMap;
		}

		// Transform Component
		if( entity->HasComponent<TransformComponent>() )
		{
			rEmitter << YAML::Key << "TransformComponent";
			rEmitter << YAML::BeginMap;

			const auto& tc = entity->GetComponent< TransformComponent >();

			rEmitter << YAML::Key << "Position" << YAML::Value << tc.Position;
			rEmitter << YAML::Key << "Rotation" << YAML::Value << glm::degrees( tc.GetRotationEuler() );
			rEmitter << YAML::Key << "Scale" << YAML::Value << tc.Scale;

			rEmitter << YAML::EndMap;
		}

		// Relationship Component
		if( entity->HasComponent<RelationshipComponent>() )
		{
			rEmitter << YAML::Key << "RelationshipComponent";
			rEmitter << YAML::BeginMap;

			const auto& rc = entity->GetComponent< RelationshipComponent >();

			rEmitter << YAML::Key << "Parent" << YAML::Value << rc.Parent;

			rEmitter << YAML::Key << "Children";
			rEmitter << YAML::BeginSeq;

			for( const auto& id : rc.ChildrenID )
			{
				rEmitter << YAML::BeginMap;

				rEmitter << YAML::Key << "ID" << YAML::Value << id;

				rEmitter << YAML::EndMap;
			}

			rEmitter << YAML::EndSeq;

			rEmitter << YAML::EndMap;
		}

		// Prefab Component
		if( entity->HasComponent<PrefabComponent>() )
		{
			rEmitter << YAML::Key << "PrefabComponent";
			rEmitter << YAML::BeginMap;

			rEmitter << YAML::Key << "AssetID" << YAML::Value << entity->GetComponent< PrefabComponent >().AssetID;
			rEmitter << YAML::Key << "EntityIDInPrefab" << YAML::Value << entity->GetComponent< PrefabComponent >().EntityIDInPrefab;
			rEmitter << YAML::Key << "Modified" << YAML::Value << entity->GetComponent< PrefabComponent >().Modified;

			rEmitter << YAML::EndMap;
		}

		// Mesh Component
		if( entity->HasComponent<StaticMeshComponent>() )
		{
			rEmitter << YAML::Key << "MeshComponent";
			rEmitter << YAML::BeginMap;

			const auto& mc = entity->GetComponent< StaticMeshComponent >();

			if( mc.Mesh )
				rEmitter << YAML::Key << "Asset" << YAML::Value << mc.Mesh->ID;
			else
				rEmitter << YAML::Key << "Asset" << YAML::Value << 0;

			rEmitter << YAML::Key << "MaterialRegistry";
			rEmitter << YAML::BeginMap;

			if( mc.MaterialRegistry )
				rEmitter << YAML::Key << "AnyOverrides" << YAML::Value << mc.MaterialRegistry->HasAnyOverrides();
			else
				rEmitter << YAML::Key << "AnyOverrides" << YAML::Value << false;

			rEmitter << YAML::Key << "MaterialOverrides";
			rEmitter << YAML::BeginSeq;

			if( mc.MaterialRegistry )
			{
				int i = 0;
				for( const auto& material : mc.MaterialRegistry->GetMaterialAssets() )
				{
					rEmitter << YAML::BeginMap;

					if( mc.MaterialRegistry->HasOverrides( i ) )
						rEmitter << YAML::Key << i << YAML::Value << material->ID;
					else
						rEmitter << YAML::Key << i << YAML::Value << 0;

					rEmitter << YAML::EndMap;

					++i;
				}
			}

			rEmitter << YAML::EndSeq;

			rEmitter << YAML::EndMap;

			rEmitter << YAML::EndMap;
		}

		// Dynamic Mesh Component
		if( entity->HasComponent<SkeletalMeshComponent>() )
		{
			rEmitter << YAML::Key << "SkeletalMeshComponent";
			rEmitter << YAML::BeginMap;

			const auto& mc = entity->GetComponent< SkeletalMeshComponent >();

			if( mc.Mesh )
				rEmitter << YAML::Key << "Asset" << YAML::Value << mc.Mesh->ID;
			else
				rEmitter << YAML::Key << "Asset" << YAML::Value << 0;

			// We have to serialise as uint16_t or greater
			// because if we do it as a uint8_t it may write it as "\x01" and will refuse to read it back...
			rEmitter << YAML::Key << "AnimationAssetType" << YAML::Value << ( uint16_t )mc.AnimatorType;
			rEmitter << YAML::Key << "AnimationAsset" << YAML::Value << mc.AnimationControllerAssetID;

			rEmitter << YAML::Key << "MaterialRegistry";
			rEmitter << YAML::BeginMap;

			if( mc.MaterialRegistry )
				rEmitter << YAML::Key << "AnyOverrides" << YAML::Value << mc.MaterialRegistry->HasAnyOverrides();
			else
				rEmitter << YAML::Key << "AnyOverrides" << YAML::Value << false;

			rEmitter << YAML::Key << "MaterialOverrides";
			rEmitter << YAML::BeginSeq;

			if( mc.MaterialRegistry )
			{
				int i = 0;
				for( const auto& material : mc.MaterialRegistry->GetMaterialAssets() )
				{
					rEmitter << YAML::BeginMap;

					if( mc.MaterialRegistry->HasOverrides( i ) )
						rEmitter << YAML::Key << i << YAML::Value << material->ID;
					else
						rEmitter << YAML::Key << i << YAML::Value << 0;

					rEmitter << YAML::EndMap;

					++i;
				}
			}

			rEmitter << YAML::EndSeq;

			rEmitter << YAML::EndMap;

			rEmitter << YAML::EndMap;
		}

		// Sky light component
		if( entity->HasComponent<SkylightComponent>() )
		{
			rEmitter << YAML::Key << "SkyLightComponent";
			rEmitter << YAML::BeginMap;

			const auto& slc = entity->GetComponent< SkylightComponent >();

			rEmitter << YAML::Key << "IsPreetham" << YAML::Value << slc.DynamicSky;

			if( slc.DynamicSky )
			{
				rEmitter << YAML::Key << "Preetham Settings" << YAML::Value;
				rEmitter << YAML::BeginMap;

				rEmitter << YAML::Key << "Turbidity" << YAML::Value << slc.Turbidity;
				rEmitter << YAML::Key << "Azimuth" << YAML::Value << slc.Azimuth;
				rEmitter << YAML::Key << "Inclination" << YAML::Value << slc.Inclination;

				rEmitter << YAML::EndMap;
			}

			rEmitter << YAML::EndMap;
		}

		// Directional Light Component
		if( entity->HasComponent<DirectionalLightComponent>() )
		{
			rEmitter << YAML::Key << "DirectionalLightComponent";
			rEmitter << YAML::BeginMap;

			const auto& dlc = entity->GetComponent< DirectionalLightComponent >();

			rEmitter << YAML::Key << "Radiance" << YAML::Value << dlc.Radiance;
			rEmitter << YAML::Key << "Intensity" << YAML::Value << dlc.Intensity;
			rEmitter << YAML::Key << "CastShadows" << YAML::Value << dlc.CastShadows;

			rEmitter << YAML::EndMap;
		}

		// Point Light Component
		if( entity->HasComponent<PointLightComponent>() )
		{
			rEmitter << YAML::Key << "PointLightComponent";
			rEmitter << YAML::BeginMap;

			const auto& plc = entity->GetComponent< PointLightComponent >();

			rEmitter << YAML::Key << "Radiance" << YAML::Value << plc.Radiance;
			rEmitter << YAML::Key << "Intensity" << YAML::Value << plc.Intensity;
			rEmitter << YAML::Key << "Falloff" << YAML::Value << plc.Falloff;
			rEmitter << YAML::Key << "LightSize" << YAML::Value << plc.LightSize;
			rEmitter << YAML::Key << "MinRadius" << YAML::Value << plc.MinRadius;
			rEmitter << YAML::Key << "Multiplier" << YAML::Value << plc.Multiplier;
			rEmitter << YAML::Key << "Radius" << YAML::Value << plc.Radius;

			rEmitter << YAML::EndMap;
		}

		// Box collider
		if( entity->HasComponent<BoxColliderComponent>() )
		{
			rEmitter << YAML::Key << "BoxColliderComponent";
			rEmitter << YAML::BeginMap;

			const auto& bcc = entity->GetComponent< BoxColliderComponent >();

			rEmitter << YAML::Key << "Extents" << YAML::Value << bcc.HalfExtents;
			rEmitter << YAML::Key << "Offset" << YAML::Value << bcc.Offset;
			rEmitter << YAML::Key << "IsTrigger" << YAML::Value << bcc.IsTrigger;
			rEmitter << YAML::Key << "AutoAdjustExtent" << YAML::Value << bcc.AutoAdjustExtent;

			rEmitter << YAML::EndMap;
		}

		// Sphere collider
		if( entity->HasComponent<SphereColliderComponent>() )
		{
			rEmitter << YAML::Key << "SphereColliderComponent";
			rEmitter << YAML::BeginMap;

			const auto& scc = entity->GetComponent< SphereColliderComponent >();

			rEmitter << YAML::Key << "Radius" << YAML::Value << scc.Radius;
			rEmitter << YAML::Key << "Offset" << YAML::Value << scc.Offset;
			rEmitter << YAML::Key << "IsTrigger" << YAML::Value << scc.IsTrigger;

			rEmitter << YAML::EndMap;
		}

		// Capsule collider
		if( entity->HasComponent<CapsuleColliderComponent>() )
		{
			rEmitter << YAML::Key << "CapsuleColliderComponent";
			rEmitter << YAML::BeginMap;

			const auto& ccc = entity->GetComponent< CapsuleColliderComponent >();

			rEmitter << YAML::Key << "Height" << YAML::Value << ccc.HalfHeight;
			rEmitter << YAML::Key << "Radius" << YAML::Value << ccc.Radius;
			rEmitter << YAML::Key << "Offset" << YAML::Value << ccc.Offset;
			rEmitter << YAML::Key << "IsTrigger" << YAML::Value << ccc.IsTrigger;

			rEmitter << YAML::EndMap;
		}

		// Rigid body
		if( entity->HasComponent<RigidbodyComponent>() )
		{
			rEmitter << YAML::Key << "RigidbodyComponent";
			rEmitter << YAML::BeginMap;

			const auto& rbc = entity->GetComponent< RigidbodyComponent >();

			rEmitter << YAML::Key << "BodyType" << YAML::Value << ( uint32_t ) rbc.BodyType;
			rEmitter << YAML::Key << "Mass" << YAML::Value << rbc.Mass;

			rEmitter << YAML::Key << "LockFlags" << YAML::Value << ( int ) rbc.LockFlags;

			rEmitter << YAML::EndMap;
		}

		// Character Movement Component
		if( entity->HasComponent<CharacterMovementComponent>() )
		{
			rEmitter << YAML::Key << "CharacterMovementComponent";
			rEmitter << YAML::BeginMap;

			const auto& cmc = entity->GetComponent< CharacterMovementComponent >();

			rEmitter << YAML::Key << "StepOffset" << YAML::Value << cmc.StepOffset;
			rEmitter << YAML::Key << "NoGravity" << YAML::Value << cmc.NoGravity;
			rEmitter << YAML::Key << "ControlMovementInAir" << YAML::Value << cmc.ControlMovementInAir;
			rEmitter << YAML::Key << "ControlRotationInAir" << YAML::Value << cmc.ControlRotationInAir;

			rEmitter << YAML::EndMap;
		}

		// Camera Component
		if( entity->HasComponent<CameraComponent>() )
		{
			rEmitter << YAML::Key << "CameraComponent";
			rEmitter << YAML::BeginMap;

			const auto& cc = entity->GetComponent< CameraComponent >();

			rEmitter << YAML::Key << "MainCamera" << YAML::Value << cc.MainCamera;

			rEmitter << YAML::EndMap;
		}

		// Audio Player Component
		if( entity->HasComponent<AudioPlayerComponent>() )
		{
			rEmitter << YAML::Key << "AudioPlayerComponent";
			rEmitter << YAML::BeginMap;

			const auto& spc = entity->GetComponent< AudioPlayerComponent >();

			rEmitter << YAML::Key << "AssetID" << YAML::Value << spc.SpecAssetID;
			rEmitter << YAML::Key << "Loop" << YAML::Value << spc.Loop;
			rEmitter << YAML::Key << "Mute" << YAML::Value << spc.Mute;
			rEmitter << YAML::Key << "Spatialization" << YAML::Value << spc.Spatialisation;
			rEmitter << YAML::Key << "VolumeMultiplier" << YAML::Value << spc.Volume;
			rEmitter << YAML::Key << "PitchMultiplier" << YAML::Value << spc.Pitch;

			rEmitter << YAML::EndMap;
		}

		// Audio Listener Component
		if( entity->HasComponent<AudioListenerComponent>() )
		{
			rEmitter << YAML::Key << "AudioListenerComponent";
			rEmitter << YAML::BeginMap;

			const auto& alc = entity->GetComponent< AudioListenerComponent >();

			rEmitter << YAML::Key << "Primary" << YAML::Value << alc.Primary;
			rEmitter << YAML::Key << "Direction" << YAML::Value << alc.Direction;
			rEmitter << YAML::Key << "ConeInner" << YAML::Value << alc.ConeInnerAngle;
			rEmitter << YAML::Key << "ConeOuter" << YAML::Value << alc.ConeOuterAngle;

			rEmitter << YAML::EndMap;
		}

		// Billboard Component
		if( entity->HasComponent<BillboardComponent>() )
		{
			rEmitter << YAML::Key << "BillboardComponent";
			rEmitter << YAML::BeginMap;

			const auto& bbc = entity->GetComponent< BillboardComponent >();

			rEmitter << YAML::Key << "TextureID" << YAML::Value << bbc.AssetID;

			rEmitter << YAML::EndMap;
		}

		// Navigation Mesh Specification Component
		if( entity->HasComponent<NavigationMeshSpecificationComponent>() )
		{
			rEmitter << YAML::Key << "NavigationMeshSpecificationComponent";
			rEmitter << YAML::BeginMap;

			const auto& nmsc = entity->GetComponent< NavigationMeshSpecificationComponent >();

			const unsigned int bit = nmsc.HasBuilt ? 1 : 0;

			rEmitter << YAML::Key << "Extent" << YAML::Value << nmsc.Extent;
			rEmitter << YAML::Key << "HasBuilt" << YAML::Value << bit;

			rEmitter << YAML::EndMap;
		}

		// Behaviour Tree Component
		if( entity->HasComponent<BehaviourTreeComponent>() )
		{
			rEmitter << YAML::Key << "BehaviourTreeComponent";
			rEmitter << YAML::BeginMap;

			const auto& btc = entity->GetComponent< BehaviourTreeComponent >();

			rEmitter << YAML::Key << "AssetID" << YAML::Value << btc.BehaviourTreeAssetID;

			rEmitter << YAML::EndMap;
		}

		// Text Component
		if( entity->HasComponent<TextComponent>() )
		{
			rEmitter << YAML::Key << "TextComponent";
			rEmitter << YAML::BeginMap;

			const auto& textComp = entity->GetComponent< TextComponent >();

			// TODO: What if we have unicode characters in the Text?
			//		 The filesystem will not be happy about that...
			rEmitter << YAML::Key << "Text" << YAML::Value << textComp.Text;
			rEmitter << YAML::Key << "AssetID" << YAML::Value << textComp.FontAssetID;
			rEmitter << YAML::Key << "Color" << YAML::Value << textComp.Color;

			rEmitter << YAML::EndMap;
		}
	}

	void ComponentSerialisation::DeserialiseComponents( const YAML::Node& rEntityNode, Ref<Scene> scene )
	{
		const UUID entityID = rEntityNode[ "Entity" ].as< uint64_t >();
		// Fall back to entity because this scene may be pre 0.2.1
		const std::string className = rEntityNode[ "Class" ].as< std::string >( "Entity" );
		const std::string Tag = rEntityNode[ "TagComponent" ][ "Tag" ].as< std::string >( "Empty Entity" );

		SAT_CORE_INFO( "Deserialised entity with ID: ENTITY/{0}, with name: {1} and class name: {2}", entityID, Tag, className );

		// Pre 0.2.1, this would be an entity
		SharedPtr<Entity> DeserialisedEntity = scene->CreateEntityWithIDScript( entityID, Tag, className, false );

		const auto classInfo = rEntityNode[ "ClassInformation" ];
		if( classInfo )
		{
			const auto propertyCount = classInfo[ "LastPropertyCount" ].as<int>();

			if( propertyCount != DeserialisedEntity->GetClass()->GetPropertyCount() )
			{
				SAT_CORE_WARN( "Property count does not match!, Last/{0}, Current/{1}", propertyCount, DeserialisedEntity->GetClass()->GetPropertyCount() );
			}

			std::vector<std::string> savedPropertyNames;

			const auto lastProperties = classInfo[ "Properties" ];
			for( const auto property : lastProperties )
			{
				savedPropertyNames.push_back( property[ "Name" ].as<std::string>() );
			}

			// Now, we get the current properties from the SClass.
			const auto SClassPropCount = DeserialisedEntity->GetClass()->GetPropertyCount();
			auto SClassProps = DeserialisedEntity->GetClass()->GetProperties();

			std::vector<std::string> compiledInPropertyNames;
			compiledInPropertyNames.reserve( SClassPropCount );

			for( size_t i = 0; i < SClassPropCount; ++i )
			{
				SProperty* pProperty = ( SProperty* ) SClassProps[ i ];

				compiledInPropertyNames.push_back( pProperty->GetName() );
			}

#define SAT_DESERIALISE_PROPERTY_YAML( PropertyType ) \
{ \
const auto value = property[ "Value" ].as<typename PropertyTypeTraits<Saturn::SPropertyType::PropertyType>::Type>(); \
\
pCompiledInProperty->SetProperty( DeserialisedEntity.Get(), value ); \
} break

			// Now, we must make sure we use the compiled in SProperties
			for( size_t i = 0; i < SClassPropCount; ++i )
			{
				// Try to find the name at i, in both maps
				const auto savedNameItr = std::find( savedPropertyNames.begin(), savedPropertyNames.end(), compiledInPropertyNames[ i ] );

				if( savedNameItr != savedPropertyNames.end() )
				{
					const auto property = lastProperties[ i ];

					// Property exists
					const SPropertyType savedType = ( SPropertyType ) property[ "ValueType" ].as<int>( ( int ) SPropertyType::Unknown );

					SPropertyEditor* pCompiledInProperty = ( SPropertyEditor* ) SClassProps[ i ];
					if( savedType == pCompiledInProperty->GetType() )
					{
						// Set current (compiled in) value to saved value.
						switch( savedType )
						{
							case SPropertyType::Char:
								SAT_DESERIALISE_PROPERTY_YAML( Char );

							case SPropertyType::Float:
								SAT_DESERIALISE_PROPERTY_YAML( Float );

							case SPropertyType::Int:
								SAT_DESERIALISE_PROPERTY_YAML( Int );

							case SPropertyType::Double:
								SAT_DESERIALISE_PROPERTY_YAML( Double );

							case SPropertyType::Uint8:
								SAT_DESERIALISE_PROPERTY_YAML( Uint8 );

							case SPropertyType::Uint16:
								SAT_DESERIALISE_PROPERTY_YAML( Uint16 );

							case SPropertyType::Uint32:
								SAT_DESERIALISE_PROPERTY_YAML( Uint32 );

							case SPropertyType::Uint64:
								SAT_DESERIALISE_PROPERTY_YAML( Uint64 );

							case SPropertyType::Int8:
								SAT_DESERIALISE_PROPERTY_YAML( Int8 );

							case SPropertyType::Int16:
								SAT_DESERIALISE_PROPERTY_YAML( Int16 );

							case SPropertyType::Int64:
								SAT_DESERIALISE_PROPERTY_YAML( Int64 );

							case SPropertyType::Vector2:
							{
								auto value = property[ "Value" ].as<glm::vec2>();
								pCompiledInProperty->SetProperty<glm::vec2&>( DeserialisedEntity.Get(), value );
							} break;

							case SPropertyType::Vector3:
							{
								auto value = property[ "Value" ].as<glm::vec3>();
								pCompiledInProperty->SetProperty( DeserialisedEntity.Get(), value );
							} break;

							case SPropertyType::Vector4:
							{
								auto value = property[ "Value" ].as<glm::vec4>();
								pCompiledInProperty->SetProperty( DeserialisedEntity.Get(), value );
							} break;

							case SPropertyType::String:
							{
								auto value = property[ "Value" ].as<std::string>();
								pCompiledInProperty->SetProperty( DeserialisedEntity.Get(), value );
							} break;

							case SPropertyType::Asset:
							{
								auto value = property[ "Value" ].as<uint64_t>();
								auto expectedType = property[ "ExpectedType" ].as<int>();

								AssetReference& rAssetReference = pCompiledInProperty->Read<SPropertyType::Asset>( DeserialisedEntity.Get() );

								rAssetReference.ID = value;
								rAssetReference.ExpectedType = ( AssetType ) expectedType;
							} break;
						}

						//						pCompiledInProperty->MarkClean();
					}
				}
				else
				{
					SAT_CORE_WARN( "SProperty \"{0}\" could not be found!", *savedNameItr );
				}
			}
		}

		const auto pc = rEntityNode[ "PrefabComponent" ];
		if( pc )
		{
			auto& p = DeserialisedEntity->AddComponent< PrefabComponent >();

			p.AssetID = pc[ "AssetID" ].as< uint64_t >();

			const auto ogPrefabIDNode = pc[ "EntityIDInPrefab" ];
			if( !ogPrefabIDNode.IsNull() )
			{
				p.EntityIDInPrefab = ogPrefabIDNode.as<uint64_t>();
			}
		}

		auto tc = rEntityNode[ "TransformComponent" ];
		if( tc )
		{
			auto& t = DeserialisedEntity->GetComponent< TransformComponent >();

			t.Position = tc[ "Position" ].as< glm::vec3 >();
			t.SetRotation( glm::radians( tc[ "Rotation" ].as< glm::vec3 >() ) );
			t.Scale = tc[ "Scale" ].as< glm::vec3 >();
		}

		auto mc = rEntityNode[ "MeshComponent" ];
		if( mc )
		{
			auto& m = DeserialisedEntity->AddComponent< StaticMeshComponent >();

			auto id = mc[ "Asset" ].as<uint64_t>( 0 );
			if( id != 0 )
			{
				auto mesh = AssetManager::Get()->GetAssetAs<StaticMesh>( id );

				m.Mesh = mesh;
				m.MaterialRegistry = Ref<MaterialRegistry>::Create();

				if( m.Mesh )
				{
					const auto materialRegistry = mc[ "MaterialRegistry" ];
					if( materialRegistry )
					{
						const bool hasOverrides = materialRegistry[ "AnyOverrides" ].as<bool>();

						if( hasOverrides )
						{
							auto materialOverrides = materialRegistry[ "MaterialOverrides" ];

							int i = 0;
							for( auto override : materialOverrides )
							{
								id = override[ i ].as<uint64_t>();

								if( id != 0 )
								{
									m.MaterialRegistry->AddAsset( AssetManager::Get()->GetAssetAs<MaterialAsset>( id ) );
									m.MaterialRegistry->SetOverrides( i, true );
								}

								++i;
							}
						}
						else
						{
							m.MaterialRegistry->Copy( m.Mesh->GetMaterialRegistry() );
						}
					}
				}
			}
		}

		auto skm = rEntityNode[ "SkeletalMeshComponent" ];
		if( skm )
		{
			auto& m = DeserialisedEntity->AddComponent< SkeletalMeshComponent >();

			auto id = skm[ "Asset" ].as<uint64_t>( 0 );
			if( id != 0 )
			{
				auto mesh = AssetManager::Get()->GetAssetAs<SkeletalMesh>( id );

				// Ensure that we always have a material registry even if we don't have a mesh
				m.MaterialRegistry = Ref<MaterialRegistry>::Create();

				if( mesh )
				{
					m.Mesh = mesh;

					const auto materialRegistry = skm[ "MaterialRegistry" ];
					if( materialRegistry )
					{
						const bool hasOverrides = materialRegistry[ "AnyOverrides" ].as<bool>();

						if( hasOverrides )
						{
							auto materialOverrides = materialRegistry[ "MaterialOverrides" ];

							int i = 0;
							for( auto override : materialOverrides )
							{
								id = override[ i ].as<uint64_t>();

								if( id != 0 )
								{
									m.MaterialRegistry->AddAsset( AssetManager::Get()->GetAssetAs<MaterialAsset>( id ) );
									m.MaterialRegistry->SetOverrides( i, true );
								}

								++i;
							}
						}
						else
						{
							m.MaterialRegistry->Copy( m.Mesh->GetMaterialRegistry() );
						}
					}
				}
			}

			const auto animAsset = skm[ "AnimationAsset" ].as<uint64_t>( 0 );
			const auto animationAssetType = skm[ "AnimationAssetType" ].as<uint16_t>( 0 );
			if( animationAssetType > std::numeric_limits<std::underlying_type_t<AnimatorType>>::max() )
			{
				SAT_CORE_WARN( "AnimationAssetType is greater than max numerical limit! ({0} > {1})", animationAssetType, std::numeric_limits<std::underlying_type_t<AnimatorType>>::max() );
			
				m.AnimatorType = AnimatorType::Single;
			}
			else
			{
				m.AnimatorType = ( AnimatorType ) animationAssetType;
			}

			m.AnimationControllerAssetID = animAsset;
		}

		const auto rcNode = rEntityNode[ "RelationshipComponent" ];
		auto& rc = DeserialisedEntity->GetComponent<RelationshipComponent>();
		rc.Parent = rcNode[ "Parent" ] ? rcNode[ "Parent" ].as<uint64_t>() : 0;

		const auto rcChildren = rcNode[ "Children" ];
		if( rcChildren )
		{
			for( auto child : rcChildren )
			{
				uint64_t id = child[ "ID" ].as<uint64_t>();
				rc.ChildrenID.push_back( id );
			}
		}

		const auto slc = rEntityNode[ "SkyLightComponent" ];
		if( slc )
		{
			auto& s = DeserialisedEntity->AddComponent< SkylightComponent >();

			s.DynamicSky = slc[ "IsPreetham" ].as< bool >();

			if( s.DynamicSky )
			{
				auto PreethamSettings = slc[ "Preetham Settings" ];

				s.Turbidity = PreethamSettings[ "Turbidity" ].as< float >();
				s.Azimuth = PreethamSettings[ "Azimuth" ].as< float >();
				s.Inclination = PreethamSettings[ "Inclination" ].as< float >();
			}
			else
			{
				// TODO...
			}
		}

		const auto dlc = rEntityNode[ "DirectionalLightComponent" ];
		if( dlc )
		{
			auto& d = DeserialisedEntity->AddComponent< DirectionalLightComponent >();

			d.Radiance = dlc[ "Radiance" ].as< glm::vec3 >();
			d.Intensity = dlc[ "Intensity" ].as< float >();
			d.CastShadows = dlc[ "CastShadows" ].as< bool >();
		}

		const auto plc = rEntityNode[ "PointLightComponent" ];
		if( plc )
		{
			auto& p = DeserialisedEntity->AddComponent< PointLightComponent >();

			p.Radiance = plc[ "Radiance" ].as< glm::vec3 >();
			p.Intensity = plc[ "Intensity" ].as< float >();
			p.Multiplier = plc[ "Multiplier" ].as< float >();
			p.LightSize = plc[ "LightSize" ].as< float >();
			p.Radius = plc[ "Radius" ].as< float >();
			p.MinRadius = plc[ "MinRadius" ].as< float >();
			p.Falloff = plc[ "Falloff" ].as< float >();
		}

		const auto bcc = rEntityNode[ "BoxColliderComponent" ];
		if( bcc )
		{
			auto& b = DeserialisedEntity->AddComponent< BoxColliderComponent >();

			b.HalfExtents = bcc[ "Extents" ].as< glm::vec3 >( glm::vec3( 0.5f ) );
			b.Offset = bcc[ "Offset" ].as< glm::vec3 >();
			b.IsTrigger = bcc[ "IsTrigger" ].as< bool >();
			b.AutoAdjustExtent = bcc[ "AutoAdjustExtent" ].as< bool >( false );
		}

		const auto scc = rEntityNode[ "SphereColliderComponent" ];
		if( scc )
		{
			auto& s = DeserialisedEntity->AddComponent< SphereColliderComponent >();

			s.Radius = scc[ "Radius" ].as< float >();
			s.Offset = scc[ "Offset" ].as< glm::vec3 >();
			s.IsTrigger = scc[ "IsTrigger" ].as< bool >();
		}

		const auto ccc = rEntityNode[ "CapsuleColliderComponent" ];
		if( ccc )
		{
			auto& c = DeserialisedEntity->AddComponent< CapsuleColliderComponent >();

			c.HalfHeight = ccc[ "Height" ].as< float >();
			c.Radius = ccc[ "Radius" ].as< float >();
			c.Offset = ccc[ "Offset" ].as< glm::vec3 >();
			c.IsTrigger = ccc[ "IsTrigger" ].as< bool >();
		}

		const auto rbc = rEntityNode[ "RigidbodyComponent" ];
		if( rbc )
		{
			auto& rb = DeserialisedEntity->AddComponent< RigidbodyComponent >();

			rb.BodyType = ( PhysicsRigidBodyType )rbc[ "BodyType" ].as< uint8_t >( 2 /*PhysicsRigidBodyType::Dynamic*/ );
			rb.Mass = rbc[ "Mass" ].as< float >();

			auto lockNode = rbc[ "LockFlags" ];

			if( lockNode )
			{
				rb.LockFlags = lockNode.as< uint8_t >( 0 );
			}
			else
			{
				rb.LockFlags = 0;
			}
		}

		const auto cmc = rEntityNode[ "CharacterMovementComponent" ];
		if( cmc )
		{
			auto& cm = DeserialisedEntity->AddComponent< CharacterMovementComponent >();

			cm.StepOffset			= cmc[ "StepOffset" ].as< float >();
			cm.NoGravity			= cmc[ "NoGravity" ].as< bool >( false );
			cm.ControlMovementInAir = cmc[ "ControlMovementInAir" ].as< bool >( false );
			cm.ControlRotationInAir = cmc[ "ControlRotationInAir" ].as< bool>( false );
		}

		const auto cc = rEntityNode[ "CameraComponent" ];
		if( cc )
		{
			auto& c = DeserialisedEntity->AddComponent< CameraComponent >();

			c.MainCamera = cc[ "MainCamera" ].as< bool >();
		}

		const auto spc = rEntityNode[ "AudioPlayerComponent" ];
		if( spc )
		{
			auto& sp = DeserialisedEntity->AddComponent< AudioPlayerComponent >();

			sp.SpecAssetID = spc[ "AssetID" ].as< uint64_t >( 0 );
			sp.Loop = spc[ "Loop" ].as< bool >( false );
			sp.Mute = spc[ "Mute" ].as< bool >( false );
			sp.Spatialisation = spc[ "Spatialization" ].as<bool>( false );
			sp.Volume = spc[ "VolumeMultiplier" ].as<float>( 1.0f );
			sp.Pitch = spc[ "PitchMultiplier" ].as<float>( 1.0f );
		}

		const auto alc = rEntityNode[ "AudioListenerComponent" ];
		if( alc )
		{
			auto& al = DeserialisedEntity->AddComponent< AudioListenerComponent >();

			al.Primary = alc[ "Primary" ].as< bool >();
			al.Direction = alc[ "Direction" ].as< glm::vec3 >();
			al.ConeInnerAngle = alc[ "ConeInner" ].as< float >( 0.0f );
			al.ConeOuterAngle = alc[ "ConeOuter" ].as< float >( 0.0f );
		}

		const auto bbc = rEntityNode[ "BillboardComponent" ];
		if( bbc	)
		{
			auto& bc = DeserialisedEntity->AddComponent<BillboardComponent>();

			bc.AssetID = bbc[ "TextureID" ].as<uint64_t>( 0 );
		}

		const auto nmsc = rEntityNode[ "NavigationMeshSpecificationComponent" ];
		if( nmsc )
		{
			auto& nms = DeserialisedEntity->AddComponent< NavigationMeshSpecificationComponent >();
			nms.Extent = nmsc[ "Extent" ].as< glm::vec3 >();

			uint8_t bit = nmsc[ "HasBuilt" ].as<uint8_t>();
			unsigned int externalData = bit ? 1 : 0;

			nms.HasBuilt = externalData;
		}

		const auto btc = rEntityNode[ "BehaviourTreeComponent" ];
		if( btc )
		{
			auto& bt = DeserialisedEntity->AddComponent< BehaviourTreeComponent >();
			bt.BehaviourTreeAssetID = btc[ "AssetID" ].as< uint64_t >();
		}

		const auto textComp = rEntityNode[ "TextComponent" ];
		if( textComp )
		{
			auto& rTextComp = DeserialisedEntity->AddComponent<TextComponent>();

			rTextComp.Text = textComp[ "Text" ].as<std::string>();
			rTextComp.FontAssetID = textComp[ "AssetID" ].as<uint64_t>();
			rTextComp.Color = textComp[ "Color" ].as<glm::vec4>();
		}
	}
}
