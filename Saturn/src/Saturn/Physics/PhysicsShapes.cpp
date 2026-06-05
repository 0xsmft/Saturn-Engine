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
#include "Saturn/Scene/Scene.h"

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////

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
				materialAsset = Ref<PhysicsMaterialAsset>::Create( 1.0f, 0.5f );
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

	void BoxShape::Create( float mass )
	{
		BoxColliderComponent& bcc = m_Entity->GetComponent<BoxColliderComponent>();
		TransformComponent& transform = m_Entity->GetComponent<TransformComponent>();
		
		// Check for rigid body
		UUID rigidBodyMaterialID = 0;
		if( m_Entity->HasComponent<RigidbodyComponent>() )
			rigidBodyMaterialID = m_Entity->GetComponent<RigidbodyComponent>().MaterialAssetID;

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

		Ref<PhysicsMaterialAsset> materialAsset = GetMaterial( mesh, rigidBodyMaterialID );

		const glm::vec3 halfColliderSize = glm::abs( transform.Scale * halfSize );
	
		// v = lbh
		const float volume = halfColliderSize.x * 2.0f * halfColliderSize.y * 2.0f * halfColliderSize.z * 2.0f;

		JPH::BoxShapeSettings shapeSettings( Auxiliary::GLMToJolt( halfColliderSize ) );
		// d = m/v
		shapeSettings.mDensity = mass / volume;
		shapeSettings.mMaterial = materialAsset->GetNative();

		m_Shape = shapeSettings.Create().Get();
	}

	void BoxShape::ExportRc( RecastInputGeometryExpData& rData, AABB& rNavMeshBounds )
	{
		const glm::vec3 localVertices[ 8 ] = {
			{ -1.0f, -1.0f, -1.0f },
			{  1.0f, -1.0f, -1.0f },
			{  1.0f,  1.0f, -1.0f },
			{ -1.0f,  1.0f, -1.0f },
			{ -1.0f, -1.0f,  1.0f },
			{  1.0f, -1.0f,  1.0f },
			{  1.0f,  1.0f,  1.0f },
			{ -1.0f,  1.0f,  1.0f },
		};

		const uint32_t indices[ 36 ] = {
			0, 1, 2,  0, 2, 3,
			4, 6, 5,  4, 7, 6,
			4, 5, 1,  4, 1, 0,
			3, 2, 6,  3, 6, 7,
			0, 3, 7,  0, 7, 4,
			1, 5, 6,  1, 6, 2
		};

		const glm::mat4 worldTransform = m_Entity->GetScene()->GetTransformRelativeToParent( m_Entity );

		JPH::Ref<JPH::BoxShape> boxShape = JPH::StaticCast<JPH::BoxShape>( m_Shape );

		const glm::vec3 halfExtent = Auxiliary::JoltToGLM( boxShape->GetHalfExtent() );
		for( uint64_t i = 0; i < 8; ++i )
		{
			const glm::vec3 scaled = localVertices[ i ] * halfExtent;
			const glm::vec4 worldPos = worldTransform * glm::vec4( scaled, 1.0f );

//			if( rData.Bounds.Contains( rNavMeshBounds ) )
			{
				rData.VertexBuffer.push_back( worldPos.x );
				rData.VertexBuffer.push_back( worldPos.y );
				rData.VertexBuffer.push_back( worldPos.z );
			}

			rNavMeshBounds.Min = glm::min( rNavMeshBounds.Min, glm::vec3{ worldPos } );
			rNavMeshBounds.Max = glm::max( rNavMeshBounds.Max, glm::vec3{ worldPos } );
		}

		rData.IndexBuffer.insert( rData.IndexBuffer.end(), std::begin( indices ), std::end( indices ) );
	}

	void BoxShape::SetTrigger( bool isTrigger )
	{
		m_Entity->GetComponent<BoxColliderComponent>().IsTrigger = isTrigger;
	}

	bool BoxShape::IsTrigger()
	{
		return m_Entity->GetComponent<BoxColliderComponent>().IsTrigger;
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

	void SphereShape::Create( float mass )
	{
		SphereColliderComponent& scc = m_Entity->GetComponent<SphereColliderComponent>();
		TransformComponent& transform = m_Entity->GetComponent<TransformComponent>();

		// Check for rigid body
		UUID rigidBodyMaterialID = 0;
		if( m_Entity->HasComponent<RigidbodyComponent>() )
			rigidBodyMaterialID = m_Entity->GetComponent<RigidbodyComponent>().MaterialAssetID;

		Ref<StaticMesh> mesh = nullptr;
		if( auto* pSM = m_Entity->TryGetComponent<StaticMeshComponent>(); pSM )
			mesh = pSM->Mesh;

		float radius = scc.Radius;
		glm::vec3 scale = transform.Scale;

		if( scale.x != 0.0f )
			radius *= scale.x;

		Ref<PhysicsMaterialAsset> materialAsset = GetMaterial( mesh, rigidBodyMaterialID );
		
		const float largestComponent = glm::abs( glm::max( transform.Scale.x, glm::max( transform.Scale.y, transform.Scale.z ) ) );
		const float scaledRadius = largestComponent * radius;
		const float volume = ( 4.0f / 3.0f ) * glm::pi<float>() * ( float ) glm::pow( scaledRadius, 3.0f );

		JPH::SphereShapeSettings sphereSetting( radius, nullptr );
		sphereSetting.mDensity = mass / volume;
		sphereSetting.mMaterial = materialAsset->GetNative();

		m_Shape = sphereSetting.Create().Get();
	}

	void SphereShape::ExportRc( RecastInputGeometryExpData& rData, AABB& rNavMeshBounds )
	{
		const auto& rTc = m_Entity->GetScene()->GetWorldSpaceTransform( m_Entity );
		
		const JPH::Mat44 transform = JPH::Mat44::sRotationTranslation( 
			Auxiliary::GLMQToJoltQ( rTc.GetRotation() ), 
			Auxiliary::GLMToJolt( rTc.Position )
		);

		const JPH::AABox boundingBox = m_Shape->GetWorldSpaceBounds( transform, Auxiliary::GLMToJolt( rTc.Scale ) );

		JPH::SphereShape::GetTrianglesContext context{};
		m_Shape->GetTrianglesStart( context, boundingBox, Auxiliary::GLMToJolt( rTc.Position ), Auxiliary::GLMQToJoltQ( rTc.GetRotation() ), Auxiliary::GLMToJolt( rTc.Scale ) );

		JPH::Float3 data[ 64 * 3 ]{};

		int vertStart = ( int ) rData.VertexBuffer.size();
		while( true )
		{
			const auto count = m_Shape->GetTrianglesNext( context, 64, data );

			if( count == 0 )
				break;

			for( int i = 0; i < count; i++ )
			{
				JPH::Float3 v0 = data[ i * 3 + 0 ];
				JPH::Float3 v1 = data[ i * 3 + 1 ];
				JPH::Float3 v2 = data[ i * 3 + 2 ];

				const glm::vec3 glmV0( v0.x, v0.y, v0.z );
				const glm::vec3 glmV1( v1.x, v1.y, v1.z );
				const glm::vec3 glmV2( v2.x, v2.y, v2.z );

				// If any point in this triangle is in the specified NavMesh bounds,
				// we need to add the whole triangle so we have a complete vertex.
				if( rData.Bounds.Contains( glmV0 ) ||
					rData.Bounds.Contains( glmV1 ) ||
					rData.Bounds.Contains( glmV2 ) )
				{
					rData.VertexBuffer.push_back( v0.x ); rData.VertexBuffer.push_back( v0.y ); rData.VertexBuffer.push_back( v0.z );
					rData.VertexBuffer.push_back( v1.x ); rData.VertexBuffer.push_back( v1.y ); rData.VertexBuffer.push_back( v1.z );
					rData.VertexBuffer.push_back( v2.x ); rData.VertexBuffer.push_back( v2.y ); rData.VertexBuffer.push_back( v2.z );

					rData.IndexBuffer.push_back( vertStart + 0 );
					rData.IndexBuffer.push_back( vertStart + 1 );
					rData.IndexBuffer.push_back( vertStart + 2 );

					// Set the "real" bounding box size
					rNavMeshBounds.Min = glm::min( rNavMeshBounds.Min, glm::min( glmV0, glm::min( glmV1, glmV2 ) ) );
					rNavMeshBounds.Max = glm::max( rNavMeshBounds.Max, glm::max( glmV0, glm::max( glmV1, glmV2 ) ) );

					vertStart += 3;
				}
			}
		}
	}

	void SphereShape::SetTrigger( bool isTrigger )
	{
		m_Entity->GetComponent<SphereColliderComponent>().IsTrigger = isTrigger;
	}

	bool SphereShape::IsTrigger()
	{
		return m_Entity->GetComponent<SphereColliderComponent>().IsTrigger;
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

	void CapsuleShape::Create( float mass )
	{
		CapsuleColliderComponent& cap = m_Entity->GetComponent<CapsuleColliderComponent>();
		TransformComponent& transform = m_Entity->GetComponent<TransformComponent>();

		// Check for rigid body
		UUID rigidBodyMaterialID = 0;
		if( m_Entity->HasComponent<RigidbodyComponent>() )
			rigidBodyMaterialID = m_Entity->GetComponent<RigidbodyComponent>().MaterialAssetID;

		// Check for static mesh
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

		Ref<PhysicsMaterialAsset> materialAsset = GetMaterial( mesh, rigidBodyMaterialID );
		
		JPH::CapsuleShapeSettings capsuleSetting( height, radius, nullptr );
		capsuleSetting.mDensity = 10.0f;
		capsuleSetting.mMaterial = materialAsset->GetNative();

		m_Shape = capsuleSetting.Create().Get();
	}

	void CapsuleShape::ExportRc( RecastInputGeometryExpData& rData, AABB& rNavMeshBounds )
	{
		const auto& rTc = m_Entity->GetScene()->GetWorldSpaceTransform( m_Entity );

		const JPH::Mat44 transform = JPH::Mat44::sRotationTranslation(
			Auxiliary::GLMQToJoltQ( rTc.GetRotation() ),
			Auxiliary::GLMToJolt( rTc.Position )
		);

		const JPH::AABox boundingBox = m_Shape->GetWorldSpaceBounds( transform, Auxiliary::GLMToJolt( rTc.Scale ) );

		JPH::Ref<JPH::CapsuleShape> capsuleShape = JPH::StaticCast<JPH::CapsuleShape>( m_Shape );
		const float radius = capsuleShape->GetRadius();
		const float halfHeight = capsuleShape->GetHalfHeightOfCylinder();

		JPH::CapsuleShape::GetTrianglesContext context{};
		m_Shape->GetTrianglesStart( context, boundingBox, Auxiliary::GLMToJolt( rTc.Position ), Auxiliary::GLMQToJoltQ( rTc.GetRotation() ), Auxiliary::GLMToJolt( rTc.Scale ) );

		constexpr int segments = 6;

		int vertStart = ( int )rData.VertexBuffer.size();
		for( size_t i = 0; i < segments; ++i )
		{
			const float a0 = ( float ) i / segments * glm::two_pi<float>();
			const float a1 = ( float ) ( i + 1 ) / segments * glm::two_pi< float >();

			glm::vec3 p0( radius * glm::cos( a0 ), -halfHeight, radius * glm::sin( a0 ) );
			glm::vec3 p1( radius * glm::cos( a1 ), -halfHeight, radius * glm::sin( a1 ) );
			glm::vec3 p2( radius * glm::cos( a0 ), halfHeight, radius * glm::sin( a0 ) );
			glm::vec3 p3( radius * glm::cos( a1 ), halfHeight, radius * glm::sin( a1 ) );

			glm::vec3 vertices[ 4 ] = { p0, p1, p2, p3 };

			for( int v = 0; v < 4; ++v )
			{
				JPH::Vec3 w = transform * JPH::Vec3( vertices[ v ].x, vertices[ v ].y, vertices[ v ].z );
				vertices[ v ] = glm::vec3( w.GetX(), w.GetY(), w.GetZ() );
			}

			rData.VertexBuffer.insert( rData.VertexBuffer.end(), 
				{
					vertices[ 0 ].x, vertices[ 0 ].y, vertices[ 0 ].z,
					vertices[ 2 ].x, vertices[ 2 ].y, vertices[ 2 ].z,
					vertices[ 1 ].x, vertices[ 1 ].y, vertices[ 1 ].z,
				} );

			rData.IndexBuffer.push_back( vertStart + 0 );
			rData.IndexBuffer.push_back( vertStart + 1 );
			rData.IndexBuffer.push_back( vertStart + 2 );

			vertStart += 3;

			rData.VertexBuffer.insert( rData.VertexBuffer.end(),
				{
					vertices[ 1 ].x, vertices[ 1 ].y, vertices[ 1 ].z,
					vertices[ 2 ].x, vertices[ 2 ].y, vertices[ 2 ].z,
					vertices[ 3 ].x, vertices[ 3 ].y, vertices[ 3 ].z,
				} );

			rData.IndexBuffer.push_back( vertStart + 0 );
			rData.IndexBuffer.push_back( vertStart + 1 );
			rData.IndexBuffer.push_back( vertStart + 2 );

			vertStart += 3;

			for( const auto& rVertex : vertices )
			{
				rNavMeshBounds.Min = glm::min( rNavMeshBounds.Min, rVertex );
				rNavMeshBounds.Max = glm::max( rNavMeshBounds.Max, rVertex );
			}
		}
	}

	void CapsuleShape::SetTrigger( bool isTrigger )
	{
		m_Entity->GetComponent<CapsuleColliderComponent>().IsTrigger = isTrigger;
	}

	bool CapsuleShape::IsTrigger()
	{
		return m_Entity->GetComponent<CapsuleColliderComponent>().IsTrigger;
	}

	//////////////////////////////////////////////////////////////////////////
	// Triangle

	TriangleMeshShape::TriangleMeshShape( SharedPtr<Entity> entity )
		: PhysicsShape( entity )
	{
		m_Type = PhysicsShapeType::TriangleMesh;

		SAT_CORE_ASSERT( m_Entity->HasComponent<StaticMeshComponent>(), "Entity does not have a static mesh component!" );
	}

	TriangleMeshShape::~TriangleMeshShape()
	{
	}

	void TriangleMeshShape::Create( float mass )
	{
		Ref<StaticMesh> staticMesh = m_Entity->GetComponent<StaticMeshComponent>().Mesh;
		m_Shape = PhysicsFoundation::Get()->GetCooking().CreateTriangleMesh( m_Entity, staticMesh );
	}

	bool TriangleMeshShape::IsTrigger()
	{
		// TODO: Mesh colliders as triggers
		return false;
	}

	void TriangleMeshShape::ExportRcScaledShaped( RecastInputGeometryExpData& rData, AABB& rNavMeshBounds )
	{
		const auto& rTc = m_Entity->GetScene()->GetWorldSpaceTransform( m_Entity );

		JPH::Ref<JPH::ScaledShape> scaledShape = JPH::StaticCast<JPH::ScaledShape>( m_Shape );

		int vertStart = ( int ) rData.VertexBuffer.size();

		const auto scale = Auxiliary::GLMToJolt( rTc.Scale );
		const JPH::Mat44 transform = JPH::Mat44::sRotationTranslation(
			Auxiliary::GLMQToJoltQ( rTc.GetRotation() ),
			Auxiliary::GLMToJolt( rTc.Position )
		);
		const JPH::AABox boundingBox = scaledShape->GetWorldSpaceBounds( transform, scale );

		JPH::MeshShape::GetTrianglesContext context{};
		scaledShape->GetInnerShape()->GetTrianglesStart( context, boundingBox, Auxiliary::GLMToJolt( rTc.Position ), Auxiliary::GLMQToJoltQ( rTc.GetRotation() ), scale );

		while( true )
		{
			JPH::Float3 data[ 64 * 3 ]{};

			const int count = scaledShape->GetInnerShape()->GetTrianglesNext( context, 64, data, nullptr );
			if( count == 0 )
				break;

			for( int i = 0; i < count; i++ )
			{
				JPH::Float3 v0 = data[ i * 3 + 0 ];
				JPH::Float3 v1 = data[ i * 3 + 1 ];
				JPH::Float3 v2 = data[ i * 3 + 2 ];

				const glm::vec3 glmV0( v0.x, v0.y, v0.z );
				const glm::vec3 glmV1( v1.x, v1.y, v1.z );
				const glm::vec3 glmV2( v2.x, v2.y, v2.z );

				// If any point in this triangle is in the specified NavMesh bounds,
				// we need to add the whole triangle so we have a complete vertex.
				if( rData.Bounds.Contains( glmV0 ) ||
					rData.Bounds.Contains( glmV1 ) ||
					rData.Bounds.Contains( glmV2 ) )
				{
					rData.VertexBuffer.push_back( v0.x ); rData.VertexBuffer.push_back( v0.y ); rData.VertexBuffer.push_back( v0.z );
					rData.VertexBuffer.push_back( v1.x ); rData.VertexBuffer.push_back( v1.y ); rData.VertexBuffer.push_back( v1.z );
					rData.VertexBuffer.push_back( v2.x ); rData.VertexBuffer.push_back( v2.y ); rData.VertexBuffer.push_back( v2.z );

					rData.IndexBuffer.push_back( vertStart + 0 );
					rData.IndexBuffer.push_back( vertStart + 1 );
					rData.IndexBuffer.push_back( vertStart + 2 );

					// Set the "real" bounding box size
					rNavMeshBounds.Min = glm::min( rNavMeshBounds.Min, glm::min( glmV0, glm::min( glmV1, glmV2 ) ) );
					rNavMeshBounds.Max = glm::max( rNavMeshBounds.Max, glm::max( glmV0, glm::max( glmV1, glmV2 ) ) );

					vertStart += 3;
				}
			}
		}
	}

	void TriangleMeshShape::ExportRc( RecastInputGeometryExpData& rData, AABB& rNavMeshBounds )
	{
		if( m_Entity->GetComponent<StaticMeshComponent>().Mesh->Submeshes().size() > 1 )
		{
			// export static compound shape
		}
		else
		{
			ExportRcScaledShaped( rData, rNavMeshBounds );
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Convex

	ConvexMeshShape::ConvexMeshShape( SharedPtr<Entity> entity )
		: PhysicsShape( entity )
	{
		m_Type = PhysicsShapeType::ConvexMesh;

		SAT_CORE_ASSERT( m_Entity->HasComponent<StaticMeshComponent>(), "Entity does not have a static mesh component!" );
	}

	ConvexMeshShape::~ConvexMeshShape()
	{
	}

	void ConvexMeshShape::Create( float mass )
	{
		Ref<StaticMesh> staticMesh = m_Entity->GetComponent<StaticMeshComponent>().Mesh;
		m_Shape = PhysicsFoundation::Get()->GetCooking().CreateConvexMesh( m_Entity, staticMesh );
	}

	bool ConvexMeshShape::IsTrigger()
	{
		// TODO: Mesh colliders as triggers
		return false;
	}

#if SAT_WITH_PHYSX
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
							glm::vec3 v0 = glm::vec3( actorTransform * glm::vec4( Auxiliary::JoltToGLM( pVertexBuffer[ pPolyIndices[ 0 ] ] ), 1.0f ) );
							glm::vec3 v1 = glm::vec3( actorTransform * glm::vec4( Auxiliary::JoltToGLM( pVertexBuffer[ pPolyIndices[ j ] ] ), 1.0f ) );
							glm::vec3 v2 = glm::vec3( actorTransform * glm::vec4( Auxiliary::JoltToGLM( pVertexBuffer[ pPolyIndices[ i + 1 ] ] ), 1.0f ) );

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
#endif

	void ConvexMeshShape::ExportRc( RecastInputGeometryExpData& rData, AABB& rNavMeshBounds )
	{

	}

}
