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

#include "sppch.h"
#include "PhysicsShapes.h"

#include "PhysicsAuxiliary.h"
#include "PhysicsFoundation.h"
#include "PhysicsCooking.h"
#include "Saturn/Asset/PhysicsMaterialAsset.h"
#include "Saturn/Asset/AssetManager.h"

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

	Ref<PhysicsMaterialAsset> PhysicsShape::GetMaterial( const Ref<StaticMesh>& rMesh )
	{
		Ref<PhysicsMaterialAsset> materialAsset;

		Ref<Project> activeProject = Project::GetActiveProject();
		if( rMesh->GetPhysicsMaterial() == 0 || rMesh->GetPhysicsMaterial() == activeProject->GetDefaultPhysicsMaterialAsset() )
		{
			materialAsset = AssetManager::Get().GetAssetAs<PhysicsMaterialAsset>( activeProject->GetDefaultPhysicsMaterialAsset() );
		}
		else
		{
			materialAsset = AssetManager::Get().GetAssetAs<PhysicsMaterialAsset>( rMesh->GetPhysicsMaterial() );
		}

		return materialAsset;
	}

	//////////////////////////////////////////////////////////////////////////
	// Box

	BoxShape::BoxShape( Ref<Entity> entity )
		: PhysicsShape( entity )
	{
		m_Type = ShapeType::Box;
	}

	BoxShape::~BoxShape()
	{
	}

	void BoxShape::Create( physx::PxRigidActor& rActor )
	{
		BoxColliderComponent& bcc = m_Entity->GetComponent<BoxColliderComponent>();
		TransformComponent& transform = m_Entity->GetComponent<TransformComponent>();

		const Ref<StaticMesh>& mesh = m_Entity->GetComponent<StaticMeshComponent>().Mesh;

		glm::vec3 size = bcc.Extents;

		// Very rare path, only happens if something else modifies the scale.
		if( bcc.AutoAdjustExtent && size != transform.Scale )
			size = transform.Scale;

		glm::vec3 halfSize = size / 2.0f;

		Ref<PhysicsMaterialAsset> materialAsset = GetMaterial( mesh );
		physx::PxMaterial* mat = nullptr;

		// No material was found, so this means two things, one the project has no default material or (two) the mesh/project has a material asset but it can't be found.
		if( !materialAsset )
		{
			materialAsset = Ref<PhysicsMaterialAsset>::Create( 1.0f, 1.0f, 1.0f );
		}

		mat = &materialAsset->GetMaterial();

		physx::PxBoxGeometry BoxGeometry = physx::PxBoxGeometry( halfSize.x, halfSize.y, halfSize.z );
		physx::PxShape* pShape = physx::PxRigidActorExt::createExclusiveShape( rActor, BoxGeometry, *mat );

		pShape->setFlag( physx::PxShapeFlag::eSIMULATION_SHAPE, !bcc.IsTrigger );
		pShape->setFlag( physx::PxShapeFlag::eTRIGGER_SHAPE, bcc.IsTrigger );

		pShape->setLocalPose( Auxiliary::GLMTransformToPx( glm::translate( glm::mat4( 1.0f ), bcc.Offset ) ) );

		m_Shape = pShape;

		rActor.attachShape( *pShape );

		SetFilterData();
	}

	//////////////////////////////////////////////////////////////////////////
	// Sphere

	SphereShape::SphereShape( Ref<Entity> entity )
		: PhysicsShape( entity )
	{
		m_Type = ShapeType::Sphere;
	}

	SphereShape::~SphereShape()
	{
	}

	void SphereShape::Create( physx::PxRigidActor& rActor )
	{
		SphereColliderComponent& scc = m_Entity->GetComponent<SphereColliderComponent>();
		TransformComponent& transform = m_Entity->GetComponent<TransformComponent>();

		const Ref<StaticMesh>& mesh = m_Entity->GetComponent<StaticMeshComponent>().Mesh;

		float size = scc.Radius;
		glm::vec scale = transform.Scale;

		if( scale.x != 0.0f )
			size *= scale.x;

		float halfSize = size / 2.0f;

		Ref<PhysicsMaterialAsset> materialAsset = GetMaterial( mesh );
		physx::PxMaterial* mat = nullptr;

		// No material was found, so this means two things, one the project has no default material or (two) the mesh/project has a material asset but it can't be found.
		if( !materialAsset )
		{
			materialAsset = Ref<PhysicsMaterialAsset>::Create( 1.0f, 1.0f, 1.0f );
		}

		mat = &materialAsset->GetMaterial();

		physx::PxSphereGeometry SphereGoemetry( halfSize );

		physx::PxShape* pShape = physx::PxRigidActorExt::createExclusiveShape( rActor, SphereGoemetry, *mat );

		pShape->setFlag( physx::PxShapeFlag::eSIMULATION_SHAPE, !scc.IsTrigger );
		pShape->setFlag( physx::PxShapeFlag::eTRIGGER_SHAPE, scc.IsTrigger );

		m_Shape = pShape;
		rActor.attachShape( *pShape );

		SetFilterData();
	}

	//////////////////////////////////////////////////////////////////////////
	// Capsule

	CapsuleShape::CapsuleShape( Ref<Entity> entity )
		: PhysicsShape( entity )
	{
		m_Type = ShapeType::Capusle;
	}

	CapsuleShape::~CapsuleShape()
	{
	}

	void CapsuleShape::Create( physx::PxRigidActor& rActor )
	{
		CapsuleColliderComponent& cap = m_Entity->GetComponent<CapsuleColliderComponent>();
		TransformComponent& transform = m_Entity->GetComponent<TransformComponent>();

		const Ref<StaticMesh>& mesh = m_Entity->GetComponent<StaticMeshComponent>().Mesh;

		float size = cap.Radius;
		float height = cap.Height;

		glm::vec3 scale = transform.Scale;

		if( scale.x != 0.0f && height == 0.0f )
			size *= scale.x;

		if( scale.y != 0.0f && height == 0.0f )
			height *= scale.y;

		Ref<PhysicsMaterialAsset> materialAsset = GetMaterial( mesh );
		physx::PxMaterial* mat = nullptr;

		// No material was found, so this means two things, one the project has no default material or (two) the mesh/project has a material asset but it can't be found.
		if( !materialAsset )
		{
			materialAsset = Ref<PhysicsMaterialAsset>::Create( 1.0f, 1.0f, 1.0f );
		}

		mat = &materialAsset->GetMaterial();

		float halfHeight = height / 2.0f;
		physx::PxCapsuleGeometry CapsuleGemetry( size, halfHeight );

		physx::PxShape* pShape = physx::PxRigidActorExt::createExclusiveShape( rActor, CapsuleGemetry, *mat );
		pShape->setFlag( physx::PxShapeFlag::eSIMULATION_SHAPE, !cap.IsTrigger );
		pShape->setFlag( physx::PxShapeFlag::eTRIGGER_SHAPE, cap.IsTrigger );

		pShape->setLocalPose( physx::PxTransform( physx::PxQuat( physx::PxHalfPi, physx::PxVec3( 0, 0, 1 ) ) ) );

		m_Shape = pShape;
		rActor.attachShape( *pShape );

		SetFilterData();
	}

	//////////////////////////////////////////////////////////////////////////
	// Triangle

	TriangleMeshShape::TriangleMeshShape( Ref<Entity> entity )
		: PhysicsShape( entity )
	{
		m_Type = ShapeType::TriangleMesh;

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

		const std::vector<physx::PxShape*>& rShapes = PhysicsCooking::Get().CreateTriangleMesh( m_Mesh, rActor, transform.Scale );

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

	//////////////////////////////////////////////////////////////////////////
	// Convex

	ConvexMeshShape::ConvexMeshShape( Ref<Entity> entity )
		: PhysicsShape( entity )
	{
		m_Type = ShapeType::ConvexMesh;

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

		const std::vector<physx::PxShape*>& rShapes = PhysicsCooking::Get().CreateConvexMesh( m_Mesh, rActor, transform.Scale );

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

}