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
#include "PhysicsShapes.h"

#include "PhysicsAuxiliary.h"
#include "PhysicsFoundation.h"
#include "PhysicsMaterialAsset.h"

#include "Saturn/Asset/AssetManager.h"

#include "Saturn/Project/Project.h"

#include "Saturn/AI/Navigation/RecastInputGeometry.h"

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////

	void PhysicsShape::Detach( physx::PxRigidActor& rActor )
	{
		rActor.detachShape( *m_Shape );
	}

	void PhysicsShape::SetFilterData()
	{
		physx::PxFilterData data;
		data.word0 = BIT( 0 );
		data.word1 = BIT( 0 );

		m_Shape->setSimulationFilterData( data );
	}

	void PhysicsShape::SetUserData( void* pData )
	{
		m_Shape->userData = pData;
	}

	void PhysicsShape::SetTrigger( bool isTrigger )
	{
		if( m_Shape )
		{
			m_Shape->setFlag( physx::PxShapeFlag::eSIMULATION_SHAPE, !isTrigger );
//			m_Shape->setFlag( physx::PxShapeFlag::eSCENE_QUERY_SHAPE, !isTrigger );
			m_Shape->setFlag( physx::PxShapeFlag::eTRIGGER_SHAPE, isTrigger );
		}
	}

	Ref<PhysicsMaterialAsset> PhysicsShape::GetMaterial( Ref<StaticMesh> mesh, UUID physMaterialAssetID )
	{
		Ref<PhysicsMaterialAsset> materialAsset;

		Ref<Project> activeProject = Project::GetActiveProject();
		const UUID fallbackID = activeProject->GetDefaultPhysicsMaterialAsset();

		// The PhysMat ID from the rigidbody takes priority over the mesh and the project fallback.
		if( physMaterialAssetID )
		{
			materialAsset = AssetManager::Get()->GetAssetAs<PhysicsMaterialAsset>( physMaterialAssetID );
		}

		// Still null then we go to the mesh and see what its material is.
		if( !materialAsset )
		{
			// Check if no mesh?
			if( !mesh )
			{
				// No mesh? Use prj default
				if( fallbackID != 0 )
				{
					materialAsset = AssetManager::Get()->GetAssetAs<PhysicsMaterialAsset>( fallbackID );
				}
			}
			// Hooray, we have a mesh
			// However, we must make sure that it's ID is not zero
			else if( mesh->GetPhysicsMaterial() == 0 || mesh->GetPhysicsMaterial() == fallbackID )
			{
				materialAsset = AssetManager::Get()->GetAssetAs<PhysicsMaterialAsset>( fallbackID );
			}
			// We have a mesh and it has a unique ID
			else
			{
				materialAsset = AssetManager::Get()->GetAssetAs<PhysicsMaterialAsset>( mesh->GetPhysicsMaterial() );
			}

			// If we get here, then we had no mesh and no fallback ID OR the specified ID could not be found.
			// So create memory only asset
			if( !materialAsset )
			{
				materialAsset = Ref<PhysicsMaterialAsset>::Create( 1.0f, 1.0f, 0.5f );
			}
		}

		SAT_CORE_ASSERT( materialAsset, "Material cannot be null at this stage!, All possible ways have failed, Rigidbody PhysMat could not be loaded and/or mesh PhysMat could not be loaded and/or project fallback could not be loaded and somehow the memeory only asset has failed as well." );

		return materialAsset;
	}

	//////////////////////////////////////////////////////////////////////////
	// Box

	BoxShape::BoxShape( SharedPtr<Entity> entity )
		: PhysicsShape( entity )
	{
		m_Type = PhysicsShapeType::Box;
	}

	BoxShape::~BoxShape()
	{
	}

	void BoxShape::Create( physx::PxRigidActor& rActor )
	{
		BoxColliderComponent& bcc = m_Entity->GetComponent<BoxColliderComponent>();
		TransformComponent& transform = m_Entity->GetComponent<TransformComponent>();
		auto& rPhysMaterialAssetID = m_Entity->GetComponent<RigidbodyComponent>().MaterialAssetID;

		Ref<StaticMesh> mesh = nullptr;
		if( auto* pSM = m_Entity->TryGetComponent<StaticMeshComponent>(); pSM )
			mesh = pSM->Mesh;

		glm::vec3 halfSize = bcc.HalfExtents;

		// Very rare path, only happens if something else modifies the scale.
#if !defined(SAT_DIST)
		if( bcc.AutoAdjustExtent && halfSize != transform.Scale )
#else
		if( halfSize != ( transform.Scale * 0.5f ) )
#endif
			halfSize = transform.Scale * 0.5f;

		Ref<PhysicsMaterialAsset> materialAsset = GetMaterial( mesh, rPhysMaterialAssetID );
		physx::PxMaterial* pPxMaterial = materialAsset->GetMaterial();

		physx::PxBoxGeometry BoxGeometry = physx::PxBoxGeometry( halfSize.x, halfSize.y, halfSize.z );
		physx::PxShape* pShape = physx::PxRigidActorExt::createExclusiveShape( rActor, BoxGeometry, *pPxMaterial );

		pShape->setFlag( physx::PxShapeFlag::eSIMULATION_SHAPE, !bcc.IsTrigger );
		pShape->setFlag( physx::PxShapeFlag::eTRIGGER_SHAPE, bcc.IsTrigger );

		pShape->setLocalPose( Auxiliary::GLMTransformToPx( glm::translate( glm::mat4( 1.0f ), bcc.Offset ) ) );

		m_Shape = pShape;

		rActor.attachShape( *pShape );

		SetFilterData();
	}

	void BoxShape::ExportRc( physx::PxRigidActor& rActor, RecastInputGeometryExpData& rData, AABB& rNavMeshBounds )
	{
		if( m_Shape->getGeometryType() == physx::PxGeometryType::eBOX )
		{
			physx::PxBoxGeometry geometry;
			if( m_Shape->getBoxGeometry( geometry ) )
			{
				glm::vec3 localVertices[ 8 ] = {
					{ -1.0f, -1.0f, -1.0f },
					{  1.0f, -1.0f, -1.0f },
					{  1.0f,  1.0f, -1.0f },
					{ -1.0f,  1.0f, -1.0f },
					{ -1.0f, -1.0f,  1.0f },
					{  1.0f, -1.0f,  1.0f },
					{  1.0f,  1.0f,  1.0f },
					{ -1.0f,  1.0f,  1.0f },
				};

				uint32_t indices[ 36 ] = {
					0, 1, 2,  0, 2, 3,
					4, 6, 5,  4, 7, 6,
					4, 5, 1,  4, 1, 0,
					3, 2, 6,  3, 6, 7,
					0, 3, 7,  0, 7, 4,
					1, 5, 6,  1, 6, 2
				};

				physx::PxTransform globalPose = physx::PxShapeExt::getGlobalPose( *m_Shape, rActor );
				glm::vec3 halfExtents = { geometry.halfExtents.x, geometry.halfExtents.y, geometry.halfExtents.z };

				glm::mat4 actorTransform = m_Entity->Transform();
				for( int i = 0; i < 8; i++ )
				{
					glm::vec3 scaled = localVertices[ i ] * halfExtents;
					glm::vec4 worldPos = actorTransform * glm::vec4( scaled, 1.0f );

					rData.VertexBuffer.push_back( worldPos.x );
					rData.VertexBuffer.push_back( worldPos.y );
					rData.VertexBuffer.push_back( worldPos.z );
				}

				rData.IndexBuffer.insert( rData.IndexBuffer.end(), std::begin( indices ), std::end( indices ) );
			}
		}
	}

	void BoxShape::SetTrigger( bool isTrigger )
	{
		PhysicsShape::SetTrigger( isTrigger );
		m_Entity->GetComponent<BoxColliderComponent>().IsTrigger = isTrigger;
	}

	//////////////////////////////////////////////////////////////////////////
	// Sphere

	SphereShape::SphereShape( SharedPtr<Entity> entity )
		: PhysicsShape( entity )
	{
		m_Type = PhysicsShapeType::Sphere;
	}

	SphereShape::~SphereShape()
	{
	}

	void SphereShape::Create( physx::PxRigidActor& rActor )
	{
		SphereColliderComponent& scc = m_Entity->GetComponent<SphereColliderComponent>();
		TransformComponent& transform = m_Entity->GetComponent<TransformComponent>();
		auto& rPhysMaterialAssetID = m_Entity->GetComponent<RigidbodyComponent>().MaterialAssetID;

		Ref<StaticMesh> mesh = nullptr;
		if( auto* pSM = m_Entity->TryGetComponent<StaticMeshComponent>(); pSM )
			mesh = pSM->Mesh;

		float radius = scc.Radius;
		glm::vec3 scale = transform.Scale;

		if( scale.x != 0.0f )
			radius *= scale.x;

		Ref<PhysicsMaterialAsset> materialAsset = GetMaterial( mesh, rPhysMaterialAssetID );
		physx::PxMaterial* pPxMaterial = materialAsset->GetMaterial();

		physx::PxSphereGeometry SphereGoemetry( radius );

		physx::PxShape* pShape = physx::PxRigidActorExt::createExclusiveShape( rActor, SphereGoemetry, *pPxMaterial );

		pShape->setFlag( physx::PxShapeFlag::eSIMULATION_SHAPE, !scc.IsTrigger );
		pShape->setFlag( physx::PxShapeFlag::eTRIGGER_SHAPE, scc.IsTrigger );

		m_Shape = pShape;
		rActor.attachShape( *pShape );

		SetFilterData();
	}

	void SphereShape::ExportRc( physx::PxRigidActor& rActor, RecastInputGeometryExpData& rData, AABB& rNavMeshBounds )
	{
		physx::PxTransform actorTransform = rActor.getGlobalPose();

		if( m_Shape->getGeometryType() == physx::PxGeometryType::eSPHERE )
		{
			physx::PxSphereGeometry geometry;
			if( m_Shape->getSphereGeometry( geometry ) )
			{
				physx::PxTransform localTransform = actorTransform * m_Shape->getLocalPose();

				const float radius = geometry.radius;

				// Sphere tessellation resolution
				// vertical slices
				constexpr int latSegments = 6;

				// horizontal rings
				constexpr int lonSegments = 12;

				uint32_t baseIndex = ( uint32_t ) rData.VertexBuffer.size() / 3;
				uint32_t vertexCounter = baseIndex;

				std::vector<uint32_t> vertexIndices;

				// Top pole
				physx::PxVec3 top = localTransform.transform( physx::PxVec3( 0, radius, 0 ) );
				glm::vec3 glmTop = Auxiliary::PxToGLM( top );
				if( rData.Bounds.Contains( glmTop ) )
				{
					rData.VertexBuffer.push_back( top.x );
					rData.VertexBuffer.push_back( top.y );
					rData.VertexBuffer.push_back( top.z );
					vertexIndices.push_back( vertexCounter++ );
					rNavMeshBounds.Min = glm::min( rNavMeshBounds.Min, glmTop );
					rNavMeshBounds.Max = glm::max( rNavMeshBounds.Max, glmTop );
				}
				else
				{
					// Mark as invalid.
					vertexIndices.push_back( UINT32_MAX );
					++vertexCounter;
				}

				// Rings (excluding poles)
				for( int lat = 1; lat < latSegments; lat++ )
				{
					float theta = lat * ( physx::PxPi / latSegments );
					float y = cos( theta );
					float r = sin( theta );

					for( int lon = 0; lon < lonSegments; lon++ )
					{
						float phi = lon * ( physx::PxTwoPi / lonSegments );
						float x = cos( phi ) * r;
						float z = sin( phi ) * r;

						physx::PxVec3 localPos( x * radius, y * radius, z * radius );
						physx::PxVec3 worldPos = localTransform.transform( localPos );
						glm::vec3 glmPos = Auxiliary::PxToGLM( worldPos );

						if( rData.Bounds.Contains( glmPos ) )
						{
							rData.VertexBuffer.push_back( worldPos.x );
							rData.VertexBuffer.push_back( worldPos.y );
							rData.VertexBuffer.push_back( worldPos.z );
							vertexIndices.push_back( vertexCounter++ );
							rNavMeshBounds.Min = glm::min( rNavMeshBounds.Min, glmPos );
							rNavMeshBounds.Max = glm::max( rNavMeshBounds.Max, glmPos );
						}
						else
						{
							vertexIndices.push_back( UINT32_MAX );
							++vertexCounter;
						}
					}
				}

				// Bottom pole
				physx::PxVec3 bottom = localTransform.transform( physx::PxVec3( 0, -radius, 0 ) );
				glm::vec3 glmBottom = Auxiliary::PxToGLM( bottom );
				if( rData.Bounds.Contains( glmBottom ) )
				{
					rData.VertexBuffer.push_back( bottom.x );
					rData.VertexBuffer.push_back( bottom.y );
					rData.VertexBuffer.push_back( bottom.z );
					vertexIndices.push_back( vertexCounter++ );
					rNavMeshBounds.Min = glm::min( rNavMeshBounds.Min, glmBottom );
					rNavMeshBounds.Max = glm::max( rNavMeshBounds.Max, glmBottom );
				}
				else
				{
					vertexIndices.push_back( UINT32_MAX );
					++vertexCounter;
				}

				// Build triangle indices
				auto index = [ & ]( int lat, int lon ) -> uint32_t {
					return 1 + ( lat - 1 ) * lonSegments + ( lon % lonSegments );
				};

				// Top cap
				for( int lon = 0; lon < lonSegments; ++lon )
				{
					uint32_t topIdx = vertexIndices[ 0 ];
					uint32_t a = vertexIndices[ index( 1, lon ) ];
					uint32_t b = vertexIndices[ index( 1, lon + 1 ) ];

					if( topIdx != UINT32_MAX && a != UINT32_MAX && b != UINT32_MAX )
					{
						rData.IndexBuffer.push_back( topIdx );
						rData.IndexBuffer.push_back( b );
						rData.IndexBuffer.push_back( a );
					}
				}

				// Middle bands
				for( int lat = 1; lat < latSegments - 1; lat++ )
				{
					for( int lon = 0; lon < lonSegments; lon++ )
					{
						uint32_t a = vertexIndices[ index( lat, lon ) ];
						uint32_t b = vertexIndices[ index( lat + 1, lon ) ];
						uint32_t c = vertexIndices[ index( lat, lon + 1 ) ];
						uint32_t d = vertexIndices[ index( lat + 1, lon + 1 ) ];

						if( a != UINT32_MAX && b != UINT32_MAX && c != UINT32_MAX )
						{
							rData.IndexBuffer.push_back( a );
							rData.IndexBuffer.push_back( b );
							rData.IndexBuffer.push_back( c );
						}
						if( c != UINT32_MAX && b != UINT32_MAX && d != UINT32_MAX )
						{
							rData.IndexBuffer.push_back( c );
							rData.IndexBuffer.push_back( b );
							rData.IndexBuffer.push_back( d );
						}
					}
				}

				// Bottom cap
				uint32_t bottomIdx = vertexIndices.back();
				int lastRingStart = 1 + ( latSegments - 2 ) * lonSegments;
				for( int lon = 0; lon < lonSegments; lon++ )
				{
					uint32_t a = vertexIndices[ lastRingStart + lon ];
					uint32_t b = vertexIndices[ lastRingStart + ( lon + 1 ) % lonSegments ];

					if( bottomIdx != UINT32_MAX && a != UINT32_MAX && b != UINT32_MAX )
					{
						rData.IndexBuffer.push_back( a );
						rData.IndexBuffer.push_back( b );
						rData.IndexBuffer.push_back( bottomIdx );
					}
				}
			}
		}
	}

	void SphereShape::SetTrigger( bool isTrigger )
	{
		PhysicsShape::SetTrigger( isTrigger );
		m_Entity->GetComponent<SphereColliderComponent>().IsTrigger = isTrigger;
	}

	//////////////////////////////////////////////////////////////////////////
	// Capsule

	CapsuleShape::CapsuleShape( SharedPtr<Entity> entity )
		: PhysicsShape( entity )
	{
		m_Type = PhysicsShapeType::Capusle;
	}

	CapsuleShape::~CapsuleShape()
	{
	}

	void CapsuleShape::Create( physx::PxRigidActor& rActor )
	{
		CapsuleColliderComponent& cap = m_Entity->GetComponent<CapsuleColliderComponent>();
		TransformComponent& transform = m_Entity->GetComponent<TransformComponent>();
		auto& rPhysMaterialAssetID = m_Entity->GetComponent<RigidbodyComponent>().MaterialAssetID;

		Ref<StaticMesh> mesh = nullptr;
		if( auto* pSM = m_Entity->TryGetComponent<StaticMeshComponent>(); pSM )
			mesh = pSM->Mesh;

		float radius = cap.Radius;
		float height = cap.HalfHeight;

		glm::vec3 scale = transform.Scale;

		if( scale.x != 0.0f && height == 0.0f )
			radius *= scale.x;

		if( scale.y != 0.0f && height == 0.0f )
			height *= scale.y;

		Ref<PhysicsMaterialAsset> materialAsset = GetMaterial( mesh, rPhysMaterialAssetID );
		physx::PxMaterial* pPxMaterial = materialAsset->GetMaterial();

		physx::PxCapsuleGeometry CapsuleGemetry( radius, height );

		physx::PxShape* pShape = physx::PxRigidActorExt::createExclusiveShape( rActor, CapsuleGemetry, *pPxMaterial );
		pShape->setFlag( physx::PxShapeFlag::eSIMULATION_SHAPE, !cap.IsTrigger );
		pShape->setFlag( physx::PxShapeFlag::eTRIGGER_SHAPE, cap.IsTrigger );

		pShape->setLocalPose( physx::PxTransform( physx::PxQuat( physx::PxHalfPi, physx::PxVec3( 0, 0, 1 ) ) ) );

		m_Shape = pShape;
		rActor.attachShape( *pShape );

		SetFilterData();
	}

	void CapsuleShape::ExportRc( physx::PxRigidActor& rActor, RecastInputGeometryExpData& rData, AABB& rNavMeshBounds )
	{
		physx::PxTransform actorTransform = rActor.getGlobalPose();

		if( m_Shape->getGeometryType() == physx::PxGeometryType::eCAPSULE )
		{
			physx::PxCapsuleGeometry geometry;
			if( m_Shape->getCapsuleGeometry( geometry ) )
			{
				physx::PxTransform localTransform = actorTransform * m_Shape->getLocalPose();

				const float radius = geometry.radius;
				const float halfHeight = geometry.halfHeight;

				const int segmentCount = 12;
				const uint32_t baseIndex = ( uint32_t ) rData.VertexBuffer.size() / 3;

				std::vector<glm::vec3> ring0, ring1;
				std::vector<uint32_t> validIndices;
				uint32_t vertexCounter = baseIndex;

				for( int i = 0; i < segmentCount; ++i )
				{
					float angle = ( i / float( segmentCount ) ) * physx::PxPi * 2.0f;
					float x = cos( angle ) * radius;
					float z = sin( angle ) * radius;

					physx::PxVec3 local0( x, -halfHeight, z );
					physx::PxVec3 local1( x, +halfHeight, z );

					physx::PxVec3 world0 = localTransform.transform( local0 );
					physx::PxVec3 world1 = localTransform.transform( local1 );

					glm::vec3 glm0 = Auxiliary::PxToGLM( world0 );
					glm::vec3 glm1 = Auxiliary::PxToGLM( world1 );

					bool inBounds = rData.Bounds.Contains( glm0 ) || rData.Bounds.Contains( glm1 );

					if( inBounds )
					{
						// Add both vertices to buffer
						rData.VertexBuffer.push_back( world0.x );
						rData.VertexBuffer.push_back( world0.y );
						rData.VertexBuffer.push_back( world0.z );

						rData.VertexBuffer.push_back( world1.x );
						rData.VertexBuffer.push_back( world1.y );
						rData.VertexBuffer.push_back( world1.z );

						ring0.push_back( glm0 );
						ring1.push_back( glm1 );

						validIndices.push_back( vertexCounter );     // index for lower vertex
						validIndices.push_back( vertexCounter + 1 ); // index for upper vertex
						vertexCounter += 2;

						// Update nav mesh bounds
						for( const auto& v : { glm0, glm1 } )
						{
							rNavMeshBounds.Min = glm::min( rNavMeshBounds.Min, v );
							rNavMeshBounds.Max = glm::max( rNavMeshBounds.Max, v );
						}
					}
					else
					{
						// Still increment counters to stay aligned
						vertexCounter += 2;
					}
				}

				// Only generate triangles if we have enough valid vertices
				if( validIndices.size() >= 6 )
				{
					const uint32_t validSegments = ( uint32_t ) validIndices.size() / 2;
					for( uint32_t i = 0; i < validSegments; ++i )
					{
						uint32_t next = ( i + 1 ) % validSegments;

						uint32_t i0 = validIndices[ i * 2 ];
						uint32_t i1 = validIndices[ next * 2 ];
						uint32_t i2 = validIndices[ i * 2 + 1 ];
						uint32_t i3 = validIndices[ next * 2 + 1 ];

						// Triangle 1
						rData.IndexBuffer.push_back( i0 );
						rData.IndexBuffer.push_back( i1 );
						rData.IndexBuffer.push_back( i2 );

						// Triangle 2
						rData.IndexBuffer.push_back( i2 );
						rData.IndexBuffer.push_back( i1 );
						rData.IndexBuffer.push_back( i3 );
					}
				}
			}
		}
	}

	void CapsuleShape::SetTrigger( bool isTrigger )
	{
		PhysicsShape::SetTrigger( isTrigger );
		m_Entity->GetComponent<CapsuleColliderComponent>().IsTrigger = isTrigger;
	}

	//////////////////////////////////////////////////////////////////////////
	// Triangle

	TriangleMeshShape::TriangleMeshShape( SharedPtr<Entity> entity )
		: PhysicsShape( entity )
	{
		m_Type = PhysicsShapeType::TriangleMesh;

		SAT_CORE_ASSERT( m_Entity->HasComponent<StaticMeshComponent>(), "Entity does not have a static mesh component!" );

		m_Mesh = m_Entity->GetComponent<StaticMeshComponent>().Mesh;
	}

	TriangleMeshShape::~TriangleMeshShape()
	{
	}

	void TriangleMeshShape::Create( physx::PxRigidActor& rActor )
	{
		TransformComponent& transform = m_Entity->GetComponent<TransformComponent>();
		physx::PxTransform PxTrans = Auxiliary::GLMTransformToPx( transform.GetTransform() );

		const std::vector<physx::PxShape*>& rShapes = PhysicsFoundation::Get()->GetCookingContext().CreateTriangleMesh( m_Mesh, rActor, transform.Scale );

		if( rShapes.size() )
		{
			m_Shapes = rShapes;
			m_Shape = rShapes.front();
		}
		else
		{
			SAT_CORE_WARN( "No shapes we created from 'CreateTriangleMesh' this could mean the path does not exist or file header is not valid." );
		}
	}

	void TriangleMeshShape::Detach( physx::PxRigidActor& rActor )
	{
		for( physx::PxShape* rShape : m_Shapes )
		{
			rActor.detachShape( *rShape );
		}
	}

	void TriangleMeshShape::ExportRc( physx::PxRigidActor& rActor, RecastInputGeometryExpData& rData, AABB& rNavMeshBounds )
	{
		physx::PxTransform actorTransform = rActor.getGlobalPose();

		for( physx::PxShape* pShape : m_Shapes )
		{
			physx::PxTransform localActorTransform = actorTransform * pShape->getLocalPose();

			if( pShape->getGeometryType() == physx::PxGeometryType::eTRIANGLEMESH )
			{
				physx::PxTriangleMeshGeometry geometry;
				if( pShape->getTriangleMeshGeometry( geometry ) )
				{
					physx::PxTriangleMesh* pMesh = geometry.triangleMesh;

					auto vertCount = pMesh->getNbVertices();
					auto triCount = pMesh->getNbTriangles();

					const auto* verts = pMesh->getVertices();

					// Convert to Recast
					int offset = ( int ) rData.VertexBuffer.size() / 3;
					rData.VertexBuffer.reserve( vertCount * 3 );
					rData.IndexBuffer.reserve( triCount * 3 );

					physx::PxMeshScale meshScale = geometry.scale;
					physx::PxMat33 scaleRotation( meshScale.rotation );
					physx::PxVec3 scale = meshScale.scale;

					physx::PxU8 flags = pMesh->getTriangleMeshFlags();
					if( ( flags & physx::PxTriangleMeshFlag::e16_BIT_INDICES ) != 0 )
					{
						const physx::PxU16* tris = ( const physx::PxU16* ) pMesh->getTriangles();

						for( physx::PxU32 i = 0; i < triCount; i++ )
						{
							bool inBounds = false;
							for( auto vert = 0; vert < 3; vert++ )
							{
								physx::PxVec3 vertexPos = verts[ tris[ vert ] ];
								physx::PxVec3 scaledVertex = scaleRotation * ( vertexPos.multiply( scale ) );
								physx::PxVec3 worldSpace = localActorTransform.transform( scaledVertex );

								if( rData.Bounds.Contains( Auxiliary::PxToGLM( worldSpace ) ) )
								{
									rNavMeshBounds.Min.x = glm::min( rNavMeshBounds.Min.x, worldSpace.x );
									rNavMeshBounds.Min.y = glm::min( rNavMeshBounds.Min.y, worldSpace.y );
									rNavMeshBounds.Min.z = glm::min( rNavMeshBounds.Min.z, worldSpace.z );

									rNavMeshBounds.Max.x = glm::max( rNavMeshBounds.Max.x, worldSpace.x );
									rNavMeshBounds.Max.y = glm::max( rNavMeshBounds.Max.y, worldSpace.y );
									rNavMeshBounds.Max.z = glm::max( rNavMeshBounds.Max.z, worldSpace.z );

									rData.VertexBuffer.push_back( worldSpace.x );
									rData.VertexBuffer.push_back( worldSpace.y );
									rData.VertexBuffer.push_back( worldSpace.z );

									inBounds = true;
								}
							}
							tris += 3;

							if( inBounds )
							{
								rData.IndexBuffer.push_back( offset );
								rData.IndexBuffer.push_back( offset + 1 );
								rData.IndexBuffer.push_back( offset + 2 );

								offset += 3;
							}
						}
					}
					else
					{
						const physx::PxU32* tris = ( const physx::PxU32* ) pMesh->getTriangles();

						for( physx::PxU32 i = 0; i < triCount; i++ )
						{
							for( auto vert = 0; vert < 3; vert++ )
							{
								physx::PxVec3 vertexPos = verts[ tris[ vert ] ];
								physx::PxVec3 transformedVertex = actorTransform.transform( vertexPos );

								rData.VertexBuffer.push_back( transformedVertex.x );
								rData.VertexBuffer.push_back( transformedVertex.y );
								rData.VertexBuffer.push_back( transformedVertex.z );
							}
							tris += 3;

							rData.IndexBuffer.push_back( offset /*+0*/ );
							rData.IndexBuffer.push_back( offset + 1 );
							rData.IndexBuffer.push_back( offset + 2 );

							offset += 3;
						}
					}
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Convex

	ConvexMeshShape::ConvexMeshShape( SharedPtr<Entity> entity )
		: PhysicsShape( entity )
	{
		m_Type = PhysicsShapeType::ConvexMesh;

		SAT_CORE_ASSERT( m_Entity->HasComponent<StaticMeshComponent>(), "Entity does not have a static mesh component!" );

		m_Mesh = m_Entity->GetComponent<StaticMeshComponent>().Mesh;
	}

	ConvexMeshShape::~ConvexMeshShape()
	{
	}

	void ConvexMeshShape::Create( physx::PxRigidActor& rActor )
	{
		TransformComponent& transform = m_Entity->GetComponent<TransformComponent>();
		physx::PxTransform PxTrans = Auxiliary::GLMTransformToPx( transform.GetTransform() );

		const std::vector<physx::PxShape*>& rShapes = PhysicsFoundation::Get()->GetCookingContext().CreateConvexMesh( m_Mesh, rActor, transform.Scale );

		if( rShapes.size() )
		{
			m_Shapes = rShapes;
			m_Shape = rShapes.front();
		}
		else
		{
			SAT_CORE_WARN( "No shapes were created from 'CreateConvexMesh' this could mean the path does not exist or file header is not valid." );
		}
	}

	void ConvexMeshShape::Detach( physx::PxRigidActor& rActor )
	{
		for( physx::PxShape* rShape : m_Shapes )
		{
			rActor.detachShape( *rShape );
		}
	}

	void ConvexMeshShape::ExportRc( physx::PxRigidActor& rActor, RecastInputGeometryExpData& rData, AABB& rNavMeshBounds )
	{
		glm::mat4 actorTransform = m_Entity->Transform();

		for( physx::PxShape* pShape : m_Shapes )
		{
			if( pShape->getGeometryType() == physx::PxGeometryType::eCONVEXMESH )
			{
				physx::PxConvexMeshGeometry geometry;
				if( pShape->getConvexMeshGeometry( geometry ) )
				{
					physx::PxConvexMesh* pMesh = geometry.convexMesh;

					const auto triCount = pMesh->getNbPolygons();

					auto* pVertexBuffer = pMesh->getVertices();
					auto* pIndexBuffer = pMesh->getIndexBuffer();

					for( physx::PxU32 i = 0; i < triCount; i++ )
					{
						physx::PxHullPolygon poly{};
						if( pMesh->getPolygonData( i, poly ) ) continue;

						const uint32_t vertexCount = poly.mNbVerts;
						const physx::PxU8* pPolyIndices = pIndexBuffer + poly.mIndexBase;

						for( physx::PxU32 j = 1; j < vertexCount - 1; j++ )
						{
							glm::vec3 v0 = glm::vec3( actorTransform * glm::vec4( Auxiliary::PxToGLM( pVertexBuffer[ pPolyIndices[ 0 ] ] ), 1.0f ) );
							glm::vec3 v1 = glm::vec3( actorTransform * glm::vec4( Auxiliary::PxToGLM( pVertexBuffer[ pPolyIndices[ j ] ] ), 1.0f ) );
							glm::vec3 v2 = glm::vec3( actorTransform * glm::vec4( Auxiliary::PxToGLM( pVertexBuffer[ pPolyIndices[ i + 1 ] ] ), 1.0f ) );

							const uint32_t baseIndex = ( uint32_t ) rData.VertexBuffer.size();
							rData.VertexBuffer.push_back( v0.x );
							rData.VertexBuffer.push_back( v0.y );
							rData.VertexBuffer.push_back( v0.z );

							rData.VertexBuffer.push_back( v1.x );
							rData.VertexBuffer.push_back( v1.y );
							rData.VertexBuffer.push_back( v1.z );

							rData.VertexBuffer.push_back( v2.x );
							rData.VertexBuffer.push_back( v2.y );
							rData.VertexBuffer.push_back( v2.z );

							rData.IndexBuffer.push_back( baseIndex );
							rData.IndexBuffer.push_back( baseIndex + 1 );
							rData.IndexBuffer.push_back( baseIndex + 2 );
						}
					}
				}
			}
		}
	}

}
